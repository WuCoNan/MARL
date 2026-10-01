"""
MAPPO 训练入口脚本（轻量仿真器版）

当前用途：在 NetworkX 轻量仿真器上跑通完整训练流程（MLP Actor/Critic）。
第 4 章的 GNN Actor 与第 5 章的改进 MAPPO 尚未接入，见 docs/01_实施计划.md。

用法：
    source ~/marl_env/bin/activate
    cd <仓库根目录>
    python python/train.py [--config python/configs/default.yaml]

产物：TensorBoard 日志写入 results/logs/，模型检查点写入 results/models/。
"""

import os
import sys
import yaml
import time
import argparse
import numpy as np
import torch

# 添加项目根目录到 path
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from env.simple_network_env import SimpleNetworkEnv
from algorithms.buffer import RolloutBuffer
from algorithms.mappo import MAPPOTrainer


def load_config(config_path: str) -> dict:
    """加载 YAML 配置"""
    with open(config_path, "r") as f:
        config = yaml.safe_load(f)
    return config


def set_seed(seed: int):
    """设置全局随机种子"""
    np.random.seed(seed)
    torch.manual_seed(seed)
    if torch.cuda.is_available():
        torch.cuda.manual_seed_all(seed)


def train(config: dict):
    """主训练循环"""
    # ============ 解析配置 ============
    env_cfg = config["env"]
    train_cfg = config["training"]
    log_cfg = config["logging"]
    device = config.get("device", "cpu")
    seed = config.get("seed", 42)

    set_seed(seed)

    # ============ 创建环境 ============
    env = SimpleNetworkEnv(
        num_nodes=env_cfg["num_nodes"],
        max_degree=env_cfg["max_degree"],
        buffer_capacity=env_cfg["buffer_capacity"],
        link_bandwidth=env_cfg["link_bandwidth"],
        num_flows=env_cfg["num_flows"],
        episode_length=env_cfg["episode_length"],
        topology_type=env_cfg["topology_type"],
        connectivity_radius=env_cfg["connectivity_radius"],
        er_probability=env_cfg["er_probability"],
        reward_weights=tuple(env_cfg["reward_weights"]),
        seed=seed,
    )

    num_nodes = env_cfg["num_nodes"]
    obs_dim = env.obs_dim
    max_degree = env_cfg["max_degree"]
    episode_length = env_cfg["episode_length"]

    print("=" * 60)
    print("  MAPPO Training - Task 1.4 验证")
    print("=" * 60)
    print(f"  节点数: {num_nodes}")
    print(f"  观测维度: {obs_dim}")
    print(f"  动作空间: {max_degree} (最大邻居数)")
    print(f"  Episode 长度: {episode_length} steps")
    print(f"  总 Episodes: {train_cfg['num_episodes']}")
    print(f"  设备: {device}")
    print(f"  隐藏层: {train_cfg['hidden_dim']}")
    print("=" * 60)

    # ============ 创建 Buffer 和 Trainer ============
    buffer = RolloutBuffer(
        max_steps=episode_length,
        num_nodes=num_nodes,
        obs_dim=obs_dim,
        max_degree=max_degree,
        gamma=train_cfg["gamma"],
        gae_lambda=train_cfg["gae_lambda"],
    )

    trainer = MAPPOTrainer(
        obs_dim=obs_dim,
        num_nodes=num_nodes,
        max_degree=max_degree,
        hidden_dim=train_cfg["hidden_dim"],
        lr=train_cfg["lr"],
        gamma=train_cfg["gamma"],
        gae_lambda=train_cfg["gae_lambda"],
        clip_epsilon=train_cfg["clip_epsilon"],
        entropy_coef=train_cfg["entropy_coef"],
        value_coef=train_cfg["value_coef"],
        max_grad_norm=train_cfg["max_grad_norm"],
        ppo_epochs=train_cfg["ppo_epochs"],
        device=device,
    )

    # ============ TensorBoard (可选) ============
    writer = None
    try:
        from torch.utils.tensorboard import SummaryWriter
        log_dir = os.path.join(
            os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
            log_cfg["log_dir"]
        )
        os.makedirs(log_dir, exist_ok=True)
        writer = SummaryWriter(log_dir=log_dir)
        print(f"  TensorBoard 日志: {log_dir}")
    except ImportError:
        print("  [WARN] tensorboard 未安装，使用纯文本日志")

    # ============ 训练循环 ============
    episode_rewards = []
    episode_lengths = []
    start_time = time.time()

    for episode in range(1, train_cfg["num_episodes"] + 1):
        obs, info = env.reset()
        action_mask = info["action_mask"]
        buffer.clear()

        ep_reward = 0.0
        ep_steps = 0
        done = False

        while not done:
            # Actor 推理
            actions, log_probs, value = trainer.select_actions(obs, action_mask)

            # 环境交互
            next_obs, reward, terminated, truncated, info = env.step(actions)
            done = terminated or truncated
            next_action_mask = info["action_mask"]

            # 存入 buffer
            buffer.add(
                obs=obs,
                action=actions,
                log_prob=log_probs,
                reward=reward,
                value=value,
                done=done,
                action_mask=action_mask,
            )

            obs = next_obs
            action_mask = next_action_mask
            ep_reward += reward
            ep_steps += 1

        # Episode 结束，计算 GAE 并更新
        buffer.compute_returns(last_value=0.0)
        buffer_data = buffer.get_tensors(device=torch.device(device))
        update_info = trainer.update(buffer_data)

        # 记录
        episode_rewards.append(ep_reward)
        episode_lengths.append(ep_steps)

        # ============ 日志输出 ============
        if episode % log_cfg["log_interval"] == 0 or episode == 1:
            elapsed = time.time() - start_time
            avg_reward = np.mean(episode_rewards[-log_cfg["log_interval"]:])
            avg_length = np.mean(episode_lengths[-log_cfg["log_interval"]:])

            print(
                f"  Episode {episode:4d}/{train_cfg['num_episodes']} | "
                f"Reward: {ep_reward:7.3f} | "
                f"Avg({log_cfg['log_interval']}): {avg_reward:7.3f} | "
                f"Steps: {ep_steps:3d} | "
                f"Actor Loss: {update_info['actor_loss']:.4f} | "
                f"Critic Loss: {update_info['critic_loss']:.4f} | "
                f"Entropy: {update_info['entropy']:.4f} | "
                f"Time: {elapsed:.1f}s"
            )

        # TensorBoard 记录
        if writer is not None:
            writer.add_scalar("Rollout/ep_reward", ep_reward, episode)
            writer.add_scalar("Rollout/ep_length", ep_steps, episode)
            writer.add_scalar("Rollout/total_delivered", info.get("total_delivered", 0), episode)
            writer.add_scalar("Rollout/total_dropped", info.get("total_dropped", 0), episode)
            writer.add_scalar("Train/actor_loss", update_info["actor_loss"], episode)
            writer.add_scalar("Train/critic_loss", update_info["critic_loss"], episode)
            writer.add_scalar("Train/entropy", update_info["entropy"], episode)

        # 保存模型
        if episode % log_cfg["save_interval"] == 0:
            save_dir = os.path.join(
                os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                log_cfg["save_dir"]
            )
            os.makedirs(save_dir, exist_ok=True)
            model_path = os.path.join(save_dir, f"mappo_ep{episode}.pt")
            trainer.save(model_path)
            print(f"  [SAVE] 模型已保存: {model_path}")

    # ============ 训练结束 ============
    total_time = time.time() - start_time
    print("\n" + "=" * 60)
    print("  训练完成!")
    print("=" * 60)
    print(f"  总 Episodes: {train_cfg['num_episodes']}")
    print(f"  总时间: {total_time:.1f}s")
    print(f"  平均每 Episode: {total_time / train_cfg['num_episodes']:.2f}s")
    print(f"  最终 Avg Reward (last 10): {np.mean(episode_rewards[-10:]):.4f}")
    print(f"  最终 Avg Length (last 10): {np.mean(episode_lengths[-10:]):.1f}")
    print("=" * 60)

    # 维度验证输出
    print("\n[维度验证]")
    print(f"  obs shape: ({num_nodes}, {obs_dim}) = {obs.shape}")
    print(f"  action shape: ({num_nodes},) = {actions.shape}")
    print(f"  action_mask shape: ({num_nodes}, {max_degree}) = {action_mask.shape}")
    print(f"  reward: scalar = {type(reward).__name__}")
    print(f"  buffer size: {buffer.size} steps")

    if writer is not None:
        writer.close()

    return episode_rewards


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="MAPPO Training - Task 1.4")
    parser.add_argument(
        "--config", type=str,
        default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "configs", "default.yaml"),
        help="配置文件路径"
    )
    args = parser.parse_args()

    config = load_config(args.config)
    train(config)
