# Task C.1：DSDV 源码精读 + 修改点标注

> 源码路径：`/home/wuconan/ns-allinone-3.40/ns-3.40/src/dsdv/`
> 分析日期：2026-07-26

---

## 一、文件结构总览

| 文件 | 行数 | 职责 |
|------|------|------|
| `model/dsdv-routing-protocol.h` | 278 | 协议主类声明 |
| `model/dsdv-routing-protocol.cc` | 1279 | 协议核心逻辑（路由更新收发、转发） |
| `model/dsdv-rtable.h` | 471 | 路由表 + 路由表条目类声明 |
| `model/dsdv-rtable.cc` | 374 | 路由表操作实现 |
| `model/dsdv-packet.h` | 153 | DsdvHeader 包格式定义 |
| `model/dsdv-packet.cc` | ~80 | 包序列化/反序列化 |
| `model/dsdv-packet-queue.h/cc` | ~200 | 分组缓冲队列 |
| `helper/dsdv-helper.h/cc` | ~100 | 安装辅助类 |
| `test/dsdv-testcase.cc` | ~200 | 单元测试 |

---

## 二、关键发现：DSDV 没有独立的 Hello 包

**核心事实**：ns3 DSDV 实现中**不存在独立的 Hello/Beacon 包**。节点通过周期性广播完整路由表（`SendPeriodicUpdate`）来宣告自身存在和路由信息。邻居发现完全依赖于收到对方的路由更新包。

这意味着我们的 **FeatureBroadcastPacket 必须是全新的包类型**，不能"扩展Hello包"——因为没有Hello包可扩展。

---

## 三、路由更新处理函数定位（C1-1）

### 接收处理：`RecvDsdv()`

- **文件**：`model/dsdv-routing-protocol.cc`
- **行号**：566-823
- **触发方式**：socket 回调，在 `NotifyInterfaceUp()` (line 1001) 中通过 `socket->SetRecvCallback(MakeCallback(&RoutingProtocol::RecvDsdv, this))` 注册
- **处理逻辑**：
  - 逐条解析 DsdvHeader（每条12字节：dst + hopCount + seqNo）
  - 情况1 (line 619-644)：路由表中无此目的 → 添加新路由
  - 情况2 (line 663-707)：收到更高seq → 更新路由（可能等WST）
  - 情况3 (line 709-761)：相同seq → 仅当hop更小时更新，否则丢弃
  - 情况4 (line 763-773)：更旧seq → 丢弃
  - 情况5 (line 775-807)：无穷度量（seq为奇数）→ 链路断裂处理
- **结尾** (line 811-822)：调度一次 SendTriggeredUpdate

### 周期广播：`SendPeriodicUpdate()`

- **文件**：`model/dsdv-routing-protocol.cc`
- **行号**：901-972
- **触发方式**：`m_periodicUpdateTimer`（在 `Start()` line 274 中设置，默认间隔由 `m_periodicUpdateInterval` 控制）
- **逻辑**：Purge过期路由 → 遍历全部路由 → 每条封装为DsdvHeader → 广播

### 触发更新：`SendTriggeredUpdate()`

- **文件**：`model/dsdv-routing-protocol.cc`
- **行号**：826-898
- **触发方式**：由 RecvDsdv 末尾调度 / 由 WST 定时器调度
- **逻辑**：仅广播 `m_advRoutingTable` 中 `entriesChanged=true` 的条目

---

## 四、包格式定义位置（C1-2）

### DsdvHeader

- **文件**：`model/dsdv-packet.h`，line 60-141
- **格式**：固定12字节
  - `m_dst`：Ipv4Address（4B）
  - `m_hopCount`：uint32（4B）
  - `m_dstSeqNo`：uint32（4B）
- **序列化**：`model/dsdv-packet.cc` 中 `Serialize()/Deserialize()`

### 周期性广播触发机制

- **定时器**：`m_periodicUpdateTimer`（line 266，类型 `Timer`）
- **初始化**：`Start()` (line 274)：`m_periodicUpdateTimer.SetFunction(&RoutingProtocol::SendPeriodicUpdate, this)`
- **重调度**：`SendPeriodicUpdate()` 末尾 (line 970)：`m_periodicUpdateTimer.Schedule(m_periodicUpdateInterval + jitter)`
- **默认间隔**：由 `GetTypeId()` (line 112) 中 Attributes 设置

