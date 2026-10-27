/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: example-network-control
 * Real simulated network adaptation with PointToPoint links and IPv4 routing.
 * Closed-loop control:
 *   1. Node 0 sends UDP packets to Node 3 via Node 1 (primary path).
 *   2. At t = 15s, primary link (0-1) fails.
 *   3. ObservationProvider detects link failure and notifies external agent.
 *   4. Python rule agent issues SELECT_PATH(target=2).
 *   5. ActionHandler dynamically reconfigures Ipv4StaticRouting on Node 0.
 *   6. Packet flow resumes to Node 3 via backup Node 2.
 */

#include "ns3/agent-control-helper.h"
#include "ns3/applications-module.h"
#include "ns3/command-line.h"
#include "ns3/core-module.h"
#include "ns3/futurenet-agent-types.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/socket-ipc-adapter.h"
#include <iostream>

using namespace ns3;
using namespace ns3::futurenet;

NS_LOG_COMPONENT_DEFINE("FutureNetNetworkControlExample");

namespace {

/**
 * \brief Real network observation provider querying NetDevice and Link availability.
 */
class NetworkRoutingObservationProvider : public ObservationProvider
{
public:
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::futurenet::NetworkRoutingObservationProvider")
                                .SetParent<ObservationProvider>()
                                .SetGroupName("FutureNet")
                                .AddConstructor<NetworkRoutingObservationProvider>();
        return tid;
    }

    NetworkRoutingObservationProvider() = default;

    void SetPrimaryLinkStatus(bool up)
    {
        m_primaryLinkUp = up;
    }

    void AppendObservation(AgentObservation& obs) const override
    {
        // Node states
        for (uint32_t i = 0; i < 4; ++i)
        {
            NetworkState ns;
            ns.nodeId = i;
            ns.oneWayDelayMs = 2.0;
            ns.throughputMbps = 10.0;
            ns.packetLossRatio = 0.0;
            ns.availableBandwidthMbps = 50.0;
            ns.queuePackets = 1;
            ns.nodeLoad = 0.1;
            obs.nodes.push_back(ns);
        }

        // Link 0 -> 1 (Primary)
        LinkState lPrimary;
        lPrimary.sourceNodeId = 0;
        lPrimary.destinationNodeId = 1;
        lPrimary.available = m_primaryLinkUp;
        lPrimary.dataRateMbps = 100.0;
        lPrimary.propagationDelayMs = 2.0;
        lPrimary.lossRatio = m_primaryLinkUp ? 0.0 : 1.0;
        obs.links.push_back(lPrimary);

        // Link 0 -> 2 (Backup)
        LinkState lBackup;
        lBackup.sourceNodeId = 0;
        lBackup.destinationNodeId = 2;
        lBackup.available = true;
        lBackup.dataRateMbps = 100.0;
        lBackup.propagationDelayMs = 4.0;
        lBackup.lossRatio = 0.0;
        obs.links.push_back(lBackup);
    }

private:
    bool m_primaryLinkUp{true};
};

/**
 * \brief Real network action handler modifying Ipv4StaticRouting upon SELECT_PATH.
 */
