"""
MAPPO 基础训练器 (MLP 版本)
Task 1.4：验证训练流程，后续 Milestone 3/4 替换为 GNN Actor/Critic

架构：
- Actor: MLP，输入单节点 obs → 输出动作 logits (over max_degree neighbors)
- Critic: MLP，输入全局状态 (所有节点 obs flatten) → 输出 V(s)
- 共享策略：所有节点共享同一个 Actor 网络（参数共享）
"""

import torch
import torch.nn as nn
import torch.nn.functional as F
from torch.distributions import Categorical
import numpy as np
from typing import Tuple


class MLPActor(nn.Module):
    """
    MLP Actor 网络（参数共享）

    输入：单节点观测 [obs_dim]
    输出：动作 logits [max_degree]（对应选择哪个邻居转发）
    """

    def __init__(self, obs_dim: int, max_degree: int, hidden_dim: int = 64):
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(obs_dim, hidden_dim),
            nn.ReLU(),
            nn.Linear(hidden_dim, hidden_dim),
            nn.ReLU(),
            nn.Linear(hidden_dim, max_degree),
        )

    def forward(self, obs: torch.Tensor, action_mask: torch.Tensor = None) -> torch.Tensor:
        """
        Args:
            obs: [batch, obs_dim] 或 [num_nodes, obs_dim]
            action_mask: [batch, max_degree] 有效动作 mask (1=有效, 0=无效)

        Returns:
            logits: [batch, max_degree]
        """
        logits = self.net(obs)

        # 对无效动作施加极大负值
        if action_mask is not None:
            logits = logits.masked_fill(action_mask == 0, -1e9)

        return logits


class MLPCritic(nn.Module):
    """
    MLP Critic 网络

    输入：全局状态（所有节点 obs 拼接）[num_nodes * obs_dim]
    输出：全局价值估计 V(s) [1]
    """

    def __init__(self, num_nodes: int, obs_dim: int, hidden_dim: int = 128):
        super().__init__()
        input_dim = num_nodes * obs_dim
        self.net = nn.Sequential(
            nn.Linear(input_dim, hidden_dim),
            nn.ReLU(),
            nn.Linear(hidden_dim, hidden_dim),
            nn.ReLU(),
            nn.Linear(hidden_dim, 1),
        )

    def forward(self, global_obs: torch.Tensor) -> torch.Tensor:
        """
        Args:
            global_obs: [batch, num_nodes * obs_dim] 全局观测展平

        Returns:
            value: [batch, 1]
        """
        return self.net(global_obs)


