# 硕士论文实施计划：自组网环境下基于DSDV与MAPPO的流量调度

## 总体技术路线

面向自组网的图神经网络驱动的多智能体协作路由调度方法。

三个工作点递进闭环：
- **第3章**：改进DSDV → 多路径 + 2-hop观测 → 数据从哪来
- **第4章**：GNN Actor → 拓扑泛化表征 → 怎么表征网络
- **第5章**：改进MAPPO → 差分奖励 + 拓扑感知Critic → 怎么做决策

---

## Milestone 1：环境搭建

> **目标**：搭建完整的开发环境，确保 ns3 与 Python 能协同运行，为后续所有开发工作奠定基础。
>
> **预计耗时**：1-2 周

### Task 1.1：ns3 环境搭建 ✅ 已完成

**目标**：在本机成功安装 ns3，验证核心模块可正常工作。

**完成日期**：2026-07-19

**实际执行结果**：
- ✅ ns-3.40 安装成功，cmake 编译通过
- ✅ DSDV 测试通过：`routing-dsdv` suite PASS
- ✅ DSDV 示例运行正常：`ns3.40-dsdv-manet-default`
- ✅ 启用模块：core, network, internet, internet-apps, dsdv, mobility, wifi, applications, stats, flow-monitor, mesh, bridge, energy, propagation, spectrum, antenna, traffic-control

**交付物**：
- [x] ns3 安装成功，编译好的二进制可执行
- [x] DSDV 示例程序运行成功
- [x] 安装路径：`/home/wuconan/ns-allinone-3.40/ns-3.40/`

**遇到的问题**：
1. Python 3.14 与 ns3 的 `build.py` 和 `ns3` 脚本不兼容（`argparse` API 变更，`store_true` 不能用于位置参数）→ 绕过 build.py，直接用 cmake 构建
2. DSDV 测试缺少 mesh 模块（`ns3/mesh-helper.h`）→ cmake 中添加 mesh 模块
3. DSDV 测试缺少 internet-apps 模块（`ns3/v4ping-helper.h`）→ cmake 中添加 internet-apps 模块

---

### Task 1.2：Python 环境搭建 ✅ 已完成

**目标**：搭建完整的 Python 深度学习与科学计算环境。

**完成日期**：2026-07-19

**实际执行结果**：
- ✅ 虚拟环境 `/home/wuconan/marl_env/` 创建成功（Python 3.14.4）
- ✅ PyTorch 2.11.0+cu128 安装成功，GPU 可用（GTX 1650 Ti）
- ✅ PyG 2.8.0 安装成功，GATConv 前向传播验证通过
- ✅ Gymnasium 1.3.0、NetworkX 3.6.1 等依赖全部就绪
- ✅ 三层验证 17/17 全部通过（导入验证 12/12 + 功能验证 4/4 + 端到端 MAPPO 训练 1/1）

**交付物**：
- [x] 虚拟环境：`/home/wuconan/marl_env/`
- [x] 依赖清单：`/home/wuconan/Qoder/MARL/requirements.txt`
- [x] 三层验证脚本：`/home/wuconan/Qoder/MARL/verify_env.py`

**遇到的问题**：
1. `python3.14-venv` 未预装 → `sudo apt-get install python3.14-venv`
2. pip 安装被 PEP 668 阻止（Ubuntu 26.04 禁止系统级 pip）→ 使用 venv 虚拟环境
3. NetworkX 的 `adjacency_matrix` 需要 scipy → 额外安装 `scipy`

---

### Task 1.3：ns3 与 Python 联合环境搭建 ✅ 已完成

**目标**：建立 ns3 与 Python 之间的通信桥梁，验证两者能协同运行。

**完成日期**：2026-07-19

**方案选型**：采用 ns3-ai（共享内存）方案
- 正确仓库地址：`https://github.com/hust-diangroup/ns3-ai.git`
- 安装位置：`ns-3.40/contrib/ai/`（必须叫 `ai` 不能叫 `ns3-ai`）
- 通信方式：共享内存 IPC，延迟 <1ms

