/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: example-mock-control
 * Demonstrates C++ <-> Python closed-loop interaction using SocketIpcAdapter.
 * Meets SRS Section 13 demonstration scale:
 *   - 30 controlled entities (15 nodes + 15 links > 25 minimum)
 *   - 100+ control decisions over 120 seconds simulation time.
 */

#include "ns3/agent-control-helper.h"
#include "ns3/command-line.h"
#include "ns3/core-module.h"
#include "ns3/futurenet-agent-types.h"
#include "ns3/socket-ipc-adapter.h"
#include <iostream>

using namespace ns3;
using namespace ns3::futurenet;

NS_LOG_COMPONENT_DEFINE("FutureNetMockControlExample");

namespace {

/**
 * \brief Mock network observation provider modeling 15 nodes and 15 links.
 */
class ScalableMockObservationProvider : public ObservationProvider
{
public:
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::futurenet::ScalableMockObservationProvider")
                                .SetParent<ObservationProvider>()
                                .SetGroupName("FutureNet")
                                .AddConstructor<ScalableMockObservationProvider>();
        return tid;
    }

    ScalableMockObservationProvider()
    {
        // Initialize 15 nodes
        for (uint32_t i = 0; i < 15; ++i)
        {
            NetworkState n;
            n.nodeId = i;
            n.oneWayDelayMs = 2.0 + (i * 0.5);
            n.throughputMbps = 20.0 + (i * 2.0);
            n.packetLossRatio = 0.001 * i;
            n.availableBandwidthMbps = 80.0 - (i * 2.0);
            n.queuePackets = (i == 4) ? 15 : 2; // Node 4 has overloaded queue to test Rule 2
            n.nodeLoad = 0.2 + (i * 0.04);
            n.energyJ = 500.0 - (i * 10.0);
            n.energyValid = true;
            m_nodes.push_back(n);
        }

        // Initialize 15 links
        for (uint32_t i = 0; i < 15; ++i)
        {
            LinkState l;
            l.sourceNodeId = i;
            l.destinationNodeId = (i + 1) % 15;
            l.available = true;
            l.dataRateMbps = 100.0;
            l.propagationDelayMs = 5.0 + (i * 0.2);
            l.lossRatio = 0.001;
            m_links.push_back(l);
        }
    }

    void SetLinkDown(uint32_t src, uint32_t dst)
    {
        for (auto& l : m_links)
        {
            if (l.sourceNodeId == src && l.destinationNodeId == dst)
            {
                l.available = false;
                std::cout << "[" << Simulator::Now().GetSeconds()
                          << "s] EVENT: Link " << src << " -> " << dst << " FAILED (DOWN)!"
                          << std::endl;
                break;
            }
        }
    }

    void SetQueueLength(uint32_t nodeId, uint32_t qlen)
    {
        for (auto& n : m_nodes)
        {
            if (n.nodeId == nodeId)
            {
                n.queuePackets = qlen;
                break;
            }
        }
    }

    void AppendObservation(AgentObservation& obs) const override
    {
        for (const auto& n : m_nodes)
        {
            obs.nodes.push_back(n);
        }
        for (const auto& l : m_links)
        {
            obs.links.push_back(l);
        }
    }

private:
    std::vector<NetworkState> m_nodes;
    std::vector<LinkState> m_links;
};

/**
 * \brief Mock action handler executing SELECT_PATH, SELECT_EDGE, and SET_RATE_OR_WEIGHT.
 */
class ScalableMockActionHandler : public ActionHandler
{
public:
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::futurenet::ScalableMockActionHandler")
                                .SetParent<ActionHandler>()
                                .SetGroupName("FutureNet")
                                .AddConstructor<ScalableMockActionHandler>();
        return tid;
    }

    ScalableMockActionHandler() = default;

    bool Supports(const AgentAction& action) const override
    {
        return (action.type == AgentActionType::SELECT_PATH ||
                action.type == AgentActionType::SELECT_EDGE ||
                action.type == AgentActionType::SET_RATE_OR_WEIGHT ||
                action.type == AgentActionType::TRIGGER_REEVALUATION);
    }

    ActionResult Execute(const AgentAction& action) override
    {
        std::cout << "[" << Simulator::Now().GetSeconds() << "s] EXECUTED ACTION: "
                  << AgentActionTypeToString(action.type)
                  << " target=" << action.targetId
                  << " secTarget=" << action.secondaryTargetId
                  << " val=" << action.value << std::endl;
        return {true, "Action executed in network"};
    }
};

} // anonymous namespace

int main(int argc, char* argv[])
{
    uint16_t port = 12345;
    double simTimeS = 120.0;
    double intervalS = 1.0;
    std::string csvPath = "agent-mock-control-trace.csv";

    CommandLine cmd(__FILE__);
    cmd.AddValue("port", "TCP port for Python agent connection", port);
    cmd.AddValue("simTime", "Total simulation time in seconds", simTimeS);
    cmd.AddValue("interval", "Control decision interval in seconds", intervalS);
    cmd.AddValue("csv", "Path to write machine-readable CSV output", csvPath);
    cmd.Parse(argc, argv);

    std::cout << "========================================================\n"
              << " FutureNet M10 - Mock Control Demonstration\n"
              << " Simulation Time: " << simTimeS << " s\n"
              << " Decision Interval: " << intervalS << " s\n"
              << " TCP Port: " << port << "\n"
              << " CSV Output: " << csvPath << "\n"
              << "========================================================" << std::endl;

    Ptr<ScalableMockObservationProvider> provider = CreateObject<ScalableMockObservationProvider>();
    Ptr<ScalableMockActionHandler> handler = CreateObject<ScalableMockActionHandler>();

    // Set up Socket IPC Adapter
    Ptr<SocketIpcAdapter> ipc = CreateObject<SocketIpcAdapter>();
    ipc->SetAttribute("Port", UintegerValue(port));

    // Configure AgentControlHelper
    AgentControlHelper helper;
    helper.SetInterfaceAttribute("DecisionInterval", TimeValue(Seconds(intervalS)));
    helper.SetInterfaceAttribute("ActionTimeout", TimeValue(MilliSeconds(500)));
    helper.SetInterfaceAttribute("StrictActionValidation", BooleanValue(true));

    helper.RegisterProvider(provider);
    helper.RegisterHandler(handler);

    Ptr<AgentInterface> agentInterface = helper.Install();
    agentInterface->SetIpcAdapter(ipc);
    helper.EnableCsvTracing(csvPath);

    // Schedule controlled scenario events to test validation rules:
    // 1. Force Link 0 -> 1 failure at t = 30s (V1 validation: link failure reroute)
    Simulator::Schedule(Seconds(30.0), &ScalableMockObservationProvider::SetLinkDown, provider, 0, 1);

    // 2. Clear overloaded queue on Node 4 at t = 60s
    Simulator::Schedule(Seconds(60.0), &ScalableMockObservationProvider::SetQueueLength, provider, 4, 1);

    // Start agent periodic control loop
    agentInterface->Start();

    std::cout << "[Simulator] Listening for Python agent on port " << port << "...\n"
              << "[Simulator] Running simulation until t = " << simTimeS << " s..." << std::endl;

    Simulator::Stop(Seconds(simTimeS));
    Simulator::Run();
    Simulator::Destroy();

    std::cout << "[Simulator] Simulation complete. Trace saved to: " << csvPath << std::endl;
    return 0;
}
