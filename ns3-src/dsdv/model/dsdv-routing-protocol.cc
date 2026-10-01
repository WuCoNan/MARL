/*
 * Copyright (c) 2010 Hemanth Narra, Yufei Cheng
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
 * Author: Yufei Cheng   <yfcheng@ittc.ku.edu>
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

#include "dsdv-routing-protocol.h"

#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/inet-socket-address.h"
#include "ns3/log.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/uinteger.h"

#include <algorithm>
#include <sstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DsdvRoutingProtocol");

namespace dsdv
{

NS_OBJECT_ENSURE_REGISTERED(RoutingProtocol);

/// UDP Port for DSDV control traffic
const uint32_t RoutingProtocol::DSDV_PORT = 269;

/// Tag used by DSDV implementation
struct DeferredRouteOutputTag : public Tag
{
    /// Positive if output device is fixed in RouteOutput
    int32_t oif;

    /**
     * Constructor
     *
     * \param o outgoing interface (OIF)
     */
    DeferredRouteOutputTag(int32_t o = -1)
        : Tag(),
          oif(o)
    {
    }

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::dsdv::DeferredRouteOutputTag")
                                .SetParent<Tag>()
                                .SetGroupName("Dsdv")
                                .AddConstructor<DeferredRouteOutputTag>();
        return tid;
    }

    TypeId GetInstanceTypeId() const override
    {
        return GetTypeId();
    }

    uint32_t GetSerializedSize() const override
    {
        return sizeof(int32_t);
    }

    void Serialize(TagBuffer i) const override
    {
        i.WriteU32(oif);
    }

    void Deserialize(TagBuffer i) override
    {
        oif = i.ReadU32();
    }

    void Print(std::ostream& os) const override
    {
        os << "DeferredRouteOutputTag: output interface = " << oif;
    }
};

TypeId
RoutingProtocol::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::dsdv::RoutingProtocol")
            .SetParent<Ipv4RoutingProtocol>()
            .SetGroupName("Dsdv")
            .AddConstructor<RoutingProtocol>()
            .AddAttribute("PeriodicUpdateInterval",
                          "Periodic interval between exchange of full routing tables among nodes.",
                          TimeValue(Seconds(15)),
                          MakeTimeAccessor(&RoutingProtocol::m_periodicUpdateInterval),
                          MakeTimeChecker())
            .AddAttribute("SettlingTime",
                          "Minimum time an update is to be stored in adv table before sending out "
                          "in case of change in metric (in seconds)",
                          TimeValue(Seconds(5)),
                          MakeTimeAccessor(&RoutingProtocol::m_settlingTime),
                          MakeTimeChecker())
            .AddAttribute("MaxQueueLen",
                          "Maximum number of packets that we allow a routing protocol to buffer.",
                          UintegerValue(500 /*assuming maximum nodes in simulation is 100*/),
                          MakeUintegerAccessor(&RoutingProtocol::m_maxQueueLen),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("MaxQueuedPacketsPerDst",
                          "Maximum number of packets that we allow per destination to buffer.",
                          UintegerValue(5),
                          MakeUintegerAccessor(&RoutingProtocol::m_maxQueuedPacketsPerDst),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("MaxQueueTime",
                          "Maximum time packets can be queued (in seconds)",
                          TimeValue(Seconds(30)),
                          MakeTimeAccessor(&RoutingProtocol::m_maxQueueTime),
                          MakeTimeChecker())
            .AddAttribute(
                "EnableBuffering",
                "Enables buffering of data packets if no route to destination is available",
                BooleanValue(true),
                MakeBooleanAccessor(&RoutingProtocol::SetEnableBufferFlag,
                                    &RoutingProtocol::GetEnableBufferFlag),
                MakeBooleanChecker())
            .AddAttribute(
                "EnableWST",
                "Enables Weighted Settling Time for the updates before advertising",
                BooleanValue(true),
                MakeBooleanAccessor(&RoutingProtocol::SetWSTFlag, &RoutingProtocol::GetWSTFlag),
                MakeBooleanChecker())
            .AddAttribute("Holdtimes",
                          "Times the forwarding Interval to purge the route.",
                          UintegerValue(3),
                          MakeUintegerAccessor(&RoutingProtocol::Holdtimes),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute(
                "WeightedFactor",
                "WeightedFactor for the settling time if Weighted Settling Time is enabled",
                DoubleValue(0.875),
                MakeDoubleAccessor(&RoutingProtocol::m_weightedFactor),
                MakeDoubleChecker<double>())
            .AddAttribute("EnableRouteAggregation",
                          "Enables Weighted Settling Time for the updates before advertising",
                          BooleanValue(false),
                          MakeBooleanAccessor(&RoutingProtocol::SetEnableRAFlag,
                                              &RoutingProtocol::GetEnableRAFlag),
                          MakeBooleanChecker())
            .AddAttribute("RouteAggregationTime",
                          "Time to aggregate updates before sending them out (in seconds)",
                          TimeValue(Seconds(1)),
                          MakeTimeAccessor(&RoutingProtocol::m_routeAggregationTime),
                          MakeTimeChecker());
    return tid;
}

void
RoutingProtocol::SetEnableBufferFlag(bool f)
{
    EnableBuffering = f;
}

bool
RoutingProtocol::GetEnableBufferFlag() const
{
    return EnableBuffering;
}

void
RoutingProtocol::SetWSTFlag(bool f)
{
    EnableWST = f;
}

bool
RoutingProtocol::GetWSTFlag() const
{
    return EnableWST;
}

void
RoutingProtocol::SetEnableRAFlag(bool f)
{
    EnableRouteAggregation = f;
}

bool
RoutingProtocol::GetEnableRAFlag() const
{
    return EnableRouteAggregation;
}

int64_t
RoutingProtocol::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_uniformRandomVariable->SetStream(stream);
    return 1;
}

RoutingProtocol::RoutingProtocol()
    : m_routingTable(),
      m_advRoutingTable(),
      m_queue(),
      m_periodicUpdateTimer(Timer::CANCEL_ON_DESTROY),
      m_lastTriggeredUpdateTime(Seconds(0)),
      m_triggerUpdatePending(false),
      m_featureBroadcastTimer(Timer::CANCEL_ON_DESTROY)
{
    m_uniformRandomVariable = CreateObject<UniformRandomVariable>();
}

RoutingProtocol::~RoutingProtocol()
{
}

void
RoutingProtocol::DoDispose()
{
    m_ipv4 = nullptr;
    for (auto iter = m_socketAddresses.begin(); iter != m_socketAddresses.end(); iter++)
    {
        iter->first->Close();
    }
    m_socketAddresses.clear();
    Ipv4RoutingProtocol::DoDispose();
}

void
RoutingProtocol::PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
    *stream->GetStream() << "Node: " << m_ipv4->GetObject<Node>()->GetId()
                         << ", Time: " << Now().As(unit)
                         << ", Local time: " << m_ipv4->GetObject<Node>()->GetLocalTime().As(unit)
                         << ", DSDV Routing table" << std::endl;

    m_routingTable.Print(stream, unit);
    *stream->GetStream() << std::endl;
}

void
RoutingProtocol::Start()
{
    m_queue.SetMaxPacketsPerDst(m_maxQueuedPacketsPerDst);
    m_queue.SetMaxQueueLen(m_maxQueueLen);
    m_queue.SetQueueTimeout(m_maxQueueTime);
    m_routingTable.Setholddowntime(Time(Holdtimes * m_periodicUpdateInterval));
    m_advRoutingTable.Setholddowntime(Time(Holdtimes * m_periodicUpdateInterval));
    m_scb = MakeCallback(&RoutingProtocol::Send, this);
    m_ecb = MakeCallback(&RoutingProtocol::Drop, this);
    m_periodicUpdateTimer.SetFunction(&RoutingProtocol::SendPeriodicUpdate, this);
    m_periodicUpdateTimer.Schedule(MicroSeconds(m_uniformRandomVariable->GetInteger(0, 1000)));
    // MP-DSDV-GNN: start feature broadcast (1s interval, with random jitter to avoid collision)
    Simulator::Schedule(Seconds(FEATURE_BROADCAST_INTERVAL_S) +
                        MicroSeconds(m_uniformRandomVariable->GetInteger(0, 500000)),
                        &RoutingProtocol::SendFeatureBroadcast, this);
    // MP-DSDV-GNN: start GNN observation generation (every 10ms)
    Simulator::Schedule(Seconds(TIME_SLOT_S), &RoutingProtocol::GenerateGnnObservation, this);
}

