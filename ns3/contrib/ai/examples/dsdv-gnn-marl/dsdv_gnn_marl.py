"""
MP-DSDV-GNN: Python side of ns3-ai joint simulation.
Receives REAL GNN observations from ns-3 DSDV protocol via shared memory,
constructs PyG graph, runs GNN inference, returns traffic distribution actions.
"""
import sys
import os
import numpy as np
import torch
import torch.nn.functional as F
from torch_geometric.data import Data

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from ns3_paths import ensure_ns3ai_utils_on_path, resolve_ns3_root

# ns-3 根目录：优先 NS3_ROOT 环境变量，其次向上查找，最后回退默认安装路径
NS3_ROOT = resolve_ns3_root(__file__)
ensure_ns3ai_utils_on_path(NS3_ROOT)

import ns3ai_dsdv_gnn_marl_py as py_binding
from ns3ai_utils import Experiment

NODE_FEAT_DIM = 6
EDGE_FEAT_DIM = 4
MAX_NEIGHBORS = 10


def build_pyg_graph(obs_msg):
    """Convert shared memory GnnObsMsg → PyG Data object."""
    num_nodes = obs_msg.numNodes
    num_edges = obs_msg.numEdges

    # Node features: [num_nodes, 6]
    nf = np.array(obs_msg.nodeFeatures, dtype=np.float32).reshape(num_nodes, NODE_FEAT_DIM)
    x = torch.from_numpy(nf)

    # Edge index: [2, num_edges]
    src = np.array(obs_msg.edgeSrc[:num_edges], dtype=np.int64)
    dst = np.array(obs_msg.edgeDst[:num_edges], dtype=np.int64)
    edge_index = torch.tensor(np.array([src, dst]), dtype=torch.long)

    # Edge features: [num_edges, 4]
    ef = np.array(obs_msg.edgeFeatures, dtype=np.float32).reshape(num_edges, EDGE_FEAT_DIM)
    edge_attr = torch.from_numpy(ef)

    data = Data(x=x, edge_index=edge_index, edge_attr=edge_attr)
    data.num_neighbors = obs_msg.numNeighbors
    data.path_diversity = obs_msg.pathDiversity
    return data


def main():
    print("[Python] MP-DSDV-GNN: Initializing ns3-ai experiment...")
    ns3_root = NS3_ROOT
    print(f"[Python] ns3 root: {ns3_root}")

    exp = Experiment("ns3ai_dsdv_gnn_marl", ns3_root, py_binding,
                     handleFinish=True, useVector=True, vectorSize=1)
    msgIf = exp.run(show_output=True)

    step = 0
    try:
        while True:
            # === Receive GNN observation from ns-3 ===
            msgIf.PyRecvBegin()
            if msgIf.PyGetFinished():
                break
            obs_vec = msgIf.GetCpp2PyVector()
            obs_msg = obs_vec[0]

            # === Build PyG graph from REAL ns-3 data ===
            data = build_pyg_graph(obs_msg)

            print(f"\n[Python] Step {step}: REAL GNN Observation from ns-3:")
            print(f"  Nodes: {data.num_nodes}, Edges: {data.num_edges}")
            print(f"  Node features shape: {data.x.shape}")
            print(f"  Edge features shape: {data.edge_attr.shape}")
            print(f"  Edge index shape: {data.edge_index.shape}")
            print(f"  Neighbors (action dim): {data.num_neighbors}")
            print(f"  Path diversity: {data.path_diversity:.2f}")
            print(f"  Node 0 features: {data.x[0].tolist()}")
            if data.num_edges > 0:
                print(f"  Edge 0 features: {data.edge_attr[0].tolist()}")

            # === Derive action space from graph structure ===
            # Self node is always index 0; its outgoing edges = possible actions
            self_mask = data.edge_index[0] == 0
            action_targets = data.edge_index[1][self_mask]  # neighbor node indices
            action_edge_feats = data.edge_attr[self_mask]   # [num_nbrs, 4]
            action_dim = action_targets.shape[0]

            print(f"  --- Action Space (derived from graph) ---")
            print(f"  Action dim (1-hop neighbors of self): {action_dim}")
            print(f"  Neighbor node indices: {action_targets.tolist()}")
            print(f"  Per-neighbor edge features (quality/util/rel/stab):")
            for i in range(action_dim):
                print(f"    -> node {action_targets[i].item()}: {action_edge_feats[i].tolist()}")

            # === Generate action (uniform distribution for now) ===
            # TODO: Replace with trained GNN Actor
            num_nbrs = max(1, data.num_neighbors)
            probs = np.zeros(MAX_NEIGHBORS, dtype=np.float32)
            probs[:num_nbrs] = 1.0 / num_nbrs  # uniform distribution

            # === Send action back to ns-3 ===
            msgIf.PySendBegin()
            act_vec = msgIf.GetPy2CppVector()
            act_vec[0].probs = probs.tolist()
            msgIf.PyRecvEnd()
            msgIf.PySendEnd()

            print(f"  Action (traffic split): {probs[:num_nbrs].tolist()}")
            step += 1

    except Exception as e:
        print(f"[Python] Exception: {e}")
        import traceback
        traceback.print_exc()
    finally:
        print(f"\n[Python] Finished after {step} steps.")


if __name__ == "__main__":
    main()