class MAPPOTrainer:
    """
    MAPPO 训练器

    核心流程：
    1. Actor 推理 → 采样动作 + log_prob
    2. Critic 估计 V(s)
    3. Episode 结束后计算 GAE
    4. PPO clip 更新 Actor + Critic
    """

    def __init__(
        self,
        obs_dim: int,
        num_nodes: int,
        max_degree: int,
        hidden_dim: int = 64,
        lr: float = 3e-4,
        gamma: float = 0.99,
        gae_lambda: float = 0.95,
        clip_epsilon: float = 0.2,
        entropy_coef: float = 0.01,
        value_coef: float = 0.5,
        max_grad_norm: float = 0.5,
        ppo_epochs: int = 4,
        device: str = "cpu",
    ):
        self.obs_dim = obs_dim
        self.num_nodes = num_nodes
        self.max_degree = max_degree
        self.gamma = gamma
        self.gae_lambda = gae_lambda
        self.clip_epsilon = clip_epsilon
        self.entropy_coef = entropy_coef
        self.value_coef = value_coef
        self.max_grad_norm = max_grad_norm
        self.ppo_epochs = ppo_epochs
        self.device = torch.device(device)

        # 构建网络
        self.actor = MLPActor(obs_dim, max_degree, hidden_dim).to(self.device)
        self.critic = MLPCritic(num_nodes, obs_dim, hidden_dim * 2).to(self.device)

        # 优化器
        self.optimizer = torch.optim.Adam([
            {"params": self.actor.parameters(), "lr": lr},
            {"params": self.critic.parameters(), "lr": lr},
        ])

        # 训练统计
        self.total_updates = 0

    @torch.no_grad()
    def select_actions(
        self,
        obs: np.ndarray,
        action_mask: np.ndarray,
    ) -> Tuple[np.ndarray, np.ndarray, float]:
        """
        Actor 推理：为所有节点选择动作

        Args:
            obs: [num_nodes, obs_dim]
            action_mask: [num_nodes, max_degree]

        Returns:
            actions: [num_nodes] 选择的邻居索引
            log_probs: [num_nodes] 对应 log 概率
            value: float, Critic 估计的 V(s)
        """
        obs_tensor = torch.FloatTensor(obs).to(self.device)
        mask_tensor = torch.FloatTensor(action_mask).to(self.device)

        # Actor 推理
        logits = self.actor(obs_tensor, mask_tensor)  # [N, max_degree]
        dist = Categorical(logits=logits)
        actions = dist.sample()  # [N]
        log_probs = dist.log_prob(actions)  # [N]

        # Critic 推理
        global_obs = obs.flatten()  # [N * obs_dim]
        global_obs_tensor = torch.FloatTensor(global_obs).unsqueeze(0).to(self.device)
        value = self.critic(global_obs_tensor).squeeze().item()

        return (
            actions.cpu().numpy(),
            log_probs.cpu().numpy(),
            value,
        )

    def update(self, buffer_data: dict) -> dict:
        """
        PPO 更新

        Args:
            buffer_data: dict from RolloutBuffer.get_tensors()
                - observations: [T, N, obs_dim]
                - actions: [T, N]
                - log_probs: [T, N]
                - advantages: [T]
                - returns: [T]
                - action_masks: [T, N, max_degree]

        Returns:
            dict: 训练统计信息
        """
        observations = buffer_data["observations"]   # [T, N, obs_dim]
        actions = buffer_data["actions"]             # [T, N]
        old_log_probs = buffer_data["log_probs"]     # [T, N]
        advantages = buffer_data["advantages"]       # [T]
        returns = buffer_data["returns"]             # [T]
        action_masks = buffer_data["action_masks"]   # [T, N, max_degree]

        T, N, _ = observations.shape

        # 优势归一化
        adv_mean = advantages.mean()
        adv_std = advantages.std() + 1e-8
        advantages = (advantages - adv_mean) / adv_std

        total_actor_loss = 0.0
        total_critic_loss = 0.0
        total_entropy = 0.0

        for epoch in range(self.ppo_epochs):
            # 展平为 [T*N, ...] 进行 batch 计算
            obs_flat = observations.reshape(T * N, -1)           # [T*N, obs_dim]
            mask_flat = action_masks.reshape(T * N, -1)          # [T*N, max_degree]
            actions_flat = actions.reshape(T * N)                # [T*N]
            old_lp_flat = old_log_probs.reshape(T * N)           # [T*N]

            # 每个时间步的优势广播到所有节点
            adv_broadcast = advantages.unsqueeze(1).expand(T, N).reshape(T * N)

            # Actor 前向
            logits = self.actor(obs_flat, mask_flat)
            dist = Categorical(logits=logits)
            new_log_probs = dist.log_prob(actions_flat)
            entropy = dist.entropy().mean()

            # PPO clip
            ratio = torch.exp(new_log_probs - old_lp_flat)
            surr1 = ratio * adv_broadcast
            surr2 = torch.clamp(ratio, 1.0 - self.clip_epsilon, 1.0 + self.clip_epsilon) * adv_broadcast
            actor_loss = -torch.min(surr1, surr2).mean()

            # Critic 前向
            global_obs = observations.reshape(T, N * self.obs_dim)  # [T, N*obs_dim]
            values = self.critic(global_obs).squeeze(-1)            # [T]
            critic_loss = F.mse_loss(values, returns)

            # 总损失
            loss = actor_loss + self.value_coef * critic_loss - self.entropy_coef * entropy

            # 反向传播
            self.optimizer.zero_grad()
            loss.backward()
            nn.utils.clip_grad_norm_(
                list(self.actor.parameters()) + list(self.critic.parameters()),
                self.max_grad_norm
            )
            self.optimizer.step()

            total_actor_loss += actor_loss.item()
            total_critic_loss += critic_loss.item()
            total_entropy += entropy.item()

        self.total_updates += 1

        return {
            "actor_loss": total_actor_loss / self.ppo_epochs,
            "critic_loss": total_critic_loss / self.ppo_epochs,
            "entropy": total_entropy / self.ppo_epochs,
            "ppo_epochs": self.ppo_epochs,
        }

    def save(self, path: str):
        """保存模型"""
        torch.save({
            "actor": self.actor.state_dict(),
            "critic": self.critic.state_dict(),
            "optimizer": self.optimizer.state_dict(),
            "total_updates": self.total_updates,
        }, path)

    def load(self, path: str):
        """加载模型"""
        checkpoint = torch.load(path, map_location=self.device)
        self.actor.load_state_dict(checkpoint["actor"])
        self.critic.load_state_dict(checkpoint["critic"])
        self.optimizer.load_state_dict(checkpoint["optimizer"])
        self.total_updates = checkpoint["total_updates"]