Ptr<Ipv4Route>
RoutingProtocol::RouteOutput(Ptr<Packet> p,
                             const Ipv4Header& header,
                             Ptr<NetDevice> oif,
                             Socket::SocketErrno& sockerr)
{
    NS_LOG_FUNCTION(this << header << (oif ? oif->GetIfIndex() : 0));

    if (!p)
    {
        return LoopbackRoute(header, oif);
    }
    if (m_socketAddresses.empty())
    {
        sockerr = Socket::ERROR_NOROUTETOHOST;
        NS_LOG_LOGIC("No dsdv interfaces");
        Ptr<Ipv4Route> route;
        return route;
    }
    std::map<Ipv4Address, RoutingTableEntry> removedAddresses;
    sockerr = Socket::ERROR_NOTERROR;
    Ptr<Ipv4Route> route;
    Ipv4Address dst = header.GetDestination();
    NS_LOG_DEBUG("Packet Size: " << p->GetSize() << ", Packet id: " << p->GetUid()
                                 << ", Destination address in Packet: " << dst);
    RoutingTableEntry rt;
    m_routingTable.Purge(removedAddresses);
    for (auto rmItr = removedAddresses.begin(); rmItr != removedAddresses.end(); ++rmItr)
    {
        rmItr->second.SetEntriesChanged(true);
        rmItr->second.SetSeqNo(rmItr->second.GetSeqNo() + 1);
        m_advRoutingTable.AddRoute(rmItr->second);
    }
    if (!removedAddresses.empty())
    {
        Simulator::Schedule(MicroSeconds(m_uniformRandomVariable->GetInteger(0, 1000)),
                            &RoutingProtocol::SendTriggeredUpdate,
                            this);
    }
    if (m_routingTable.LookupRoute(dst, rt))
    {
        if (EnableBuffering)
        {
            LookForQueuedPackets();
        }
        if (rt.GetHop() == 1)
        {
            route = rt.GetRoute();
            NS_ASSERT(route);
            NS_LOG_DEBUG("A route exists from " << route->GetSource()
                                                << " to neighboring destination "
                                                << route->GetDestination());
            if (oif && route->GetOutputDevice() != oif)
            {
                NS_LOG_DEBUG("Output device doesn't match. Dropped.");
                sockerr = Socket::ERROR_NOROUTETOHOST;
                return Ptr<Ipv4Route>();
            }
            // MP-DSDV-GNN: count forwarded bytes for link_utilization (skip broadcast)
            {
                Ipv4Address gw = route->GetGateway();
                if (!gw.IsBroadcast() && (gw.Get() & 0xFF) != 0xFF)
                {
                    m_linkStats[gw].txBytes += p->GetSize();
                    m_totalForwardedBytes += p->GetSize();
                }
            }
            return route;
        }
        else
        {
            RoutingTableEntry newrt;
            if (m_routingTable.LookupRoute(rt.GetNextHop(), newrt))
            {
                route = newrt.GetRoute();
                NS_ASSERT(route);
                NS_LOG_DEBUG("A route exists from " << route->GetSource() << " to destination "
                                                    << dst << " via " << rt.GetNextHop());
                if (oif && route->GetOutputDevice() != oif)
                {
                    NS_LOG_DEBUG("Output device doesn't match. Dropped.");
                    sockerr = Socket::ERROR_NOROUTETOHOST;
                    return Ptr<Ipv4Route>();
                }
                // MP-DSDV-GNN: count forwarded bytes for link_utilization (skip broadcast)
                {
                    Ipv4Address nh = rt.GetNextHop();
                    if (!nh.IsBroadcast() && (nh.Get() & 0xFF) != 0xFF)
                    {
                        m_linkStats[nh].txBytes += p->GetSize();
                        m_totalForwardedBytes += p->GetSize();
                    }
                }
                return route;
            }
        }
    }

    if (EnableBuffering)
    {
        uint32_t iif = (oif ? m_ipv4->GetInterfaceForDevice(oif) : -1);
        DeferredRouteOutputTag tag(iif);
        if (!p->PeekPacketTag(tag))
        {
            p->AddPacketTag(tag);
        }
    }
    return LoopbackRoute(header, oif);
}

void
RoutingProtocol::DeferredRouteOutput(Ptr<const Packet> p,
                                     const Ipv4Header& header,
                                     UnicastForwardCallback ucb,
                                     ErrorCallback ecb)
{
    NS_LOG_FUNCTION(this << p << header);
    NS_ASSERT(p && p != Ptr<Packet>());
    QueueEntry newEntry(p, header, ucb, ecb);
    bool result = m_queue.Enqueue(newEntry);
    if (result)
    {
        NS_LOG_DEBUG("Added packet " << p->GetUid() << " to queue.");
    }
}

bool
RoutingProtocol::RouteInput(Ptr<const Packet> p,
                            const Ipv4Header& header,
                            Ptr<const NetDevice> idev,
                            const UnicastForwardCallback& ucb,
                            const MulticastForwardCallback& mcb,
                            const LocalDeliverCallback& lcb,
                            const ErrorCallback& ecb)
{
    NS_LOG_FUNCTION(m_mainAddress << " received packet " << p->GetUid() << " from "
                                  << header.GetSource() << " on interface " << idev->GetAddress()
                                  << " to destination " << header.GetDestination());
    if (m_socketAddresses.empty())
    {
        NS_LOG_DEBUG("No dsdv interfaces");
        return false;
    }
    NS_ASSERT(m_ipv4);
    // Check if input device supports IP
    NS_ASSERT(m_ipv4->GetInterfaceForDevice(idev) >= 0);
    int32_t iif = m_ipv4->GetInterfaceForDevice(idev);

    Ipv4Address dst = header.GetDestination();
    Ipv4Address origin = header.GetSource();

    // DSDV is not a multicast routing protocol
    if (dst.IsMulticast())
    {
        return false;
    }

    // Deferred route request
    if (EnableBuffering && idev == m_lo)
    {
        DeferredRouteOutputTag tag;
        if (p->PeekPacketTag(tag))
        {
            DeferredRouteOutput(p, header, ucb, ecb);
            return true;
        }
    }
    for (auto j = m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j)
    {
        Ipv4InterfaceAddress iface = j->second;
        if (origin == iface.GetLocal())
        {
            return true;
        }
    }
    // LOCAL DELIVARY TO DSDV INTERFACES
    for (auto j = m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j)
    {
        Ipv4InterfaceAddress iface = j->second;
        if (m_ipv4->GetInterfaceForAddress(iface.GetLocal()) == iif)
        {
            if (dst == iface.GetBroadcast() || dst.IsBroadcast())
            {
                Ptr<Packet> packet = p->Copy();
                if (!lcb.IsNull())
                {
                    NS_LOG_LOGIC("Broadcast local delivery to " << iface.GetLocal());
                    lcb(p, header, iif);
                    // Fall through to additional processing
                }
                else
                {
                    NS_LOG_ERROR("Unable to deliver packet locally due to null callback "
                                 << p->GetUid() << " from " << origin);
                    ecb(p, header, Socket::ERROR_NOROUTETOHOST);
                }
                if (header.GetTtl() > 1)
                {
                    NS_LOG_LOGIC("Forward broadcast. TTL " << (uint16_t)header.GetTtl());
                    RoutingTableEntry toBroadcast;
                    if (m_routingTable.LookupRoute(dst, toBroadcast, true))
                    {
                        Ptr<Ipv4Route> route = toBroadcast.GetRoute();
                        ucb(route, packet, header);
                    }
                    else
                    {
                        NS_LOG_DEBUG("No route to forward. Drop packet " << p->GetUid());
                    }
                }
                return true;
            }
        }
    }

    if (m_ipv4->IsDestinationAddress(dst, iif))
    {
        if (!lcb.IsNull())
        {
            NS_LOG_LOGIC("Unicast local delivery to " << dst);
            lcb(p, header, iif);
        }
        else
        {
            NS_LOG_ERROR("Unable to deliver packet locally due to null callback "
                         << p->GetUid() << " from " << origin);
            ecb(p, header, Socket::ERROR_NOROUTETOHOST);
        }
        return true;
    }

    // Check if input device supports IP forwarding
    if (!m_ipv4->IsForwarding(iif))
    {
        NS_LOG_LOGIC("Forwarding disabled for this interface");
        ecb(p, header, Socket::ERROR_NOROUTETOHOST);
        return true;
    }

    RoutingTableEntry toDst;
    if (m_routingTable.LookupRoute(dst, toDst))
    {
        RoutingTableEntry ne;
        if (m_routingTable.LookupRoute(toDst.GetNextHop(), ne))
        {
            Ptr<Ipv4Route> route = ne.GetRoute();
            NS_LOG_LOGIC(m_mainAddress << " is forwarding packet " << p->GetUid() << " to " << dst
                                       << " from " << header.GetSource() << " via nexthop neighbor "
                                       << toDst.GetNextHop());
            ucb(route, p, header);
            return true;
        }
    }
    NS_LOG_LOGIC("Drop packet " << p->GetUid() << " as there is no route to forward it.");
    return false;
}