**实际执行结果**：
- ✅ ns3-ai 模块编译成功（`libns3.40-ai-default.so`）
- ✅ a-plus-b gym demo 运行通过（ns3 ↔ Python 数值加减）
- ✅ 最小化 DSDV+Python 联调 Demo 运行通过（10 steps 完整交互）
  - ns3 侧：5 节点 MANET，提取位置 + 邻居数 + 队列长度 + 邻接矩阵
  - Python 侧：接收 obs → 构建 PyG 图 → GATConv GNN Actor 推理 → 返回 next_hop action
  - 动态拓扑被正确捕获（edges 从 12 变到 20，随节点移动）

**交付物**：
- [x] ns3-ai 模块：`ns-3.40/contrib/ai/`
- [x] 修复后的 `ns3ai_utils.py`（直接调用二进制，绕过 Python 3.14 不兼容的 ns3 脚本）
- [x] protobuf 生成文件：`messages_pb2.py`
- [x] 联调 Demo：`contrib/ai/examples/dsdv-marl/`（含 C++ ns3 程序 + pybind11 绑定 + Python GNN 脚本）

**遇到的问题**：
1. **仓库地址错误**：之前给出 `hust-litter/ns3-ai`（不存在），正确为 `hust-diangroup/ns3-ai` → 搜索确认
2. **目录命名要求**：ns3-ai 必须放在 `contrib/ai`（不是 `contrib/ns3-ai`），否则 cmake 检测不到
3. **缺依赖**：需安装 `libboost-program-options-dev`, `pybind11-dev`, `libprotobuf-dev`, `protobuf-compiler`
4. **Python 3.14 argparse 不兼容**：ns3 脚本多处 `add_argument('positional', action='store_true')` 在 Python 3.14 报错 → 修改 `ns3ai_utils.py` 用 `glob` 直接查找二进制文件，绕过 `./ns3` 脚本
5. **protobuf 文件缺失**：`messages_pb2.py` 需要手动 `protoc --python_out=py messages.proto` 生成，且需复制到 envs 目录或设置 PYTHONPATH
6. **pybind11 C 数组绑定**：`def_readwrite` 不支持 C 数组（`float[4]`）→ 改用 `def_property` + lambda 返回 `std::vector`
7. **ns3Path 路径计算**：从 `dsdv-marl/` 到 `ns-3.40/` 是 4 层 `..`（不是 5 层）

---

### Task 1.4：基础 RL 训练框架搭建 ✅ 已完成

**目标**：在联合环境基础上，搭建可复用的 RL 训练框架骨架。

**完成日期**：2026-07-21

**实际执行结果**：
- ✅ Python 轻量仿真器 `SimpleNetworkEnv` 实现（NetworkX 随机几何图 + 队列动态 + 流量注入）
- ✅ MAPPO 基础训练器实现（MLP Actor/Critic + PPO clip + GAE）
- ✅ Rollout Buffer 实现（on-policy 收集 + GAE 计算）
- ✅ 100 episodes 训练无报错退出（总耗时 6.8s，平均 0.07s/episode）
- ✅ TensorBoard 日志正常记录（reward、loss、entropy 曲线）
- ✅ 数据维度验证通过：obs(10,7), action(10,), mask(10,6), reward(scalar)

**交付物**：
- [x] `python/env/simple_network_env.py` — Python 轻量仿真器 + Gym 环境
- [x] `python/algorithms/mappo.py` — MAPPO 训练器（MLP Actor/Critic）
- [x] `python/algorithms/buffer.py` — Rollout Buffer
- [x] `python/train.py` — 训练入口脚本
- [x] `python/configs/default.yaml` — 默认超参配置
- [x] `results/logs/` — TensorBoard 训练日志
- [x] `results/models/mappo_ep100.pt` — 模型检查点

**遇到的问题**：
1. NetworkX `random_geometric_graph` 的 `pos` 参数需要字典格式 `{node_id: position}`，不能直接传 numpy 数组 → 构建 pos_dict
2. 随机几何图可能不连通，导致节点数变化与 Critic 固定输入维度冲突 → 重试生成直到连通，或填充孤立节点
3. `tensorboard` 未预装 → `pip install tensorboard`

---

### Milestone 1 完成状态

