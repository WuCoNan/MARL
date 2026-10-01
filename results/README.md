# results —— 运行产物

本目录存放实验运行产生的文件，**内容不进版本库**（见根目录 `.gitignore`）。

| 子目录 | 内容 |
|---|---|
| `logs/` | TensorBoard 事件文件，`python python/train.py` 生成 |
| `models/` | 模型检查点（`.pt`） |
| `figures/` | 论文图表输出 |
| `legacy_toy_env/` | 历史遗留：早期在轻量仿真器上跑出的模型与日志 |

## 关于 legacy_toy_env

该目录中的模型与日志来自项目早期的**轻量仿真器训练**（NetworkX 随机几何图 +
MLP Actor/Critic），仅用于验证训练流程可跑通，**与 ns-3 协议侧无关**，
不能用于任何性能结论。保留它们只是为了追溯，后续可以安全删除。
