# Task A.1：多路径 MANET 路由文献调研报告

> 调研时间：2026-07-21
> 调研范围：多路径路由（DSDV改进）、节点不相交路径、GNN/RL路由、2-hop观测
> 目的：为 Milestone 2 Phase B 算法设计提供文献依据

---

## 一、技术路线分类总览

已有工作可分为 4 条技术路线：

- 路线A：扩展主动式路由表（DSDV系列改进）
- 路线B：按需多路径发现（AOMDV系列）
- 路线C：GNN/RL驱动路由决策
- 路线D：2-hop信息利用

---

## 二、路线A：扩展主动式路由表（DSDV系列）

### 论文1：MDSDV — Ad Hoc网络中多径路由协议的研究

- 作者：成小惠, 麦力军
- 来源：南京邮电大学硕士学位论文, 2008
- 期刊/会议：硕士论文（NS-2仿真验证）

**核心方法：**

在DSDV基础上提出MDSDV（Multipath DSDV），核心改动：

1. 路由表扩展：每个目的节点存储多条路径（而非仅最优一条）
2. 多路径获取方式：
   - 源节点准备通信时，向邻居发送"路由探测包"
   - 邻居节点收到后，反馈自己路由表中到目的的路由信息
   - 源节点收集所有邻居反馈，组装多条独立路径
3. 路径独立性保证：要求不同路径的第一跳（下一跳）不同
4. 路径选择：从多条路径中选跳数最短的为主路径，其余为备用

**算法框架：**

```
MDSDV算法流程：
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
初始化阶段（与标准DSDV相同）：
  每个节点周期性广播路由更新包
  路由表存储：目的地址 | 下一跳 | 跳数 | 序列号

多路径发现阶段（新增）：
  1. 源节点S有数据要发给目的D
  2. S向所有邻居广播 RouteProbe(D) 包
  3. 邻居N收到后：
     - 查自己路由表到D的条目
     - 回复 RouteReply(D, next_hop_N, hops_N, seq_N)
  4. S收集所有Reply，构建路径集：
     paths = {(N1, hops1), (N2, hops2), ...}
  5. 筛选：去除下一跳相同的路径（保证第一跳不相交）
  6. 排序：按跳数升序，第一条为主路径

数据转发阶段：
  主路径失效 → 切换到备用路径
  所有路径失效 → 重新触发多路径发现
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**优点：** 实现简单，与DSDV兼容，探测仅在需要时触发
**缺点：** 探测包增加额外开销；不相交约束弱（仅第一跳不同）；路径数受限于邻居数

---

### 论文2：HMP — 基于剩余带宽的逐跳多路径路由协议

- 作者：Pham P., Perreau S.
- 来源：4th International Workshop on Mobile and Wireless Communications Network, 2002
- 补充来源：CSDN技术博客详细解析

**核心方法：**

直接修改DSDV路由表结构，零额外控制包：

1. 路由表改造：
   - 标准DSDV：每目的存1个下一跳
   - HMP：每目的存所有"最小跳数的下一跳"（多个）
2. 邻居表新增：
   - 存储每个邻居的剩余带宽信息
   - 周期性交换（搭载在DSDV路由更新中）
3. 转发决策：
   - 从多个下一跳中，选剩余带宽最大的那个
   - 实现逐跳负载均衡

**算法框架：**

```
HMP算法流程：
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
路由表结构（改造后）：
  目的D: {next_hop_1: hops=2, bw=50%}
         {next_hop_2: hops=2, bw=30%}
         {next_hop_3: hops=3, bw=80%}  ← 跳数大1，不选

路由更新处理（修改DSDV更新逻辑）：
  收到邻居N的更新包：
    for 每个目的D in 更新包:
      new_hops = N.hops_to_D + 1
      if new_hops < 当前最小跳数:
        清空多路径列表
        添加 (N, new_hops)
      elif new_hops == 当前最小跳数:
        添加 (N, new_hops)  ← 关键：保留所有等跳数下一跳

转发决策：
  查路由表到目的D → 得到候选下一跳集合
  查邻居表 → 获取各候选的剩余带宽
  选择：argmax(剩余带宽) 作为本包下一跳
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**优点：** 零额外控制包（利用已有DSDV更新）；实现极简
**缺点：** 路径多样性受限于"等跳数"约束；无法发现跳数不同但质量更好的路径

---

### 论文3：EA-MDSDV — 能量感知多径DSDV