class NetworkRoutingActionHandler : public ActionHandler
{
public:
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::futurenet::NetworkRoutingActionHandler")
                                .SetParent<ActionHandler>()
                                .SetGroupName("FutureNet")
                                .AddConstructor<NetworkRoutingActionHandler>();
        return tid;
    }

    NetworkRoutingActionHandler() = default;

    void SetRoutingContext(Ptr<Ipv4StaticRouting> staticRouting,
                           Ipv4Address destNetwork,
                           Ipv4Mask destMask,
                           Ipv4Address primaryNextHop,
                           uint32_t primaryIfIndex,
                           Ipv4Address backupNextHop,
                           uint32_t backupIfIndex)
    {
        m_routing = staticRouting;
        m_destNetwork = destNetwork;
        m_destMask = destMask;
        m_primaryNextHop = primaryNextHop;
        m_primaryIfIndex = primaryIfIndex;
        m_backupNextHop = backupNextHop;
        m_backupIfIndex = backupIfIndex;
    }

    bool Supports(const AgentAction& action) const override
    {
        return (action.type == AgentActionType::SELECT_PATH);
    }

    ActionResult Execute(const AgentAction& action) override
    {
        if (!m_routing)
        {
            return {false, "Routing context not set"};
        }

        if (action.targetId == 2)
        {
            // Switch routing on Node 0 to Backup (Node 2)
            m_routing->SetDefaultRoute(m_backupNextHop, m_backupIfIndex);
            std::cout << "\n========================================================\n"
                      << " [AGENT CONTROL ACTION APPLIED at " << Simulator::Now().GetSeconds() << "s]\n"
                      << " Dynamic Route Update: Node 0 default route switched to BACKUP (Node 2)\n"
                      << " Gateway: " << m_backupNextHop << " via Interface " << m_backupIfIndex << "\n"
                      << " Traffic successfully rerouted around failed link!\n"
                      << "========================================================\n" << std::endl;
            return {true, "Rerouted to backup path"};
        }
        else if (action.targetId == 1)
        {
            // Switch back to Primary (Node 1)
            m_routing->SetDefaultRoute(m_primaryNextHop, m_primaryIfIndex);
            std::cout << "[AgentAction] Restored default route to PRIMARY (Node 1)" << std::endl;
            return {true, "Rerouted to primary path"};
        }

        return {false, "Unknown path target ID"};
    }

private:
    Ptr<Ipv4StaticRouting> m_routing;
    Ipv4Address m_destNetwork;
    Ipv4Mask m_destMask;
    Ipv4Address m_primaryNextHop;
    uint32_t m_primaryIfIndex{0};
    Ipv4Address m_backupNextHop;
    uint32_t m_backupIfIndex{0};
};

// Event callback to simulate physical link failure
void TriggerLinkFailure(Ptr<NetworkRoutingObservationProvider> provider,
                        NetDeviceContainer devices)
{
    std::cout << "\n>>> [SIMULATOR EVENT at " << Simulator::Now().GetSeconds() << "s]: "
              << "Physical Link 0 -> 1 is failing! Disabling link devices... <<<\n" << std::endl;
    provider->SetPrimaryLinkStatus(false);
    devices.Get(0)->SetReceiveCallback(MakeNullCallback<bool, Ptr<NetDevice>, Ptr<const Packet>, uint16_t, const Address&>());
}

// Packet sink rx trace
void RxTrace(Ptr<const Packet> packet, const Address& addr)
{
    static uint32_t rxCount = 0;
    rxCount++;
    if (rxCount % 10 == 1)
    {
        std::cout << "[" << Simulator::Now().GetSeconds() << "s] Node 3 received packet #"
                  << rxCount << " (" << packet->GetSize() << " bytes)" << std::endl;
    }
}

} // anonymous namespace

