/*
 * Copyright (c) 2010 Hemanth Narra
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Hemanth Narra <hemanth@ittc.ku.com>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  https://resilinets.org/
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */
#include "dsdv-rtable.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <iomanip>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DsdvRoutingTable");

namespace dsdv
{
RoutingTableEntry::RoutingTableEntry(Ptr<NetDevice> dev,
                                     Ipv4Address dst,
                                     uint32_t seqNo,
                                     Ipv4InterfaceAddress iface,
                                     uint32_t hops,
                                     Ipv4Address nextHop,
                                     Time lifetime,
                                     Time settlingTime,
                                     bool areChanged)
    : m_seqNo(seqNo),
      m_hops(hops),
      m_lifeTime(lifetime),
      m_iface(iface),
      m_flag(VALID),
      m_settlingTime(settlingTime),
      m_entriesChanged(areChanged)
{
    m_ipv4Route = Create<Ipv4Route>();
    m_ipv4Route->SetDestination(dst);
    m_ipv4Route->SetGateway(nextHop);
    m_ipv4Route->SetSource(m_iface.GetLocal());
    m_ipv4Route->SetOutputDevice(dev);
}

RoutingTableEntry::~RoutingTableEntry()
{
}

RoutingTable::RoutingTable()
{
}

bool
RoutingTable::LookupRoute(Ipv4Address id, RoutingTableEntry& rt)
{
    if (m_ipv4AddressEntry.empty())
    {
        return false;
    }
    auto i = m_ipv4AddressEntry.find(id);
    if (i == m_ipv4AddressEntry.end())
    {
        return false;
    }
    rt = i->second;
    return true;
}

bool
RoutingTable::LookupRoute(Ipv4Address id, RoutingTableEntry& rt, bool forRouteInput)
{
    if (m_ipv4AddressEntry.empty())
    {
        return false;
    }
    auto i = m_ipv4AddressEntry.find(id);
    if (i == m_ipv4AddressEntry.end())
    {
        return false;
    }
    if (forRouteInput && id == i->second.GetInterface().GetBroadcast())
    {
        return false;
    }
    rt = i->second;
    return true;
}

bool
RoutingTable::DeleteRoute(Ipv4Address dst)
{
    return m_ipv4AddressEntry.erase(dst) != 0;
}

uint32_t
RoutingTable::RoutingTableSize()
{
    return m_ipv4AddressEntry.size();
}

bool
RoutingTable::AddRoute(RoutingTableEntry& rt)
{
    auto result = m_ipv4AddressEntry.insert(std::make_pair(rt.GetDestination(), rt));
    return result.second;
}

bool
RoutingTable::Update(RoutingTableEntry& rt)
{
    auto i = m_ipv4AddressEntry.find(rt.GetDestination());
    if (i == m_ipv4AddressEntry.end())
    {
        return false;
    }
    i->second = rt;
    return true;
}

void
RoutingTable::DeleteAllRoutesFromInterface(Ipv4InterfaceAddress iface)
{
    if (m_ipv4AddressEntry.empty())
    {
        return;
    }
    for (auto i = m_ipv4AddressEntry.begin(); i != m_ipv4AddressEntry.end();)
    {
        if (i->second.GetInterface() == iface)
        {
            auto tmp = i;
            ++i;
            m_ipv4AddressEntry.erase(tmp);
        }
        else
        {
            ++i;
        }
    }
}

void
RoutingTable::GetListOfAllRoutes(std::map<Ipv4Address, RoutingTableEntry>& allRoutes) const
{
    for (auto i = m_ipv4AddressEntry.begin(); i != m_ipv4AddressEntry.end(); ++i)
    {
        if (i->second.GetDestination() != Ipv4Address("127.0.0.1") && i->second.GetFlag() == VALID)
        {
            allRoutes.insert(std::make_pair(i->first, i->second));
        }
    }
}

void
RoutingTable::GetListOfDestinationWithNextHop(Ipv4Address nextHop,
                                              std::map<Ipv4Address, RoutingTableEntry>& unreachable)
{
    unreachable.clear();
    for (auto i = m_ipv4AddressEntry.begin(); i != m_ipv4AddressEntry.end(); ++i)
    {
        if (i->second.GetNextHop() == nextHop)
        {
            unreachable.insert(std::make_pair(i->first, i->second));
        }
    }
}

void
RoutingTableEntry::Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit /*= Time::S*/) const
{
    std::ostream* os = stream->GetStream();
    // Copy the current ostream state
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);

    std::ostringstream dest;
    std::ostringstream gw;
    std::ostringstream iface;
    std::ostringstream ltime;
    std::ostringstream stime;
    dest << m_ipv4Route->GetDestination();
    gw << m_ipv4Route->GetGateway();
    iface << m_iface.GetLocal();
    ltime << std::setprecision(3) << (Simulator::Now() - m_lifeTime).As(unit);
    stime << m_settlingTime.As(unit);

    *os << std::setw(16) << dest.str();
    *os << std::setw(16) << gw.str();
    *os << std::setw(16) << iface.str();
    *os << std::setw(16) << m_hops;
    *os << std::setw(16) << m_seqNo;
    *os << std::setw(16) << ltime.str();
    *os << stime.str() << std::endl;
    // Restore the previous ostream state
    (*os).copyfmt(oldState);
}

