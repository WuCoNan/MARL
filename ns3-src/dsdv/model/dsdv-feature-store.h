/*
 * MP-DSDV-GNN: Feature Store for GNN observation
 * Stores neighbor node features and link features received from feature broadcasts.
 */

#ifndef DSDV_FEATURE_STORE_H
#define DSDV_FEATURE_STORE_H

#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"

#include <map>
#include <vector>

namespace ns3
{
namespace dsdv
{

/**
 * \brief Node feature vector for GNN input (4 fields broadcasted).
 */
struct NodeFeature
{
    float queueRatio = 0.0f;   ///< Queue occupancy ratio [0,1]
    uint8_t numNeighbors = 0;  ///< Number of active neighbors
    float localLoad = 0.0f;    ///< Local traffic load (normalized)
    float bufferUsage = 0.0f;  ///< Buffer usage ratio [0,1]

    bool IsZero() const
    {
        return queueRatio == 0.0f && numNeighbors == 0 && localLoad == 0.0f && bufferUsage == 0.0f;
    }
};

/**
 * \brief Link feature for a single direct link (4 fields broadcasted + peer_id).
 */
struct LinkFeature
{
    Ipv4Address peerId;       ///< Peer node address
    float etx = 1.0f;         ///< link_quality: SNR normalized [0,1]
    float linkLoad = 0.0f;    ///< link_utilization: bytes/rate [0,2]
    float bandwidth = 1.0f;   ///< link_reliability: MAC delivery ratio [0,1]
    float stability = 1.0f;   ///< link_stability: alive time ratio [0,1]
};

/**
 * \brief Complete feature entry for a 1-hop neighbor (received from its broadcast).
 */
struct NbrFeatureEntry
{
    Ipv4Address nbrId;                    ///< Neighbor's address
    NodeFeature selfFeature;              ///< Neighbor's own node feature
    std::vector<LinkFeature> linkFeatures; ///< Neighbor's all direct link features
    std::map<Ipv4Address, NodeFeature> nbrFeatures; ///< Neighbor's neighbors' node features (2-hop)
    Time lastUpdate;                      ///< Last time this entry was updated
};

/**
 * \brief FeatureStore: per-node storage of neighbor features for GNN observation.
 *
 * Lifecycle: event-driven (OnNewNeighbor creates, OnLinkBreak deletes)
 *            + periodic safety net (RoutingTableMaintenance cleans stale entries).
 */
class FeatureStore
{
  public:
    FeatureStore() = default;

    /**
     * Create an empty entry for a new neighbor (awaiting feature broadcast).
     * \param nbr neighbor address
     */
    void CreateEntry(Ipv4Address nbr);

    /**
     * Delete a neighbor's entry entirely (link break).
     * \param nbr neighbor address
     */
    void DeleteEntry(Ipv4Address nbr);

    /**
     * Check if an entry exists for this neighbor.
     * \param nbr neighbor address
     * \return true if exists
     */
    bool HasEntry(Ipv4Address nbr) const;

    /**
     * Get mutable reference to a neighbor's entry.
     * \param nbr neighbor address
     * \return pointer to entry, or nullptr if not found
     */
    NbrFeatureEntry* GetEntry(Ipv4Address nbr);

    /**
     * Get const reference to a neighbor's entry.
     * \param nbr neighbor address
     * \return const pointer to entry, or nullptr if not found
     */
    const NbrFeatureEntry* GetEntry(Ipv4Address nbr) const;

    /**
     * Update a neighbor's complete feature data (called on receiving broadcast).
     * \param nbr neighbor address
     * \param selfF the neighbor's own node feature
     * \param links the neighbor's link features
     * \param nbrFeats the neighbor's neighbors' node features
     */
    void UpdateEntry(Ipv4Address nbr,
                     const NodeFeature& selfF,
                     const std::vector<LinkFeature>& links,
                     const std::map<Ipv4Address, NodeFeature>& nbrFeats);

    /**
     * Get a 2-hop node's feature (searched from all 1-hop neighbors' nbr_features).
     * \param node2hop the 2-hop node address
     * \param feat output node feature
     * \return true if found
     */
    bool Get2HopFeature(Ipv4Address node2hop, NodeFeature& feat) const;

    /**
     * Get all 1-hop neighbor IDs that have valid entries.
     * \return vector of neighbor addresses
     */
    std::vector<Ipv4Address> GetNeighborList() const;

    /**
     * Get number of entries.
     * \return entry count
     */
    uint32_t Size() const;

    /**
     * Clear all entries.
     */
    void Clear();

  private:
    std::map<Ipv4Address, NbrFeatureEntry> m_entries;
};

} // namespace dsdv
} // namespace ns3

#endif /* DSDV_FEATURE_STORE_H */
