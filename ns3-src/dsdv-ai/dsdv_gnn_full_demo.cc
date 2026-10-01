/*
 * MP-DSDV-GNN: Full verification with 6-node grid topology
 * Demonstrates: routing tables + GNN observations + Python verification
 *
 * Topology (80m x 40m grid):
 *   0(0,0) --- 1(40,0) --- 2(80,0)
 *   |            |            |
 *   3(0,40) --- 4(40,40) --- 5(80,40)
 *
 * Physical links (802.11b, ~100m range):
 *   Horizontal: 0-1, 1-2, 3-4, 4-5 (40m)
 *   Vertical: 0-3, 1-4, 2-5 (40m)
 *   Diagonal: 0-4, 1-3, 1-5, 2-4 (56.6m)
 *   NOT connected: 0-2(80m>range at high path loss), 0-5(89m), 3-5(80m), 3-2(89m)
 */
#include "dsdv_gnn_marl_msg.h"

#include <ns3/ai-module.h>
#include <ns3/applications-module.h>
#include <ns3/core-module.h>
#include <ns3/dsdv-helper.h>
#include <ns3/internet-module.h>
#include <ns3/mobility-module.h>
#include <ns3/network-module.h>
#include <ns3/wifi-module.h>

#define private public
#include <ns3/dsdv-routing-protocol.h>
#undef private

#include <iostream>
#include <iomanip>

#define NUM_NODES 6
#define NUM_STEPS 5

using namespace ns3;
using namespace ns3::dsdv;

NS_LOG_COMPONENT_DEFINE("DsdvGnnFullDemo");

void
PrintRoutingTables(NodeContainer& nodes)
{
    std::cout << "\n========== ROUTING TABLES (t=5s) ==========" << std::endl;
    for (uint32_t i = 0; i < nodes.GetN(); i++)
    {
        Ptr<Ipv4> ipv4 = nodes.Get(i)->GetObject<Ipv4>();
        Ptr<RoutingProtocol> dsdv = DynamicCast<RoutingProtocol>(ipv4->GetRoutingProtocol());
        std::cout << "\nNode " << i << " (" << dsdv->m_mainAddress << "):" << std::endl;

        std::map<Ipv4Address, RoutingTableEntry> allRoutes;
        dsdv->m_routingTable.GetListOfAllRoutes(allRoutes);
        std::cout << "  Primary:" << std::endl;
        for (const auto& r : allRoutes)
        {
            if (r.second.GetHop() > 0 && !r.first.IsBroadcast())
            {
                std::cout << "    -> " << r.first << " via " << r.second.GetNextHop()
                          << " hops=" << r.second.GetHop() << std::endl;
            }
        }
        uint32_t mpSize = dsdv->m_routingTable.MultipathTableSize();
        if (mpSize > 0)
        {
            std::cout << "  Backups (" << mpSize << "):" << std::endl;
            for (const auto& r : allRoutes)
            {
                if (r.second.GetHop() == 0 || r.first.IsBroadcast()) continue;
                std::vector<RoutingTableEntry> backups;
                if (dsdv->m_routingTable.GetBackupPaths(r.first, backups))
                {
                    for (const auto& bp : backups)
                    {
                        std::cout << "    -> " << r.first << " via " << bp.GetNextHop()
                                  << " hops=" << bp.GetHop() << " [backup]" << std::endl;
                    }
                }
            }
        }
    }
}