---

## 五、路由表接口梳理（C1-3）

### RoutingTableEntry 类（dsdv-rtable.h, line 58-321）

**当前字段**：
| 字段 | 类型 | 含义 |
|------|------|------|
| `m_seqNo` | uint32 | 目的序列号 |
| `m_hops` | uint32 | 跳数 |
| `m_lifeTime` | Time | 安装/过期时间 |
| `m_ipv4Route` | Ptr<Ipv4Route> | 含dest+nextHop+outputDev |
| `m_iface` | Ipv4InterfaceAddress | 输出接口 |
| `m_flag` | RouteFlags | VALID/INVALID |
| `m_settlingTime` | Time | WST等待时间 |
| `m_entriesChanged` | bool | 是否有变化（触发更新用） |

**关键方法**：GetDestination(), GetNextHop(), SetNextHop(), GetHop(), SetHop(), GetSeqNo(), SetSeqNo(), GetLifeTime(), SetLifeTime(), GetFlag(), SetFlag()

### RoutingTable 类（dsdv-rtable.h, line 327-467）

**核心存储**：`std::map<Ipv4Address, RoutingTableEntry> m_ipv4AddressEntry`（line 462）
- **每个目的仅一条路由** — 这是需要改造的核心

**需修改的公开方法**：
| 方法签名 | 行号 | 修改需求 |
|----------|------|----------|
| `bool AddRoute(RoutingTableEntry& r)` | 337 | 改为支持同dest多条目 |
| `bool DeleteRoute(Ipv4Address dst)` | 343 | 需支持删除单条/全部 |
| `bool LookupRoute(Ipv4Address dst, RoutingTableEntry& rt)` | 350 | 需返回primary或全部 |
| `bool Update(RoutingTableEntry& rt)` | 364 | 需支持多路径更新逻辑 |
| `void GetListOfDestinationWithNextHop(...)` | 370 | 需适配多路径结构 |
| `void GetListOfAllRoutes(...)` | 377 | 需适配（广播仅发primary） |
| `void Purge(...)` | 394 | 需适配多路径过期 |
| `uint32_t RoutingTableSize()` | 405 | 语义可能变化 |

---

## 六、修改点清单（C1-4：改什么 + 怎么改 + 影响范围）

### 修改点 1：路由表存储结构改造

- **改什么**：`dsdv-rtable.h` 中 `RoutingTable` 类的核心存储 `m_ipv4AddressEntry`
- **怎么改**：将 `std::map<Ipv4Address, RoutingTableEntry>` 改为 `std::map<Ipv4Address, std::vector<RoutingTableEntry>>`，每个目的支持多条 PathEntry；新增 K_MAX/DELTA 常量；新增 `AddMultipathRoute()` 方法实现设计文档3.3的分支逻辑
- **影响范围**：所有调用 RoutingTable 方法的代码（RecvDsdv、SendPeriodicUpdate、RouteOutput、RouteInput）

### 修改点 2：路由更新处理逻辑（多路径发现）

- **改什么**：`dsdv-routing-protocol.cc` 中 `RecvDsdv()` 函数 (line 566-823)
- **怎么改**：在"相同seq+相同/更大hop"的分支（line 741-761，原逻辑是丢弃）中，新增多路径添加逻辑：若 hop <= best_hop+DELTA 且 next_hop 不同 → 添加为备选路径
- **影响范围**：路由表内容变化 → 影响 RouteOutput 转发决策

### 修改点 3：新增 FeatureBroadcastPacket 包类型

- **改什么**：新建文件 `model/dsdv-feature-packet.h/cc`
- **怎么改**：定义 `FeatureBroadcastHeader : public Header`，含三部分：self_feature(4 float) + neighbor_features(变长) + link_features(变长)；实现 Serialize/Deserialize
- **影响范围**：仅新增文件，不影响现有代码；需在 CMakeLists.txt 中注册

### 修改点 4：新增 FeatureStore 数据结构

- **改什么**：新建文件 `model/dsdv-feature-store.h/cc`
- **怎么改**：定义 NodeFeature / LinkFeature / NbrFeatureEntry 结构体；定义 FeatureStore 类（增删查接口）；实现事件驱动生命周期（OnLinkBreak清除、OnNewNeighbor创建）
- **影响范围**：仅新增文件；被 RoutingProtocol 类持有

