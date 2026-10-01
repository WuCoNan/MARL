/*
 * MP-DSDV-GNN: ns3-ai joint simulation (C++ side)
 * Runs real DSDV protocol with multipath + feature broadcast,
 * exports GNN observations to shared memory for Python GNN inference.
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

// White-box access to DSDV internals
#define private public
#include <ns3/dsdv-routing-protocol.h>
#undef private

#include <iostream>

#define NUM_NODES 5
#define NUM_STEPS 20

using namespace ns3;
using namespace ns3::dsdv;

NS_LOG_COMPONENT_DEFINE("DsdvGnnMarl");

// Fill shared memory message from GnnObservation
void
FillObsMsg(GnnObsMsg& msg, const GnnObservation& obs)
{
    msg.numNodes = std::min((uint32_t)obs.numNodes, (uint32_t)MAX_NODES);
    msg.numEdges = std::min((uint32_t)obs.numEdges, (uint32_t)MAX_EDGES);
    msg.numNeighbors = std::min((uint32_t)obs.num1Hop, (uint32_t)MAX_NEIGHBORS);
    msg.pathDiversity = obs.pathDiversity;

    // Node features [numNodes x MSG_NODE_FEAT]
    for (uint32_t i = 0; i < msg.numNodes; i++)
    {
        for (uint32_t d = 0; d < MSG_NODE_FEAT && d < obs.nodeFeatures[i].size(); d++)
        {
            msg.nodeFeatures[i * MSG_NODE_FEAT + d] = obs.nodeFeatures[i][d];
        }
    }

    // Edge features [numEdges x MSG_EDGE_FEAT]
    for (uint32_t i = 0; i < msg.numEdges; i++)
    {
        for (uint32_t d = 0; d < MSG_EDGE_FEAT && d < obs.edgeFeatures[i].size(); d++)
        {
            msg.edgeFeatures[i * MSG_EDGE_FEAT + d] = obs.edgeFeatures[i][d];
        }
        msg.edgeSrc[i] = obs.edgeIndex[i].first;
        msg.edgeDst[i] = obs.edgeIndex[i].second;
    }
}

int
main()
{
    // === ns3-ai interface setup ===
    auto interface = Ns3AiMsgInterface::Get();
    interface->SetIsMemoryCreator(false);
    interface->SetUseVector(true);
    interface->SetHandleFinish(true);
    auto* msgIf = interface->GetInterface<GnnObsMsg, GnnActionMsg>();

    // === Network setup (same topology as test) ===
    NodeContainer nodes;
    nodes.Create(NUM_NODES);

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

    // UDP traffic: node 0 → node 3
    uint16_t port = 9;
    Address sinkAddr(InetSocketAddress(Ipv4Address("10.1.1.4"), port));
    PacketSinkHelper sinkHelper("ns3::UdpSocketFactory", sinkAddr);
    ApplicationContainer sinkApp = sinkHelper.Install(nodes.Get(3));
    sinkApp.Start(Seconds(1.0));
    sinkApp.Stop(Seconds(30.0));

    OnOffHelper onOff("ns3::UdpSocketFactory", sinkAddr);
    onOff.SetAttribute("DataRate", DataRateValue(DataRate("200kb/s")));
    onOff.SetAttribute("PacketSize", UintegerValue(512));
    onOff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onOff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer srcApp = onOff.Install(nodes.Get(0));
    srcApp.Start(Seconds(2.0));
    srcApp.Stop(Seconds(30.0));

    // === Wait for convergence ===
    std::cout << "[ns3] Waiting 5s for routing convergence..." << std::endl;
    Simulator::Stop(Seconds(5));
    Simulator::Run();

    // === Main loop: export GNN obs, receive actions ===
    std::cout << "[ns3] Starting GNN interaction loop (" << NUM_STEPS << " steps)" << std::endl;

    for (int step = 0; step < NUM_STEPS; step++)
    {
        // Run 10ms of simulation (one time slot)
        Simulator::Stop(Seconds(5.0 + 0.01 * (step + 1)));
        Simulator::Run();

        // Get GNN observation from node 0's DSDV protocol
        Ptr<Ipv4> ipv4_0 = nodes.Get(0)->GetObject<Ipv4>();
        Ptr<RoutingProtocol> dsdv0 = DynamicCast<RoutingProtocol>(ipv4_0->GetRoutingProtocol());
        GnnObservation& obs = dsdv0->m_latestGnnObs;

        // Write to shared memory
        msgIf->CppSendBegin();
        auto* vec = msgIf->GetCpp2PyVector();
        // Use vector[0] for node 0's observation
        FillObsMsg((*vec)[0], obs);
        msgIf->CppSendEnd();

        std::cout << "[ns3] Step " << step << ": sent obs (nodes=" << obs.numNodes
                  << " edges=" << obs.numEdges << " nbrs=" << obs.num1Hop << ")" << std::endl;

        // Receive action from Python
        msgIf->CppRecvBegin();
        auto* actVec = msgIf->GetPy2CppVector();
        std::cout << "[ns3] Step " << step << ": action probs=[";
        for (uint32_t i = 0; i < obs.num1Hop && i < MAX_NEIGHBORS; i++)
        {
            std::cout << (*actVec)[0].probs[i];
            if (i < obs.num1Hop - 1) std::cout << ", ";
        }
        std::cout << "]" << std::endl;
        msgIf->CppRecvEnd();
    }

    std::cout << "[ns3] Simulation complete." << std::endl;
    Simulator::Destroy();
    return 0;
}
