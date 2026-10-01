/*
 * MP-DSDV-GNN Multipath Verification Test
 * Topology guarantees multipath: Node 0 → Node 3 has 3 disjoint 2-hop paths
 *
 *   1(50,0) ---- 3(50,50)
 *   |    \       |
 *   |     4(25,25)
 *   |    /       |
 *   0(0,0) ---- 2(0,50)
 *
 * Connectivity (802.11b, ~100m range):
 *   0↔1(50m), 0↔2(50m), 0↔4(35m)
 *   1↔3(50m), 1↔4(35m)
 *   2↔3(50m), 2↔4(35m)
 *   3↔4(35m)
 *
 * Multipath from Node 0 to Node 3 (all 2-hop, disjoint next-hop):
 *   0→1→3, 0→2→3, 0→4→3
 */

// White-box access to DSDV internals
#define private public
#define protected public

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

#include "ns3/dsdv-helper.h"
#include "ns3/dsdv-routing-protocol.h"

#include <iostream>

#undef private
#undef protected

using namespace ns3;
using namespace ns3::dsdv;

NS_LOG_COMPONENT_DEFINE("MpDsdvGnnTest");

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, msg)                                                                             \
    do                                                                                               \
    {                                                                                                \
        if (cond) { std::cout << "[PASS] " << msg << std::endl; g_pass++; }                         \
        else      { std::cout << "[FAIL] " << msg << std::endl; g_fail++; }                         \
    } while (0)

void
PrintRoutingTable(Ptr<Node> node, uint32_t idx)
{
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    Ptr<RoutingProtocol> dsdv = DynamicCast<RoutingProtocol>(ipv4->GetRoutingProtocol());
    if (!dsdv)
    {
        std::cout << "  Node " << idx << ": NOT DSDV!" << std::endl;
        return;
    }

    std::cout << "  Node " << idx << " (" << dsdv->m_mainAddress << "):" << std::endl;

    // Print primary routes
    std::map<Ipv4Address, RoutingTableEntry> allRoutes;
    dsdv->m_routingTable.GetListOfAllRoutes(allRoutes);
    std::cout << "    Primary routes: " << allRoutes.size() << " entries" << std::endl;
    for (const auto& r : allRoutes)
    {
        if (r.second.GetHop() > 0 && !r.first.IsBroadcast())
        {
            std::cout << "      dst=" << r.first
                      << " next_hop=" << r.second.GetNextHop()
                      << " hops=" << r.second.GetHop()
                      << " seq=" << r.second.GetSeqNo()
                      << std::endl;
        }
    }

    // Print multipath backup routes
    uint32_t mpTotal = dsdv->m_routingTable.MultipathTableSize();
    std::cout << "    Multipath backups: " << mpTotal << " entries" << std::endl;
    for (const auto& r : allRoutes)
    {
        if (r.second.GetHop() == 0 || r.first.IsBroadcast())
        {
            continue;
        }
        std::vector<RoutingTableEntry> backups;
        if (dsdv->m_routingTable.GetBackupPaths(r.first, backups))
        {
            for (const auto& bp : backups)
            {
                std::cout << "      [MP] dst=" << r.first
                          << " next_hop=" << bp.GetNextHop()
                          << " hops=" << bp.GetHop()
                          << " seq=" << bp.GetSeqNo()
                          << std::endl;
            }
        }
    }

    // FeatureStore status
    std::cout << "    FeatureStore: " << dsdv->m_featureStore.Size() << " neighbors" << std::endl;
}

