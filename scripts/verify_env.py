"""
环境验证脚本 - 三层验证
第一层：基础导入验证
第二层：核心模块功能验证（PyG, Gymnasium, NetworkX）
第三层：端到端训练验证（GNN Actor + MAPPO on CartPole）
"""

import sys
import time

PASS = "\033[92m[PASS]\033[0m"
FAIL = "\033[91m[FAIL]\033[0m"
INFO = "\033[94m[INFO]\033[0m"

results = {"layer1": [], "layer2": [], "layer3": []}


# ============================================================
# 第一层：基础导入验证
# ============================================================
print("=" * 60)
print("第一层：基础导入验证")
print("=" * 60)

imports = {
    "torch": "torch",
    "torch_geometric": "torch_geometric",
    "torch_geometric.nn.GATConv": "torch_geometric.nn.GATConv",
    "torch_geometric.data.Data": "torch_geometric.data.Data",
    "torch_geometric.nn.global_pool": "torch_geometric.nn.global_mean_pool",
    "networkx": "networkx",
    "gymnasium": "gymnasium",
    "numpy": "numpy",
    "matplotlib": "matplotlib",
    "pandas": "pandas",
    "yaml": "yaml",
}

for name, module_path in imports.items():
    try:
        parts = module_path.split(".")
        mod = __import__(parts[0])
        for p in parts[1:]:
            mod = getattr(mod, p)
        print(f"  {PASS} import {name}")
        results["layer1"].append(True)
    except Exception as e:
        print(f"  {FAIL} import {name}: {e}")
        results["layer1"].append(False)

# CUDA check
import torch
cuda_ok = torch.cuda.is_available()
if cuda_ok:
    gpu_name = torch.cuda.get_device_name(0)
    vram = torch.cuda.get_device_properties(0).total_memory / 1024**3
    print(f"  {PASS} CUDA available: {gpu_name} ({vram:.1f} GB)")
else:
    print(f"  {FAIL} CUDA not available")
results["layer1"].append(cuda_ok)

print(f"\n第一层结果: {sum(results['layer1'])}/{len(results['layer1'])} 通过\n")


# ============================================================
# 第二层：核心模块功能验证
# ============================================================
print("=" * 60)
print("第二层：核心模块功能验证")
print("=" * 60)

# --- 2.1 PyG: GATConv 前向传播 ---
try:
    import torch
    from torch_geometric.nn import GATConv
    from torch_geometric.data import Data

    # 构建一个 5 节点的小图
    x = torch.randn(5, 8)  # 5 nodes, 8 features
    edge_index = torch.tensor([[0,1,1,2,2,3,3,4,0,4],
                                [1,0,2,1,3,2,4,3,4,0]], dtype=torch.long)

    conv = GATConv(8, 16, heads=2, concat=True)
    out = conv(x, edge_index)

    assert out.shape == (5, 32), f"Expected (5, 32), got {out.shape}"
    print(f"  {PASS} PyG GATConv: input {x.shape} -> output {out.shape}")
    results["layer2"].append(True)
except Exception as e:
    print(f"  {FAIL} PyG GATConv: {e}")
    results["layer2"].append(False)

# --- 2.2 Gymnasium: CartPole reset + step ---
try:
    import gymnasium as gym

    env = gym.make("CartPole-v1")
    obs, info = env.reset()
    total_reward = 0
    done = False
    steps = 0
    while not done and steps < 100:
        action = env.action_space.sample()
        obs, reward, terminated, truncated, info = env.step(action)
        total_reward += reward
        done = terminated or truncated
        steps += 1

    assert obs.shape == (4,), f"Expected obs shape (4,), got {obs.shape}"
    assert steps > 0, "No steps executed"
    print(f"  {PASS} Gymnasium CartPole: {steps} steps, total_reward={total_reward}")
    env.close()
    results["layer2"].append(True)
except Exception as e:
    print(f"  {FAIL} Gymnasium CartPole: {e}")
    results["layer2"].append(False)

# --- 2.3 NetworkX: 随机几何图 + 邻接矩阵 ---
try:
    import networkx as nx
    import numpy as np

    G = nx.random_geometric_graph(20, 0.4)
    adj = nx.adjacency_matrix(G).toarray()
    num_edges = G.number_of_edges()

    assert adj.shape == (20, 20), f"Expected (20,20), got {adj.shape}"
    assert num_edges > 0, "No edges"
    print(f"  {PASS} NetworkX: 20-node graph, {num_edges} edges, adj shape={adj.shape}")
    results["layer2"].append(True)
