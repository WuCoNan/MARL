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

#ifndef DSDV_ROUTING_PROTOCOL_H
#define DSDV_ROUTING_PROTOCOL_H

#include "dsdv-feature-packet.h"
#include "dsdv-feature-store.h"
#include "dsdv-gnn-observer.h"
#include "dsdv-packet-queue.h"
#include "dsdv-packet.h"
#include "dsdv-rtable.h"

#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/node.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/random-variable-stream.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-tx-vector.h"
#include "ns3/phy-entity.h"

#include <array>

namespace ns3
{
namespace dsdv
{

/**
 * \ingroup dsdv
 * \brief DSDV routing protocol.
 */
class RoutingProtocol : public Ipv4RoutingProtocol
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();
    static const uint32_t DSDV_PORT;

    /// c-tor
    RoutingProtocol();

    ~RoutingProtocol() override;
    void DoDispose() override;

    // From Ipv4RoutingProtocol
    Ptr<Ipv4Route> RouteOutput(Ptr<Packet> p,
                               const Ipv4Header& header,
                               Ptr<NetDevice> oif,
                               Socket::SocketErrno& sockerr) override;
    /**
     * Route input packet
     * \param p The packet
     * \param header The IPv4 header
     * \param idev The device
     * \param ucb The unicast forward callback
     * \param mcb The multicast forward callback
     * \param lcb The local deliver callback
     * \param ecb The error callback
     * \returns true if successful
     */
    bool RouteInput(Ptr<const Packet> p,
                    const Ipv4Header& header,
                    Ptr<const NetDevice> idev,
                    const UnicastForwardCallback& ucb,
                    const MulticastForwardCallback& mcb,
                    const LocalDeliverCallback& lcb,
                    const ErrorCallback& ecb) override;
    void PrintRoutingTable(Ptr<OutputStreamWrapper> stream,
                           Time::Unit unit = Time::S) const override;
    void NotifyInterfaceUp(uint32_t interface) override;
    void NotifyInterfaceDown(uint32_t interface) override;
    void NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address) override;
    void NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address) override;
    void SetIpv4(Ptr<Ipv4> ipv4) override;

    // Methods to handle protocol parameters
    /**
     * Set enable buffer flag
     * \param f The enable buffer flag
     */
    void SetEnableBufferFlag(bool f);
    /**
     * Get enable buffer flag
     * \returns the enable buffer flag
     */
    bool GetEnableBufferFlag() const;
    /**
     * Set weighted settling time (WST) flag
     * \param f the weighted settling time (WST) flag
     */
    void SetWSTFlag(bool f);
    /**
     * Get weighted settling time (WST) flag
     * \returns the weighted settling time (WST) flag
     */
    bool GetWSTFlag() const;
    /**
     * Set enable route aggregation (RA) flag
     * \param f the enable route aggregation (RA) flag
     */
    void SetEnableRAFlag(bool f);
    /**
     * Get enable route aggregation (RA) flag
     * \returns the enable route aggregation (RA) flag
     */
    bool GetEnableRAFlag() const;

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * \param stream first stream index to use
     * \return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

  private:
    // Protocol parameters.
    /// Holdtimes is the multiplicative factor of PeriodicUpdateInterval for which the node waits
    /// since the last update before flushing a route from the routing table. If
    /// PeriodicUpdateInterval is 8s and Holdtimes is 3, the node waits for 24s since the last
    /// update to flush this route from its routing table.
    uint32_t Holdtimes;
    /// PeriodicUpdateInterval specifies the periodic time interval between which the a node
    /// broadcasts its entire routing table.
    Time m_periodicUpdateInterval;
    /// SettlingTime specifies the time for which a node waits before propagating an update.
    /// It waits for this time interval in hope of receiving an update with a better metric.
    Time m_settlingTime;
    /// Nodes IP address
    Ipv4Address m_mainAddress;
    /// IP protocol
    Ptr<Ipv4> m_ipv4;
    /// Raw socket per each IP interface, map socket -> iface address (IP + mask)
    std::map<Ptr<Socket>, Ipv4InterfaceAddress> m_socketAddresses;
    /// Loopback device used to defer route requests until a route is found
    Ptr<NetDevice> m_lo;
    /// Main Routing table for the node
    RoutingTable m_routingTable;
    /// Advertised Routing table for the node
    RoutingTable m_advRoutingTable;
    /// The maximum number of packets that we allow a routing protocol to buffer.
    uint32_t m_maxQueueLen;
    /// The maximum number of packets that we allow per destination to buffer.
    uint32_t m_maxQueuedPacketsPerDst;
    /// The maximum period of time that a routing protocol is allowed to buffer a packet for.
    Time m_maxQueueTime;
    /// A "drop front on full" queue used by the routing layer to buffer packets to which it does
    /// not have a route.
    PacketQueue m_queue;
    /// Flag that is used to enable or disable buffering
    bool EnableBuffering;
    /// Flag that is used to enable or disable Weighted Settling Time
    bool EnableWST;
    /// This is the weighted factor to determine the weighted settling time
    double m_weightedFactor;
    /// This is a flag to enable route aggregation. Route aggregation will aggregate all routes for
    /// 'RouteAggregationTime' from the time an update is received by a node and sends them as a
    /// single update .
    bool EnableRouteAggregation;
    /// Parameter that holds the route aggregation time interval
    Time m_routeAggregationTime;
    /// Unicast callback for own packets
    UnicastForwardCallback m_scb;
    /// Error callback for own packets
    ErrorCallback m_ecb;

  private:
    /// Start protocol operation
    void Start();
    /**
     * Queue packet until we find a route
     * \param p the packet to route
     * \param header the Ipv4Header
     * \param ucb the UnicastForwardCallback function
     * \param ecb the ErrorCallback function
     */
    void DeferredRouteOutput(Ptr<const Packet> p,
                             const Ipv4Header& header,
                             UnicastForwardCallback ucb,
                             ErrorCallback ecb);
    /// Look for any queued packets to send them out
    void LookForQueuedPackets();
    /**
     * Send packet from queue
     * \param dst - destination address to which we are sending the packet to
     * \param route - route identified for this packet
     */
    void SendPacketFromQueue(Ipv4Address dst, Ptr<Ipv4Route> route);
    /**
     * Find socket with local interface address iface
     * \param iface the interface
     * \returns the socket
     */
    Ptr<Socket> FindSocketWithInterfaceAddress(Ipv4InterfaceAddress iface) const;

    // Receive dsdv control packets
    /**
     * Receive and process dsdv control packet
     * \param socket the socket for receiving dsdv control packets
     */
    void RecvDsdv(Ptr<Socket> socket);
    /**
     * Send a packet
     * \param route the route
     * \param packet the packet
     * \param header the IPv4 header
     */
    void Send(Ptr<Ipv4Route> route, Ptr<const Packet> packet, const Ipv4Header& header);

    /**
     * Create loopback route for given header
     *
     * \param header the IP header
     * \param oif the device
     * \returns the route
     */
    Ptr<Ipv4Route> LoopbackRoute(const Ipv4Header& header, Ptr<NetDevice> oif) const;
    /**
     * Get settlingTime for a destination
     * \param dst - destination address
     * \return settlingTime for the destination if found
     */
    Time GetSettlingTime(Ipv4Address dst);
    /// Sends trigger update from a node
    void SendTriggeredUpdate();
    /// Broadcasts the entire routing table for every PeriodicUpdateInterval
    void SendPeriodicUpdate();
    /// Merge periodic updates
    void MergeTriggerPeriodicUpdates();

    // === MP-DSDV-GNN: Topology event handling (C.3) ===
    /**
     * Handle link break event: remove all routes via this neighbor,
     * clean multipath entries, and send triggered update.
     * \param nbr the neighbor that became unreachable
     */
    void OnLinkBreak(Ipv4Address nbr);
    /**
     * Handle new neighbor detection: log event and send triggered update.
     * \param nbr the newly discovered neighbor
     */
    void OnNewNeighbor(Ipv4Address nbr);
    /**
     * Send feature broadcast packet (every HELLO_INTERVAL = 1s).
     * Contains: self node feature + all neighbors' node features + all link features.
     */
    void SendFeatureBroadcast();
    /**
     * Receive and process feature broadcast from a neighbor.
     * \param socket the receiving socket
     */
    void RecvFeatureBroadcast(Ptr<Socket> socket);
    /**
     * Send triggered update with suppression (at most once per TRIGGER_MIN_INTERVAL).
     * Prevents broadcast storm when multiple links break simultaneously.
     */
    void SendSuppressedTriggeredUpdate();
    /// Minimum interval between triggered updates (suppression)
    static constexpr double TRIGGER_MIN_INTERVAL_S = 1.0;
    /**
     * Notify that packet is dropped for some reason
     * \param packet the dropped packet
     * \param header the IPv4 header
     * \param err the error number
     */
    void Drop(Ptr<const Packet> packet, const Ipv4Header& header, Socket::SocketErrno err);
    /// Timer to trigger periodic updates from a node
    Timer m_periodicUpdateTimer;
    /// Timer used by the trigger updates in case of Weighted Settling Time is used
    Timer m_triggeredExpireTimer;
    /// Timestamp of last triggered update sent (for suppression)
    Time m_lastTriggeredUpdateTime;
    /// Whether a suppressed trigger is already scheduled
    bool m_triggerUpdatePending;

    // === MP-DSDV-GNN: Feature broadcast members (C.4) ===
    /// Feature store for neighbor features (GNN observation source)
    FeatureStore m_featureStore;
    /// Self node feature (updated every time slot)
    NodeFeature m_selfFeature;
    /// Self link features (measured locally)
    std::vector<LinkFeature> m_selfLinkFeatures;
    /// Neighbor node features cache (for broadcasting in Part 2)
    std::map<Ipv4Address, NodeFeature> m_nbrNodeFeatures;
    /// Timer for periodic feature broadcast (1s interval)
    Timer m_featureBroadcastTimer;
    /// Sockets for receiving feature broadcasts (one per interface)
    std::vector<Ptr<Socket>> m_featureSockets;
    /// Feature broadcast interval
    static constexpr double FEATURE_BROADCAST_INTERVAL_S = 1.0;
    /// Feature broadcast port (DSDV_PORT + 1 to distinguish from route updates)
    static const uint32_t DSDV_FEATURE_PORT = 270;

    // === MP-DSDV-GNN: GNN Observer (C.5) ===
    /// GNN observation generator
    GnnObserver m_gnnObserver;
    /// Latest GNN observation (for ns3-ai shared memory export)
    GnnObservation m_latestGnnObs;
    /// Generate GNN observation (called every TIME_SLOT=10ms)
    void GenerateGnnObservation();
    /// Time slot interval for GNN observation
    static constexpr double TIME_SLOT_S = 0.01; // 10ms

    // === MP-DSDV-GNN: Link feature measurement (cross-layer) ===
    /// Per-neighbor PHY/MAC statistics for link features
    struct LinkStats
    {
        // PHY: SNR accumulation (for link_quality)
        double sumSnr = 0.0;
        uint32_t snrCount = 0;
        // MAC: TX success/fail (for link_reliability)
        uint32_t macTxSuccess = 0;
        uint32_t macTxFail = 0;
        // Network: TX bytes (for link_utilization)
        uint64_t txBytes = 0;
        // Stability: alive seconds (for link_stability)
        uint32_t aliveSeconds = 0;
        uint32_t totalSeconds = 0;
        // Computed features (updated every 1s)
        float linkQuality = 1.0f;
        float linkUtilization = 0.0f;
        float linkReliability = 1.0f;
        float linkStability = 1.0f;
    };
    std::map<Ipv4Address, LinkStats> m_linkStats;
    /// MAC-to-IP mapping for per-neighbor SNR attribution (learned from protocol exchanges)
    std::map<Mac48Address, Ipv4Address> m_macToIpMap;
    /// Last received MAC frame's transmitter address (for MAC→IP learning)
    Mac48Address m_lastRxMac;
    /// Total forwarded bytes in current 1s window (for local_load)
    uint64_t m_totalForwardedBytes = 0;
    /// PHY trace callback: monitor received frames for SNR (ns-3.40 signature)
    void PhyRxMonitor(Ptr<const Packet> packet, uint16_t channelFreqMhz,
                      WifiTxVector txVector, MpduInfo aMpdu,
                      SignalNoiseDbm signalNoise, uint16_t staId);
    /// MAC TX trace callback: frame successfully sent
    void MacTxOk(Ptr<const Packet> p);
    /// MAC TX drop trace callback: frame failed after max retries
    void MacTxDrop(Ptr<const Packet> p);
    /// Compute link features from accumulated stats (called every 1s)
    void ComputeLinkFeatures();
    /// Connect PHY/MAC traces for a given NetDevice
    void ConnectLinkTraces(Ptr<NetDevice> dev);
    /// Map MAC address to IP using simulation address heuristic
    Ipv4Address MacToIpHeuristic(Mac48Address mac) const;
    /// Get link feature for a neighbor (returns 4-dim array)
    std::array<float, 4> GetLinkFeature(Ipv4Address nbr);
    /// SNR value considered as "perfect" link (for normalization)
    static constexpr double SNR_MAX_DB = 25.0;
    /// Default link rate in bytes/sec (802.11b 11Mbps)
    static constexpr double DEFAULT_LINK_RATE_BPS = 11000000.0 / 8.0;

    /// Provides uniform random variables.
    Ptr<UniformRandomVariable> m_uniformRandomVariable;
};

} // namespace dsdv
} // namespace ns3

#endif /* DSDV_ROUTING_PROTOCOL_H */
