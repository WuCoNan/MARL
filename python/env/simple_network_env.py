"""
Python 轻量级网络路由仿真环境 (Gymnasium 接口)
用于 Task 1.4：验证 MAPPO 训练流程
后续 Milestone 2+ 将替换为 ns3 联合仿真

仿真模型：
- 拓扑：NetworkX 随机几何图 / ER 图
- 队列：每节点一个 FIFO 队列，有容量上限
- 流量：CBR 流（源→目的），每 step 注入固定包
- 路由：每节点选择下一跳邻居转发
- 奖励：吞吐量 - 时延惩罚 - 丢包惩罚
"""

import gymnasium as gym
import numpy as np
import networkx as nx
from gymnasium import spaces
from typing import Optional


class SimpleNetworkEnv(gym.Env):
    """
    多智能体网络路由环境

    观测空间 (per node):
        - queue_len: 队列积压 (归一化)
        - buffer_use: 缓冲区占用率 [0, 1]
        - num_nbrs: 活跃邻居数 (归一化)
        - traffic_load: 本地流量负载 (归一化)
        - is_source: 是否源节点 {0, 1}
        - is_dest: 是否目的节点 {0, 1}
        - avg_link_quality: 平均链路质量 [0, 1]

    动作空间 (per node):
        - 离散动作：选择第 k 个邻居转发 (0 ~ max_degree-1)
        - 无效邻居通过 action_mask 屏蔽

    奖励:
        - R = w1*throughput - w2*avg_delay - w3*loss_rate
    """

    metadata = {"render_modes": ["human"]}

    def __init__(
        self,
        num_nodes: int = 10,
        max_degree: int = 6,
        buffer_capacity: int = 100,
        link_bandwidth: int = 10,
        num_flows: int = 3,
        episode_length: int = 50,
        topology_type: str = "random_geometric",
        connectivity_radius: float = 0.4,
        er_probability: float = 0.4,
        reward_weights: tuple = (1.0, 0.5, 1.0),
        seed: Optional[int] = None,
    ):
        super().__init__()

        self.num_nodes = num_nodes
        self.max_degree = max_degree
        self.buffer_capacity = buffer_capacity
        self.link_bandwidth = link_bandwidth
        self.num_flows = num_flows
        self.episode_length = episode_length
        self.topology_type = topology_type
        self.connectivity_radius = connectivity_radius
        self.er_probability = er_probability
        self.reward_weights = reward_weights

        # 观测维度 (per node)
        self.obs_dim = 7
        # 动作空间：选择邻居索引
        self.action_dim = max_degree

        # Gymnasium spaces (用于兼容性检查)
        self.observation_space = spaces.Box(
            low=0.0, high=1.0,
            shape=(num_nodes, self.obs_dim),
            dtype=np.float32
        )
        self.action_space = spaces.MultiDiscrete(
            [self.action_dim] * num_nodes
        )

        # 内部状态
        self.graph: Optional[nx.Graph] = None
        self.queues = None          # [num_nodes] 当前队列长度
        self.neighbors = None       # [num_nodes] 邻居列表 (padded)
        self.neighbor_mask = None   # [num_nodes, max_degree] 有效邻居 mask
        self.flows = None           # [(src, dst), ...]
        self.current_step = 0
        self.node_positions = None  # [num_nodes, 2]

        # 统计量
        self.total_delivered = 0
        self.total_generated = 0
        self.total_dropped = 0
        self.total_delay = 0.0
        self.delivered_this_step = 0

        # 包追踪 (简化：只记录队列中的包数量)
        self.packet_ages = None  # [num_nodes] 队列中包的平均年龄

        if seed is not None:
            self._rng = np.random.default_rng(seed)
        else:
            self._rng = np.random.default_rng()

    def _generate_topology(self):
        """生成随机网络拓扑（保证连通）"""
        max_attempts = 50
        for attempt in range(max_attempts):
            if self.topology_type == "random_geometric":
                # 随机几何图：节点随机分布，距离 < radius 则相连
                self.node_positions = self._rng.random((self.num_nodes, 2))
                # NetworkX 需要 pos 为 {node_id: position} 字典
                pos_dict = {i: self.node_positions[i] for i in range(self.num_nodes)}
                graph = nx.random_geometric_graph(
                    self.num_nodes,
                    self.connectivity_radius,
                    pos=pos_dict,
                )
            elif self.topology_type == "erdos_renyi":
                graph = nx.erdos_renyi_graph(
                    self.num_nodes,
                    self.er_probability,
                    seed=int(self._rng.integers(0, 2**31))
                )
                self.node_positions = self._rng.random((self.num_nodes, 2))
            else:
                raise ValueError(f"Unknown topology type: {self.topology_type}")

            # 检查连通性，不连通则重试
            if nx.is_connected(graph):
                self.graph = graph
                break
        else:
            # 如果多次尝试仍不连通，取最大连通子图并填充孤立节点
            if not nx.is_connected(graph):
                largest_cc = max(nx.connected_components(graph), key=len)
                subgraph = graph.subgraph(largest_cc).copy()
                # 重建完整图：保留所有节点，孤立节点自环
                self.graph = nx.Graph()
                self.graph.add_nodes_from(range(self.num_nodes))
                mapping = {old: new for new, old in enumerate(sorted(largest_cc))}
                for u, v in subgraph.edges():
                    self.graph.add_edge(mapping.get(u, u), mapping.get(v, v))
                # 确保孤立节点至少有一个连接（连到节点 0）
                for i in range(self.num_nodes):
                    if self.graph.degree(i) == 0 and i != 0:
                        self.graph.add_edge(i, 0)
            else:
                self.graph = graph

        # 构建邻居表 (padded to max_degree)
        self.neighbors = np.zeros((self.num_nodes, self.max_degree), dtype=np.int64)
        self.neighbor_mask = np.zeros((self.num_nodes, self.max_degree), dtype=np.float32)

        for i in range(self.num_nodes):
            nbrs = list(self.graph.neighbors(i))
            # 如果邻居数超过 max_degree，随机截断
            if len(nbrs) > self.max_degree:
                nbrs = list(self._rng.choice(nbrs, self.max_degree, replace=False))
            for j, nbr in enumerate(nbrs):
                self.neighbors[i, j] = nbr
                self.neighbor_mask[i, j] = 1.0

    def _generate_flows(self):
        """生成随机流量流 (源-目的对)"""
        self.flows = []
        available_nodes = list(range(self.num_nodes))
        num_flows = min(self.num_flows, self.num_nodes // 2)

        for _ in range(num_flows):
            if len(available_nodes) < 2:
                break
            src, dst = self._rng.choice(available_nodes, 2, replace=False)
            self.flows.append((int(src), int(dst)))
            # 允许节点参与多条流

    def _get_obs(self) -> np.ndarray:
        """构建所有节点的观测矩阵 [num_nodes, obs_dim]"""
        obs = np.zeros((self.num_nodes, self.obs_dim), dtype=np.float32)

        for i in range(self.num_nodes):
            obs[i, 0] = self.queues[i] / self.buffer_capacity  # queue_len (归一化)
            obs[i, 1] = self.queues[i] / self.buffer_capacity  # buffer_use
            obs[i, 2] = np.sum(self.neighbor_mask[i]) / self.max_degree  # num_nbrs
            # traffic_load: 源节点有额外负载
            is_src = any(i == src for src, dst in self.flows)
            obs[i, 3] = 1.0 if is_src else 0.0  # traffic_load
            obs[i, 4] = 1.0 if is_src else 0.0  # is_source
            obs[i, 5] = 1.0 if any(i == dst for src, dst in self.flows) else 0.0  # is_dest
            # avg_link_quality: 基于距离
            nbr_count = int(np.sum(self.neighbor_mask[i]))
            if nbr_count > 0 and self.node_positions is not None:
                nbr_indices = self.neighbors[i, :nbr_count]
                dists = np.linalg.norm(
                    self.node_positions[nbr_indices] - self.node_positions[i], axis=1
                )
                avg_dist = np.mean(dists)
                obs[i, 6] = 1.0 - min(avg_dist / self.connectivity_radius, 1.0)
            else:
                obs[i, 6] = 0.0

        return obs

    def _get_action_mask(self) -> np.ndarray:
        """返回动作 mask [num_nodes, max_degree]，1=有效，0=无效"""
        return self.neighbor_mask.copy()

    def reset(self, seed=None, options=None):
        """重置环境"""
        if seed is not None:
            self._rng = np.random.default_rng(seed)

        # 重新生成拓扑（每个 episode 随机化）
        self._generate_topology()
        self._generate_flows()

        # 初始化队列
        self.queues = np.zeros(self.num_nodes, dtype=np.float32)
        self.packet_ages = np.zeros(self.num_nodes, dtype=np.float32)

        # 重置统计
        self.current_step = 0
        self.total_delivered = 0
        self.total_generated = 0
        self.total_dropped = 0
        self.total_delay = 0.0
        self.delivered_this_step = 0

        obs = self._get_obs()
        info = {
            "action_mask": self._get_action_mask(),
            "num_nodes": self.num_nodes,
            "num_flows": len(self.flows),
            "flows": self.flows,
        }

        return obs, info

    def step(self, actions: np.ndarray):
        """
        执行一步

        Args:
            actions: [num_nodes] 每个节点选择的邻居索引 (0 ~ max_degree-1)

        Returns:
            obs, reward, terminated, truncated, info
        """
        self.current_step += 1
        self.delivered_this_step = 0

        # 1. 流量注入（源节点生成包）
        for src, dst in self.flows:
            inject_amount = self.link_bandwidth * 0.5  # 每 step 注入带宽的一半
            self.queues[src] += inject_amount
            self.total_generated += inject_amount

        # 2. 路由转发（根据 action 分配流量）
        new_queues = self.queues.copy()
        forwarded = np.zeros(self.num_nodes, dtype=np.float32)
        received = np.zeros(self.num_nodes, dtype=np.float32)

        for i in range(self.num_nodes):
            if self.queues[i] <= 0:
                continue

            action_idx = int(actions[i])

            # 检查 action 是否有效
            if self.neighbor_mask[i, action_idx] == 0:
                # 无效动作：包留在队列中（相当于不转发）
                continue

            next_hop = self.neighbors[i, action_idx]

            # 转发量 = min(队列量, 链路带宽)
            forward_amount = min(self.queues[i], self.link_bandwidth)
            forwarded[i] = forward_amount

            # 检查下一跳是否是某条流的目的节点
            is_delivered = any(next_hop == dst and i == src or
                              (next_hop == dst) for src, dst in self.flows
                              if self._is_on_path(i, next_hop, dst))

            # 简化：如果下一跳是目的节点，算作送达
            if any(next_hop == dst for _, dst in self.flows):
                # 送达
                delivered_amount = forward_amount
                self.total_delivered += delivered_amount
                self.delivered_this_step += delivered_amount
                self.total_delay += self.packet_ages[i] * delivered_amount
                new_queues[i] -= forward_amount
            else:
                # 转发到下一跳的队列
                received[next_hop] += forward_amount
                new_queues[i] -= forward_amount

        # 3. 更新队列（接收转发的包）
        new_queues += received

        # 4. 丢包（超出缓冲区容量）
        for i in range(self.num_nodes):
            if new_queues[i] > self.buffer_capacity:
                dropped = new_queues[i] - self.buffer_capacity
                self.total_dropped += dropped
                new_queues[i] = self.buffer_capacity

        # 5. 更新包年龄
        for i in range(self.num_nodes):
            if new_queues[i] > 0:
                self.packet_ages[i] += 1.0
            else:
                self.packet_ages[i] = 0.0

        self.queues = new_queues

        # 6. 计算奖励
        reward = self._compute_reward()

        # 7. 判断是否结束
        terminated = False
        truncated = self.current_step >= self.episode_length

        obs = self._get_obs()
        info = {
            "action_mask": self._get_action_mask(),
            "step_delivered": self.delivered_this_step,
            "total_delivered": self.total_delivered,
            "total_dropped": self.total_dropped,
            "total_generated": self.total_generated,
            "avg_queue": np.mean(self.queues),
        }

        return obs, reward, terminated, truncated, info

    def _is_on_path(self, node, next_hop, dst) -> bool:
        """简化判断：next_hop 是否比 node 更接近 dst"""
        if self.node_positions is None:
            return True
        dist_node = np.linalg.norm(self.node_positions[node] - self.node_positions[dst])
        dist_next = np.linalg.norm(self.node_positions[next_hop] - self.node_positions[dst])
        return dist_next < dist_node

    def _compute_reward(self) -> float:
        """
        计算全局奖励
        R = w1*throughput - w2*avg_delay - w3*loss_rate
        """
        w1, w2, w3 = self.reward_weights

        # 吞吐量：本 step 送达的包量 (归一化)
        max_throughput = self.link_bandwidth * len(self.flows)
        throughput = self.delivered_this_step / max(max_throughput, 1.0)

        # 平均时延：队列中包的平均年龄 (归一化)
        total_queue = np.sum(self.queues)
        if total_queue > 0:
            avg_delay = np.sum(self.packet_ages * self.queues) / total_queue
            avg_delay = avg_delay / self.episode_length  # 归一化
        else:
            avg_delay = 0.0

        # 丢包率
        if self.total_generated > 0:
            loss_rate = self.total_dropped / self.total_generated
        else:
            loss_rate = 0.0

        reward = w1 * throughput - w2 * avg_delay - w3 * loss_rate
        return float(reward)

    def render(self):
        """打印当前状态"""
        print(f"Step {self.current_step}/{self.episode_length} | "
              f"Queues: {self.queues.astype(int)} | "
              f"Delivered: {self.total_delivered:.0f} | "
              f"Dropped: {self.total_dropped:.0f}")

    def get_action_mask(self) -> np.ndarray:
        """获取当前动作 mask（供外部调用）"""
        return self._get_action_mask()