- 作者：成小惠, 麦力军
- 来源：同论文1硕士论文中的第三个协议
- 年份：2008

**核心方法：**

在MDSDV基础上引入能量权重：

1. 路径度量从"跳数"改为"加权代价"：
   cost = α × hops + β × (1/剩余能量)
2. 路径选择时优先避开低能量节点
3. 多路径中能量最低的路径标记为"保护路径"（非紧急不用）

**算法框架：**

```
EA-MDSDV = MDSDV + 能量感知权重
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
与MDSDV区别仅在路径排序阶段：
  标准MDSDV: sort by hops (升序)
  EA-MDSDV:  sort by cost = α*hops + β*(1/E_residual)

路由更新包新增字段：
  原始DSDV: [dest, seq, hops]
  EA-MDSDV: [dest, seq, hops, E_residual]  ← 新增能量字段
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**优点：** 延长网络寿命，公平使用节点能量
**缺点：** 需跨层获取能量信息；能量字段增加更新包大小

---

## 三、路线B：按需多路径发现（AOMDV系列）

### 论文4：AOMDV — Ad Hoc On-demand Multipath Distance Vector

- 作者：Marina M.K., Das S.R.
- 来源：ACM MobiHoc 2001 / Ad Hoc Networks Journal, 2003
- 被引量：2000+（多路径路由领域最经典）

**核心方法：**

扩展AODV的路由发现过程，一次RREQ/RREP收集多条路径：

1. RREQ泛洪改造：
   - 标准AODV：中间节点只转发第一次收到的RREQ
   - AOMDV：中间节点可转发多次（但每个节点对同一RREQ只接受最短路径）
2. RREP收集：
   - 目的节点收到多条RREQ → 沿不同路径回复多个RREP
   - 每个RREP携带完整路径信息
3. 不相交约束：
   - 链路不相交（默认）：两条路径不共享任何链路
   - 节点不相交（可选）：两条路径不共享任何中间节点
4. 路由表结构：
   - 每目的维护一个"下一跳列表" + 每个下一跳对应的完整路径

**算法框架：**

```
AOMDV多路径发现：
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
源节点S发起路由发现：
  1. S广播 RREQ(src=S, dest=D, seq, hop_count=0, path=[S])

中间节点I收到RREQ：
  2. 检查：是否已存在到D的更新路由？
     - 是 → 丢弃
     - 否 → 记录"反向路径"（到S的路径）
  3. 检查：本节点是否已在path中？（防环）
     - 是 → 丢弃
     - 否 → hop_count++, path.append(I), 继续广播

目的节点D收到RREQ：
  4. 沿该RREQ的反向路径单播 RREP(path)
  5. 可回复多个RREP（给不同方向的RREQ）

源节点S收到多个RREP：
  6. 提取各RREP的路径
  7. 不相交检查：
     for 新路径p in RREPs:
       if p与已有路径节点不相交:
         添加到路由表
       else:
         丢弃（或存为链路不相交备用）

结果：路由表[D] = {path1: S→A→D, path2: S→B→D, ...}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**优点：** 经典成熟；一次发现多条路径；链路/节点不相交可选
**缺点：** 按需式延迟大（首次通信需等路由发现）；泛洪开销大；不适合主动式场景

---

### 论文5：ENDMR — Energy-saving Node-Disjoint Multipath Routing

- 作者：Zhang B., Zhu G., Zhang H., Wang M.
- 来源：IEEE ESIAT 2009 (Environmental Science and Information Application Technology)
- DOI：10.1109/ESIAT.2009.193

**核心方法：**

三重机制减少开销 + 保证节点不相交：

1. 地理信息限制泛洪：
   - 利用节点GPS坐标，只在"源→目的方向扇形区域"内泛洪RREQ
   - 减少无效泛洪50%+
2. 移动性预测排除短命链路：
   - 根据节点速度和方向，预测链路存活时间
   - 存活时间 < 阈值的节点不转发RREQ
3. 严格节点不相交：
   - 路径中除源和目的外，不共享任何中间节点

**算法框架：**