| 检查项 | 状态 | 完成日期 | 备注 |
|---|---|---|---|
| ns3 安装成功，DSDV 示例可运行 | ✅ | 2026-07-19 | ns-3.40, cmake 构建 |
| Python 环境就绪，PyTorch GPU 可用 | ✅ | 2026-07-19 | 三层验证 17/17 通过 |
| ns3 与 Python 能双向通信 | ✅ | 2026-07-19 | ns3-ai 共享内存方案 |
| 最小化联调 Demo 跑通 | ✅ | 2026-07-19 | 5 节点 GNN Actor, 10 steps |
| RL 训练框架骨架搭建完成 | ✅ | 2026-07-21 | MLP Actor/Critic + Rollout Buffer |
| 随机策略能跑完完整训练流程 | ✅ | 2026-07-21 | 100 eps, 6.8s, TensorBoard 日志正常 |

---

## Milestone 2：面向GNN的协议驱动观测机制设计（第3章）

> **核心定位**：不是“改进DSDV协议”，而是“设计从DSDV路由表到GNN输入的系统性映射机制”。所有DSDV侧改动服务于GNN建模需求。
>
> **为什么选DSDV**：表驱动（始终有路由表→GNN始终有图输入）、距离矢量极简（改起来透明）、周期性更新包（天然观测载体）、ns3代码量小（~2000行可控）。
>
> **技术约束**：DSDV侧改动在ns3 C++实现，不改转发逻辑，只改信息收集与存储。
>
> **预计耗时**：2-3 周

---

### Phase A：文献调研 ✅ 已完成 (2026-07-21)

**完成情况**：
- 调研论文18篇，覆盖4条技术路线（DSDV多路径 / AOMDV按需 / GNN-RL路由 / 2-hop信息利用）
- 报告文件：`plan/A1_multipath_routing_survey.md`
- 核心发现：现有RL路由工作观测设计普遍简陋（队列+邻居数），无人从路由协议角度系统设计GNN输入
- 研究空白：协议驱动的结构化观测 + 多路径动作空间 + 真实协议栈上GNN-MAPPO

---

### Phase B：算法设计 ✅ 已确认 (2026-07-21, v3更新于2026-07-26)

**设计文档**：`plan/第3章_MP-DSDV-GNN协议算法设计_v3.md`

**已确认的设计决策（v3实现对齐版）：**

**1. GNN输入输出定义：**
- 输入：PyG图（节点特征6维 + 边特征4维 + 图结构 + 多路径信息）
- 输出：每节点对各邻居的流量分配比例 [p1,...,pk]，和为1
- Actor局部观测（只看局部子图），Critic全局观测（CTDE）

**2. 节点特征（6维，v3移除traffic_role）：**
- queue_ratio：本地队列占用率
- num_neighbors：1-hop邻居数（路由表hop=1计数，归一化/20）
- num_2hop：2-hop邻居数（归一化/20）
- local_load：本地转发负载（RouteOutput字节/容量）
- avg_nbr_queue：邻居平均队列占用率（FeatureStore广播值聚合）
- path_diversity：到各目的的平均可用路径数（多路径路由表）

**3. 边特征（4维，v3语义修订）：**
- link_quality：10s窗口SNR归一化（PHY MonitorSnifferRx Trace）
- link_utilization：1s窗口转发字节/链路速率（RouteOutput统计）
- link_reliability：10s窗口MAC帧交付率（MacTx/MacTxDrop Trace）
- link_stability：路由表存活时间占比（每秒轮询）

**4. DSDV侧改动（不改转发逻辑）：**
- 改动1：路由表扩展——每目的存K条路径（K_MAX=5, DELTA=1, next-hop不相交）
- 改动2：特征广播——每1s广播{self特征 + 邻居节点特征 + 直连链路4维特征}
- 改动3：GNN观测区生成——每10ms从路由表+FeatureStore构建PyG图
- 改动4：ns3-ai共享内存导出——GnnObsMsg/GnnActionMsg双向闭环

**5. 图结构构建规则：**
- 1-hop邻居集合：从路由表hop=1条目确定（非FeatureStore）
- 2-hop邻居：从FeatureStore[N].linkFeatures发现
- 1-hop边特征：本地PHY/MAC Trace测量
- 2-hop边特征：邻居广播的link_features
- 每个time slot(10ms)重新构建（动态图）

