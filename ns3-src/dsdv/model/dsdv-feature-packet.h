/*
 * MP-DSDV-GNN: Feature Broadcast Packet Header
 * Carries node features + neighbor features + link features for GNN observation.
 */

#ifndef DSDV_FEATURE_PACKET_H
#define DSDV_FEATURE_PACKET_H

#include "dsdv-feature-store.h"

#include "ns3/header.h"
#include "ns3/ipv4-address.h"

#include <map>
#include <vector>

namespace ns3
{
namespace dsdv
{

/**
 * \brief FeatureBroadcastHeader
 *
 * Packet format:
 *   Part 1: self_feature (4 floats = 16B)
 *   Part 2: neighbor_count (1B) + N * (4B addr + 16B features)
 *   Part 3: link_count (1B) + N * (4B addr + 16B features = 20B)
 *
 * Total for degree-5 node: 16 + 1 + 5*20 + 1 + 5*20 = 218 bytes
 */
class FeatureBroadcastHeader : public Header
{
  public:
    FeatureBroadcastHeader();
    ~FeatureBroadcastHeader() override;

    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    // === Setters ===
    void SetSelfFeature(const NodeFeature& f)
    {
        m_selfFeature = f;
    }

    void SetNeighborFeatures(const std::map<Ipv4Address, NodeFeature>& nbrFeats)
    {
        m_neighborFeatures = nbrFeats;
    }

    void SetLinkFeatures(const std::vector<LinkFeature>& links)
    {
        m_linkFeatures = links;
    }

    // === Getters ===
    NodeFeature GetSelfFeature() const
    {
        return m_selfFeature;
    }

    std::map<Ipv4Address, NodeFeature> GetNeighborFeatures() const
    {
        return m_neighborFeatures;
    }

    std::vector<LinkFeature> GetLinkFeatures() const
    {
        return m_linkFeatures;
    }

  private:
    NodeFeature m_selfFeature;                     ///< Part 1: sender's own feature
    std::map<Ipv4Address, NodeFeature> m_neighborFeatures; ///< Part 2: sender's neighbors' features
    std::vector<LinkFeature> m_linkFeatures;       ///< Part 3: sender's link features
};

} // namespace dsdv
} // namespace ns3

#endif /* DSDV_FEATURE_PACKET_H */