Ptr<Ipv4Route>
RoutingProtocol::LoopbackRoute(const Ipv4Header& hdr, Ptr<NetDevice> oif) const
{
    NS_ASSERT(m_lo);
    Ptr<Ipv4Route> rt = Create<Ipv4Route>();
    rt->SetDestination(hdr.GetDestination());
    // rt->SetSource (hdr.GetSource ());
    //
    // Source address selection here is tricky.  The loopback route is
    // returned when DSDV does not have a route; this causes the packet
    // to be looped back and handled (cached) in RouteInput() method
    // while a route is found. However, connection-oriented protocols
    // like TCP need to create an endpoint four-tuple (src, src port,
    // dst, dst port) and create a pseudo-header for checksumming.  So,
    // DSDV needs to guess correctly what the eventual source address
    // will be.
    //
    // For single interface, single address nodes, this is not a problem.
    // When there are possibly multiple outgoing interfaces, the policy
    // implemented here is to pick the first available DSDV interface.
    // If RouteOutput() caller specified an outgoing interface, that
    // further constrains the selection of source address
    //
    auto j = m_socketAddresses.begin();
    if (oif)
    {
        // Iterate to find an address on the oif device
        for (j = m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j)
        {
            Ipv4Address addr = j->second.GetLocal();
            int32_t interface = m_ipv4->GetInterfaceForAddress(addr);
            if (oif == m_ipv4->GetNetDevice(static_cast<uint32_t>(interface)))
            {
                rt->SetSource(addr);
                break;
            }
        }
    }
    else
    {
        rt->SetSource(j->second.GetLocal());
    }
    NS_ASSERT_MSG(rt->GetSource() != Ipv4Address(), "Valid DSDV source address not found");
    rt->SetGateway(Ipv4Address("127.0.0.1"));
    rt->SetOutputDevice(m_lo);
    return rt;
}