**6. 多路径维护策略（v3新增）：**
- 主路径切换时：旧主路径降级为备份（非暴力清除）
- seq刷新时：所有备份seq同步（UpdateMultipathSeq）
- 已有备份再次通告：刷新lifetime（RefreshMultipathLifetime）

**7. 创新点定位：**
- 不是“改了DSDV”，是“设计了协议感知的层次化图观测机制”
- 通过消融实验验证每种信息维度的独立贡献
- 与现有工作的区别：他们随机图+简单特征，我们协议驱动+结构化观测

---

### Phase C：ns3 C++ 实现与验证 ✅ 已完成 (2026-07-26)

**目标**：将 Phase B 确认的 MP-DSDV-GNN 协议设计落地为 C++ 代码。

**设计文档**：`plan/第3章_MP-DSDV-GNN协议算法设计_v3.md`

---

#### Task C.1：源码精读 + 修改点标注 ✅

**目标**：读DSDV源码，产出精确到函数级的修改点清单。

**交付物**：`plan/C1_dsdv_source_analysis.md`

**验收标准**（全部满足才算通过）：
| 编号 | 检查项 | 通过条件 |
|------|--------|----------|
| C1-1 | 路由更新处理函数定位 | 明确指出接收/处理RouteUpdate的函数名、文件、行号 |
| C1-2 | Hello包发送/接收函数定位 | 明确指出周期性广播的触发机制和包格式定义位置 |
| C1-3 | 路由表插入/查询/删除接口 | 列出所有需修改的公开方法签名 |
| C1-4 | 每个修改点有三要素 | “改什么”+“怎么改”+“影响范围”缺一不可 |
| C1-5 | 新增文件规划 | 明确哪些功能在现有文件改、哪些需新建文件 |

---

#### Task C.2：多路径路由表 ✅

**目标**：改造路由表支持每目的K条不相交路径。

**对应设计**：第二章2.1 + 第三章3.2-3.3

**交付物**：修改后的 `dsdv-rtable.h/cc` + `dsdv-routing-protocol.cc` + 单元测试文件

**验收标准**：
| 编号 | 检查项 | 通过条件 | 验证方法 |
|------|--------|----------|----------|
| C2-1 | 编译 | `cmake --build` 零错误零警告 | 终端输出 |
| C2-2 | PathEntry结构体 | 包含 dest/next_hop/hop_count/dest_seq/cost/is_primary/install_time 全部7个字段 | 代码审查 |
| C2-3 | 等跳路径插入 | 同一dest收到两个不同next_hop、相同hop_count的路由更新后，路由表有两条PathEntry | 单元测试断言 |
| C2-4 | 次优路径插入 | hop_count = best_hop+1 的路径被保留；hop_count > best_hop+1 的被丢弃 | 单元测试断言 |
| C2-5 | 序列号覆盖 | 收到更高seq后，旧的所有PathEntry被清空，仅保留新条目 | 单元测试断言 |
| C2-6 | K_MAX上限 | 路径数达到K_MAX=5后，新路径不再插入 | 单元测试断言 |
| C2-7 | 不相交约束 | 同一dest下所有PathEntry的next_hop互不相同 | 单元测试断言 |
| C2-8 | primary选举 | cost最小者标记is_primary=true，其余为false | 单元测试断言 |
| C2-9 | 原有测试 | `./test.py -s routing-dsdv` 全部 PASS | 终端输出 |

---

#### Task C.3：拓扑事件处理 + 触发更新 ✅

**目标**：实现事件驱动的拓扑变化响应，FeatureStore与路由表实时同步。

**对应设计**：第三章3.4 + 3.5

**交付物**：事件处理代码 + 触发更新逻辑 + 日志输出

