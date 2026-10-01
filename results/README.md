# results —— 运行产物

本目录存放实验运行产生的文件，**内容不进版本库**（见根目录 `.gitignore`）。

| 子目录 | 内容 |
|---|---|
| `logs/` | TensorBoard 事件文件，`python python/train.py` 生成 |
| `models/` | 模型检查点（`.pt`） |
| `figures/` | 论文图表输出 |

## 关于 toy env 产物

`logs/` 与 `models/` 中的产物来自**轻量仿真器训练**（NetworkX 随机几何图 +
MLP Actor/Critic），仅用于验证训练流程可跑通，**与 ns-3 协议侧无关**，
不能用于任何性能结论，可随时删除（`python/train.py` 会自动重建目录）。

原 `legacy_toy_env/` 已清理：其内容与 `models/` 中的检查点逐字节相同（MD5 一致），
属纯重复，无追溯价值。
