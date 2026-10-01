/*
 * MP-DSDV-GNN: Shared memory message format for ns3-ai bridge.
 * This struct is the contract between ns-3 (C++) and Python (PyTorch GNN).
 * All arrays are fixed-size for shared memory compatibility.
 */
#ifndef DSDV_GNN_MARL_MSG_H
#define DSDV_GNN_MARL_MSG_H

#include <cstdint>

// Maximum graph size (ego graph: self + 1-hop + 2-hop)
#define MAX_NODES 20
#define MAX_EDGES 80
#define MAX_NEIGHBORS 10

// Feature dimensions (must match dsdv-gnn-observer.h)
#define MSG_NODE_FEAT 6
#define MSG_EDGE_FEAT 4

/**
 * C++ → Python: GNN observation (one per time slot, per node)
 * Python reads this to construct a PyG Data object.
 */
struct GnnObsMsg
{
    uint32_t numNodes;
    uint32_t numEdges;
    uint32_t numNeighbors; ///< number of 1-hop neighbors (action dimension)
    float pathDiversity;

    // Flattened feature matrices
    float nodeFeatures[MAX_NODES * MSG_NODE_FEAT]; ///< [numNodes x 6]
    float edgeFeatures[MAX_EDGES * MSG_EDGE_FEAT]; ///< [numEdges x 4]

    // Edge index (COO format)
    uint32_t edgeSrc[MAX_EDGES];
    uint32_t edgeDst[MAX_EDGES];
};

/**
 * Python → C++: Traffic distribution action
 * probs[i] = fraction of traffic to forward to neighbor i.
 * Sum of probs[0..numNeighbors-1] should be 1.0 (softmax output).
 */
struct GnnActionMsg
{
    float probs[MAX_NEIGHBORS];
};

#endif // DSDV_GNN_MARL_MSG_H