### 修改点 5：特征广播发送/接收逻辑

- **改什么**：`dsdv-routing-protocol.h/cc` 中新增方法和定时器
- **怎么改**：
  - 新增成员：`Timer m_featureBroadcastTimer`、`FeatureStore m_featureStore`
  - 新增方法：`SendFeatureBroadcast()`、`RecvFeatureBroadcast(Ptr<Socket>)`
  - 在 `Start()` 中调度特征广播定时器（1s间隔）
  - 在 `NotifyInterfaceUp()` 中注册特征包接收回调
- **影响范围**：RoutingProtocol 类扩展；新增 socket 或使用同一 DSDV_PORT 加子类型标识

### 修改点 6：拓扑事件处理

- **改什么**：`dsdv-routing-protocol.cc` 中 `RecvDsdv()` 的无穷度量处理分支 (line 775-807) + 新增事件方法
- **怎么改**：
  - 新增 `OnLinkBreak(Ipv4Address nbr)`：删除路由表中经该邻居的所有路径 + 清除 FeatureStore + 触发更新
  - 新增 `OnNewNeighbor(Ipv4Address nbr)`：创建 FeatureStore 空条目 + 触发更新
  - 在 RecvDsdv 的"新路由"分支(line 619-644)中检测是否为新邻居
  - 在 RecvDsdv 的"无穷度量"分支(line 775-807)中调用 OnLinkBreak
- **影响范围**：路由收敛速度变化；FeatureStore 与路由表同步

### 修改点 7：GNN 观测区生成器

- **改什么**：新建文件 `model/dsdv-gnn-observer.h/cc`
- **怎么改**：定义 `GnnObserver` 类，含 `GenerateGnnObs()` 方法；从 RoutingTable + FeatureStore + SelfFeature 组装节点集合(7维特征) + 边集合(4维特征) + 多路径信息；通过 ns3-ai 共享内存接口输出
- **影响范围**：仅新增文件 + 在 RoutingProtocol 中每 time slot 调用

### 修改点 8：转发决策改造（GNN动作执行）

- **改什么**：`dsdv-routing-protocol.cc` 中 `RouteOutput()` (line 279) 和 `RouteInput()` (line 387)
- **怎么改**：原来直接查路由表取唯一下一跳 → 改为查多路径路由表取候选集 → 按 GNN 输出的比例分配选择下一跳（需从共享内存读取 action）
- **影响范围**：数据面转发行为变化（核心改动，需谨慎）

---

## 七、新增文件规划（C1-5）

| 操作 | 文件 | 内容 |
|------|------|------|
| **新建** | `model/dsdv-feature-packet.h/cc` | FeatureBroadcastHeader 包定义 |
| **新建** | `model/dsdv-feature-store.h/cc` | FeatureStore + NodeFeature + LinkFeature |
| **新建** | `model/dsdv-gnn-observer.h/cc` | GNN观测区生成器 |
| **修改** | `model/dsdv-rtable.h/cc` | 多路径存储结构 + 新增方法 |
| **修改** | `model/dsdv-routing-protocol.h/cc` | 事件处理 + 特征广播 + 定时器 + 转发改造 |
| **修改** | `model/dsdv-packet.h/cc` | 可能需要添加包类型标识字段 |
| **修改** | `CMakeLists.txt` | 注册新文件 |
| **新建** | `test/dsdv-multipath-testcase.cc` | 多路径单元测试 |

---

## 八、关键设计约束（从源码中得出）

1. **无独立Hello包**：特征广播必须是全新包类型，不能"扩展Hello"
2. **双路由表设计**：DSDV使用 `m_routingTable`（转发用）+ `m_advRoutingTable`（广播用），多路径改造主要影响 `m_routingTable`
3. **WST机制**：Settling Time 用于防止频繁更新，多路径添加不应触发 WST（等跳路径不是"变化"）
4. **奇偶seq约定**：奇数seq表示不可达（infinite metric），多路径逻辑需跳过奇数seq条目
5. **每条目12字节**：DsdvHeader固定大小，路由更新包不携带特征信息（与设计一致）
6. **广播地址**：使用 `255.255.255.255` 或子网广播，特征广播可复用同一 socket
