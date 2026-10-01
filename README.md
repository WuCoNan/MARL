# MP-DSDV-GNN：面向自组网的多路径 DSDV 与图神经网络多智能体路由调度

本仓库是硕士论文项目的完整工程实现。核心思路：以 **DSDV** 为信息基础设施，
把它在运行中本就产生的状态组织成"以节点为中心、恰好覆盖两跳"的**图观测**，
再用**图神经网络 + 多智能体 PPO** 学习"对各候选下一跳按比例分配流量"的调度策略。

网络仿真由 **ns-3**（C++，包级保真）承担，策略学习由 **Python（PyTorch + PyG）** 承担，
两者通过 **ns3-ai 共享内存** 交换观测与动作。

---

## 目录结构

```
MARL/
├── README.md                     本文件：项目入口
├── requirements.txt              Python 依赖清单
│
├── docs/                         文档（按编号排序，00-03 为总纲，10+ 为专题）
│   ├── 00_研究设计报告.md         研究目标、RQ1-3、方法设计、相关工作与定位
│   ├── 01_实施计划.md             里程碑 M1-M5 与逐任务验收标准
│   ├── 02_系统架构.md             分层架构、数据流、模型结构
│   ├── 03_工作流程规范.md         任务预告 / 执行 / 汇报的协作规范
│   ├── 10_文献综述_多路径路由.md   13 篇文献调研（4 条技术路线）
│   ├── 11_源码分析_DSDV.md        DSDV 源码精读与修改点清单
│   ├── 12_协议设计_第3章_v3.md    现行协议设计（与代码逐函数对应）
│   └── legacy/                    历史版本（仅供追溯）
│       └── 第3章_协议设计_v2.md
│
├── ns3/                          ns-3 移植层（源码镜像，见其中 README）
│   ├── README.md                 目录映射、同步方式、编译运行说明
│   ├── src/dsdv/model/           协议侧改动：多路径路由表 / 特征存储 / GNN 观测器
│   ├── scratch/                  多路径验证程序
│   └── contrib/ai/               ns3-ai 桥接与示例
│
├── python/                       学习侧（PyTorch + PyG）
│   ├── train.py                  训练入口
│   ├── configs/default.yaml      超参配置
│   ├── env/                      环境封装（当前为轻量仿真器）
│   ├── algorithms/               MAPPO 训练器与经验回放
│   ├── models/                   GNN Actor / Critic（第 4 章，待实现）
│   └── utils/                    工具函数
│
├── scripts/                      运维脚本
│   ├── verify_env.py             环境自检（三层共 17 项）
│   └── sync_ns3.sh               ns3/ 镜像与 ns-3 安装目录的同步工具
│
├── references/                   参考资料（文献、抽取文本、配图）
└── results/                      运行产物：logs / models / figures / legacy_toy_env
```

---

## 环境要求

| 组件 | 版本 | 说明 |
|---|---|---|
| ns-3 | 3.40 | 安装于 `~/ns-allinone-3.40/ns-3.40`，含 ns3-ai（`contrib/ai`） |
| Python | 3.14 | 与 pybind11 模块的 `cpython-314` ABI 匹配 |
| PyTorch | 2.11.0+cu128 | GPU：GTX 1650 Ti |
| PyTorch Geometric | 2.8.0 | `GATConv` 等 |
| 其它 | gymnasium / networkx / tensorboard | 见 `requirements.txt` |

Python 虚拟环境位于 **`~/marl_env`**（仓库内的 `.venv/` 已被 git 忽略）。

```bash
source ~/marl_env/bin/activate
```

---

## 快速开始

### 1. 环境自检

```bash
source ~/marl_env/bin/activate
python scripts/verify_env.py        # 期望：17/17 全部通过
```

### 2. 编译 ns-3 并跑协议侧单测

```bash
export NS3_ROOT=~/ns-allinone-3.40/ns-3.40

# 把本仓库的协议侧改动同步到 ns-3（首次或改动后）
bash scripts/sync_ns3.sh --apply

cd $NS3_ROOT
./ns3 configure --enable-examples --enable-tests
./ns3 build

# 多路径 + GNN 观测验证（期望：PASSED=18 FAILED=0）
./build/scratch/ns3.40-test-mp-dsdv-gnn-default
```

### 3. 跑 ns3-ai 联合仿真（ns-3 ↔ Python 共享内存闭环）

```bash
cd $NS3_ROOT
python contrib/ai/examples/dsdv-gnn-marl/dsdv_gnn_marl.py   # 期望：20 步交互，输出真实观测
```

### 4. 跑 Python 侧训练流程

```bash
cd <本仓库根目录>
python python/train.py              # 期望：100 episode，约 7 秒，TensorBoard 日志写入 results/logs
tensorboard --logdir results/logs
```

---

## 当前进展

| 里程碑 | 内容 | 状态 |
|---|---|---|
| M1 | 环境搭建（ns-3 + Python + ns3-ai 联通） | ✅ 完成 |
| M2 | 第 3 章：协议侧观测机制（多路径 + 特征广播 + 两跳图观测） | ✅ 完成 |
| M3 | 第 4 章：GNN Actor 网络（`python/models/` 待实现） | ⬜ 未开始 |
| M4 | 第 5 章：改进 MAPPO（差分奖励 + 拓扑感知图 Critic） | ⬜ 未开始 |
| M5 | 第 6-7 章：对比 / 消融 / 泛化实验与论文撰写 | ⬜ 未开始 |

**重要说明**：当前 Python 侧的训练跑在**轻量仿真器**（NetworkX 随机几何图）上，
用的是 MLP Actor/Critic，仅用于打通训练流程；协议侧的动作**回传链路已具备，
但尚未接入实际转发面**。把两侧真正打通是 M3 的前置工作。

详细任务清单见 [docs/01_实施计划.md](docs/01_实施计划.md)。

---

## 开发约定

1. **协议侧代码只改本仓库**，再通过 `scripts/sync_ns3.sh --apply` 推送到 ns-3，
   保证镜像与实装一致、改动可追溯。详见 [ns3/README.md](ns3/README.md)。
2. **实验产物不进版本库**：`results/` 下的日志与模型由 `.gitignore` 排除。
3. **提交粒度**：一个可验证的改动一次提交，提交信息说明"改了什么 + 怎么验证的"。

---

## 文档索引

| 想了解 | 看这里 |
|---|---|
| 研究问题、创新点、与已有工作的区别 | [docs/00_研究设计报告.md](docs/00_研究设计报告.md) |
| 现在做到哪一步、下一步做什么 | [docs/01_实施计划.md](docs/01_实施计划.md) |
| 系统怎么组织、数据怎么流动 | [docs/02_系统架构.md](docs/02_系统架构.md) |
| 协议侧改了什么、每维特征怎么算 | [docs/12_协议设计_第3章_v3.md](docs/12_协议设计_第3章_v3.md) |
| ns-3 源码从哪下手 | [docs/11_源码分析_DSDV.md](docs/11_源码分析_DSDV.md) |
| 相关工作与设计依据 | [docs/10_文献综述_多路径路由.md](docs/10_文献综述_多路径路由.md) |