void
RoutingProtocol::RecvDsdv(Ptr<Socket> socket)
{
    Address sourceAddress;
    Ptr<Packet> advpacket = Create<Packet>();
    Ptr<Packet> packet = socket->RecvFrom(sourceAddress);
    InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom(sourceAddress);
    Ipv4Address sender = inetSourceAddr.GetIpv4();
    Ipv4Address receiver = m_socketAddresses[socket].GetLocal();
    Ptr<NetDevice> dev = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(receiver));
    uint32_t packetSize = packet->GetSize();
    NS_LOG_FUNCTION(m_mainAddress << " received dsdv packet of size: " << packetSize
                                  << " and packet id: " << packet->GetUid());

    // MP-DSDV-GNN: Detect feature broadcast packet by magic byte 0xFF.
    // DSDV route updates start with an IPv4 address (first byte = 10 for 10.x.x.x),
    // while feature broadcasts always start with 0xFF magic byte.
    if (packetSize > 0)
    {
        uint8_t firstByte;
        packet->CopyData(&firstByte, 1);
        if (firstByte == 0xFF)
        {
            // This is a feature broadcast packet, handle separately
            FeatureBroadcastHeader featHeader;
            packet->RemoveHeader(featHeader);
            NodeFeature senderSelfFeature = featHeader.GetSelfFeature();
            std::map<Ipv4Address, NodeFeature> senderNbrFeatures = featHeader.GetNeighborFeatures();
            std::vector<LinkFeature> senderLinkFeatures = featHeader.GetLinkFeatures();
            m_featureStore.UpdateEntry(sender, senderSelfFeature, senderLinkFeatures, senderNbrFeatures);
            m_nbrNodeFeatures[sender] = senderSelfFeature;
            // MAC→IP learning: associate last PHY-level MAC with this sender's IP
            if (m_lastRxMac != Mac48Address())
            {
                m_macToIpMap[m_lastRxMac] = sender;
            }
            NS_LOG_DEBUG("Feature broadcast received from " << sender
                          << ": nbr_feats=" << senderNbrFeatures.size()
                          << " link_feats=" << senderLinkFeatures.size());
            return; // done, don't process as route update
        }
    }

    // MAC→IP learning: also learn from route update sender
    if (m_lastRxMac != Mac48Address())
    {
        m_macToIpMap[m_lastRxMac] = sender;
    }

    uint32_t count = 0;
    for (; packetSize > 0; packetSize = packetSize - 12)
    {
        count = 0;
        DsdvHeader dsdvHeader;
        DsdvHeader tempDsdvHeader;
        packet->RemoveHeader(dsdvHeader);
        NS_LOG_DEBUG("Processing new update for " << dsdvHeader.GetDst());
        /*Verifying if the packets sent by me were returned back to me. If yes, discarding them!*/
        for (auto j = m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j)
        {
            Ipv4InterfaceAddress interface = j->second;
            if (dsdvHeader.GetDst() == interface.GetLocal())
            {
                if (dsdvHeader.GetDstSeqno() % 2 == 1)
                {
                    NS_LOG_DEBUG("Sent Dsdv update back to the same Destination, "
                                 "with infinite metric. Time left to send fwd update: "
                                 << m_periodicUpdateTimer.GetDelayLeft());
                    count++;
                }
                else
                {
                    NS_LOG_DEBUG("Received update for my address. Discarding this.");
                    count++;
                }
            }
        }
        if (count > 0)
        {
            continue;
        }
        NS_LOG_DEBUG("Received a DSDV packet from "
                     << sender << " to " << receiver << ". Details are: Destination: "
                     << dsdvHeader.GetDst() << ", Seq No: " << dsdvHeader.GetDstSeqno()
                     << ", HopCount: " << dsdvHeader.GetHopCount());
        RoutingTableEntry fwdTableEntry;
        RoutingTableEntry advTableEntry;
        EventId event;
        bool permanentTableVerifier =
            m_routingTable.LookupRoute(dsdvHeader.GetDst(), fwdTableEntry);
        if (!permanentTableVerifier)
        {
            if (dsdvHeader.GetDstSeqno() % 2 != 1)
            {
                NS_LOG_DEBUG("Received New Route!");
                RoutingTableEntry newEntry(
                    /*dev=*/dev,
                    /*dst=*/dsdvHeader.GetDst(),
                    /*seqNo=*/dsdvHeader.GetDstSeqno(),
                    /*iface=*/m_ipv4->GetAddress(m_ipv4->GetInterfaceForAddress(receiver), 0),
                    /*hops=*/dsdvHeader.GetHopCount(),
                    /*nextHop=*/sender,
                    /*lifetime=*/Simulator::Now(),
                    /*settlingTime=*/m_settlingTime,
                    /*changedEntries=*/true);
                newEntry.SetFlag(VALID);
                m_routingTable.AddRoute(newEntry);
                NS_LOG_DEBUG("New Route added to both tables");
                m_advRoutingTable.AddRoute(newEntry);
                // MP-DSDV-GNN: detect new neighbor (hop=1 means sender is direct neighbor)
                if (dsdvHeader.GetHopCount() == 1)
                {
                    OnNewNeighbor(sender);
                }
            }
            else
            {
                // received update not present in main routing table and also with infinite metric
                NS_LOG_DEBUG("Discarding this update as this route is not present in "
                             "main routing table and received with infinite metric");
            }
        }
        else
        {
            if (!m_advRoutingTable.LookupRoute(dsdvHeader.GetDst(), advTableEntry))
            {
                RoutingTableEntry tr;
                std::map<Ipv4Address, RoutingTableEntry> allRoutes;
                m_advRoutingTable.GetListOfAllRoutes(allRoutes);
                for (auto i = allRoutes.begin(); i != allRoutes.end(); ++i)
                {
                    NS_LOG_DEBUG("ADV table routes are:" << i->second.GetDestination());
                }
                // present in fwd table and not in advtable
                m_advRoutingTable.AddRoute(fwdTableEntry);
                m_advRoutingTable.LookupRoute(dsdvHeader.GetDst(), advTableEntry);
            }
            if (dsdvHeader.GetDstSeqno() % 2 != 1)
            {
                if (dsdvHeader.GetDstSeqno() > advTableEntry.GetSeqNo())
                {
                    // Received update with better seq number. Clear any old events that are running
                    if (m_advRoutingTable.ForceDeleteIpv4Event(dsdvHeader.GetDst()))
                    {
                        NS_LOG_DEBUG("Canceling the timer to update route with better seq number");
                    }
                    // if its a changed metric *nomatter* where the update came from, wait  for WST
                    if (dsdvHeader.GetHopCount() != advTableEntry.GetHop())
                    {
                        // === MP-DSDV-GNN: primary path changed (metric different) ===
                        Ipv4Address oldNextHop = advTableEntry.GetNextHop();
                        uint32_t oldHop = advTableEntry.GetHop();

                        advTableEntry.SetSeqNo(dsdvHeader.GetDstSeqno());
                        advTableEntry.SetLifeTime(Simulator::Now());
                        advTableEntry.SetFlag(VALID);
                        advTableEntry.SetEntriesChanged(true);
                        advTableEntry.SetNextHop(sender);
                        advTableEntry.SetHop(dsdvHeader.GetHopCount());
                        NS_LOG_DEBUG("Received update with better sequence number and changed "
                                     "metric.Waiting for WST");
                        Time tempSettlingtime = GetSettlingTime(dsdvHeader.GetDst());
                        advTableEntry.SetSettlingTime(tempSettlingtime);
                        event = Simulator::Schedule(tempSettlingtime,
                                                    &RoutingProtocol::SendTriggeredUpdate,
                                                    this);
                        m_advRoutingTable.AddIpv4Event(dsdvHeader.GetDst(), event);
                        m_routingTable.Update(advTableEntry);
                        m_advRoutingTable.Update(advTableEntry);

                        // MP-DSDV-GNN: demote old primary to backup if next-hop changed
                        if (sender != oldNextHop && oldHop > 0)
                        {
                            RoutingTableEntry demoted = fwdTableEntry; // old primary
                            demoted.SetLifeTime(Simulator::Now());
                            m_routingTable.AddMultipathRoute(demoted);
                            NS_LOG_DEBUG("MP-DSDV-GNN: Demoted old primary via " << oldNextHop
                                         << " to backup for " << dsdvHeader.GetDst());
                        }
                        // Remove backups that now violate DELTA constraint
                        // (keep those within new best_hop + DELTA)
                        m_routingTable.UpdateMultipathSeq(dsdvHeader.GetDst(),
                                                          dsdvHeader.GetDstSeqno());
                    }
                    else
                    {
                        // Received update with better seq number and same metric.
                        Ipv4Address oldNextHop = advTableEntry.GetNextHop();
                        advTableEntry.SetSeqNo(dsdvHeader.GetDstSeqno());
                        advTableEntry.SetLifeTime(Simulator::Now());
                        advTableEntry.SetFlag(VALID);
                        advTableEntry.SetEntriesChanged(true);
                        advTableEntry.SetNextHop(sender);
                        advTableEntry.SetHop(dsdvHeader.GetHopCount());
                        m_advRoutingTable.Update(advTableEntry);

                        if (sender != oldNextHop)
                        {
                            // MP-DSDV-GNN: primary next-hop changed, demote old to backup
                            RoutingTableEntry demoted = fwdTableEntry;
                            demoted.SetSeqNo(dsdvHeader.GetDstSeqno());
                            demoted.SetLifeTime(Simulator::Now());
                            m_routingTable.AddMultipathRoute(demoted);
                            m_routingTable.DeleteMultipathByNextHop(sender); // avoid dup
                            NS_LOG_DEBUG("MP-DSDV-GNN: Primary switched to " << sender
                                         << ", demoted " << oldNextHop << " to backup");
                        }
                        // Sync backup seq numbers with new primary seq
                        m_routingTable.UpdateMultipathSeq(dsdvHeader.GetDst(),
                                                          dsdvHeader.GetDstSeqno());
                        NS_LOG_DEBUG("Route with better sequence number and same metric received. "
                                     "Advertised without WST");
                    }
                }
                else if (dsdvHeader.GetDstSeqno() == advTableEntry.GetSeqNo())
                {
                    if (dsdvHeader.GetHopCount() < advTableEntry.GetHop())
                    {
                        /*Received update with same seq number and better hop count.
                         * As the metric is changed, we will have to wait for WST before sending out
                         * this update.
                         */
                        NS_LOG_DEBUG("Canceling any existing timer to update route with same "
                                     "sequence number "
                                     "and better hop count");
                        m_advRoutingTable.ForceDeleteIpv4Event(dsdvHeader.GetDst());

                        // === MP-DSDV-GNN: demote old primary before replacing ===
                        Ipv4Address oldNextHop = advTableEntry.GetNextHop();
                        if (sender != oldNextHop)
                        {
                            RoutingTableEntry demoted = fwdTableEntry;
                            demoted.SetLifeTime(Simulator::Now());
                            m_routingTable.AddMultipathRoute(demoted);
                        }

                        advTableEntry.SetSeqNo(dsdvHeader.GetDstSeqno());
                        advTableEntry.SetLifeTime(Simulator::Now());
                        advTableEntry.SetFlag(VALID);
                        advTableEntry.SetEntriesChanged(true);
                        advTableEntry.SetNextHop(sender);
                        advTableEntry.SetHop(dsdvHeader.GetHopCount());
                        Time tempSettlingtime = GetSettlingTime(dsdvHeader.GetDst());
                        advTableEntry.SetSettlingTime(tempSettlingtime);
                        event = Simulator::Schedule(tempSettlingtime,
                                                    &RoutingProtocol::SendTriggeredUpdate,
                                                    this);
                        m_advRoutingTable.AddIpv4Event(dsdvHeader.GetDst(), event);
                        m_routingTable.Update(advTableEntry);
                        m_advRoutingTable.Update(advTableEntry);
                        // Sync backup seq
                        m_routingTable.UpdateMultipathSeq(dsdvHeader.GetDst(),
                                                          dsdvHeader.GetDstSeqno());
                    }
                    else
                    {
                        /*Received update with same seq number but with same or greater hop count.
                         * MP-DSDV-GNN: Try to add as multipath if next_hop differs and hop is
                         * within DELTA of best. Otherwise discard.
                         */
                        if (!m_advRoutingTable.AnyRunningEvent(dsdvHeader.GetDst()))
                        {
                            /*update the timer only if nexthop address matches thus discarding
                             * updates to that destination from other nodes.
                             */
                            if (advTableEntry.GetNextHop() == sender)
                            {
                                advTableEntry.SetLifeTime(Simulator::Now());
                                m_routingTable.Update(advTableEntry);
                            }
                            m_advRoutingTable.DeleteRoute(dsdvHeader.GetDst());
                        }
                        // === MP-DSDV-GNN multipath maintenance ===
                        if (advTableEntry.GetNextHop() != sender)
                        {
                            // Check if sender is already a backup next-hop
                            if (m_routingTable.HasMultipathNextHop(dsdvHeader.GetDst(), sender))
                            {
                                // Existing backup: refresh lifetime (prevent expiry)
                                m_routingTable.RefreshMultipathLifetime(dsdvHeader.GetDst(), sender);
                            }
                            else
                            {
                                // New potential backup: try to add
                                RoutingTableEntry mpEntry(
                                    /*dev=*/dev,
                                    /*dst=*/dsdvHeader.GetDst(),
                                    /*seqNo=*/dsdvHeader.GetDstSeqno(),
                                    /*iface=*/m_ipv4->GetAddress(
                                        m_ipv4->GetInterfaceForAddress(receiver), 0),
                                    /*hops=*/dsdvHeader.GetHopCount(),
                                    /*nextHop=*/sender,
                                    /*lifetime=*/Simulator::Now(),
                                    /*settlingTime=*/Simulator::Now(),
                                    /*changedEntries=*/false);
                                mpEntry.SetFlag(VALID);
                                if (m_routingTable.AddMultipathRoute(mpEntry))
                                {
                                    NS_LOG_DEBUG("MP-DSDV-GNN: Added multipath to "
                                                 << dsdvHeader.GetDst() << " via " << sender
                                                 << " hops=" << dsdvHeader.GetHopCount());
                                }
                            }
                        }
                        else
                        {
                            NS_LOG_DEBUG("Received update with same seq number and "
                                         "same/worst metric for, "
                                         << dsdvHeader.GetDst() << ". Discarding the update.");
                        }
                    }
                }
                else
                {
                    // Received update with an old sequence number. Discard the update
                    if (!m_advRoutingTable.AnyRunningEvent(dsdvHeader.GetDst()))
                    {
                        m_advRoutingTable.DeleteRoute(dsdvHeader.GetDst());
                    }
                    NS_LOG_DEBUG(
                        dsdvHeader.GetDst()
                        << " : Received update with old seq number. Discarding the update.");
                }
            }
            else
            {
                NS_LOG_DEBUG("Route with infinite metric received for " << dsdvHeader.GetDst()
                                                                        << " from " << sender);
                // Delete route only if update was received from my nexthop neighbor
                if (sender == advTableEntry.GetNextHop())
                {
                    NS_LOG_DEBUG("Triggering an update for this unreachable route:");
                    std::map<Ipv4Address, RoutingTableEntry> dstsWithNextHopSrc;
                    m_routingTable.GetListOfDestinationWithNextHop(dsdvHeader.GetDst(),
                                                                   dstsWithNextHopSrc);
                    m_routingTable.DeleteRoute(dsdvHeader.GetDst());
                    // MP-DSDV-GNN: clear multipath for deleted dest + remove all paths via sender
                    m_routingTable.ClearMultipath(dsdvHeader.GetDst());
                    m_routingTable.DeleteMultipathByNextHop(sender);
                    advTableEntry.SetSeqNo(dsdvHeader.GetDstSeqno());
                    advTableEntry.SetEntriesChanged(true);
                    m_advRoutingTable.Update(advTableEntry);
                    for (auto i = dstsWithNextHopSrc.begin(); i != dstsWithNextHopSrc.end(); ++i)
                    {
                        i->second.SetSeqNo(i->second.GetSeqNo() + 1);
                        i->second.SetEntriesChanged(true);
                        m_advRoutingTable.AddRoute(i->second);
                        m_routingTable.DeleteRoute(i->second.GetDestination());
                    }
                }
                else
                {
                    if (!m_advRoutingTable.AnyRunningEvent(dsdvHeader.GetDst()))
                    {
                        m_advRoutingTable.DeleteRoute(dsdvHeader.GetDst());
                    }
                    NS_LOG_DEBUG(dsdvHeader.GetDst() << " : Discard this link break update as it "
                                                        "was received from a different neighbor "
                                                        "and I can reach the destination");
                }
            }
        }
    }
    std::map<Ipv4Address, RoutingTableEntry> allRoutes;
    m_advRoutingTable.GetListOfAllRoutes(allRoutes);
    if (EnableRouteAggregation && !allRoutes.empty())
    {
        Simulator::Schedule(m_routeAggregationTime, &RoutingProtocol::SendTriggeredUpdate, this);
    }
    else
    {
        Simulator::Schedule(MicroSeconds(m_uniformRandomVariable->GetInteger(0, 1000)),
                            &RoutingProtocol::SendTriggeredUpdate,
                            this);
    }
}