void
VerifyAtSecond10(NodeContainer& nodes)
{
    std::cout << "\n========================================================" << std::endl;
    std::cout << "  VERIFICATION AT t=10s (after full convergence)" << std::endl;
    std::cout << "========================================================" << std::endl;

    // === Print topology ===
    std::cout << "\n--- Network Topology (physical connectivity) ---" << std::endl;
    std::cout << "  0(0,0) -- 1(50,0)" << std::endl;
    std::cout << "  0(0,0) -- 2(0,50)" << std::endl;
    std::cout << "  0(0,0) -- 4(25,25)" << std::endl;
    std::cout << "  1(50,0) -- 3(50,50)" << std::endl;
    std::cout << "  1(50,0) -- 4(25,25)" << std::endl;
    std::cout << "  2(0,50) -- 3(50,50)" << std::endl;
    std::cout << "  2(0,50) -- 4(25,25)" << std::endl;
    std::cout << "  3(50,50) -- 4(25,25)" << std::endl;

    // === Print all routing tables ===
    std::cout << "\n--- Routing Tables (all nodes) ---" << std::endl;
    for (uint32_t i = 0; i < nodes.GetN(); i++)
    {
        PrintRoutingTable(nodes.Get(i), i);
        std::cout << std::endl;
    }

    // === Verify multipath for Node 0 → Node 3 ===
    std::cout << "--- Multipath Verification ---" << std::endl;
    Ptr<Ipv4> ipv4_0 = nodes.Get(0)->GetObject<Ipv4>();
    Ptr<RoutingProtocol> dsdv0 = DynamicCast<RoutingProtocol>(ipv4_0->GetRoutingProtocol());

    Ipv4Address dst3("10.1.1.4"); // node 3's IP
    uint32_t pathCount = dsdv0->m_routingTable.GetPathCount(dst3);
    std::cout << "  Node 0 → Node 3 (10.1.1.4): " << pathCount << " paths" << std::endl;

    std::vector<RoutingTableEntry> paths;
    dsdv0->m_routingTable.GetAllPaths(dst3, paths);
    for (const auto& p : paths)
    {
        std::cout << "    via " << p.GetNextHop() << " hops=" << p.GetHop() << std::endl;
    }

    CHECK(pathCount >= 2, "Multipath: Node 0 has >=2 paths to Node 3 (got " +
          std::to_string(pathCount) + ")");

    // Verify next-hop disjointness
    std::set<Ipv4Address> nextHops;
    for (const auto& p : paths)
    {
        nextHops.insert(p.GetNextHop());
    }
    CHECK(nextHops.size() == paths.size(), "Multipath: all paths have disjoint next-hops");

    // === Verify basic connectivity for all nodes ===
    std::cout << "\n--- Connectivity Verification ---" << std::endl;
    for (uint32_t i = 0; i < nodes.GetN(); i++)
    {
        Ptr<Ipv4> ipv4_i = nodes.Get(i)->GetObject<Ipv4>();
        Ptr<RoutingProtocol> dsdv_i = DynamicCast<RoutingProtocol>(ipv4_i->GetRoutingProtocol());
        uint32_t reachable = 0;
        for (uint32_t j = 0; j < nodes.GetN(); j++)
        {
            if (i == j) continue;
            Ipv4Address dst = Ipv4Address(("10.1.1." + std::to_string(j + 1)).c_str());
            RoutingTableEntry rt;
            if (dsdv_i->m_routingTable.LookupRoute(dst, rt) && rt.GetHop() > 0)
            {
                reachable++;
            }
        }
        CHECK(reachable == 4, "Node " + std::to_string(i) + " can reach all 4 other nodes (got " +
              std::to_string(reachable) + ")");
    }

    // === Verify FeatureStore populated ===
    std::cout << "\n--- FeatureStore Verification ---" << std::endl;
    for (uint32_t i = 0; i < nodes.GetN(); i++)
    {
        Ptr<Ipv4> ipv4_i = nodes.Get(i)->GetObject<Ipv4>();
        Ptr<RoutingProtocol> dsdv_i = DynamicCast<RoutingProtocol>(ipv4_i->GetRoutingProtocol());
        uint32_t fsSize = dsdv_i->m_featureStore.Size();
        std::cout << "  Node " << i << ": FeatureStore has " << fsSize << " entries" << std::endl;
        CHECK(fsSize >= 2, "Node " + std::to_string(i) + " FeatureStore >= 2 neighbors");
    }

    // === Verify GNN Observation (what GNN actually receives) ===
    std::cout << "\n--- GNN Observation Verification (Node 0) ---" << std::endl;
    {
        Ptr<Ipv4> ipv4_0 = nodes.Get(0)->GetObject<Ipv4>();
        Ptr<RoutingProtocol> dsdv_0 = DynamicCast<RoutingProtocol>(ipv4_0->GetRoutingProtocol());
        GnnObservation& obs = dsdv_0->m_latestGnnObs;

        std::cout << "  selfId: " << obs.selfId << std::endl;
        std::cout << "  numNodes: " << obs.numNodes
                  << " (1hop=" << obs.num1Hop << " 2hop=" << obs.num2Hop << ")" << std::endl;
        std::cout << "  numEdges: " << obs.numEdges << std::endl;
        std::cout << "  pathDiversity: " << obs.pathDiversity << std::endl;
        std::cout << "  multipathInfo entries: " << obs.multipathInfo.size() << std::endl;

        // Print node features
        std::cout << "  Node features (" << NODE_FEAT_DIM << "-dim):" << std::endl;
        for (uint32_t i = 0; i < obs.nodeIds.size() && i < 6; i++)
        {
            std::cout << "    " << obs.nodeIds[i] << ": [";
            for (uint32_t d = 0; d < obs.nodeFeatures[i].size(); d++)
            {
                std::cout << obs.nodeFeatures[i][d];
                if (d < obs.nodeFeatures[i].size() - 1) std::cout << ", ";
            }
            std::cout << "]" << std::endl;
        }

        // Print edge features
        std::cout << "  Edge features (" << EDGE_FEAT_DIM << "-dim):" << std::endl;
        for (uint32_t i = 0; i < obs.edgeIndex.size() && i < 8; i++)
        {
            auto& e = obs.edgeIndex[i];
            std::cout << "    " << obs.nodeIds[e.first] << "->" << obs.nodeIds[e.second] << ": [";
            for (uint32_t d = 0; d < obs.edgeFeatures[i].size(); d++)
            {
                std::cout << obs.edgeFeatures[i][d];
                if (d < obs.edgeFeatures[i].size() - 1) std::cout << ", ";
            }
            std::cout << "]" << std::endl;
        }

        // Print multipath info
        std::cout << "  Multipath candidates:" << std::endl;
        for (const auto& mp : obs.multipathInfo)
        {
            std::cout << "    dst=" << mp.dest << " primary=" << mp.primary << " candidates=[";
            for (uint32_t i = 0; i < mp.candidates.size(); i++)
            {
                std::cout << mp.candidates[i] << "(h" << mp.hopCounts[i] << ")";
                if (i < mp.candidates.size() - 1) std::cout << ", ";
            }
            std::cout << "]" << std::endl;
        }

        // Assertions
        CHECK(obs.numNodes >= 3, "GNN obs has >= 3 nodes");
        CHECK(obs.numEdges >= 2, "GNN obs has >= 2 edges");
        CHECK(obs.nodeFeatures.size() == obs.numNodes, "nodeFeatures count matches numNodes");
        CHECK(obs.edgeFeatures.size() == obs.numEdges, "edgeFeatures count matches numEdges");
        CHECK(obs.nodeFeatures[0].size() == NODE_FEAT_DIM, "Node feature dim == " +
              std::to_string(NODE_FEAT_DIM));
        CHECK(obs.edgeFeatures[0].size() == EDGE_FEAT_DIM, "Edge feature dim == " +
              std::to_string(EDGE_FEAT_DIM));
    }

    // === Summary ===
    std::cout << "\n========================================================" << std::endl;
    std::cout << "  RESULTS: PASSED=" << g_pass << " FAILED=" << g_fail << std::endl;
    std::cout << "========================================================" << std::endl;
    if (g_fail == 0)
    {
        std::cout << "  ALL TESTS PASSED!" << std::endl;
    }
}