except Exception as e:
    print(f"  {FAIL} NetworkX: {e}")
    results["layer2"].append(False)

# --- 2.4 PyG: 图读出 (global_mean_pool) ---
try:
    from torch_geometric.nn import global_mean_pool

    x = torch.randn(5, 16)
    batch = torch.tensor([0, 0, 0, 1, 1])
    out = global_mean_pool(x, batch)

    assert out.shape == (2, 16), f"Expected (2, 16), got {out.shape}"
    print(f"  {PASS} PyG global_mean_pool: {x.shape} -> {out.shape}")
    results["layer2"].append(True)
except Exception as e:
    print(f"  {FAIL} PyG global_mean_pool: {e}")
    results["layer2"].append(False)

print(f"\n第二层结果: {sum(results['layer2'])}/{len(results['layer2'])} 通过\n")


# ============================================================
# 第三层：端到端训练验证（GNN Actor + MAPPO on CartPole）
# ============================================================
print("=" * 60)
print("第三层：端到端训练验证")
print("=" * 60)

try:
    import torch
    import torch.nn as nn
    import torch.optim as optim
    from torch_geometric.nn import GATConv, global_mean_pool
    from torch_geometric.data import Data, Batch
    import gymnasium as gym
    import numpy as np

    # --- 定义简化版 GNN Actor ---
    class GNNActor(nn.Module):
        def __init__(self, node_feat_dim, hidden_dim, action_dim):
            super().__init__()
            self.conv1 = GATConv(node_feat_dim, hidden_dim, heads=2, concat=True)
            self.conv2 = GATConv(hidden_dim * 2, hidden_dim, heads=1, concat=False)
            self.policy_head = nn.Linear(hidden_dim, action_dim)

        def forward(self, data):
            x, edge_index, batch = data.x, data.edge_index, data.batch
            x = torch.relu(self.conv1(x, edge_index))
            x = torch.relu(self.conv2(x, edge_index))
            graph_emb = global_mean_pool(x, batch)
            logits = self.policy_head(graph_emb)
            return logits

    # --- 定义简化版 GNN Critic ---
    class GNNCritic(nn.Module):
        def __init__(self, node_feat_dim, hidden_dim):
            super().__init__()
            self.conv1 = GATConv(node_feat_dim, hidden_dim, heads=2, concat=True)
            self.conv2 = GATConv(hidden_dim * 2, hidden_dim, heads=1, concat=False)
            self.value_head = nn.Linear(hidden_dim, 1)

        def forward(self, data):
            x, edge_index, batch = data.x, data.edge_index, data.batch
            x = torch.relu(self.conv1(x, edge_index))
            x = torch.relu(self.conv2(x, edge_index))
            graph_emb = global_mean_pool(x, batch)
            value = self.value_head(graph_emb)
            return value

    # --- 将 CartPole obs 转为 PyG Data ---
    def obs_to_graph(obs):
        """将 CartPole 4维 obs 转为 2 节点图（简化演示）"""
        x = torch.tensor([[obs[0], obs[1]], [obs[2], obs[3]]], dtype=torch.float32)
        edge_index = torch.tensor([[0, 1], [1, 0]], dtype=torch.long)
        return Data(x=x, edge_index=edge_index)

    # --- MAPPO 训练循环 ---
    env = gym.make("CartPole-v1")
    actor = GNNActor(node_feat_dim=2, hidden_dim=16, action_dim=2)
    critic = GNNCritic(node_feat_dim=2, hidden_dim=16)

    # GPU
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    actor.to(device)
    critic.to(device)

    actor_opt = optim.Adam(actor.parameters(), lr=1e-3)
    critic_opt = optim.Adam(critic.parameters(), lr=1e-3)

    num_episodes = 100
    episode_rewards = []
    episode_losses = []

    print(f"  {INFO} 设备: {device}")
    print(f"  {INFO} 开始训练 {num_episodes} episodes...")

    for ep in range(num_episodes):
        obs, _ = env.reset()
        ep_reward = 0
        done = False
        trajectory = []

        while not done:
            # 构建图数据
            data = obs_to_graph(obs)
            data_batch = Batch.from_data_list([data]).to(device)

            # Actor 推理
            logits = actor(data_batch)
            probs = torch.softmax(logits, dim=-1)
            dist = torch.distributions.Categorical(probs)
            action = dist.sample()

            # 环境交互
            next_obs, reward, terminated, truncated, info = env.step(action.item())
            done = terminated or truncated
            ep_reward += reward

            # 记录
            value = critic(data_batch)
            trajectory.append({
                "log_prob": dist.log_prob(action),
                "value": value.squeeze(),
                "reward": reward,
            })

            obs = next_obs

        episode_rewards.append(ep_reward)

        # 计算 returns 和 losses
        returns = []
        R = 0
        gamma = 0.99
        for t in reversed(range(len(trajectory))):
            R = trajectory[t]["reward"] + gamma * R
            returns.insert(0, R)
        returns = torch.tensor(returns, dtype=torch.float32, device=device)

        # Critic loss
        values = torch.stack([t["value"] for t in trajectory])
        critic_loss = nn.functional.mse_loss(values, returns.detach())

        # Actor loss (简化版 PPO)
        advantages = returns.detach() - values.detach()
        log_probs = torch.stack([t["log_prob"] for t in trajectory])
        actor_loss = -(log_probs * advantages).mean()

        total_loss = actor_loss + 0.5 * critic_loss

        actor_opt.zero_grad()
        critic_opt.zero_grad()
        total_loss.backward()
        actor_opt.step()
        critic_opt.step()

        episode_losses.append(total_loss.item())

        if (ep + 1) % 20 == 0:
            avg_r = np.mean(episode_rewards[-20:])
            avg_l = np.mean(episode_losses[-20:])
            print(f"  {INFO} Episode {ep+1}/{num_episodes}: "
                  f"avg_reward={avg_r:.1f}, avg_loss={avg_l:.4f}")

    env.close()

    # 验证结果
    final_avg = np.mean(episode_rewards[-20:])
    loss_valid = all(not np.isnan(l) for l in episode_losses)
    reward_trend = np.mean(episode_rewards[-10:]) > np.mean(episode_rewards[:10])

    print(f"\n  {INFO} 训练完成!")
    print(f"  {INFO} 前10集平均reward: {np.mean(episode_rewards[:10]):.1f}")
    print(f"  {INFO} 后10集平均reward: {np.mean(episode_rewards[-10:]):.1f}")
    print(f"  {INFO} Loss 有效 (无NaN): {loss_valid}")
    print(f"  {INFO} Reward 有上升趋势: {reward_trend}")

    # GPU 验证
    if torch.cuda.is_available():
        mem_used = torch.cuda.memory_allocated() / 1024**2
        print(f"  {INFO} GPU 显存占用: {mem_used:.1f} MB")

    all_pass = loss_valid and final_avg > 0
    if all_pass:
        print(f"  {PASS} 端到端训练验证通过")
    else:
        print(f"  {FAIL} 端到端训练验证失败")
    results["layer3"].append(all_pass)

except Exception as e:
    import traceback
    print(f"  {FAIL} 端到端训练: {e}")
    traceback.print_exc()
    results["layer3"].append(False)

print(f"\n第三层结果: {sum(results['layer3'])}/{len(results['layer3'])} 通过\n")


# ============================================================
# 总结
# ============================================================
print("=" * 60)
print("总结")
print("=" * 60)
total_pass = sum(results["layer1"]) + sum(results["layer2"]) + sum(results["layer3"])
total_all = len(results["layer1"]) + len(results["layer2"]) + len(results["layer3"])
print(f"  第一层 (导入): {sum(results['layer1'])}/{len(results['layer1'])}")
print(f"  第二层 (功能): {sum(results['layer2'])}/{len(results['layer2'])}")
print(f"  第三层 (训练): {sum(results['layer3'])}/{len(results['layer3'])}")
print(f"  总计: {total_pass}/{total_all}")

if total_pass == total_all:
    print(f"\n  {PASS} 所有验证通过！环境就绪。")
    sys.exit(0)
else:
    print(f"\n  {FAIL} 部分验证未通过，请检查。")
    sys.exit(1)