void
RoutingProtocol::SendTriggeredUpdate()
{
    NS_LOG_FUNCTION(m_mainAddress << " is sending a triggered update");
    std::map<Ipv4Address, RoutingTableEntry> allRoutes;
    m_advRoutingTable.GetListOfAllRoutes(allRoutes);
    for (auto j = m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j)
    {
        DsdvHeader dsdvHeader;
        Ptr<Socket> socket = j->first;
        Ipv4InterfaceAddress iface = j->second;
        Ptr<Packet> packet = Create<Packet>();
        for (auto i = allRoutes.begin(); i != allRoutes.end(); ++i)
        {
            NS_LOG_LOGIC("Destination: " << i->second.GetDestination()
                                         << " SeqNo:" << i->second.GetSeqNo()
                                         << " HopCount:" << i->second.GetHop() + 1);
            RoutingTableEntry temp = i->second;
            if (i->second.GetEntriesChanged() &&
                !m_advRoutingTable.AnyRunningEvent(temp.GetDestination()))
            {
                dsdvHeader.SetDst(i->second.GetDestination());
                dsdvHeader.SetDstSeqno(i->second.GetSeqNo());
                dsdvHeader.SetHopCount(i->second.GetHop() + 1);
                temp.SetFlag(VALID);
                temp.SetEntriesChanged(false);
                m_advRoutingTable.DeleteIpv4Event(temp.GetDestination());
                if (!(temp.GetSeqNo() % 2))
                {
                    m_routingTable.Update(temp);
                }
                packet->AddHeader(dsdvHeader);
                m_advRoutingTable.DeleteRoute(temp.GetDestination());
                NS_LOG_DEBUG("Deleted this route from the advertised table");
            }
            else
            {
                EventId event = m_advRoutingTable.GetEventId(temp.GetDestination());
                NS_ASSERT(event.GetUid() != 0);
                NS_LOG_DEBUG("EventID " << event.GetUid() << " associated with "
                                        << temp.GetDestination()
                                        << " has not expired, waiting in adv table");
            }
        }
        if (packet->GetSize() >= 12)
        {
            RoutingTableEntry temp2;
            m_routingTable.LookupRoute(m_ipv4->GetAddress(1, 0).GetBroadcast(), temp2);
            dsdvHeader.SetDst(m_ipv4->GetAddress(1, 0).GetLocal());
            dsdvHeader.SetDstSeqno(temp2.GetSeqNo());
            dsdvHeader.SetHopCount(temp2.GetHop() + 1);
            NS_LOG_DEBUG("Adding my update as well to the packet");
            packet->AddHeader(dsdvHeader);
            // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
            Ipv4Address destination;
            if (iface.GetMask() == Ipv4Mask::GetOnes())
            {
                destination = Ipv4Address("255.255.255.255");
            }
            else
            {
                destination = iface.GetBroadcast();
            }
            socket->SendTo(packet, 0, InetSocketAddress(destination, DSDV_PORT));
            NS_LOG_FUNCTION("Sent Triggered Update from "
                            << dsdvHeader.GetDst() << " with packet id : " << packet->GetUid()
                            << " and packet Size: " << packet->GetSize());
        }
        else
        {
            NS_LOG_FUNCTION("Update not sent as there are no updates to be triggered");
        }
    }
}

// === MP-DSDV-GNN: Topology event handling (C.3) ===

void
RoutingProtocol::OnLinkBreak(Ipv4Address nbr)
{
    NS_LOG_FUNCTION(m_mainAddress << " link break detected with neighbor " << nbr);

    // 1. Find all destinations reachable via this neighbor
    std::map<Ipv4Address, RoutingTableEntry> affectedDests;
    m_routingTable.GetListOfDestinationWithNextHop(nbr, affectedDests);

    // 2. Delete primary routes via this neighbor
    for (auto& entry : affectedDests)
    {
        NS_LOG_DEBUG("  Removing route to " << entry.first << " via " << nbr);
        m_routingTable.DeleteRoute(entry.first);
        m_routingTable.ClearMultipath(entry.first);
        // Mark as unreachable in adv table for triggered update
        entry.second.SetSeqNo(entry.second.GetSeqNo() + 1); // odd = unreachable
        entry.second.SetEntriesChanged(true);
        m_advRoutingTable.AddRoute(entry.second);
    }

    // 3. Delete all multipath backup entries that use this neighbor
    uint32_t mpRemoved = m_routingTable.DeleteMultipathByNextHop(nbr);
    NS_LOG_DEBUG("  Removed " << mpRemoved << " multipath entries via " << nbr);

    // 4. MP-DSDV-GNN: Delete from FeatureStore + nbrNodeFeatures
    m_featureStore.DeleteEntry(nbr);
    m_nbrNodeFeatures.erase(nbr);

    // 5. Send suppressed triggered update (inform network quickly)
    SendSuppressedTriggeredUpdate();
}

void
RoutingProtocol::OnNewNeighbor(Ipv4Address nbr)
{
    NS_LOG_FUNCTION(m_mainAddress << " new neighbor discovered: " << nbr);
    // MP-DSDV-GNN: Create FeatureStore entry (awaiting feature broadcast)
    m_featureStore.CreateEntry(nbr);
    // Send triggered update to inform network of improved connectivity.
    SendSuppressedTriggeredUpdate();
}

void
RoutingProtocol::SendSuppressedTriggeredUpdate()
{
    Time now = Simulator::Now();
    Time elapsed = now - m_lastTriggeredUpdateTime;

    if (elapsed >= Seconds(TRIGGER_MIN_INTERVAL_S))
    {
        // Enough time has passed, send immediately
        m_lastTriggeredUpdateTime = now;
        m_triggerUpdatePending = false;
        SendTriggeredUpdate();
    }
    else if (!m_triggerUpdatePending)
    {
        // Schedule a deferred update at the end of the suppression window
        m_triggerUpdatePending = true;
        Time remaining = Seconds(TRIGGER_MIN_INTERVAL_S) - elapsed;
        Simulator::Schedule(remaining, [this]() {
            m_lastTriggeredUpdateTime = Simulator::Now();
            m_triggerUpdatePending = false;
            SendTriggeredUpdate();
        });
        NS_LOG_DEBUG("Triggered update suppressed, deferred by " << remaining.As(Time::MS));
    }
    // else: a deferred update is already scheduled, do nothing (merge)
}

// === MP-DSDV-GNN: Feature Broadcast (C.4) ===

void
RoutingProtocol::SendFeatureBroadcast()
{
    NS_LOG_FUNCTION(m_mainAddress << " sending feature broadcast");

    // Update self feature from protocol state
    // num_neighbors: count from routing table hop==1 entries (design: "链路层确认的活跃直连邻居")
    std::map<Ipv4Address, RoutingTableEntry> allRt;
    m_routingTable.GetListOfAllRoutes(allRt);
    uint8_t oneHopCount = 0;
    for (const auto& r : allRt)
    {
        if (r.second.GetHop() == 1 && !r.first.IsBroadcast() && (r.first.Get() & 0xFF) != 0xFF)
        {
            oneHopCount++;
        }
    }
    m_selfFeature.numNeighbors = oneHopCount;
    // queue_ratio: DSDV packet buffer occupancy
    m_selfFeature.queueRatio = static_cast<float>(m_queue.GetSize()) /
                               static_cast<float>(m_maxQueueLen > 0 ? m_maxQueueLen : 1);
    m_selfFeature.bufferUsage = m_selfFeature.queueRatio;
    // local_load: forwarded bytes in past 1s / link capacity
    double capacity = DEFAULT_LINK_RATE_BPS * 1.0;
    m_selfFeature.localLoad = static_cast<float>(
        std::min(2.0, static_cast<double>(m_totalForwardedBytes) / capacity));
    m_totalForwardedBytes = 0;

    // Build the feature broadcast header
    FeatureBroadcastHeader header;
    header.SetSelfFeature(m_selfFeature);
    header.SetNeighborFeatures(m_nbrNodeFeatures);

    // MP-DSDV-GNN: Build link features for ALL routing-table-confirmed 1-hop neighbors
    // Design: "Neighbors(i) = 当前链路层确认的活跃直连邻居" (Section 4.3)
    ComputeLinkFeatures();
    m_selfLinkFeatures.clear();
    for (const auto& r : allRt)
    {
        Ipv4Address nbr = r.first;
        // Only hop==1 (direct neighbors), skip broadcast
        if (r.second.GetHop() != 1 || nbr.IsBroadcast() || (nbr.Get() & 0xFF) == 0xFF)
        {
            continue;
        }
        // Ensure link stats entry exists
        if (m_linkStats.find(nbr) == m_linkStats.end())
        {
            m_linkStats[nbr] = LinkStats{};
        }
        LinkFeature lf;
        lf.peerId = nbr;
        auto feat = GetLinkFeature(nbr);
        lf.etx = feat[0];        // link_quality
        lf.linkLoad = feat[1];   // link_utilization
        lf.bandwidth = feat[2];  // link_reliability
        lf.stability = feat[3];  // link_stability
        m_selfLinkFeatures.push_back(lf);
    }
    header.SetLinkFeatures(m_selfLinkFeatures);

    // Send on all DSDV sockets (reuse port 269, distinguished by magic byte 0xFF)
    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(header);

    for (auto& sock : m_socketAddresses)
    {
        Ptr<Socket> socket = sock.first;
        Ipv4InterfaceAddress iface = sock.second;
        Ipv4Address destination;
        if (iface.GetMask() == Ipv4Mask::GetOnes())
        {
            destination = Ipv4Address("255.255.255.255");
        }
        else
        {
            destination = iface.GetBroadcast();
        }
        socket->SendTo(packet, 0, InetSocketAddress(destination, DSDV_PORT));
    }

    NS_LOG_DEBUG("Feature broadcast sent, size=" << packet->GetSize()
                  << " nbr_feats=" << m_nbrNodeFeatures.size()
                  << " link_feats=" << m_selfLinkFeatures.size());

    // Reschedule with random jitter to avoid broadcast collision
    Simulator::Schedule(Seconds(FEATURE_BROADCAST_INTERVAL_S) +
                        MicroSeconds(m_uniformRandomVariable->GetInteger(0, 500000)),
                        &RoutingProtocol::SendFeatureBroadcast, this);
}