```
ENDMR = 受限泛洪 + 链路稳定性预测 + 节点不相交
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
1. 源S计算到目的D的方向角θ和距离d
2. 泛洪RREQ时附带(θ, d, 扇形角α)
3. 中间节点I收到RREQ：
   a. 计算自己是否在扇形区域内？
      - 否 → 丢弃
   b. 计算与上一跳的链路存活时间T_life
      - T_life < threshold → 丢弃（不转发）
   c. 否则继续转发
4. 目的D收到多条RREQ → 回复RREP
5. 源S筛选节点不相交路径
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**优点：** 节能；路径稳定性高；严格不相交
**缺点：** 依赖GPS/定位；扇形限制可能错过好路径；按需式仍有首次延迟

---

## 四、路线C：GNN/RL驱动路由决策

### 论文6：Packet Routing with Graph Attention Multi-Agent RL

- 作者：Mai X., Fu Q., Chen Y.
- 来源：IEEE GLOBECOM 2021, Madrid, Spain, pp.1-6
- DOI：10.1109/GLOBECOM46510.2021

**核心方法（与我们最接近）：**

每个路由器为一个agent，用GAT聚合邻居信息做逐跳路由决策：

1. 图建模：
   - 节点 = 路由器
   - 边 = 物理链路
   - 节点特征 = [队列长度, 邻居数, 已服务包数, 平均等待时间]
2. GAT编码：
   - 每个agent用Graph Attention Network聚合邻居特征
   - attention权重自动学习"哪个邻居更重要"
3. 动作空间：
   - 离散：选择哪个邻居作为下一跳
4. 奖励：
   - 全局：网络总吞吐量
   - 局部：本节点成功转发包数
5. 训练：
   - 多智能体PPO（参数共享）

**算法框架：**

```
GAT-MARL路由（Mai et al.）：
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
每个时间步：
  1. 每个节点i收集本地观测：
     obs_i = [queue_len, num_neighbors, served_pkts, avg_wait]

  2. 构建局部图（i + 其邻居）：
     node_features = [obs_i; obs_j1; obs_j2; ...]
     edge_index = 邻接关系

  3. GAT前向：
     h_i = GAT(node_features, edge_index)
     → 输出每个邻居的attention权重
     → 聚合得到节点i的增强表征

  4. 策略头：
     logits = MLP(h_i)  → [num_neighbors]
     action = Categorical(logits).sample()  → 选第k个邻居

  5. 执行转发，计算reward

  6. MAPPO更新（参数共享）：
     L = L_actor + c1*L_critic - c2*entropy
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**与我们的关联：** 这篇最接近我们的第4-5章设计，但它没有：
- 底层DSDV多路径支持（它的图是固定的）
- 2-hop观测（它只用1-hop邻居）
- 差分奖励（它用全局/局部reward）

---

### 论文7：Packet Routing Against Network Congestion: Deep MARL

- 作者：Ding R., Yang Y., Liu J., Li H., Gao F.
- 来源：IEEE ICNC 2020 (International Conference on Computing, Networking and Communications)
- DOI：10.1109/ICNC47757.2020.9049759

**核心方法：**

无GNN的纯DQN多智能体路由（作为我们的基线参考）：

1. 每个路由器为agent
2. 观测：本地队列状态 + 邻居路由器状态
3. 修改DQN：评估每个邻居路由器的"价值"
4. 选下一跳 = argmax(Q(neighbor))
5. 分布式训练，无参数共享

**算法框架：**

