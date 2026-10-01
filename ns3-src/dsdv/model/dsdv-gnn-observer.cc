/*
 * MP-DSDV-GNN: GNN Observation Area Generator - Implementation
 *
 * FEATURE ENGINEERING GUIDE:
 * - To add a node feature: append to BuildNodeFeature(), increment NODE_FEAT_DIM in .h
 * - To add an edge feature: append to BuildEdgeFeature(), increment EDGE_FEAT_DIM in .h
 * - To remove a feature: delete the line, decrement the DIM constant
 * - No changes needed in GenerateGnnObs() or elsewhere
 */

#include "dsdv-gnn-observer.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <set>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DsdvGnnObserver");

namespace dsdv
{

// ============================================================================
// FEATURE BUILDERS (modify these for experiments)
// ============================================================================

std::vector<float>
GnnObserver::BuildNodeFeature(Ipv4Address node,
                              const NodeFeature& nf,
                              const RoutingTable& rt,
                              const FeatureStore& fs,
                              Ipv4Address selfAddr)
{
    std::vector<float> feat;
    feat.reserve(NODE_FEAT_DIM);

    // [0] queue_ratio: local queue occupancy [0,1]
    feat.push_back(nf.queueRatio);

    // [1] num_neighbors: 1-hop neighbor count, normalized
    feat.push_back(static_cast<float>(nf.numNeighbors) / 20.0f);

    // [2] num_2hop: 2-hop neighbor count from neighbor's linkFeatures
    uint32_t twoHopCount = 0;
    const auto* entry = fs.GetEntry(node);
    if (entry)
    {
        for (const auto& link : entry->linkFeatures)
        {
            if (link.peerId != selfAddr && link.peerId != node)
            {
                twoHopCount++;
            }
        }
    }
    feat.push_back(static_cast<float>(twoHopCount) / 20.0f);

    // [3] local_load: forwarded bytes / link capacity [0,2]
    feat.push_back(nf.localLoad);

    // [4] avg_nbr_queue: average neighbor queue ratio from FeatureStore
    float sumQueue = 0.0f;
    uint32_t nbrCount = 0;
    auto nbrList = fs.GetNeighborList();
    for (const auto& nbr : nbrList)
    {
        const auto* nbrEntry = fs.GetEntry(nbr);
        if (nbrEntry && !nbrEntry->selfFeature.IsZero())
        {
            sumQueue += nbrEntry->selfFeature.queueRatio;
            nbrCount++;
        }
    }
    feat.push_back((nbrCount > 0) ? (sumQueue / nbrCount) : 0.0f);

    // [5] path_diversity: avg paths per destination from multipath routing table
    std::map<Ipv4Address, RoutingTableEntry> allRoutes;
    rt.GetListOfAllRoutes(allRoutes);
    uint32_t destCount = 0;
    float totalPaths = 0.0f;
    for (const auto& route : allRoutes)
    {
        if (route.second.GetHop() > 0)
        {
            destCount++;
            totalPaths += static_cast<float>(rt.GetPathCount(route.first));
        }
    }
    feat.push_back((destCount > 0) ? (totalPaths / destCount) : 1.0f);

    return feat;
}

std::vector<float>
GnnObserver::BuildEdgeFeature(const LinkFeature& link)
{
    std::vector<float> ef;
    ef.reserve(EDGE_FEAT_DIM);

    // [0] link_quality: SNR normalized [0,1]
    ef.push_back(link.etx);

    // [1] link_utilization: bytes/rate [0,2]
    ef.push_back(link.linkLoad);

    // [2] link_reliability: MAC delivery ratio [0,1]
    ef.push_back(link.bandwidth);

    // [3] link_stability: route alive ratio [0,1]
    ef.push_back(link.stability);

    return ef;
}

// ============================================================================
// GRAPH CONSTRUCTION (stable - rarely needs modification)
// ============================================================================

int
GnnObserver::GetNodeIndex(const GnnObservation& obs, Ipv4Address addr)
{
    for (uint32_t i = 0; i < obs.nodeIds.size(); i++)
    {
        if (obs.nodeIds[i] == addr)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void
GnnObserver::GenerateGnnObs(Ipv4Address selfAddr,
                            const RoutingTable& routingTable,
                            const FeatureStore& featureStore,
                            const NodeFeature& selfFeature,
                            const std::vector<LinkFeature>& selfLinks,
                            GnnObservation& obs)
{
    obs.Clear();
    obs.selfId = selfAddr;

    // ========== Step 1: Node set (self + 1-hop + 2-hop) ==========
    obs.nodeIds.push_back(selfAddr);
    obs.nodeFeatures.push_back(BuildNodeFeature(selfAddr, selfFeature, routingTable, featureStore, selfAddr));

    // 1-hop neighbors: from ROUTING TABLE hop==1 (design: "链路层确认的活跃直连邻居")
    std::map<Ipv4Address, RoutingTableEntry> allRoutes;
    routingTable.GetListOfAllRoutes(allRoutes);
    std::vector<Ipv4Address> nbrList;
    for (const auto& r : allRoutes)
    {
        if (r.second.GetHop() == 1 && !r.first.IsBroadcast() && (r.first.Get() & 0xFF) != 0xFF)
        {
            nbrList.push_back(r.first);
        }
    }

    for (const auto& nbr : nbrList)
    {
        if (GetNodeIndex(obs, nbr) < 0)
        {
            obs.nodeIds.push_back(nbr);
            // Get node features from FeatureStore (if broadcast received), else default
            const auto* entry = featureStore.GetEntry(nbr);
            NodeFeature nf = entry ? entry->selfFeature : NodeFeature{};
            obs.nodeFeatures.push_back(BuildNodeFeature(nbr, nf, routingTable, featureStore, selfAddr));
            obs.num1Hop++;
        }
    }

    // 2-hop neighbors: from FeatureStore[N].linkFeatures (neighbor's broadcast links)
    auto fsNbrList = featureStore.GetNeighborList();
    for (const auto& nbr : fsNbrList)
    {
        const auto* entry = featureStore.GetEntry(nbr);
        if (!entry)
        {
            continue;
        }
        for (const auto& link : entry->linkFeatures)
        {
            Ipv4Address twoHopNode = link.peerId;
            if (twoHopNode != selfAddr && GetNodeIndex(obs, twoHopNode) < 0)
            {
                obs.nodeIds.push_back(twoHopNode);
                // Get 2-hop node's real features from neighbor's nbr_features
                NodeFeature nf2;
                featureStore.Get2HopFeature(twoHopNode, nf2);
                obs.nodeFeatures.push_back(BuildNodeFeature(twoHopNode, nf2, routingTable, featureStore, selfAddr));
                obs.num2Hop++;
            }
        }
    }
    obs.numNodes = obs.nodeIds.size();

    // ========== Step 2: Edges + edge features ==========
    // 1-hop edges (self → neighbors) from local link measurements
    for (const auto& link : selfLinks)
    {
        int srcIdx = GetNodeIndex(obs, selfAddr);
        int dstIdx = GetNodeIndex(obs, link.peerId);
        if (srcIdx >= 0 && dstIdx >= 0)
        {
            obs.edgeIndex.push_back({static_cast<uint32_t>(srcIdx), static_cast<uint32_t>(dstIdx)});
            obs.edgeFeatures.push_back(BuildEdgeFeature(link));
        }
    }

    // Fill missing 1-hop edges with default features
    for (const auto& nbr : nbrList)
    {
        int srcIdx = GetNodeIndex(obs, selfAddr);
        int dstIdx = GetNodeIndex(obs, nbr);
        if (srcIdx >= 0 && dstIdx >= 0)
        {
            bool exists = false;
            for (const auto& e : obs.edgeIndex)
            {
                if (e.first == static_cast<uint32_t>(srcIdx) && e.second == static_cast<uint32_t>(dstIdx))
                {
                    exists = true;
                    break;
                }
            }
            if (!exists)
            {
                obs.edgeIndex.push_back({static_cast<uint32_t>(srcIdx), static_cast<uint32_t>(dstIdx)});
                LinkFeature defaultLink;
                obs.edgeFeatures.push_back(BuildEdgeFeature(defaultLink));
            }
        }
    }

    // 2-hop edges (neighbor → its neighbors) from broadcast linkFeatures
    for (const auto& nbr : nbrList)
    {
        const auto* entry = featureStore.GetEntry(nbr);
        if (!entry)
        {
            continue;
        }
        int srcIdx = GetNodeIndex(obs, nbr);
        if (srcIdx < 0)
        {
            continue;
        }
        for (const auto& link : entry->linkFeatures)
        {
            int dstIdx = GetNodeIndex(obs, link.peerId);
            if (dstIdx >= 0 && link.peerId != selfAddr)
            {
                bool exists = false;
                for (const auto& e : obs.edgeIndex)
                {
                    if (e.first == static_cast<uint32_t>(srcIdx) && e.second == static_cast<uint32_t>(dstIdx))
                    {
                        exists = true;
                        break;
                    }
                }
                if (!exists)
                {
                    obs.edgeIndex.push_back({static_cast<uint32_t>(srcIdx), static_cast<uint32_t>(dstIdx)});
                    obs.edgeFeatures.push_back(BuildEdgeFeature(link));
                }
            }
        }
    }
    obs.numEdges = obs.edgeIndex.size();

    // ========== Step 3: Multipath info ==========
    // (allRoutes already populated in Step 1)
    uint32_t activeDests = 0;
    float totalPathCount = 0.0f;
    for (const auto& route : allRoutes)
    {
        if (route.second.GetHop() == 0)
        {
            continue;
        }
        activeDests++;
        uint32_t pathCount = routingTable.GetPathCount(route.first);
        totalPathCount += static_cast<float>(pathCount);

        if (pathCount > 1)
        {
            GnnObservation::MultipathEntry mpEntry;
            mpEntry.dest = route.first;
            mpEntry.primary = route.second.GetNextHop();
            std::vector<RoutingTableEntry> paths;
            routingTable.GetAllPaths(route.first, paths);
            for (const auto& path : paths)
            {
                mpEntry.candidates.push_back(path.GetNextHop());
                mpEntry.hopCounts.push_back(path.GetHop());
            }
            obs.multipathInfo.push_back(mpEntry);
        }
    }
    obs.pathDiversity = (activeDests > 0) ? (totalPathCount / activeDests) : 1.0f;

    // ========== Debug output ==========
    NS_LOG_DEBUG("GnnObs for " << selfAddr << ": nodes=" << obs.numNodes
                  << " (1hop=" << obs.num1Hop << " 2hop=" << obs.num2Hop << ")"
                  << " edges=" << obs.numEdges
                  << " pathDiv=" << obs.pathDiversity);

    for (uint32_t i = 0; i < obs.edgeIndex.size() && i < 8; i++)
    {
        auto& e = obs.edgeIndex[i];
        auto& ef = obs.edgeFeatures[i];
        NS_LOG_DEBUG("  edge[" << i << "]: "
                      << obs.nodeIds[e.first] << "->" << obs.nodeIds[e.second]
                      << " quality=" << ef[0]
                      << " util=" << ef[1]
                      << " rel=" << ef[2]
                      << " stab=" << ef[3]);
    }
}

} // namespace dsdv
} // namespace ns3