void
RoutingProtocol::RecvFeatureBroadcast(Ptr<Socket> socket)
{
    Address sourceAddress;
    Ptr<Packet> packet = socket->RecvFrom(sourceAddress);
    InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom(sourceAddress);
    Ipv4Address sender = inetSourceAddr.GetIpv4();

    FeatureBroadcastHeader header;
    packet->RemoveHeader(header);

    NS_LOG_FUNCTION(m_mainAddress << " received feature broadcast from " << sender);

    // Store sender's self feature
    NodeFeature senderSelfFeature = header.GetSelfFeature();

    // Store sender's neighbor features (contains 2-hop node features)
    std::map<Ipv4Address, NodeFeature> senderNbrFeatures = header.GetNeighborFeatures();

    // Store sender's link features
    std::vector<LinkFeature> senderLinkFeatures = header.GetLinkFeatures();

    // Update FeatureStore
    m_featureStore.UpdateEntry(sender, senderSelfFeature, senderLinkFeatures, senderNbrFeatures);

    // Update local cache of neighbor node features (for our next broadcast Part 2)
    m_nbrNodeFeatures[sender] = senderSelfFeature;

    NS_LOG_DEBUG("FeatureStore updated for " << sender
                  << ": nbr_feats=" << senderNbrFeatures.size()
                  << " link_feats=" << senderLinkFeatures.size());
}

// === MP-DSDV-GNN: GNN Observation Generation (C.5) ===

void
RoutingProtocol::GenerateGnnObservation()
{
    // Generate GNN observation from current protocol state
    m_gnnObserver.GenerateGnnObs(m_mainAddress,
                                 m_routingTable,
                                 m_featureStore,
                                 m_selfFeature,
                                 m_selfLinkFeatures,
                                 m_latestGnnObs);

    // Reschedule for next time slot (10ms)
    Simulator::Schedule(Seconds(TIME_SLOT_S), &RoutingProtocol::GenerateGnnObservation, this);
}

void
RoutingProtocol::ComputeLinkFeatures()
{
    // Called every 1s (from SendFeatureBroadcast)
    // Computes 4 link features per neighbor from accumulated stats

    for (auto& [nbr, stats] : m_linkStats)
    {
        // Skip broadcast addresses
        if (nbr.IsBroadcast() || (nbr.Get() & 0xFF) == 0xFF)
        {
            continue;
        }
        // [0] link_quality: from PHY layer SNR (MonitorSnifferRx trace)
        if (stats.snrCount > 0)
        {
            double avgSnr = stats.sumSnr / stats.snrCount;
            stats.linkQuality = static_cast<float>(std::min(1.0, std::max(0.0, avgSnr / SNR_MAX_DB)));
        }
        // else keep previous value (no frames received yet)

        // [1] link_utilization: bytes sent / (link_rate * 1s)
        double capacity = DEFAULT_LINK_RATE_BPS * 1.0; // bytes per 1 second
        stats.linkUtilization = static_cast<float>(
            std::min(2.0, static_cast<double>(stats.txBytes) / capacity));

        // [2] link_reliability: MAC frame delivery ratio (MacTx success / total attempts)
        // Design: "过去10s向该邻居发送MAC帧的成功率（MacTx/MacTxDrop Trace统计）"
        uint32_t totalMacTx = stats.macTxSuccess + stats.macTxFail;
        if (totalMacTx > 0)
        {
            stats.linkReliability = static_cast<float>(stats.macTxSuccess) /
                                    static_cast<float>(totalMacTx);
        }
        // else keep previous value (no frames sent yet)

        // [3] link_stability: alive seconds / total seconds
        stats.totalSeconds++;
        RoutingTableEntry rt;
        if (m_routingTable.LookupRoute(nbr, rt))
        {
            stats.aliveSeconds++;
        }
        if (stats.totalSeconds > 0)
        {
            stats.linkStability = static_cast<float>(stats.aliveSeconds) / stats.totalSeconds;
        }

        // Reset per-second counters
        stats.txBytes = 0;

        NS_LOG_DEBUG("LinkFeat to " << nbr
                      << ": quality=" << stats.linkQuality
                      << " util=" << stats.linkUtilization
                      << " rel=" << stats.linkReliability
                      << " stab=" << stats.linkStability);
    }

    // Reset 10s window counters every 10 calls
    static int windowCounter = 0;
    if (++windowCounter >= 10)
    {
        for (auto& [nbr, stats] : m_linkStats)
        {
            stats.sumSnr = 0.0;
            stats.snrCount = 0;
            stats.macTxSuccess = 0;
            stats.macTxFail = 0;
        }
        windowCounter = 0;
    }
}

std::array<float, 4>
RoutingProtocol::GetLinkFeature(Ipv4Address nbr)
{
    auto it = m_linkStats.find(nbr);
    if (it != m_linkStats.end())
    {
        return {it->second.linkQuality, it->second.linkUtilization,
                it->second.linkReliability, it->second.linkStability};
    }
    return {1.0f, 0.0f, 1.0f, 1.0f}; // default: good link
}

void
RoutingProtocol::ConnectLinkTraces(Ptr<NetDevice> dev)
{
    // Connect WifiPhy MonitorSnifferRx for per-frame SNR measurement
    Ptr<Node> node = GetObject<Node>();
    std::ostringstream oss;
    oss << "/NodeList/" << node->GetId() << "/DeviceList/"
        << dev->GetIfIndex() << "/$ns3::WifiNetDevice/Phy/MonitorSnifferRx";
    Config::ConnectWithoutContextFailSafe(oss.str(),
        MakeCallback(&RoutingProtocol::PhyRxMonitor, this));

    // Connect MAC TX success/failure traces for link_reliability
    std::ostringstream ossTxOk;
    ossTxOk << "/NodeList/" << node->GetId() << "/DeviceList/"
            << dev->GetIfIndex() << "/$ns3::WifiNetDevice/Mac/MacTx";
    Config::ConnectWithoutContextFailSafe(ossTxOk.str(),
        MakeCallback(&RoutingProtocol::MacTxOk, this));

    std::ostringstream ossTxDrop;
    ossTxDrop << "/NodeList/" << node->GetId() << "/DeviceList/"
              << dev->GetIfIndex() << "/$ns3::WifiNetDevice/Mac/MacTxDrop";
    Config::ConnectWithoutContextFailSafe(ossTxDrop.str(),
        MakeCallback(&RoutingProtocol::MacTxDrop, this));

    NS_LOG_DEBUG("Connected PHY/MAC traces for link feature measurement");
}

Ipv4Address
RoutingProtocol::MacToIpHeuristic(Mac48Address mac) const
{
    // Look up from dynamically learned MAC→IP map.
    // Entries are learned in RecvDsdv: when a feature broadcast/route update arrives,
    // we know the sender's IP (from socket) and can observe their MAC (from last PHY frame).
    auto it = m_macToIpMap.find(mac);
    if (it != m_macToIpMap.end())
    {
        return it->second;
    }
    return Ipv4Address(); // unknown MAC, cannot map
}