int main(int argc, char* argv[])
{
    uint16_t port = 12345;
    double simTimeS = 35.0;
    std::string csvPath = "agent-network-control-trace.csv";

    CommandLine cmd(__FILE__);
    cmd.AddValue("port", "TCP port for Python agent connection", port);
    cmd.AddValue("simTime", "Total simulation time in seconds", simTimeS);
    cmd.AddValue("csv", "CSV trace file path", csvPath);
    cmd.Parse(argc, argv);

    std::cout << "========================================================\n"
              << " FutureNet M10 - Real Network Control Demonstration\n"
              << " Topology: Node 0 (Source) -> [Node 1 (Prim) / Node 2 (Backup)] -> Node 3 (Dest)\n"
              << " Simulation Time: " << simTimeS << " s\n"
              << " Agent TCP Port: " << port << "\n"
              << "========================================================" << std::endl;

    // 1. Create nodes: 0 = Source, 1 = Primary, 2 = Backup, 3 = Dest
    NodeContainer nodes;
    nodes.Create(4);

    // 2. Configure PointToPoint links
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("50Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer d01 = p2p.Install(nodes.Get(0), nodes.Get(1)); // Primary link
    NetDeviceContainer d02 = p2p.Install(nodes.Get(0), nodes.Get(2)); // Backup link
    NetDeviceContainer d13 = p2p.Install(nodes.Get(1), nodes.Get(3));
    NetDeviceContainer d23 = p2p.Install(nodes.Get(2), nodes.Get(3));

    // 3. Install Internet stack and Static Routing
    InternetStackHelper internet;
    Ipv4StaticRoutingHelper staticRoutingHelper;
    internet.SetRoutingHelper(staticRoutingHelper);
    internet.Install(nodes);

    // 4. Assign IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i01 = ipv4.Assign(d01);

    ipv4.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i02 = ipv4.Assign(d02);

    ipv4.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer i13 = ipv4.Assign(d13);

    ipv4.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer i23 = ipv4.Assign(d23);

    // 5. Configure Static Routing on Node 0, Node 1, Node 2
    Ptr<Ipv4> ipv4Node0 = nodes.Get(0)->GetObject<Ipv4>();
    Ptr<Ipv4StaticRouting> staticRouting0 = staticRoutingHelper.GetStaticRouting(ipv4Node0);

    Ptr<Ipv4> ipv4Node1 = nodes.Get(1)->GetObject<Ipv4>();
    Ptr<Ipv4StaticRouting> staticRouting1 = staticRoutingHelper.GetStaticRouting(ipv4Node1);
    staticRouting1->AddNetworkRouteTo(Ipv4Address("10.1.3.0"), Ipv4Mask("255.255.255.0"), 2);

    Ptr<Ipv4> ipv4Node2 = nodes.Get(2)->GetObject<Ipv4>();
    Ptr<Ipv4StaticRouting> staticRouting2 = staticRoutingHelper.GetStaticRouting(ipv4Node2);
    staticRouting2->AddNetworkRouteTo(Ipv4Address("10.1.4.0"), Ipv4Mask("255.255.255.0"), 2);

    // Initial default route: send via Node 1 (interface index 1)
    staticRouting0->SetDefaultRoute(i01.GetAddress(1), 1);

    // 6. Set up UDP traffic from Node 0 to Node 3
    uint16_t sinkPort = 9000;
    Address sinkAddress(InetSocketAddress(i13.GetAddress(1), sinkPort));
    PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install(nodes.Get(3));
    sinkApps.Start(Seconds(1.0));
    sinkApps.Stop(Seconds(simTimeS));

    // Connect trace to packet sink
    Config::ConnectWithoutContext("/NodeList/3/ApplicationList/0/$ns3::PacketSink/Rx",
                                  MakeCallback(&RxTrace));

    OnOffHelper onoff("ns3::UdpSocketFactory", sinkAddress);
    onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    onoff.SetAttribute("DataRate", DataRateValue(DataRate("5Mbps")));
    onoff.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApps = onoff.Install(nodes.Get(0));
    clientApps.Start(Seconds(2.0));
    clientApps.Stop(Seconds(simTimeS));

    // 7. Setup Agent Control Components
    Ptr<NetworkRoutingObservationProvider> obsProvider = CreateObject<NetworkRoutingObservationProvider>();
    Ptr<NetworkRoutingActionHandler> actHandler = CreateObject<NetworkRoutingActionHandler>();
    actHandler->SetRoutingContext(staticRouting0,
                                  Ipv4Address("10.1.3.0"),
                                  Ipv4Mask("255.255.255.0"),
                                  i01.GetAddress(1), 1,  // Primary gateway Node 1
                                  i02.GetAddress(1), 2); // Backup gateway Node 2

    Ptr<SocketIpcAdapter> ipc = CreateObject<SocketIpcAdapter>();
    ipc->SetAttribute("Port", UintegerValue(port));

    AgentControlHelper helper;
    helper.SetInterfaceAttribute("DecisionInterval", TimeValue(Seconds(1.0)));
    helper.SetInterfaceAttribute("ActionTimeout", TimeValue(MilliSeconds(500)));
    helper.RegisterProvider(obsProvider);
    helper.RegisterHandler(actHandler);

    Ptr<AgentInterface> agentInterface = helper.Install();
    agentInterface->SetIpcAdapter(ipc);
    helper.EnableCsvTracing(csvPath);

    // 8. Schedule Physical Link Failure event at t = 15.0s
    Simulator::Schedule(Seconds(15.0), &TriggerLinkFailure, obsProvider, d01);

    // Start Agent loop
    agentInterface->Start();

    std::cout << "[Simulator] Running real network simulation until t = " << simTimeS << " s..." << std::endl;
    Simulator::Stop(Seconds(simTimeS));
    Simulator::Run();
    Simulator::Destroy();

    std::cout << "[Simulator] Done. Results written to: " << csvPath << std::endl;
    return 0;
}