void
RoutingTable::Purge(std::map<Ipv4Address, RoutingTableEntry>& removedAddresses)
{
    if (m_ipv4AddressEntry.empty())
    {
        return;
    }
    for (auto i = m_ipv4AddressEntry.begin(); i != m_ipv4AddressEntry.end();)
    {
        auto itmp = i;
        if (i->second.GetLifeTime() > m_holddownTime && (i->second.GetHop() > 0))
        {
            for (auto j = m_ipv4AddressEntry.begin(); j != m_ipv4AddressEntry.end();)
            {
                if ((j->second.GetNextHop() == i->second.GetDestination()) &&
                    (i->second.GetHop() != j->second.GetHop()))
                {
                    auto jtmp = j;
                    removedAddresses.insert(std::make_pair(j->first, j->second));
                    ++j;
                    m_ipv4AddressEntry.erase(jtmp);
                }
                else
                {
                    ++j;
                }
            }
            removedAddresses.insert(std::make_pair(i->first, i->second));
            ++i;
            m_ipv4AddressEntry.erase(itmp);
        }
        /** \todo Need to decide when to invalidate a route */
        /*          else if (i->second.GetLifeTime() > m_holddownTime)
         {
         ++i;
         itmp->second.SetFlag(INVALID);
         }*/
        else
        {
            ++i;
        }
    }
}

void
RoutingTable::Print(Ptr<OutputStreamWrapper> stream, Time::Unit unit /*= Time::S*/) const
{
    std::ostream* os = stream->GetStream();
    // Copy the current ostream state
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);

    *os << "\nDSDV Routing table\n";
    *os << std::setw(16) << "Destination";
    *os << std::setw(16) << "Gateway";
    *os << std::setw(16) << "Interface";
    *os << std::setw(16) << "HopCount";
    *os << std::setw(16) << "SeqNum";
    *os << std::setw(16) << "LifeTime";
    *os << "SettlingTime" << std::endl;
    for (auto i = m_ipv4AddressEntry.begin(); i != m_ipv4AddressEntry.end(); ++i)
    {
        i->second.Print(stream, unit);
    }
    *os << std::endl;
    // Restore the previous ostream state
    (*os).copyfmt(oldState);
}

bool
RoutingTable::AddIpv4Event(Ipv4Address address, EventId id)
{
    auto result = m_ipv4Events.insert(std::make_pair(address, id));
    return result.second;
}

bool
RoutingTable::AnyRunningEvent(Ipv4Address address)
{
    EventId event;
    auto i = m_ipv4Events.find(address);
    if (m_ipv4Events.empty())
    {
        return false;
    }
    if (i == m_ipv4Events.end())
    {
        return false;
    }
    event = i->second;
    return event.IsRunning();
}

bool
RoutingTable::ForceDeleteIpv4Event(Ipv4Address address)
{
    EventId event;
    auto i = m_ipv4Events.find(address);
    if (m_ipv4Events.empty() || i == m_ipv4Events.end())
    {
        return false;
    }
    event = i->second;
    Simulator::Cancel(event);
    m_ipv4Events.erase(address);
    return true;
}

bool
RoutingTable::DeleteIpv4Event(Ipv4Address address)
{
    EventId event;
    auto i = m_ipv4Events.find(address);
    if (m_ipv4Events.empty() || i == m_ipv4Events.end())
    {
        return false;
    }
    event = i->second;
    if (event.IsRunning())
    {
        return false;
    }
    if (event.IsExpired())
    {
        event.Cancel();
        m_ipv4Events.erase(address);
        return true;
    }
    else
    {
        m_ipv4Events.erase(address);
        return true;
    }
}

EventId
RoutingTable::GetEventId(Ipv4Address address)
{
    auto i = m_ipv4Events.find(address);
    if (m_ipv4Events.empty() || i == m_ipv4Events.end())
    {
        return EventId();
    }
    else
    {
        return i->second;
    }
}

// === Multipath extensions (MP-DSDV-GNN) ===

