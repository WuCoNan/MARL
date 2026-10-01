/*
 * MP-DSDV-GNN: Feature Store implementation
 */

#include "dsdv-feature-store.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DsdvFeatureStore");

namespace dsdv
{

void
FeatureStore::CreateEntry(Ipv4Address nbr)
{
    if (m_entries.find(nbr) == m_entries.end())
    {
        NbrFeatureEntry entry;
        entry.nbrId = nbr;
        entry.lastUpdate = Simulator::Now();
        m_entries[nbr] = entry;
        NS_LOG_DEBUG("FeatureStore: Created empty entry for " << nbr);
    }
}

void
FeatureStore::DeleteEntry(Ipv4Address nbr)
{
    auto it = m_entries.find(nbr);
    if (it != m_entries.end())
    {
        m_entries.erase(it);
        NS_LOG_DEBUG("FeatureStore: Deleted entry for " << nbr);
    }
}

bool
FeatureStore::HasEntry(Ipv4Address nbr) const
{
    return m_entries.find(nbr) != m_entries.end();
}

NbrFeatureEntry*
FeatureStore::GetEntry(Ipv4Address nbr)
{
    auto it = m_entries.find(nbr);
    if (it != m_entries.end())
    {
        return &it->second;
    }
    return nullptr;
}

const NbrFeatureEntry*
FeatureStore::GetEntry(Ipv4Address nbr) const
{
    auto it = m_entries.find(nbr);
    if (it != m_entries.end())
    {
        return &it->second;
    }
    return nullptr;
}

void
FeatureStore::UpdateEntry(Ipv4Address nbr,
                          const NodeFeature& selfF,
                          const std::vector<LinkFeature>& links,
                          const std::map<Ipv4Address, NodeFeature>& nbrFeats)
{
    auto it = m_entries.find(nbr);
    if (it == m_entries.end())
    {
        // Auto-create if not exists (e.g., received broadcast before route update)
        CreateEntry(nbr);
        it = m_entries.find(nbr);
    }
    it->second.selfFeature = selfF;
    it->second.linkFeatures = links;
    it->second.nbrFeatures = nbrFeats;
    it->second.lastUpdate = Simulator::Now();
}

bool
FeatureStore::Get2HopFeature(Ipv4Address node2hop, NodeFeature& feat) const
{
    for (const auto& entry : m_entries)
    {
        auto it = entry.second.nbrFeatures.find(node2hop);
        if (it != entry.second.nbrFeatures.end())
        {
            feat = it->second;
            return true;
        }
    }
    return false;
}

std::vector<Ipv4Address>
FeatureStore::GetNeighborList() const
{
    std::vector<Ipv4Address> list;
    list.reserve(m_entries.size());
    for (const auto& entry : m_entries)
    {
        list.push_back(entry.first);
    }
    return list;
}

uint32_t
FeatureStore::Size() const
{
    return m_entries.size();
}

void
FeatureStore::Clear()
{
    m_entries.clear();
}

} // namespace dsdv
} // namespace ns3