void
PrintGnnObs(NodeContainer& nodes, uint32_t nodeIdx)
{
    Ptr<Ipv4> ipv4 = nodes.Get(nodeIdx)->GetObject<Ipv4>();
    Ptr<RoutingProtocol> dsdv = DynamicCast<RoutingProtocol>(ipv4->GetRoutingProtocol());
    GnnObservation& obs = dsdv->m_latestGnnObs;

    std::cout << "\n========== GNN OBSERVATION (Node " << nodeIdx << ") ==========" << std::endl;
    std::cout << "  numNodes=" << obs.numNodes << " (1hop=" << obs.num1Hop
              << " 2hop=" << obs.num2Hop << ") numEdges=" << obs.numEdges
              << " pathDiv=" << obs.pathDiversity << std::endl;

    std::cout << "  Node features [6-dim]: (queue, nbrs, 2hop, load, avg_q, path_div)" << std::endl;
    for (uint32_t i = 0; i < obs.numNodes && i < 8; i++)
    {
        std::cout << "    " << obs.nodeIds[i] << ": [";
        for (uint32_t d = 0; d < obs.nodeFeatures[i].size(); d++)
        {
            std::cout << std::fixed << std::setprecision(3) << obs.nodeFeatures[i][d];
            if (d < obs.nodeFeatures[i].size() - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
    }

    std::cout << "  Edge features [4-dim]: (quality, util, rel, stab)" << std::endl;
    for (uint32_t i = 0; i < obs.numEdges && i < 12; i++)
    {
        auto& e = obs.edgeIndex[i];
        std::cout << "    " << obs.nodeIds[e.first] << "->" << obs.nodeIds[e.second] << ": [";
        for (uint32_t d = 0; d < obs.edgeFeatures[i].size(); d++)
        {
            std::cout << std::fixed << std::setprecision(3) << obs.edgeFeatures[i][d];
            if (d < obs.edgeFeatures[i].size() - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
    }

    std::cout << "  Multipath info:" << std::endl;
    for (const auto& mp : obs.multipathInfo)
    {
        std::cout << "    dst=" << mp.dest << " primary=" << mp.primary << " paths=[";
        for (uint32_t i = 0; i < mp.candidates.size(); i++)
        {
            std::cout << mp.candidates[i] << "(h" << mp.hopCounts[i] << ")";
            if (i < mp.candidates.size() - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
    }
}

void
FillObsMsg(GnnObsMsg& msg, const GnnObservation& obs)
{
    msg.numNodes = std::min((uint32_t)obs.numNodes, (uint32_t)MAX_NODES);
    msg.numEdges = std::min((uint32_t)obs.numEdges, (uint32_t)MAX_EDGES);
    msg.numNeighbors = std::min((uint32_t)obs.num1Hop, (uint32_t)MAX_NEIGHBORS);
    msg.pathDiversity = obs.pathDiversity;
    for (uint32_t i = 0; i < msg.numNodes; i++)
        for (uint32_t d = 0; d < MSG_NODE_FEAT && d < obs.nodeFeatures[i].size(); d++)
            msg.nodeFeatures[i * MSG_NODE_FEAT + d] = obs.nodeFeatures[i][d];
    for (uint32_t i = 0; i < msg.numEdges; i++)
    {
        for (uint32_t d = 0; d < MSG_EDGE_FEAT && d < obs.edgeFeatures[i].size(); d++)
            msg.edgeFeatures[i * MSG_EDGE_FEAT + d] = obs.edgeFeatures[i][d];
        msg.edgeSrc[i] = obs.edgeIndex[i].first;
        msg.edgeDst[i] = obs.edgeIndex[i].second;
    }
}

int
main()
{
    auto interface = Ns3AiMsgInterface::Get();
    interface->SetIsMemoryCreator(false);
    interface->SetUseVector(true);
    interface->SetHandleFinish(true);
    auto* msgIf = interface->GetInterface<GnnObsMsg, GnnActionMsg>();

    // === 6-node grid topology ===
    NodeContainer nodes;
    nodes.Create(NUM_NODES);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> pos = CreateObject<ListPositionAllocator>();
    pos->Add(Vector(0, 0, 0));    // node 0: top-left
    pos->Add(Vector(40, 0, 0));   // node 1: top-center
    pos->Add(Vector(80, 0, 0));   // node 2: top-right
    pos->Add(Vector(0, 40, 0));   // node 3: bottom-left
    pos->Add(Vector(40, 40, 0));  // node 4: bottom-center
    pos->Add(Vector(80, 40, 0));  // node 5: bottom-right
    mobility.SetPositionAllocator(pos);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    YansWifiPhyHelper phy;
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    phy.SetChannel(channel.Create());
    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

    DsdvHelper dsdv;
    dsdv.Set("PeriodicUpdateInterval", TimeValue(Seconds(2)));
    dsdv.Set("SettlingTime", TimeValue(Seconds(1)));
    InternetStackHelper stack;
    stack.SetRoutingHelper(dsdv);
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    address.Assign(devices);

    // Traffic: node 0 → node 5 (diagonal, requires multi-hop)
    uint16_t port = 9;
    Address sinkAddr(InetSocketAddress(Ipv4Address("10.1.1.6"), port));
    PacketSinkHelper sink("ns3::UdpSocketFactory", sinkAddr);
    ApplicationContainer sinkApp = sink.Install(nodes.Get(5));
    sinkApp.Start(Seconds(1.0));
    sinkApp.Stop(Seconds(30.0));

    OnOffHelper onOff("ns3::UdpSocketFactory", sinkAddr);
    onOff.SetAttribute("DataRate", DataRateValue(DataRate("300kb/s")));
    onOff.SetAttribute("PacketSize", UintegerValue(512));
    onOff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onOff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer srcApp = onOff.Install(nodes.Get(0));
    srcApp.Start(Seconds(2.0));
    srcApp.Stop(Seconds(30.0));

    // === Converge ===
    std::cout << "[ns3] 6-node grid, converging 5s..." << std::endl;
    Simulator::Stop(Seconds(5));
    Simulator::Run();

    // === Print routing tables ===
    PrintRoutingTables(nodes);

    // === Print GNN observations for node 0 and node 4 ===
    PrintGnnObs(nodes, 0);
    PrintGnnObs(nodes, 4);

    // === Interaction loop ===
    std::cout << "\n========== GNN INTERACTION (5 steps) ==========" << std::endl;
    for (int step = 0; step < NUM_STEPS; step++)
    {
        Simulator::Stop(Seconds(5.0 + 0.01 * (step + 1)));
        Simulator::Run();

        Ptr<Ipv4> ipv4_0 = nodes.Get(0)->GetObject<Ipv4>();
        Ptr<RoutingProtocol> dsdv0 = DynamicCast<RoutingProtocol>(ipv4_0->GetRoutingProtocol());

        msgIf->CppSendBegin();
        FillObsMsg((*msgIf->GetCpp2PyVector())[0], dsdv0->m_latestGnnObs);
        msgIf->CppSendEnd();

        msgIf->CppRecvBegin();
        auto* act = msgIf->GetPy2CppVector();
        std::cout << "[ns3] Step " << step << ": action=[";
        for (uint32_t i = 0; i < dsdv0->m_latestGnnObs.num1Hop && i < MAX_NEIGHBORS; i++)
        {
            std::cout << std::fixed << std::setprecision(3) << (*act)[0].probs[i];
            if (i < dsdv0->m_latestGnnObs.num1Hop - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
        msgIf->CppRecvEnd();
    }

    std::cout << "[ns3] Done." << std::endl;
    Simulator::Destroy();
    return 0;
}