bool
RoutingTable::AddMultipathRoute(RoutingTableEntry& rt)
{
    Ipv4Address dst = rt.GetDestination();

    // Primary path must exist for this destination
    auto primaryIt = m_ipv4AddressEntry.find(dst);
    if (primaryIt == m_ipv4AddressEntry.end())
    {
        return false;
    }

    // Check hop constraint: new path hop <= best_hop + DELTA
    uint32_t bestHop = primaryIt->second.GetHop();
    if (rt.GetHop() > bestHop + DELTA)
    {
        return false;
    }

    // Check next_hop disjointness: must differ from primary and all existing backups
    Ipv4Address newNextHop = rt.GetNextHop();
    if (newNextHop == primaryIt->second.GetNextHop())
    {
        return false; // same as primary next_hop
    }

    auto mpIt = m_multipathEntries.find(dst);
    if (mpIt != m_multipathEntries.end())
    {
        // Check K_MAX limit (primary + backups <= K_MAX)
        if (mpIt->second.size() + 1 >= K_MAX)
        {
            return false;
        }
        // Check next_hop not already in backup list
        for (const auto& entry : mpIt->second)
        {
            if (entry.GetNextHop() == newNextHop)
            {
                return false; // duplicate next_hop
            }
        }
        mpIt->second.push_back(rt);
    }
    else
    {
        // First backup path for this destination
        std::vector<RoutingTableEntry> paths;
        paths.push_back(rt);
        m_multipathEntries[dst] = paths;
    }
    return true;
}

bool
RoutingTable::GetAllPaths(Ipv4Address dst, std::vector<RoutingTableEntry>& paths) const
{
    paths.clear();
    auto primaryIt = m_ipv4AddressEntry.find(dst);
    if (primaryIt == m_ipv4AddressEntry.end())
    {
        return false;
    }
    paths.push_back(primaryIt->second);
    auto mpIt = m_multipathEntries.find(dst);
    if (mpIt != m_multipathEntries.end())
    {
        paths.insert(paths.end(), mpIt->second.begin(), mpIt->second.end());
    }
    return true;
}

bool
RoutingTable::GetBackupPaths(Ipv4Address dst, std::vector<RoutingTableEntry>& paths) const
{
    paths.clear();
    auto mpIt = m_multipathEntries.find(dst);
    if (mpIt == m_multipathEntries.end() || mpIt->second.empty())
    {
        return false;
    }
    paths = mpIt->second;
    return true;
}

uint32_t
RoutingTable::DeleteMultipathByNextHop(Ipv4Address nextHop)
{
    uint32_t removed = 0;
    for (auto it = m_multipathEntries.begin(); it != m_multipathEntries.end();)
    {
        auto& vec = it->second;
        auto before = vec.size();
        vec.erase(std::remove_if(vec.begin(),
                                 vec.end(),
                                 [&nextHop](const RoutingTableEntry& e) {
                                     return e.GetNextHop() == nextHop;
                                 }),
                  vec.end());
        removed += (before - vec.size());
        if (vec.empty())
        {
            it = m_multipathEntries.erase(it);
        }
        else
        {
            ++it;
        }
    }
    return removed;
}

void
RoutingTable::ClearMultipath(Ipv4Address dst)
{
    m_multipathEntries.erase(dst);
}

uint32_t
RoutingTable::GetPathCount(Ipv4Address dst) const
{
    auto primaryIt = m_ipv4AddressEntry.find(dst);
    if (primaryIt == m_ipv4AddressEntry.end())
    {
        return 0;
    }
    uint32_t count = 1; // primary
    auto mpIt = m_multipathEntries.find(dst);
    if (mpIt != m_multipathEntries.end())
    {
        count += mpIt->second.size();
    }
    return count;
}

uint32_t
RoutingTable::MultipathTableSize()
{
    uint32_t total = 0;
    for (const auto& entry : m_multipathEntries)
    {
        total += entry.second.size();
    }
    return total;
}

bool
RoutingTable::HasMultipathNextHop(Ipv4Address dst, Ipv4Address nextHop) const
{
    auto mpIt = m_multipathEntries.find(dst);
    if (mpIt == m_multipathEntries.end())
    {
        return false;
    }
    for (const auto& entry : mpIt->second)
    {
        if (entry.GetNextHop() == nextHop)
        {
            return true;
        }
    }
    return false;
}

bool
RoutingTable::RefreshMultipathLifetime(Ipv4Address dst, Ipv4Address nextHop)
{
    auto mpIt = m_multipathEntries.find(dst);
    if (mpIt == m_multipathEntries.end())
    {
        return false;
    }
    for (auto& entry : mpIt->second)
    {
        if (entry.GetNextHop() == nextHop)
        {
            entry.SetLifeTime(Simulator::Now());
            return true;
        }
    }
    return false;
}

void
RoutingTable::UpdateMultipathSeq(Ipv4Address dst, uint32_t newSeq)
{
    auto mpIt = m_multipathEntries.find(dst);
    if (mpIt == m_multipathEntries.end())
    {
        return;
    }
    for (auto& entry : mpIt->second)
    {
        entry.SetSeqNo(newSeq);
    }
}

} // namespace dsdv
} // namespace ns3