```
Deep MARL路由（Ding et al.）：
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
每个路由器i：
  obs_i = [本节点队列, 各邻居队列, 各邻居链路利用率]

  Q网络：
    Q(obs_i, neighbor_j) → 选neighbor_j的期望回报

  动作：
    next_hop = argmax_j Q(obs_i, neighbor_j)

  奖励：
    r = -平均端到端时延（全局）

  训练：
    独立DQN，每个agent单独更新
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**与我们的关联：** 作为消融实验基线（无GNN的MARL路由）

---

### 论文8：GAT-based MADRL for FANET Routing

- 作者：（飞行自组网路由优化）
- 来源：IEEE IoT Journal / AMiner 2024-2025
- 关键词：Flying ad-hoc networks, GAT, Multi-Agent DRL, end-to-end latency

**核心方法：**

面向飞行自组网（FANET），GAT编码动态拓扑 + MADRL优化路由+调度：

1. 网络拓扑建模为图，节点=UAV
2. GAT层聚合邻居UAV的状态（位置、速度、队列）
3. 多智能体：每个UAV为agent
4. 联合优化：路由选择 + 时隙调度
5. 目标：最小化最大端到端时延

**与我们的关联：** 验证了GAT对高动态拓扑的适应性（FANET比MANET动态性更强）

---

### 论文9：GAPPO — Graph Attention RL based Robust Routing

- 作者：Li X., Xiao Y., Liu S., Lu X., Liu F., Zhou W., Liu J.
- 来源：IEEE PIMRC 2023, Toronto, Canada, pp.1-7
- DOI：10.1109/PIMRC56738.2023

**核心方法：**

图注意力 + PPO 做鲁棒路由：

1. 图注意力网络编码当前网络拓扑
2. PPO策略梯度更新路由决策
3. 鲁棒性：对链路故障/拓扑突变保持性能
4. 在多种网络规模上训练，验证泛化性

**与我们的关联：** 验证了PPO+GAT组合在路由中的有效性（我们用MAPPO，是PPO的多智能体版本）

---

### 论文10：Learning and Generating Distributed Routing Protocols

- 作者：Geyer F., Carle G.
- 来源：ACM CoNEXT Workshop 2020 / arXiv
- 补充：Graph-based Deep Learning for Communication Networks

**核心方法：**

用GNN直接"学习"一个分布式路由协议：

1. 每个节点运行相同的GNN
2. 输入：邻居的转发表信息
3. 输出：本节点的转发表更新
4. 训练目标：让学到的协议接近OSPF/DSDV的性能
5. 发现：GNN能自动学到类似距离矢量的行为

**与我们的关联：** 证明GNN可以替代/增强传统路由协议逻辑

---

## 五、路线D：2-hop信息利用

### 论文11：OLSR MPR — Multipoint Relays

- 作者：Clausen T., Jacquet P.
- 来源：RFC 3626 (IETF), 2003
- 被引量：5000+（MANET领域最经典RFC之一）

**核心方法：**

利用2-hop邻居信息选择最小广播中继集：

1. 每个节点通过Hello包获知：
   - 1-hop邻居集合 N1
   - 2-hop邻居集合 N2（邻居的邻居）
2. MPR选择算法：
   - 从N1中选最小子集S，使得N2中所有节点都被S覆盖
   - 即：N2中每个节点都至少是S中某个节点的邻居
3. 广播时：只有被选为MPR的节点转发TC消息

**算法框架：**

```
OLSR MPR选择：
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
输入：N1 = {1-hop邻居}, N2 = {2-hop邻居}
      对每个n1∈N1, 知道它能覆盖N2中的哪些节点

贪心算法：
  MPR_set = {}
  uncovered = N2（全部未覆盖）

  while uncovered ≠ ∅:
    选择 n1* = argmax_{n1∈N1} |n1能覆盖的uncovered节点数|
    MPR_set.add(n1*)
    uncovered -= n1*覆盖的节点

结果：只有MPR_set中的节点转发广播消息
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

**与我们的关联：** 2-hop信息获取机制（Hello包交换）可直接借鉴到我们的观测收集

---

### 论文12：T-Hello — 两跳信标交换协议

- 作者：（GPSR改进）
- 来源：计算机应用, 2010, 30(12)
- 关键词：地理位置路由, 邻居表维护, 2-hop

**核心方法：**

扩展Hello协议到2-hop范围：

1. 标准Hello：节点只知道1-hop邻居是否在范围内
2. T-Hello：节点知道2-hop范围内所有节点的位置
3. 好处：
   - 能显式感知邻居是否移出通信范围
   - 不必等到超时才发现链路断裂
   - 过期节点存在时间缩短50%

**与我们的关联：** 2-hop Hello包机制可用于我们的观测更新

---

### 论文13：NSI — Neighborhood State Index for WSN

- 作者：（WSN路由优化）
- 来源：Computer Communications, 2022
- 关键词：two-hop information, preemptive routing, energy

**核心方法：**

节点预览2-hop路径状态做前向决策：

1. 每个节点维护"邻居状态索引"(NSI)：
   - 包含2-hop范围内节点的：剩余能量、队列状态、链路质量
2. 转发决策时：
   - 不只看下一跳，还看下一跳的下一跳
   - 选择"2-hop路径综合质量最优"的邻居
3. 状态获取：周期性状态交换包

**与我们的关联：** 2-hop状态（队列+链路质量）正是我们GNN需要的边特征/节点特征

---

## 六、关键发现与空白分析

### 已有工作做了什么：

1. DSDV多路径：仅2-3篇（MDSDV, HMP, EA-MDSDV），且都是2008年前后
2. AOMDV多路径：研究充分，但是按需式，不适合我们的主动式场景
3. GNN+MARL路由：2021年开始兴起，但都用固定图/随机图，不关心底层路由协议
4. 2-hop信息：主要用于OLSR广播优化和邻居表维护，未面向RL/GNN设计

