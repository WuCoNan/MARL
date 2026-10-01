#!/usr/bin/env bash
#
# MP-DSDV-GNN: ns-3 源码镜像 与 ns-3 安装目录 的同步工具
#
# 用法：
#   bash scripts/sync_ns3.sh            # 只比较，不改动任何文件（默认）
#   bash scripts/sync_ns3.sh --apply    # 镜像 -> 安装目录（把本仓库改动推送到 ns-3）
#   bash scripts/sync_ns3.sh --pull     # 安装目录 -> 镜像（把 ns-3 里的改动收回来）
#
# 环境变量：
#   NS3_ROOT   ns-3 根目录，默认 ~/ns-allinone-3.40/ns-3.40
#
# 说明：只同步代码与构建文件，Markdown 文档、*.so、__pycache__ 一律跳过。

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
MIRROR="$REPO_ROOT/ns3"
NS3_ROOT="${NS3_ROOT:-$HOME/ns-allinone-3.40/ns-3.40}"

case "${1:-}" in
    --apply) MODE="apply" ;;
    --pull)  MODE="pull" ;;
    --check|"") MODE="check" ;;
    -h|--help)
        sed -n '3,13p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
        exit 0
        ;;
    *)
        echo "未知参数：$1（可用：--apply / --pull / --check）" >&2
        exit 2
        ;;
esac

if [[ ! -d "$NS3_ROOT/src/dsdv" || ! -d "$NS3_ROOT/contrib/ai" ]]; then
    echo "错误：NS3_ROOT 不像是 ns-3 根目录：$NS3_ROOT" >&2
    echo "      请用 NS3_ROOT=/path/to/ns-3.40 指定。" >&2
    exit 1
fi

# 镜像中子目录 -> ns-3 中的对应位置（相对路径，两侧同名）
SUBTREES=(
    "src/dsdv/model"
    "scratch"
    "contrib/ai/examples/dsdv-gnn-marl"
    "contrib/ai/python_utils"
)

same=0
diff_count=0
missing=0
applied=0

for subtree in "${SUBTREES[@]}"; do
    src_dir="$MIRROR/$subtree"
    dst_dir="$NS3_ROOT/$subtree"
    [[ -d "$src_dir" ]] || continue

    if [[ "$MODE" == "pull" ]]; then
        from_dir="$dst_dir"; to_dir="$src_dir"
    else
        from_dir="$src_dir"; to_dir="$dst_dir"
    fi

    [[ -d "$from_dir" ]] || continue

    while IFS= read -r -d '' file; do
        rel="${file#"$from_dir"/}"
        target="$to_dir/$rel"

        if [[ ! -e "$target" ]]; then
            if [[ "$MODE" == "apply" ]]; then
                mkdir -p "$(dirname "$target")"
                cp "$file" "$target"
                echo "  [新增] $subtree/$rel"
                applied=$((applied + 1))
            else
                echo "  [缺失] $subtree/$rel"
                missing=$((missing + 1))
            fi
        elif cmp -s "$file" "$target"; then
            same=$((same + 1))
        else
            if [[ "$MODE" == "apply" || "$MODE" == "pull" ]]; then
                cp "$file" "$target"
                echo "  [更新] $subtree/$rel"
                applied=$((applied + 1))
            else
                echo "  [差异] $subtree/$rel"
                diff_count=$((diff_count + 1))
            fi
        fi
    done < <(find "$from_dir" -type f \
        ! -name '*.md' ! -name '*.so' ! -name '*.pyc' \
        ! -path '*__pycache__*' -print0 | sort -z)
done

echo
echo "ns-3 根目录：$NS3_ROOT"
case "$MODE" in
    check)
        echo "比较结果：一致 $same 个，有差异 $diff_count 个，镜像独有 $missing 个"
        if [[ $diff_count -gt 0 || $missing -gt 0 ]]; then
            echo "如需把镜像推送到 ns-3：bash scripts/sync_ns3.sh --apply"
            exit 1
        fi
        echo "镜像与安装目录完全一致。"
        ;;
    apply)
        echo "已同步 $applied 个文件（镜像 -> ns-3）。"
        ;;
    pull)
        echo "已同步 $applied 个文件（ns-3 -> 镜像）。"
        ;;
esac