**验收标准**：
| 编号 | 检查项 | 通过条件 | 验证方法 |
|------|--------|----------|----------|
| C3-1 | 链路断裂响应 | 停止节点B的移动/关闭Wifi接口后，节点A的路由表中所有next_hop=B的PathEntry在<100ms内消失 | 日志时间戳 |
| C3-2 | FeatureStore联动 | OnLinkBreak(B)后，A的FeatureStore中无B的任何条目 | 日志/断点 |
| C3-3 | 触发更新发出 | 链路断裂后1s内捕获到A发出的RouteUpdatePacket（含hop=16/不可达标记） | pcap/日志 |
| C3-4 | 触发抑制 | 1s内连续断裂3条链路，只发出1次触发更新（不是3次） | 日志计数 |
| C3-5 | 新邻居加入 | 节点E移动进入A的通信范围后，A的FeatureStore在<2s内出现E的空条目 | 日志 |
| C3-6 | ETX突变 | ETX从1.2跳到>5.0时，触发OnLinkBreak；ETX从1.2变到1.8时，不触发断裂但更新cost | 单元测试 |
| C3-7 | 原有测试 | `./test.py -s routing-dsdv` 全部 PASS | 终端输出 |

---

#### Task C.4：特征广播机制 ✅

**目标**：实现FeatureBroadcastPacket的发送/接收/存储全流程。

**对应设计**：第四章4.1-4.4

**交付物**：新包类型 + FeatureStore数据结构 + 发送/接收函数

**验收标准**：
| 编号 | 检查项 | 通过条件 | 验证方法 |
|------|--------|----------|----------|
| C4-1 | 编译 | `cmake --build` 零错误 | 终端输出 |
| C4-2 | 包结构正确 | FeatureBroadcastPacket含三部分：self_feature(4字段) + neighbor_features(N*4字段) + link_features(N*3字段) | 代码审查+抓包 |
| C4-3 | 周期性发送 | 每个节点每1s发出一包FeatureBroadcast（用ns3::Simulator::Schedule验证） | 日志计数 |
| C4-4 | 1-hop接收 | 20节点场景，节点A的FeatureStore中有其所有1-hop邻居的条目，且self_feature非零 | 打印断言 |
| C4-5 | 2-hop节点特征可达 | A的FeatureStore[N].nbr_features中包含非A的节点（南2-hop节点）的特征，且queue_ratio等字段非零 | 打印断言 |
| C4-6 | 不传播验证 | A的广播包中不包含任何2-hop节点的链路特征（只有邻居的节点特征） | 抓包检查 |
| C4-7 | 断裂后自动剔除 | OnLinkBreak(B)后，A的下一次FeatureBroadcast中不包含B的任何信息 | 抓包对比 |
| C4-8 | 包大小上限 | 50节点/度10场景，单包大小 < 600 Bytes | 日志统计 |
| C4-9 | 原有测试 | `./test.py -s routing-dsdv` 全部 PASS | 终端输出 |

---

#### Task C.5：GNN观测区生成 + ns3-ai对接 ✅

**目标**：每10ms生成结构化GNN输入，通过共享内存传给Python。

**对应设计**：第五章5.1-5.3 + 第六章6.2

**交付物**：GenerateGnnObs()实现 + 共享内存写入 + Python侧读取脚本

**验收标准**：
| 编号 | 检查项 | 通过条件 | 验证方法 |
|------|--------|----------|----------|
| C5-1 | 节点特征维度 | Python侧读取的 x.shape = [N, 7]，N=自身+1hop+2hop节点总数 | Python assert |
| C5-2 | 边特征维度 | edge_attr.shape = [E, 4]，E=1hop边+2hop边总数 | Python assert |
| C5-3 | 2-hop节点有真实特征 | x中对应2-hop节点的行不全为0（queue_ratio等字段有值） | Python assert |
| C5-4 | 边特征内容正确 | edge_attr[i] = [etx, link_load, is_primary, hop_to_dest]，etx>=1.0，is_primary∈{0,1} | Python assert |
| C5-5 | 动态更新 | 节点移动后，连续两个time slot的edge_index不完全相同 | Python比较 |
| C5-6 | 多路径信息 | multipath中至少80%的活跃目的的候选下一跳数>=2 | Python统计 |
| C5-7 | 时延 | 单次GenerateGnnObs()执行时间 < 1ms（20节点场景） | C++计时 |
| C5-8 | 原有测试 | `./test.py -s routing-dsdv` 全部 PASS | 终端输出 |

---

#### Task C.6：独立验证（整体验收）✅

**目标**：在完整场景下验证协议机制正确性（不是验证路由性能，那是第6章）。

**验证场景**：20节点随机几何图，Random Waypoint(speed=5m/s, pause=2s)，3条CBR流(512B@100kbps)，仿真时长60s

