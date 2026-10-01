"""
经验回放池 (Rollout Buffer) 用于 MAPPO
采用 on-policy 的 rollout 收集方式，每个 episode 结束后进行 PPO 更新
"""

import numpy as np
import torch
from typing import Optional


class RolloutBuffer:
    """
    MAPPO Rollout Buffer

    存储一个 episode（或固定步数）内的所有交互数据：
    - observations: [T, N, obs_dim]
    - actions: [T, N]
    - log_probs: [T, N]
    - rewards: [T]  (全局奖励)
    - values: [T]    (Critic 估计)
    - dones: [T]
    - action_masks: [T, N, max_degree]
    """

    def __init__(
        self,
        max_steps: int,
        num_nodes: int,
        obs_dim: int,
        max_degree: int,
        gamma: float = 0.99,
        gae_lambda: float = 0.95,
    ):
        self.max_steps = max_steps
        self.num_nodes = num_nodes
        self.obs_dim = obs_dim
        self.max_degree = max_degree
        self.gamma = gamma
        self.gae_lambda = gae_lambda

        # 预分配存储
        self.observations = np.zeros((max_steps, num_nodes, obs_dim), dtype=np.float32)
        self.actions = np.zeros((max_steps, num_nodes), dtype=np.int64)
        self.log_probs = np.zeros((max_steps, num_nodes), dtype=np.float32)
        self.rewards = np.zeros(max_steps, dtype=np.float32)
        self.values = np.zeros(max_steps, dtype=np.float32)
        self.dones = np.zeros(max_steps, dtype=np.float32)
        self.action_masks = np.zeros((max_steps, num_nodes, max_degree), dtype=np.float32)

        # GAE 相关
        self.advantages = np.zeros(max_steps, dtype=np.float32)
        self.returns = np.zeros(max_steps, dtype=np.float32)

        self.ptr = 0
        self.full = False

    def add(
        self,
        obs: np.ndarray,
        action: np.ndarray,
        log_prob: np.ndarray,
        reward: float,
        value: float,
        done: bool,
        action_mask: np.ndarray,
    ):
        """添加一步数据"""
        if self.ptr >= self.max_steps:
            raise RuntimeError("Buffer is full, call compute_returns() and clear() first")

        self.observations[self.ptr] = obs
        self.actions[self.ptr] = action
        self.log_probs[self.ptr] = log_prob
        self.rewards[self.ptr] = reward
        self.values[self.ptr] = value
        self.dones[self.ptr] = float(done)
        self.action_masks[self.ptr] = action_mask

        self.ptr += 1

    def compute_returns(self, last_value: float = 0.0):
        """
        计算 GAE 优势和回报

        Args:
            last_value: 最后一步的 Critic 估计（如果 episode 未结束）
        """
        last_gae = 0.0
        for t in reversed(range(self.ptr)):
            if t == self.ptr - 1:
                next_value = last_value
                next_non_terminal = 1.0 - self.dones[t]
            else:
                next_value = self.values[t + 1]
                next_non_terminal = 1.0 - self.dones[t]

            delta = self.rewards[t] + self.gamma * next_value * next_non_terminal - self.values[t]
            last_gae = delta + self.gamma * self.gae_lambda * next_non_terminal * last_gae
            self.advantages[t] = last_gae

        self.returns[:self.ptr] = self.advantages[:self.ptr] + self.values[:self.ptr]

    def get_tensors(self, device: torch.device = torch.device("cpu")):
        """
        将 buffer 数据转为 PyTorch tensor

        Returns:
            dict of tensors
        """
        return {
            "observations": torch.FloatTensor(self.observations[:self.ptr]).to(device),
            "actions": torch.LongTensor(self.actions[:self.ptr]).to(device),
            "log_probs": torch.FloatTensor(self.log_probs[:self.ptr]).to(device),
            "advantages": torch.FloatTensor(self.advantages[:self.ptr]).to(device),
            "returns": torch.FloatTensor(self.returns[:self.ptr]).to(device),
            "values": torch.FloatTensor(self.values[:self.ptr]).to(device),
            "action_masks": torch.FloatTensor(self.action_masks[:self.ptr]).to(device),
        }

    def clear(self):
        """清空 buffer"""
        self.ptr = 0
        self.full = False

    @property
    def size(self) -> int:
        return self.ptr

    @property
    def is_full(self) -> bool:
        return self.ptr >= self.max_steps
