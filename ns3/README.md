# ns3 —— ns-3 移植层（源码镜像）

## 这个目录是什么

本目录**不是**完整的 ns-3，而是本项目对 ns-3.40 所做改动的**源码镜像**。
它只保存"我们自己写的 / 改过的"文件，便于版本管理与审阅。

真正参与编译和运行的是 ns-3 的安装目录：

```
/home/<user>/ns-allinone-3.40/ns-3.40/
```

## 目录映射

镜像的内部布局与 ns-3 安装目录**逐层对应**，因此每个文件应该放到哪里是一目了然的：

| 本仓库路径 | 对应 ns-3 安装路径 |
|---|---|
| `src/dsdv/model/*.{h,cc}` | `src/dsdv/model/` |
| `scratch/test-mp-dsdv-gnn.cc` | `scratch/` |
| `contrib/ai/examples/dsdv-gnn-marl/*` | `contrib/ai/examples/dsdv-gnn-marl/` |
| `contrib/ai/python_utils/ns3ai_utils.py` | `contrib/ai/python_utils/` |

> 注意：`src/dsdv/model/` 下 ns-3 原生的 4 个文件
> （`dsdv-packet.{h,cc}`、`dsdv-packet-queue.{h,cc}`）未被本项目修改，
> 因此不纳入镜像；`dsdv-testcase.cc` 同理。

## 同步方式

```bash
# 查看镜像与安装目录的差异（只读，不修改任何文件）
bash scripts/sync_ns3.sh

# 把镜像内容同步到 ns-3 安装目录（覆盖安装目录中的同名文件）
bash scripts/sync_ns3.sh --apply

# 指定其它 ns-3 根目录
NS3_ROOT=/path/to/ns-3.40 bash scripts/sync_ns3.sh --apply
```

**建议的修改流程**：在本仓库改 → `sync_ns3.sh` 预览差异 → `--apply` → 编译验证。
直接修改安装目录会导致镜像与实装不一致，后续无法追溯。

## 本地补丁说明

`contrib/ai/python_utils/ns3ai_utils.py` 来自上游 [ns3-ai](https://github.com/hust-diangroup/ns3-ai)，
本项目对它做了一处兼容性补丁，详见 [LOCAL_PATCH.md](contrib/ai/python_utils/LOCAL_PATCH.md)。

## 编译与运行

前置条件见仓库根目录 [README.md](../README.md)。

```bash
cd $NS3_ROOT

# 1) 配置 —— 必须用 cmake 直接配置
#    ns-3.40 自带的 ./ns3 configure 在 Python 3.14 下静默失效（argparse 不兼容），
#    且选项名是 NS3_TESTS / NS3_EXAMPLES，不是 --enable-tests。
cd build
cmake -DNS3_TESTS=ON -DNS3_EXAMPLES=ON ..
cd ..

# 2) 编译（首次约 8 分钟，增量很快）
./ns3 build

# 3) ns-3 官方测试驱动：原版 DSDV 回归
./test.py -s routing-dsdv          # 期望：PASS routing-dsdv

# 4) 多路径 + GNN 观测验证（18 项断言）
./build/scratch/ns3.40-test-mp-dsdv-gnn-default

# 5) ns3-ai 联合仿真（C++ 端 + Python 端）
python3 contrib/ai/examples/dsdv-gnn-marl/dsdv_gnn_marl.py
```

> Python 端需要 torch / torch-geometric，并使用与 pybind11 模块匹配的解释器版本
> （当前 `.so` 为 `cpython-314`，即 Python 3.14）。

## ⚠️ 已知陷阱：必须开启 NS3_TESTS

**症状**：`./build/utils/ns3.40-test-runner-default --suite=routing-dsdv` 崩溃，报

```
NS_ASSERT failed, cond="g_markingTimes->count(time) == 1",
msg="Time object ... registered 0 times (should be 1)"
```

**原因**：这**不是**协议代码的缺陷，而是**陈旧构建产物导致的 ABI 不匹配**。
当配置中未开启测试时，ns-3 不会重建 `libns3-*-test.so` 与 `test-runner`；
而 `src/dsdv` 的头文件已经改过（`RoutingTable` 增加了 `m_multipathEntries` 成员，
类布局变化），于是测试目标仍按旧布局分配对象、共享库却按新布局初始化，
析构时按旧偏移访问到 `std::map` 内部数据并当作 `Time` 解析，触发断言。

**处理**：按上文第 1 步开启 `-DNS3_TESTS=ON` 并完整重建一次即可。

**排查依据**（2026-10-01 实测）：

| 条件 | 结果 |
|---|---|
| 未开启测试（沿用 7 月的陈旧 test-runner） | CRASH |
| 换回 ns-3.40 原版 DSDV 源码 | PASS（说明崩溃随源码而变，指向构建一致性） |
| 仅把 `dsdv-rtable.{h,cc}` 换回原版 | CRASH（相同 ABI 不匹配） |
| 开启 `NS3_TESTS` 并完整重建后运行项目版 | **PASS** |

结论：项目对 DSDV 的改动**没有破坏**上游 DSDV 行为。

## 关键设计速览

| 文件 | 职责 |
|---|---|
| `dsdv-rtable.{h,cc}` | 多路径路由表（每目的 1 条主路径 + 最多 `K_MAX-1` 条备份） |
| `dsdv-feature-store.{h,cc}` | 邻居特征存储（1-hop 全量特征 + 2-hop 节点特征） |
| `dsdv-feature-packet.{h,cc}` | 特征广播包序列化（magic=0xFF 与路由包区分） |
| `dsdv-gnn-observer.{h,cc}` | 生成"恰好两跳"的 ego 图观测 |
| `dsdv-routing-protocol.{h,cc}` | 协议主体：多路径发现、特征广播、跨层 Trace、事件处理 |
| `dsdv_gnn_marl_msg.h` | ns3-ai 共享内存契约（`GnnObsMsg` / `GnnActionMsg`） |
| `dsdv_gnn_marl.cc` | ns-3 侧联合仿真入口 |
| `dsdv_gnn_marl_py.cc` | pybind11 绑定 |
| `dsdv_gnn_marl.py` | Python 侧：PyG 建图 + 动作空间推导 + 推理 |
| `ns3_paths.py` | ns-3 根目录解析（本仓库/安装目录两处通用） |

设计文档见 [docs/12_协议设计_第3章_v3.md](../docs/12_协议设计_第3章_v3.md)。