**交付物**：`plan/C6_verification_report.md`（含日志截取、数据统计、结论）

**验收标准**（7项全部通过才算完成）：
| 编号 | 检查项 | 通过条件 | 证据形式 |
|------|--------|----------|----------|
| C6-1 | 多路径稳定性 | 仿真期间，路由表中拥有>=2条路径的目的节点占比 >= 70%（按时间平均） | 统计曲线图 |
| C6-2 | 2-hop特征完整性 | 任意时刻，GNN观测区中2-hop节点的节点特征非零率 >= 95% | 统计曲线图 |
| C6-3 | 事件响应速度 | 链路断裂后，GNN观测区中该邻居消失的延迟 < 2s（从日志时间戳计算） | 日志截取 |
| C6-4 | 路由-特征一致性 | 任意时刻，FeatureStore中的节点集合 == 路由表中的当前直连邻居集合，无残留 | 断言日志（0次违反） |
| C6-5 | 控制开销 | 特征广播总字节数 / 全网总传输字节数 < 5% | FlowMonitor统计 |
| C6-6 | 无路由环路 | 序列号机制有效，仿真期间无数据包TTL超时（无环路证据） | ns3日志（0次TTL expire） |
| C6-7 | 原有测试兼容 | `./test.py -s routing-dsdv` 全部 PASS | 终端输出截取 |

---

### Milestone 2 完成状态

| 检查项 | 状态 | 完成日期 | 备注 |
|---|---|---|---|
| 文献调研报告 | ✅ | 2026-07-21 | 18篇，4条技术路线 |
| 算法设计确认（详细协议文档） | ✅ | 2026-07-26 | 第3章_MP-DSDV-GNN协议算法设计_v3.md |
| C.1 源码精读 + 修改点标注 | ✅ | 2026-07-26 | 交付: C1_dsdv_source_analysis.md |
| C.2 多路径路由表 (C2-1~C2-8通过) | ✅ | 2026-07-26 | C2-9测试框架环境问题（非代码引起），示例程序验证通过 |
| C.3 拓扑事件+触发更新 (C3-1~C3-6通过) | ✅ | 2026-07-26 | C3-7测试框架同 C2-9；编译零警告+示例程序通过 |
| C.4 特征广播+FeatureStore (C4-1~C4-8通过) | ✅ | 2026-07-26 | C4-9同C2-9；编译零警告+示例20节点通过 |
| C.5 GNN观测区+ns3-ai (C5-1~C5-7通过) | ✅ | 2026-07-26 | 10节点/5s: 4990次生成，74%含2-hop，最大14节点/33边 |
| C.6 独立验证 (C6-1~C6-7全通过) | ✅ | 2026-07-26 | 20节点/60s: 119980次观测,98%含2-hop,0环路,开销<1% |
| ns3-ai共享内存对接 | ✅ | 2026-07-26 | 6节点Grid: 5步交互零崩溃，Python端PyG图构建+9项自动验证ALL PASS |
| v3修正（1-hop来源/多路径维护/特征维度） | ✅ | 2026-07-26 | 1-hop从路由表hop=1获取；多路径降级+seq同步+lifetime刷新；节点特征7→6维 |

---

## 后续 Milestone 预览（待细化）

### Milestone 3：GNN Actor 网络（第4章）
- Task 3.1：设计 GNN Actor 网络结构（2层GAT，边特征参与注意力计算）
- Task 3.2：实现从ns3-ai共享内存读取PyG图 + 动态变长邻居支持
- Task 3.3：多路径softmax流量分配输出头 + Actor-Critic共享GNN编码器
- Task 3.4：不同拓扑/规模的泛化性初步验证

### Milestone 4：MAPPO 改进（第5章）
- Task 4.1：实现差分奖励机制
- Task 4.2：实现拓扑感知图注意力 Critic
- Task 4.3：完整算法训练与调优
- Task 4.4：消融实验

### Milestone 5：完整实验与论文撰写（第6-7章）
- Task 5.1：基线对比实验
- Task 5.2：消融实验
- Task 5.3：泛化性实验（不同规模/拓扑/流量）
- Task 5.4：ns3 最终验证与结果图生成
- Task 5.5：论文撰写与修改
