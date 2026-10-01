"""
ns-3 根目录定位工具（MP-DSDV-GNN 联合仿真示例专用）。

背景：本示例源码在本仓库（ns3/ 镜像目录）和 ns-3 安装目录
（contrib/ai/examples/dsdv-gnn-marl/）两处各存一份，两者深度不同，
原先写死的相对路径 `../../../../ns-allinone-3.40/ns-3.40` 和绝对路径
都会在另一处失效。本模块用统一的解析顺序替代它们。

解析顺序：
    1. 环境变量 NS3_ROOT
    2. 从调用文件向上逐级查找（含 src/dsdv 与 contrib/ai 的目录即视为 ns-3 根）
    3. 默认安装位置 ~/ns-allinone-3.40/ns-3.40
"""

import os
import sys

# ns-3 根目录的判定标志。
#
# 注意：必须用完整 ns-3 才有的标志（src/core、根 CMakeLists.txt），
# 而不能只用 src/dsdv、contrib/ai —— 本仓库的 ns3/ 镜像目录刻意保持了
# 与 ns-3 相同的内部布局，用弱标志会把镜像误判成 ns-3 根目录。
_ROOT_DIR_MARKERS = (
    os.path.join("src", "core"),
    os.path.join("contrib", "ai"),
)
_ROOT_FILE_MARKERS = ("CMakeLists.txt",)

# 向上查找的最大层数
_MAX_UPWARD = 10


def looks_like_ns3_root(path):
    """判断给定目录是否像 ns-3 源码/构建根目录。"""
    if not path or not os.path.isdir(path):
        return False
    dirs_ok = all(os.path.isdir(os.path.join(path, m)) for m in _ROOT_DIR_MARKERS)
    files_ok = all(os.path.isfile(os.path.join(path, m)) for m in _ROOT_FILE_MARKERS)
    return dirs_ok and files_ok


def resolve_ns3_root(start=None):
    """
    定位 ns-3 根目录。

    Args:
        start: 起始文件路径，默认为本模块文件。

    Returns:
        ns-3 根目录的绝对路径。

    Raises:
        RuntimeError: 三种方式都找不到时抛出，并给出设置 NS3_ROOT 的提示。
    """
    candidates = []

    env_root = os.environ.get("NS3_ROOT")
    if env_root:
        candidates.append(os.path.abspath(env_root))

    probe = os.path.dirname(os.path.abspath(start or __file__))
    for _ in range(_MAX_UPWARD):
        parent = os.path.dirname(probe)
        if parent == probe:  # 已到文件系统根
            break
        candidates.append(parent)
        probe = parent

    candidates.append(os.path.expanduser("~/ns-allinone-3.40/ns-3.40"))

    for candidate in candidates:
        if looks_like_ns3_root(candidate):
            return candidate

    raise RuntimeError(
        "未找到 ns-3 根目录。请设置环境变量 NS3_ROOT 指向 ns-3 源码根，例如：\n"
        "    export NS3_ROOT=/home/<user>/ns-allinone-3.40/ns-3.40"
    )


def ensure_ns3ai_utils_on_path(ns3_root):
    """
    把 ns3-ai 的 python_utils 目录加入 sys.path，使 `import ns3ai_utils` 可用，
    免去手工设置 PYTHONPATH。

    Args:
        ns3_root: ns-3 根目录。

    Returns:
        python_utils 目录路径；不存在时返回 None。
    """
    python_utils = os.path.join(ns3_root, "contrib", "ai", "python_utils")
    if os.path.isdir(python_utils) and python_utils not in sys.path:
        sys.path.insert(0, python_utils)
    return python_utils if os.path.isdir(python_utils) else None