### 没人做什么（研究空白）：

1. ❌ 主动式DSDV + 不相交多路径 + 为RL提供候选动作空间
2. ❌ 2-hop观测 + 面向GNN输入设计（节点特征+边特征+图结构）
3. ❌ 多路径DSDV + GNN-MAPPO 的完整闭环
4. ❌ 差分奖励在多路径路由调度中的应用

### 我们的差异化：

```
现有工作的断层：
  路由协议研究 ←→ RL/GNN研究
  （改协议不管RL）  （做RL不管协议）

我们的桥梁：
  改进DSDV（提供多路径+2-hop观测）
       ↓ 结构化观测
  GNN-MAPPO（利用多路径做流量调度）
       ↓ 路由决策
  改进DSDV（执行决策，反馈奖励）
```

---

## 七、对Phase B的建议

### 推荐方案方向：

**方案C（混合式）最可行：**

1. 多路径发现：修改DSDV路由更新包，搭载多下一跳信息（HMP思路，零额外包）
2. 不相交约束：下一跳不同（节点不相交的最弱形式，保证路径多样性）
3. 2-hop观测：在DSDV周期性更新中附带2-hop邻居摘要（借鉴OLSR Hello机制）
4. 与ns3-ai接口：每个time slot，从路由表+2-hop表构建PyG图传给Python

### 需要Phase B确认的问题：

1. 多路径K值：固定K=3 还是 动态（有多少存多少）？
2. 不相交粒度：仅下一跳不同 vs 全路径节点不相交？
3. 2-hop观测内容：仅拓扑连接 vs 含队列/链路质量？
4. 观测更新频率：每10ms time slot vs 与DSDV更新周期对齐？
5. 路由更新包新增字段的大小限制？

---

## 八、完整参考文献列表

[1] 成小惠, 麦力军. Ad Hoc网络中多径路由协议的研究[D]. 南京邮电大学, 2008.
[2] Pham P., Perreau S. Multi-path routing protocol with load balancing policy in mobile ad hoc network[C]. 4th Int. Workshop on Mobile and Wireless Communications Network, 2002.
[3] Marina M.K., Das S.R. Ad hoc on-demand multipath distance vector routing[C]. ACM MobiHoc, 2001.
[4] Zhang B., et al. Energy-saving based on Node-Disjoint Multipath Routing Algorithm for Ad Hoc Networks[C]. IEEE ESIAT, 2009.
[5] 李超, 等. 基于MSR的Ad Hoc网络多径路由算法MDMSR[C]. 2007.
[6] Mai X., Fu Q., Chen Y. Packet routing with graph attention multi-agent reinforcement learning[C]. IEEE GLOBECOM, 2021: 1-6.
[7] Ding R., et al. Packet Routing Against Network Congestion: A Deep Multi-agent Reinforcement Learning Approach[C]. IEEE ICNC, 2020.
[8] Li X., et al. GAPPO-A Graph Attention Reinforcement Learning based Robust Routing Algorithm[C]. IEEE PIMRC, 2023: 1-7.
[9] Geyer F., Carle G. Learning and generating distributed routing protocols using graph-based deep learning[C]. ACM CoNEXT Workshop, 2020.
[10] Rusek K., et al. RouteNet: Leveraging Graph Neural Networks for network modeling and optimization in SDN[J]. IEEE JSAC, 2020, 38(10): 2260-2270.
[11] Clausen T., Jacquet P. Optimized Link State Routing Protocol (OLSR)[S]. RFC 3626, 2003.
[12] 两跳信标交换协议(T-Hello)[J]. 计算机应用, 2010, 30(12).
[13] Real-time optimizations in energy profiles and end-to-end delay in WSN using two-hop information[J]. Computer Communications, 2022.
[14] Almasan P., et al. Deep reinforcement learning meets graph neural networks: Exploring a routing optimization use case[J]. Computer Networks, 2022.
[15] Xu Z., et al. MARL-JR: Multi-Agent Reinforcement Learning-Based Joint Routing[J]. 2025.
[16] Alliche R.A., et al. PRISMA: A Packet Routing Simulator for Multi-Agent Reinforcement Learning[C]. IFIP Networking, 2022.
[17] 移动Ad Hoc网络多路径路由协议研究[D]. 硕士论文（AOMDV_DAF + PABR）.
[18] Deep Reinforcement Learning for Routing and Scheduling Optimization in FANET-Assisted IoT Networks[J]. IEEE, 2024/2025.