void
RoutingProtocol::PhyRxMonitor(Ptr<const Packet> packet, uint16_t channelFreqMhz,
                               WifiTxVector txVector, MpduInfo aMpdu,
                               SignalNoiseDbm signalNoise, uint16_t staId)
{
    // Compute SNR = signal - noise (both in dBm)
    double snrDb = signalNoise.signal - signalNoise.noise;

    // Extract transmitter MAC address from 802.11 header
    WifiMacHeader macHdr;
    packet->PeekHeader(macHdr);
    Mac48Address srcMac = macHdr.GetAddr2(); // Transmitter Address (TA)

    // Record last received MAC for MAC→IP learning (in RecvDsdv)
    // WiFi is half-duplex: the last PHY frame before socket callback is the one that triggered it
    m_lastRxMac = srcMac;

    // Map sender MAC to IP using learned mapping
    Ipv4Address srcIp = MacToIpHeuristic(srcMac);
    if (srcIp == Ipv4Address())
    {
        return; // unknown MAC, cannot attribute SNR
    }

    // Only accumulate SNR for known neighbors (per-neighbor attribution)
    auto it = m_linkStats.find(srcIp);
    if (it != m_linkStats.end())
    {
        it->second.sumSnr += snrDb;
        it->second.snrCount++;
    }
}

void
RoutingProtocol::MacTxOk(Ptr<const Packet> p)
{
    // MAC frame successfully transmitted (ACK received)
    // Extract destination MAC to identify neighbor
    WifiMacHeader macHdr;
    p->PeekHeader(macHdr);
    Mac48Address dstMac = macHdr.GetAddr1(); // Receiver Address (RA)
    if (dstMac.IsBroadcast() || dstMac.IsGroup())
    {
        return; // skip broadcast/multicast
    }
    Ipv4Address dstIp = MacToIpHeuristic(dstMac);
    if (dstIp != Ipv4Address())
    {
        auto it = m_linkStats.find(dstIp);
        if (it != m_linkStats.end())
        {
            it->second.macTxSuccess++;
        }
    }
}

void
RoutingProtocol::MacTxDrop(Ptr<const Packet> p)
{
    // MAC frame dropped after max retries
    WifiMacHeader macHdr;
    p->PeekHeader(macHdr);
    Mac48Address dstMac = macHdr.GetAddr1();
    if (dstMac.IsBroadcast() || dstMac.IsGroup())
    {
        return;
    }
    Ipv4Address dstIp = MacToIpHeuristic(dstMac);
    if (dstIp != Ipv4Address())
    {
        auto it = m_linkStats.find(dstIp);
        if (it != m_linkStats.end())
        {
            it->second.macTxFail++;
        }
    }
}

void
RoutingProtocol::SendPeriodicUpdate()
{
    std::map<Ipv4Address, RoutingTableEntry> removedAddresses;
    std::map<Ipv4Address, RoutingTableEntry> allRoutes;
    m_routingTable.Purge(removedAddresses);
    MergeTriggerPeriodicUpdates();
    m_routingTable.GetListOfAllRoutes(allRoutes);
    if (allRoutes.empty())
    {
        return;
    }
    NS_LOG_FUNCTION(m_mainAddress << " is sending out its periodic update");
    for (auto j = m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j)
    {
        Ptr<Socket> socket = j->first;
        Ipv4InterfaceAddress iface = j->second;
        Ptr<Packet> packet = Create<Packet>();
        for (auto i = allRoutes.begin(); i != allRoutes.end(); ++i)
        {
            DsdvHeader dsdvHeader;
            if (i->second.GetHop() == 0)
            {
                RoutingTableEntry ownEntry;
                dsdvHeader.SetDst(m_ipv4->GetAddress(1, 0).GetLocal());
                dsdvHeader.SetDstSeqno(i->second.GetSeqNo() + 2);
                dsdvHeader.SetHopCount(i->second.GetHop() + 1);
                m_routingTable.LookupRoute(m_ipv4->GetAddress(1, 0).GetBroadcast(), ownEntry);
                ownEntry.SetSeqNo(dsdvHeader.GetDstSeqno());
                m_routingTable.Update(ownEntry);
                packet->AddHeader(dsdvHeader);
            }
            else
            {
                dsdvHeader.SetDst(i->second.GetDestination());
                dsdvHeader.SetDstSeqno((i->second.GetSeqNo()));
                dsdvHeader.SetHopCount(i->second.GetHop() + 1);
                packet->AddHeader(dsdvHeader);
            }
            NS_LOG_DEBUG("Forwarding the update for " << i->first);
            NS_LOG_DEBUG("Forwarding details are, Destination: "
                         << dsdvHeader.GetDst() << ", SeqNo:" << dsdvHeader.GetDstSeqno()
                         << ", HopCount:" << dsdvHeader.GetHopCount()
                         << ", LifeTime: " << i->second.GetLifeTime().As(Time::S));
        }
        for (auto rmItr = removedAddresses.begin(); rmItr != removedAddresses.end(); ++rmItr)
        {
            DsdvHeader removedHeader;
            removedHeader.SetDst(rmItr->second.GetDestination());
            removedHeader.SetDstSeqno(rmItr->second.GetSeqNo() + 1);
            removedHeader.SetHopCount(rmItr->second.GetHop() + 1);
            packet->AddHeader(removedHeader);
            NS_LOG_DEBUG("Update for removed record is: Destination: "
                         << removedHeader.GetDst() << " SeqNo:" << removedHeader.GetDstSeqno()
                         << " HopCount:" << removedHeader.GetHopCount());
        }
        socket->Send(packet);
        // Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
        Ipv4Address destination;
        if (iface.GetMask() == Ipv4Mask::GetOnes())
        {
            destination = Ipv4Address("255.255.255.255");
        }
        else
        {
            destination = iface.GetBroadcast();
        }
        socket->SendTo(packet, 0, InetSocketAddress(destination, DSDV_PORT));
        NS_LOG_FUNCTION("PeriodicUpdate Packet UID is : " << packet->GetUid());
    }
    m_periodicUpdateTimer.Schedule(m_periodicUpdateInterval +
                                   MicroSeconds(25 * m_uniformRandomVariable->GetInteger(0, 1000)));
}

void
RoutingProtocol::SetIpv4(Ptr<Ipv4> ipv4)
{
    NS_ASSERT(ipv4);
    NS_ASSERT(!m_ipv4);
    m_ipv4 = ipv4;
    // Create lo route. It is asserted that the only one interface up for now is loopback
    NS_ASSERT(m_ipv4->GetNInterfaces() == 1 &&
              m_ipv4->GetAddress(0, 0).GetLocal() == Ipv4Address("127.0.0.1"));
    m_lo = m_ipv4->GetNetDevice(0);
    NS_ASSERT(m_lo);
    // Remember lo route
    RoutingTableEntry rt(
        /*dev=*/m_lo,
        /*dst=*/Ipv4Address::GetLoopback(),
        /*seqNo=*/0,
        /*iface=*/Ipv4InterfaceAddress(Ipv4Address::GetLoopback(), Ipv4Mask("255.0.0.0")),
        /*hops=*/0,
        /*nextHop=*/Ipv4Address::GetLoopback(),
        /*lifetime=*/Simulator::GetMaximumSimulationTime());
    rt.SetFlag(INVALID);
    rt.SetEntriesChanged(false);
    m_routingTable.AddRoute(rt);
    Simulator::ScheduleNow(&RoutingProtocol::Start, this);
}

void
RoutingProtocol::NotifyInterfaceUp(uint32_t i)
{
    NS_LOG_FUNCTION(this << m_ipv4->GetAddress(i, 0).GetLocal() << " interface is up");
    Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol>();
    Ipv4InterfaceAddress iface = l3->GetAddress(i, 0);
    if (iface.GetLocal() == Ipv4Address("127.0.0.1"))
    {
        return;
    }
    // Create a socket to listen only on this interface
    Ptr<Socket> socket = Socket::CreateSocket(GetObject<Node>(), UdpSocketFactory::GetTypeId());
    NS_ASSERT(socket);
    socket->SetRecvCallback(MakeCallback(&RoutingProtocol::RecvDsdv, this));
    socket->BindToNetDevice(l3->GetNetDevice(i));
    socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), DSDV_PORT));
    socket->SetAllowBroadcast(true);
    socket->SetAttribute("IpTtl", UintegerValue(1));
    m_socketAddresses.insert(std::make_pair(socket, iface));

    // MP-DSDV-GNN: Create feature broadcast socket (reuse DSDV_PORT with tag-based demux)
    // Note: We reuse the same DSDV_PORT socket for feature broadcasts.
    // Feature packets are distinguished by a FeatureBroadcastHeader (different from DsdvHeader).
    // The RecvDsdv handler will detect and redirect feature packets.
    // No separate socket needed.

    // MP-DSDV-GNN: Connect PHY/MAC traces for link feature measurement
    ConnectLinkTraces(l3->GetNetDevice(i));

    // Add local broadcast record to the routing table
    Ptr<NetDevice> dev = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(iface.GetLocal()));
    RoutingTableEntry rt(/*dev=*/dev,
                         /*dst=*/iface.GetBroadcast(),
                         /*seqNo=*/0,
                         /*iface=*/iface,
                         /*hops=*/0,
                         /*nextHop=*/iface.GetBroadcast(),
                         /*lifetime=*/Simulator::GetMaximumSimulationTime());
    m_routingTable.AddRoute(rt);
    if (m_mainAddress == Ipv4Address())
    {
        m_mainAddress = iface.GetLocal();
    }
    NS_ASSERT(m_mainAddress != Ipv4Address());
}