int
main(int argc, char* argv[])
{
    LogComponentEnable("MpDsdvGnnTest", LOG_LEVEL_ALL);
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    std::cout << "========== MP-DSDV-GNN Multipath Test ==========" << std::endl;

    // === Create nodes ===
    NodeContainer nodes;
    nodes.Create(5);

    // === Positions (guarantee multipath topology) ===
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> pos = CreateObject<ListPositionAllocator>();
    pos->Add(Vector(0, 0, 0));    // node 0
    pos->Add(Vector(50, 0, 0));   // node 1
    pos->Add(Vector(0, 50, 0));   // node 2
    pos->Add(Vector(50, 50, 0));  // node 3
    pos->Add(Vector(25, 25, 0));  // node 4 (center)
    mobility.SetPositionAllocator(pos);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // === WiFi adhoc ===
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    YansWifiPhyHelper phy;
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    phy.SetChannel(channel.Create());
    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    // === DSDV routing ===
    DsdvHelper dsdv;
    dsdv.Set("PeriodicUpdateInterval", TimeValue(Seconds(2)));
    dsdv.Set("SettlingTime", TimeValue(Seconds(1)));
    InternetStackHelper stack;
    stack.SetRoutingHelper(dsdv);
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    address.Assign(devices);

    // === Schedule verification at t=10s ===
    Simulator::Schedule(Seconds(10.0), &VerifyAtSecond10, nodes);

    // === Run ===
    std::cout << "Running 12s simulation..." << std::endl;
    Simulator::Stop(Seconds(12));
    Simulator::Run();
    Simulator::Destroy();

    return g_fail > 0 ? 1 : 0;
}
