/*
 * MP-DSDV-GNN: GNN Observation Area Generator
 * Generates structured GNN input from RoutingTable + FeatureStore + SelfFeature.
 * Called every time slot (10ms) to produce PyG-compatible graph data.
 *
 * FEATURE LAYOUT (v2 - traffic_role removed):
 *   Node features (6-dim):
 *     [0] queue_ratio      - local queue occupancy [0,1]
 *     [1] num_neighbors    - 1-hop neighbor count / 20.0
 *     [2] num_2hop         - 2-hop neighbor count / 20.0
 *     [3] local_load       - forwarded bytes / capacity [0,2]
 *     [4] avg_nbr_queue    - average neighbor queue ratio [0,1]
 *     [5] path_diversity   - avg paths per destination [1,K_MAX]
 *
 *   Edge features (4-dim):
 *     [0] link_quality     - SNR normalized [0,1]
 *     [1] link_utilization - bytes/rate [0,2]
 *     [2] link_reliability - MAC delivery ratio [0,1]
 *     [3] link_stability   - route alive ratio [0,1]
 *
 * To add/remove features: modify BuildNodeFeature() or BuildEdgeFeature(),
 * and update NODE_FEAT_DIM / EDGE_FEAT_DIM below. No other code changes needed.
 */

#ifndef DSDV_GNN_OBSERVER_H
#define DSDV_GNN_OBSERVER_H

#include "dsdv-feature-store.h"
#include "dsdv-rtable.h"

#include "ns3/ipv4-address.h"

#include <map>
#include <vector>

namespace ns3
{
namespace dsdv
{

/// Node feature dimension (modify here when adding/removing node features)
static constexpr uint32_t NODE_FEAT_DIM = 6;
/// Edge feature dimension (modify here when adding/removing edge features)
static constexpr uint32_t EDGE_FEAT_DIM = 4;

/**
 * \brief GNN observation for a single node (ego graph).
 * Contains all data needed to construct a PyG Data object on Python side.
 * Uses vector<float> for features to support runtime dimension changes.
 */
struct GnnObservation
{
    // Node data
    std::vector<Ipv4Address> nodeIds;              ///< All node IDs in ego graph
    std::vector<std::vector<float>> nodeFeatures;  ///< [numNodes x NODE_FEAT_DIM]
    Ipv4Address selfId;                            ///< This node's ID (always nodeIds[0])

    // Edge data
    std::vector<std::pair<uint32_t, uint32_t>> edgeIndex; ///< (src_idx, dst_idx) pairs
    std::vector<std::vector<float>> edgeFeatures;  ///< [numEdges x EDGE_FEAT_DIM]

    // Multipath info
    struct MultipathEntry
    {
        Ipv4Address dest;
        std::vector<Ipv4Address> candidates; ///< next-hop candidates
        std::vector<uint32_t> hopCounts;     ///< hop count per candidate
        Ipv4Address primary;                 ///< primary next-hop
    };
    std::vector<MultipathEntry> multipathInfo;

    // Stats
    uint32_t numNodes = 0;
    uint32_t numEdges = 0;
    uint32_t num1Hop = 0;
    uint32_t num2Hop = 0;
    float pathDiversity = 0.0f; ///< avg paths per active dest

    void Clear()
    {
        nodeIds.clear();
        nodeFeatures.clear();
        edgeIndex.clear();
        edgeFeatures.clear();
        multipathInfo.clear();
        numNodes = numEdges = num1Hop = num2Hop = 0;
        pathDiversity = 0.0f;
    }
};

/**
 * \brief GnnObserver: generates GNN observation from protocol state.
 *
 * Feature engineering is centralized in BuildNodeFeature() and BuildEdgeFeature().
 * To modify features for experiments, only those two functions need changes.
 */
class GnnObserver
{
  public:
    GnnObserver() = default;

    /**
     * Generate GNN observation for a specific node.
     */
    void GenerateGnnObs(Ipv4Address selfAddr,
                        const RoutingTable& routingTable,
                        const FeatureStore& featureStore,
                        const NodeFeature& selfFeature,
                        const std::vector<LinkFeature>& selfLinks,
                        GnnObservation& obs);

  private:
    /**
     * Build node feature vector (NODE_FEAT_DIM dimensions).
     * === ADD/REMOVE NODE FEATURES HERE ===
     */
    std::vector<float> BuildNodeFeature(Ipv4Address node,
                                        const NodeFeature& nf,
                                        const RoutingTable& rt,
                                        const FeatureStore& fs,
                                        Ipv4Address selfAddr);

    /**
     * Build edge feature vector (EDGE_FEAT_DIM dimensions).
     * === ADD/REMOVE EDGE FEATURES HERE ===
     */
    std::vector<float> BuildEdgeFeature(const LinkFeature& link);

    /// Get node index in obs.nodeIds, or -1 if not found
    int GetNodeIndex(const GnnObservation& obs, Ipv4Address addr);
};

} // namespace dsdv
} // namespace ns3

#endif /* DSDV_GNN_OBSERVER_H */
