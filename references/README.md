# references —— 参考资料

## Alanazi2025_IEEE_Access

与本项目最接近的对比工作：

> Alanazi, et al. *Multi-Agent Deep Reinforcement Learning for Dynamic Routing
> in MANETs Using Graph Neural Networks*. IEEE Access, 2025: 152469-152478.
> DOI: [10.1109/ACCESS.2025.11134361](https://ieeexplore.ieee.org/document/11134361)

| 文件 | 说明 |
|---|---|
| `paper.pdf` | 论文原文（**受版权约束，已被 `.gitignore` 排除，仅本地保留**） |
| `paper_extracted.md` | 论文全文抽取文本，便于检索引用 |
| `reader_notes.md` | 阅读时的原文摘录 |
| `figures/alg-04.png` ~ `alg-06.png` | 从论文中抽取的算法框图 |

## 与本文的关系

该工作同样采用 CTDE + GNN 做 MANET 路由，是本项目的主要对照基线。
两者的关键差异见 [docs/00_研究设计报告.md](../docs/00_研究设计报告.md) 第 7 节：

| 维度 | Alanazi 2025 | 本项目 |
|---|---|---|
| 观测 | 位置、队列、链路质量 | 6 维节点 + 4 维**跨层**边特征，无位置依赖 |
| 多跳信息 | 依赖 Hello 搭载的**嵌入交换** | 由状态广播**直接测量**，恰好两跳 |
| 动作 | 邻居集合 ∪ {保持} 中选一个 | 对各候选下一跳的**流量分配比例** |
| 多路径 | 无 | 协议被动维护，作为动作空间 |
| 学习框架 | CTDE + DQN | 改进 MAPPO（差分奖励 + 图价值网络） |
