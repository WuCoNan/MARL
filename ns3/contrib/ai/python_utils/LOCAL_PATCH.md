# ns3ai_utils.py 的本地补丁说明

## 来源

`ns3ai_utils.py` 来自上游 [hust-diangroup/ns3-ai](https://github.com/hust-diangroup/ns3-ai)，
原始位置为 ns-3 安装目录下的 `contrib/ai/python_utils/ns3ai_utils.py`。

## 补丁内容

**唯一改动**：`run_single_ns3()` 中定位可执行文件的方式。

上游实现通过调用 ns-3 自带的 `./ns3 run` 脚本启动仿真，但 ns-3.40 的 `build.py`
在 Python 3.14 下不兼容（`argparse` 的 `store_true` 不能用于位置参数），
会导致联合仿真启动失败。

本项目改为**直接在 `build/` 目录下递归查找二进制**：

```python
binary_pattern = os.path.join(abs_path, 'build', '**', 'ns3.40-{}-default'.format(pname))
matches = glob.glob(binary_pattern, recursive=True)
```

仅在查找失败时才回退到 `./ns3 run`。

## 影响面

- 对外接口（`Experiment` 类、`run()` 方法签名）完全不变；
- 行为差异仅在"可执行文件从哪里找到"这一点上；
- 因此不影响任何 ns3-ai 官方示例的既有用法。

## 为什么纳入版本管理

该补丁位于 ns-3 安装目录内，一旦重装 ns-3 就会丢失，且无法被版本控制追溯。
将其纳入本仓库并由 `scripts/sync_ns3.sh` 同步，可保证环境可复现。

## 许可证

ns3-ai 采用 GPL-2.0 许可，本文件保留其原始内容与许可约束。
