"""
MP-DSDV-GNN: Python verification for 6-node grid topology.
Receives REAL observations from ns-3 and verifies correctness.
"""
import sys
import os
import numpy as np
import torch
from torch_geometric.data import Data

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ns3ai_dsdv_gnn_marl_py as py_binding
from ns3ai_utils import Experiment

NODE_FEAT_DIM = 6
EDGE_FEAT_DIM = 4
MAX_NEIGHBORS = 10


def build_pyg_graph(obs_msg):
    num_nodes = obs_msg.numNodes
    num_edges = obs_msg.numEdges
    nf = np.array(obs_msg.nodeFeatures, dtype=np.float32).reshape(num_nodes, NODE_FEAT_DIM)
    x = torch.from_numpy(nf)
    src = np.array(obs_msg.edgeSrc[:num_edges], dtype=np.int64)
    dst = np.array(obs_msg.edgeDst[:num_edges], dtype=np.int64)
    edge_index = torch.tensor(np.array([src, dst]), dtype=torch.long)
    ef = np.array(obs_msg.edgeFeatures, dtype=np.float32).reshape(num_edges, EDGE_FEAT_DIM)
    edge_attr = torch.from_numpy(ef)
    data = Data(x=x, edge_index=edge_index, edge_attr=edge_attr)
    data.num_neighbors = obs_msg.numNeighbors
    data.path_diversity = obs_msg.pathDiversity
    return data


def verify_observation(data, step):
    """Verify the observation is physically consistent."""
    errors = []

    # 1. Dimension checks
    if data.x.shape[1] != NODE_FEAT_DIM:
        errors.append(f"Node feat dim wrong: {data.x.shape[1]} != {NODE_FEAT_DIM}")
    if data.edge_attr.shape[1] != EDGE_FEAT_DIM:
        errors.append(f"Edge feat dim wrong: {data.edge_attr.shape[1]} != {EDGE_FEAT_DIM}")
    if data.edge_index.shape[0] != 2:
        errors.append(f"Edge index shape wrong: {data.edge_index.shape}")

    # 2. Node feature range checks
    queue_ratio = data.x[:, 0]
    if (queue_ratio < 0).any() or (queue_ratio > 1).any():
        errors.append(f"queue_ratio out of [0,1]: {queue_ratio.tolist()}")

    num_nbrs = data.x[:, 1]
    if (num_nbrs < 0).any() or (num_nbrs > 1).any():
        errors.append(f"num_neighbors out of [0,1]: {num_nbrs.tolist()}")

    local_load = data.x[:, 3]
    if (local_load < 0).any() or (local_load > 2).any():
        errors.append(f"local_load out of [0,2]: {local_load.tolist()}")

    path_div = data.x[:, 5]
    if (path_div < 1).any():
        errors.append(f"path_diversity < 1: {path_div.tolist()}")

    # 3. Edge feature range checks
    quality = data.edge_attr[:, 0]
    if (quality < 0).any() or (quality > 1).any():
        errors.append(f"link_quality out of [0,1]: min={quality.min():.3f} max={quality.max():.3f}")

    util = data.edge_attr[:, 1]
    if (util < 0).any() or (util > 2).any():
        errors.append(f"link_utilization out of [0,2]: {util.tolist()}")

    rel = data.edge_attr[:, 2]
    if (rel < 0).any() or (rel > 1.01).any():
        errors.append(f"link_reliability out of [0,1]: {rel.tolist()}")

    stab = data.edge_attr[:, 3]
    if (stab < 0).any() or (stab > 1.01).any():
        errors.append(f"link_stability out of [0,1]: {stab.tolist()}")

    # 4. Graph structure checks
    if data.num_nodes < 3:
        errors.append(f"Too few nodes: {data.num_nodes}")
    if data.num_edges < data.num_nodes - 1:
        errors.append(f"Too few edges ({data.num_edges}) for {data.num_nodes} nodes")

    # 5. Edge index validity
    if data.edge_index.max() >= data.num_nodes:
        errors.append(f"Edge index out of range: max={data.edge_index.max()} >= numNodes={data.num_nodes}")

    # 6. Self node is index 0 and has outgoing edges
    self_edges = (data.edge_index[0] == 0).sum().item()
    if self_edges == 0:
        errors.append("Self node (idx 0) has no outgoing edges!")

    # 7. Action space consistency
    action_dim = self_edges
    if action_dim != data.num_neighbors:
        errors.append(f"Action dim mismatch: edges_from_self={action_dim} vs numNeighbors={data.num_neighbors}")

    return errors


def main():
    print("[Python] 6-node grid verification")
    ns3_root = "/home/wuconan/ns-allinone-3.40/ns-3.40"

    exp = Experiment("ns3ai_dsdv_gnn_full_demo", ns3_root, py_binding,
                     handleFinish=True, useVector=True, vectorSize=1)
    msgIf = exp.run(show_output=True)

    step = 0
    all_pass = True
    try:
        while True:
            msgIf.PyRecvBegin()
            if msgIf.PyGetFinished():
                break
            obs_msg = msgIf.GetCpp2PyVector()[0]
            data = build_pyg_graph(obs_msg)

            # === Verify ===
            errors = verify_observation(data, step)

            # === Print ===
            self_mask = data.edge_index[0] == 0
            action_targets = data.edge_index[1][self_mask]
            action_feats = data.edge_attr[self_mask]

            print(f"\n[Python] Step {step}: nodes={data.num_nodes} edges={data.num_edges} "
                  f"action_dim={action_targets.shape[0]} pathDiv={data.path_diversity:.2f}")
            print(f"  x shape={data.x.shape}, edge_attr shape={data.edge_attr.shape}")
            print(f"  Self(node0) features: {[f'{v:.3f}' for v in data.x[0].tolist()]}")
            print(f"  Neighbors: {action_targets.tolist()}")
            for i in range(action_targets.shape[0]):
                print(f"    -> node{action_targets[i].item()}: "
                      f"q={action_feats[i][0]:.3f} util={action_feats[i][1]:.3f} "
                      f"rel={action_feats[i][2]:.3f} stab={action_feats[i][3]:.3f}")

            if errors:
                print(f"  [FAIL] Errors: {errors}")
                all_pass = False
            else:
                print(f"  [PASS] All checks passed")

            # Send uniform action
            msgIf.PySendBegin()
            act_vec = msgIf.GetPy2CppVector()
            num_nbrs = max(1, action_targets.shape[0])
            probs = [0.0] * MAX_NEIGHBORS
            for i in range(num_nbrs):
                probs[i] = 1.0 / num_nbrs
            act_vec[0].probs = probs
            msgIf.PyRecvEnd()
            msgIf.PySendEnd()
            step += 1

    except Exception as e:
        print(f"[Python] Exception: {e}")
        import traceback
        traceback.print_exc()

    print(f"\n{'='*50}")
    print(f"[Python] VERIFICATION SUMMARY: {step} steps, {'ALL PASS' if all_pass else 'SOME FAILED'}")
    print(f"{'='*50}")


if __name__ == "__main__":
    main()