void
RoutingProtocol::NotifyInterfaceDown(uint32_t i)
{
    Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol>();
    Ptr<NetDevice> dev = l3->GetNetDevice(i);
    Ptr<Socket> socket = FindSocketWithInterfaceAddress(m_ipv4->GetAddress(i, 0));
    NS_ASSERT(socket);
    socket->Close();
    m_socketAddresses.erase(socket);
    if (m_socketAddresses.empty())
    {
        NS_LOG_LOGIC("No dsdv interfaces");
        m_routingTable.Clear();
        return;
    }
    m_routingTable.DeleteAllRoutesFromInterface(m_ipv4->GetAddress(i, 0));
    m_advRoutingTable.DeleteAllRoutesFromInterface(m_ipv4->GetAddress(i, 0));
}

void
RoutingProtocol::NotifyAddAddress(uint32_t i, Ipv4InterfaceAddress address)
{
    NS_LOG_FUNCTION(this << " interface " << i << " address " << address);
    Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol>();
    if (!l3->IsUp(i))
    {
        return;
    }
    Ipv4InterfaceAddress iface = l3->GetAddress(i, 0);
    Ptr<Socket> socket = FindSocketWithInterfaceAddress(iface);
    if (!socket)
    {
        if (iface.GetLocal() == Ipv4Address("127.0.0.1"))
        {
            return;
        }
        Ptr<Socket> socket = Socket::CreateSocket(GetObject<Node>(), UdpSocketFactory::GetTypeId());
        NS_ASSERT(socket);
        socket->SetRecvCallback(MakeCallback(&RoutingProtocol::RecvDsdv, this));
        // Bind to any IP address so that broadcasts can be received
        socket->BindToNetDevice(l3->GetNetDevice(i));
        socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), DSDV_PORT));
        socket->SetAllowBroadcast(true);
        m_socketAddresses.insert(std::make_pair(socket, iface));
        Ptr<NetDevice> dev = m_ipv4->GetNetDevice(m_ipv4->GetInterfaceForAddress(iface.GetLocal()));
        RoutingTableEntry rt(/*dev=*/dev,
                             /*dst=*/iface.GetBroadcast(),
                             /*seqNo=*/0,
                             /*iface=*/iface,
                             /*hops=*/0,
                             /*nextHop=*/iface.GetBroadcast(),
                             /*lifetime=*/Simulator::GetMaximumSimulationTime());
        m_routingTable.AddRoute(rt);
    }
}

void
RoutingProtocol::NotifyRemoveAddress(uint32_t i, Ipv4InterfaceAddress address)
{
    Ptr<Socket> socket = FindSocketWithInterfaceAddress(address);
    if (socket)
    {
        m_socketAddresses.erase(socket);
        Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol>();
        if (l3->GetNAddresses(i))
        {
            Ipv4InterfaceAddress iface = l3->GetAddress(i, 0);
            // Create a socket to listen only on this interface
            Ptr<Socket> socket =
                Socket::CreateSocket(GetObject<Node>(), UdpSocketFactory::GetTypeId());
            NS_ASSERT(socket);
            socket->SetRecvCallback(MakeCallback(&RoutingProtocol::RecvDsdv, this));
            // Bind to any IP address so that broadcasts can be received
            socket->Bind(InetSocketAddress(Ipv4Address::GetAny(), DSDV_PORT));
            socket->SetAllowBroadcast(true);
            m_socketAddresses.insert(std::make_pair(socket, iface));
        }
    }
}

Ptr<Socket>
RoutingProtocol::FindSocketWithInterfaceAddress(Ipv4InterfaceAddress addr) const
{
    for (auto j = m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j)
    {
        Ptr<Socket> socket = j->first;
        Ipv4InterfaceAddress iface = j->second;
        if (iface == addr)
        {
            return socket;
        }
    }
    Ptr<Socket> socket;
    return socket;
}

void
RoutingProtocol::Send(Ptr<Ipv4Route> route, Ptr<const Packet> packet, const Ipv4Header& header)
{
    Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol>();
    NS_ASSERT(l3);
    Ptr<Packet> p = packet->Copy();
    l3->Send(p, route->GetSource(), header.GetDestination(), header.GetProtocol(), route);
}

void
RoutingProtocol::Drop(Ptr<const Packet> packet, const Ipv4Header& header, Socket::SocketErrno err)
{
    NS_LOG_DEBUG(m_mainAddress << " drop packet " << packet->GetUid() << " to "
                               << header.GetDestination() << " from queue. Error " << err);
}

void
RoutingProtocol::LookForQueuedPackets()
{
    NS_LOG_FUNCTION(this);
    Ptr<Ipv4Route> route;
    std::map<Ipv4Address, RoutingTableEntry> allRoutes;
    m_routingTable.GetListOfAllRoutes(allRoutes);
    for (auto i = allRoutes.begin(); i != allRoutes.end(); ++i)
    {
        RoutingTableEntry rt;
        rt = i->second;
        if (m_queue.Find(rt.GetDestination()))
        {
            if (rt.GetHop() == 1)
            {
                route = rt.GetRoute();
                NS_ASSERT(route);
                NS_LOG_LOGIC("A route exists from " << route->GetSource()
                                                    << " to neighboring destination "
                                                    << route->GetDestination());
            }
            else
            {
                RoutingTableEntry newrt;
                m_routingTable.LookupRoute(rt.GetNextHop(), newrt);
                route = newrt.GetRoute();
                NS_ASSERT(route);
                NS_LOG_LOGIC("A route exists from " << route->GetSource() << " to destination "
                                                    << route->GetDestination() << " via "
                                                    << rt.GetNextHop());
            }
            SendPacketFromQueue(rt.GetDestination(), route);
        }
    }
}

void
RoutingProtocol::SendPacketFromQueue(Ipv4Address dst, Ptr<Ipv4Route> route)
{
    NS_LOG_DEBUG(m_mainAddress << " is sending a queued packet to destination " << dst);
    QueueEntry queueEntry;
    if (m_queue.Dequeue(dst, queueEntry))
    {
        DeferredRouteOutputTag tag;
        Ptr<Packet> p = ConstCast<Packet>(queueEntry.GetPacket());
        if (p->RemovePacketTag(tag))
        {
            if (tag.oif != -1 && tag.oif != m_ipv4->GetInterfaceForDevice(route->GetOutputDevice()))
            {
                NS_LOG_DEBUG("Output device doesn't match. Dropped.");
                return;
            }
        }
        UnicastForwardCallback ucb = queueEntry.GetUnicastForwardCallback();
        Ipv4Header header = queueEntry.GetIpv4Header();
        header.SetSource(route->GetSource());
        header.SetTtl(header.GetTtl() +
                      1); // compensate extra TTL decrement by fake loopback routing
        ucb(route, p, header);
        if (m_queue.GetSize() != 0 && m_queue.Find(dst))
        {
            Simulator::Schedule(MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100)),
                                &RoutingProtocol::SendPacketFromQueue,
                                this,
                                dst,
                                route);
        }
    }
}

Time
RoutingProtocol::GetSettlingTime(Ipv4Address address)
{
    NS_LOG_FUNCTION("Calculating the settling time for " << address);
    RoutingTableEntry mainrt;
    Time weightedTime;
    m_routingTable.LookupRoute(address, mainrt);
    if (EnableWST)
    {
        if (mainrt.GetSettlingTime() == Seconds(0))
        {
            return Seconds(0);
        }
        else
        {
            NS_LOG_DEBUG("Route SettlingTime: " << mainrt.GetSettlingTime().As(Time::S)
                                                << " and LifeTime:"
                                                << mainrt.GetLifeTime().As(Time::S));
            weightedTime = m_weightedFactor * mainrt.GetSettlingTime() +
                           (1.0 - m_weightedFactor) * mainrt.GetLifeTime();
            NS_LOG_DEBUG("Calculated weightedTime:" << weightedTime.As(Time::S));
            return weightedTime;
        }
    }
    return mainrt.GetSettlingTime();
}

void
RoutingProtocol::MergeTriggerPeriodicUpdates()
{
    NS_LOG_FUNCTION(
        "Merging advertised table changes with main table before sending out periodic update");
    std::map<Ipv4Address, RoutingTableEntry> allRoutes;
    m_advRoutingTable.GetListOfAllRoutes(allRoutes);
    if (!allRoutes.empty())
    {
        for (auto i = allRoutes.begin(); i != allRoutes.end(); ++i)
        {
            RoutingTableEntry advEntry = i->second;
            if (advEntry.GetEntriesChanged() &&
                !m_advRoutingTable.AnyRunningEvent(advEntry.GetDestination()))
            {
                if (!(advEntry.GetSeqNo() % 2))
                {
                    advEntry.SetFlag(VALID);
                    advEntry.SetEntriesChanged(false);
                    m_routingTable.Update(advEntry);
                    NS_LOG_DEBUG("Merged update for " << advEntry.GetDestination()
                                                      << " with main routing Table");
                }
                m_advRoutingTable.DeleteRoute(advEntry.GetDestination());
            }
            else
            {
                NS_LOG_DEBUG("Event currently running. Cannot Merge Routing Tables");
            }
        }
    }
}
} // namespace dsdv
} // namespace ns3
