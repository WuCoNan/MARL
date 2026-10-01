/*
 * MP-DSDV-GNN: Feature Broadcast Packet implementation
 */

#include "dsdv-feature-packet.h"

#include "ns3/log.h"

#include <cstring>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DsdvFeaturePacket");

namespace dsdv
{

// Helper: float to network-order uint32
static uint32_t
FloatToNtoh(float f)
{
    uint32_t tmp;
    std::memcpy(&tmp, &f, sizeof(tmp));
    return tmp;
}

// Helper: network-order uint32 to float
static float
NtohToFloat(uint32_t u)
{
    float f;
    std::memcpy(&f, &u, sizeof(f));
    return f;
}

NS_OBJECT_ENSURE_REGISTERED(FeatureBroadcastHeader);

FeatureBroadcastHeader::FeatureBroadcastHeader()
{
}

FeatureBroadcastHeader::~FeatureBroadcastHeader()
{
}

TypeId
FeatureBroadcastHeader::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::dsdv::FeatureBroadcastHeader")
            .SetParent<Header>()
            .SetGroupName("Dsdv")
            .AddConstructor<FeatureBroadcastHeader>();
    return tid;
}

TypeId
FeatureBroadcastHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
FeatureBroadcastHeader::GetSerializedSize() const
{
    // 1 byte magic (0xFF) to distinguish from DsdvHeader
    uint32_t size = 1;
    // Part 1: self_feature = 4 floats = 16 bytes
    size += 16;
    // Part 2: 1 byte count + N * (4B addr + 4 floats = 20B)
    size += 1 + m_neighborFeatures.size() * 20;
    // Part 3: 1 byte count + N * (4B addr + 4 floats = 20B)
    size += 1 + m_linkFeatures.size() * 20;
    return size;
}

void
FeatureBroadcastHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    // Magic byte to distinguish from DsdvHeader (which starts with IPv4 address)
    i.WriteU8(0xFF);

    // Part 1: self feature (4 floats)
    i.WriteHtonU32(FloatToNtoh(m_selfFeature.queueRatio));
    i.WriteU8(m_selfFeature.numNeighbors);
    i.WriteU8(0); // padding to align to 4 bytes
    i.WriteU8(0);
    i.WriteU8(0);
    i.WriteHtonU32(FloatToNtoh(m_selfFeature.localLoad));
    i.WriteHtonU32(FloatToNtoh(m_selfFeature.bufferUsage));

    // Part 2: neighbor features
    i.WriteU8(static_cast<uint8_t>(m_neighborFeatures.size()));
    for (const auto& nbr : m_neighborFeatures)
    {
        uint8_t addrBuf[4];
        nbr.first.Serialize(addrBuf);
        i.Write(addrBuf, 4);
        i.WriteHtonU32(FloatToNtoh(nbr.second.queueRatio));
        i.WriteU8(nbr.second.numNeighbors);
        i.WriteU8(0);
        i.WriteU8(0);
        i.WriteU8(0);
        i.WriteHtonU32(FloatToNtoh(nbr.second.localLoad));
        i.WriteHtonU32(FloatToNtoh(nbr.second.bufferUsage));
    }

    // Part 3: link features (4 floats: quality, utilization, reliability, stability)
    i.WriteU8(static_cast<uint8_t>(m_linkFeatures.size()));
    for (const auto& link : m_linkFeatures)
    {
        uint8_t addrBuf[4];
        link.peerId.Serialize(addrBuf);
        i.Write(addrBuf, 4);
        i.WriteHtonU32(FloatToNtoh(link.etx));
        i.WriteHtonU32(FloatToNtoh(link.linkLoad));
        i.WriteHtonU32(FloatToNtoh(link.bandwidth));
        i.WriteHtonU32(FloatToNtoh(link.stability));
    }
}

uint32_t
FeatureBroadcastHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    // Read and verify magic byte
    uint8_t magic = i.ReadU8();
    NS_ASSERT(magic == 0xFF);

    // Part 1: self feature
    uint32_t tmpU32;
    tmpU32 = i.ReadNtohU32();
    m_selfFeature.queueRatio = NtohToFloat(tmpU32);
    m_selfFeature.numNeighbors = i.ReadU8();
    i.ReadU8(); // padding
    i.ReadU8();
    i.ReadU8();
    tmpU32 = i.ReadNtohU32();
    m_selfFeature.localLoad = NtohToFloat(tmpU32);
    tmpU32 = i.ReadNtohU32();
    m_selfFeature.bufferUsage = NtohToFloat(tmpU32);

    // Part 2: neighbor features
    uint8_t nbrCount = i.ReadU8();
    m_neighborFeatures.clear();
    for (uint8_t n = 0; n < nbrCount; n++)
    {
        uint8_t addrBuf[4];
        i.Read(addrBuf, 4);
        Ipv4Address addr = Ipv4Address::Deserialize(addrBuf);

        NodeFeature nf;
        tmpU32 = i.ReadNtohU32();
        nf.queueRatio = NtohToFloat(tmpU32);
        nf.numNeighbors = i.ReadU8();
        i.ReadU8();
        i.ReadU8();
        i.ReadU8();
        tmpU32 = i.ReadNtohU32();
        nf.localLoad = NtohToFloat(tmpU32);
        tmpU32 = i.ReadNtohU32();
        nf.bufferUsage = NtohToFloat(tmpU32);

        m_neighborFeatures[addr] = nf;
    }

    // Part 3: link features (4 floats: quality, utilization, reliability, stability)
    uint8_t linkCount = i.ReadU8();
    m_linkFeatures.clear();
    for (uint8_t n = 0; n < linkCount; n++)
    {
        LinkFeature lf;
        uint8_t addrBuf[4];
        i.Read(addrBuf, 4);
        lf.peerId = Ipv4Address::Deserialize(addrBuf);

        tmpU32 = i.ReadNtohU32();
        lf.etx = NtohToFloat(tmpU32);
        tmpU32 = i.ReadNtohU32();
        lf.linkLoad = NtohToFloat(tmpU32);
        tmpU32 = i.ReadNtohU32();
        lf.bandwidth = NtohToFloat(tmpU32);
        tmpU32 = i.ReadNtohU32();
        lf.stability = NtohToFloat(tmpU32);

        m_linkFeatures.push_back(lf);
    }

    return GetSerializedSize();
}

void
FeatureBroadcastHeader::Print(std::ostream& os) const
{
    os << "FeatureBroadcast: self(q=" << m_selfFeature.queueRatio
       << ",nbrs=" << (int)m_selfFeature.numNeighbors
       << ",load=" << m_selfFeature.localLoad << ") "
       << "nbr_feats=" << m_neighborFeatures.size()
       << " link_feats=" << m_linkFeatures.size();
}

} // namespace dsdv
} // namespace ns3
