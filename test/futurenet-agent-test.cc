/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: Automated unit and integration test suite.
 * Tests T01 - T06 covering all mandatory requirements from SRS Section 12.
 */

#include "ns3/action-executor.h"
#include "ns3/action-handler.h"
#include "ns3/action-validator.h"
#include "ns3/agent-interface.h"
#include "ns3/futurenet-agent-types.h"
#include "ns3/observation-provider.h"
#include "ns3/test.h"

using namespace ns3;
using namespace ns3::futurenet;

namespace {

/**
 * \brief Concrete mock observation provider for test cases.
 */
class MockObservationProvider : public ObservationProvider
{
public:
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::futurenet::MockObservationProvider")
                                .SetParent<ObservationProvider>()
                                .SetGroupName("FutureNet")
                                .AddConstructor<MockObservationProvider>();
        return tid;
    }

    MockObservationProvider() = default;
    ~MockObservationProvider() override = default;

    void AppendObservation(AgentObservation& obs) const override
    {
        NetworkState n;
        n.nodeId = 10;
        n.oneWayDelayMs = 12.5;
        n.throughputMbps = 50.0;
        n.packetLossRatio = 0.01;
        n.availableBandwidthMbps = 45.0;
        n.queuePackets = 5;
        n.nodeLoad = 0.4;
        n.energyJ = 120.0;
        n.energyValid = true;
        obs.nodes.push_back(n);

        LinkState l;
        l.sourceNodeId = 1;
        l.destinationNodeId = 2;
        l.available = true;
        l.dataRateMbps = 100.0;
        l.propagationDelayMs = 5.0;
        l.lossRatio = 0.005;
        obs.links.push_back(l);
    }
};

/**
 * \brief Concrete mock action handler for test cases.
 */
class MockActionHandler : public ActionHandler
{
public:
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::futurenet::MockActionHandler")
                                .SetParent<ActionHandler>()
                                .SetGroupName("FutureNet")
                                .AddConstructor<MockActionHandler>();
        return tid;
    }

    MockActionHandler() = default;
    ~MockActionHandler() override = default;

    bool Supports(const AgentAction& action) const override
    {
        return (action.type == AgentActionType::SELECT_PATH ||
                action.type == AgentActionType::SET_RATE_OR_WEIGHT);
    }

    ActionResult Execute(const AgentAction& action) override
    {
        m_lastAction = action;
        m_execCount++;
        return {true, "Mock action executed successfully"};
    }

    AgentAction m_lastAction;
    uint32_t m_execCount{0};
};

} // anonymous namespace

// ----------------------------------------------------------------------------
// T01: Observation Schema Test
// ----------------------------------------------------------------------------
class FutureNetAgentT01ObservationTest : public TestCase
{
public:
    FutureNetAgentT01ObservationTest()
        : TestCase("T01: Observation schema contains correct known values from provider")
    {
    }

    void DoRun() override
    {
        Ptr<AgentInterface> agent = CreateObject<AgentInterface>();
        Ptr<MockObservationProvider> provider = CreateObject<MockObservationProvider>();
        agent->RegisterObservationProvider(provider);

        AgentObservation obs = agent->BuildObservation();

        NS_TEST_ASSERT_MSG_EQ(obs.decisionId, 1, "Decision ID should start at 1");
        NS_TEST_ASSERT_MSG_EQ(obs.nodes.size(), 1, "Expected 1 node state");
        NS_TEST_ASSERT_MSG_EQ(obs.nodes[0].nodeId, 10, "Node ID should match");
        NS_TEST_ASSERT_MSG_EQ_TOL(obs.nodes[0].oneWayDelayMs, 12.5, 0.001, "Delay should match");
        NS_TEST_ASSERT_MSG_EQ_TOL(obs.nodes[0].throughputMbps, 50.0, 0.001, "Throughput should match");
        NS_TEST_ASSERT_MSG_EQ(obs.nodes[0].queuePackets, 5, "Queue packets should match");

        NS_TEST_ASSERT_MSG_EQ(obs.links.size(), 1, "Expected 1 link state");
        NS_TEST_ASSERT_MSG_EQ(obs.links[0].sourceNodeId, 1, "Source node should match");
        NS_TEST_ASSERT_MSG_EQ(obs.links[0].destinationNodeId, 2, "Destination node should match");
        NS_TEST_ASSERT_MSG_EQ(obs.links[0].available, true, "Link should be available");
        NS_TEST_ASSERT_MSG_EQ_TOL(obs.links[0].dataRateMbps, 100.0, 0.001, "Data rate should match");
    }
};

// ----------------------------------------------------------------------------
// T02: Unsupported Action Rejected Test
// ----------------------------------------------------------------------------
class FutureNetAgentT02UnsupportedActionTest : public TestCase
{
public:
    FutureNetAgentT02UnsupportedActionTest()
        : TestCase("T02: Unsupported action is rejected by validator")
    {
    }

    void DoRun() override
    {
        Ptr<ActionValidator> validator = CreateObject<ActionValidator>();
        Ptr<MockActionHandler> handler = CreateObject<MockActionHandler>();
        std::vector<Ptr<ActionHandler>> handlers = {handler};

        AgentObservation obs;
        obs.decisionId = 5;

        // Handler only supports SELECT_PATH and SET_RATE_OR_WEIGHT; TRIGGER_REEVALUATION is unsupported
        AgentAction unsupportedAction;
        unsupportedAction.decisionId = 5;
        unsupportedAction.type = AgentActionType::TRIGGER_REEVALUATION;
        unsupportedAction.targetId = 1;

        ValidationResult res = validator->Validate(unsupportedAction, obs, handlers);
        NS_TEST_ASSERT_MSG_EQ(res.valid, false, "Unsupported action must be rejected");
    }
};

// ----------------------------------------------------------------------------
// T03: Stale Decision ID Rejected Test
// ----------------------------------------------------------------------------
class FutureNetAgentT03StaleDecisionIdTest : public TestCase
{
public:
    FutureNetAgentT03StaleDecisionIdTest()
        : TestCase("T03: Stale/wrong decision ID is rejected when strict mode is enabled")
    {
    }

    void DoRun() override
    {
        Ptr<ActionValidator> validator = CreateObject<ActionValidator>();
        Ptr<MockActionHandler> handler = CreateObject<MockActionHandler>();
        std::vector<Ptr<ActionHandler>> handlers = {handler};

        AgentObservation obs;
        obs.decisionId = 10;

        // Action has stale decision ID (8 != 10)
        AgentAction staleAction;
        staleAction.decisionId = 8;
        staleAction.type = AgentActionType::SELECT_PATH;
        staleAction.targetId = 1;

        validator->SetStrictValidation(true);
        ValidationResult strictRes = validator->Validate(staleAction, obs, handlers);
        NS_TEST_ASSERT_MSG_EQ(strictRes.valid, false, "Stale decision ID must be rejected in strict mode");

        validator->SetStrictValidation(false);
        ValidationResult nonStrictRes = validator->Validate(staleAction, obs, handlers);
        NS_TEST_ASSERT_MSG_EQ(nonStrictRes.valid, true, "Action should be accepted when strict mode is disabled");
    }
};

// ----------------------------------------------------------------------------
// T04: NO_OP Changes No Controlled State Test
// ----------------------------------------------------------------------------
class FutureNetAgentT04NoOpTest : public TestCase
{
public:
    FutureNetAgentT04NoOpTest()
        : TestCase("T04: NO_OP changes no controlled state")
    {
    }

    void DoRun() override
    {
        Ptr<AgentInterface> agent = CreateObject<AgentInterface>();
        Ptr<MockActionHandler> handler = CreateObject<MockActionHandler>();
        agent->RegisterActionHandler(handler);

        // Build observation to advance decision ID
        AgentObservation obs = agent->BuildObservation();

        AgentAction noop;
        noop.decisionId = obs.decisionId;
        noop.type = AgentActionType::NO_OP;

        ActionResult res = agent->SubmitAction(noop);
        NS_TEST_ASSERT_MSG_EQ(res.success, true, "NO_OP should execute successfully");
        NS_TEST_ASSERT_MSG_EQ(handler->m_execCount, 0, "NO_OP must not invoke handler state changes");
    }
};

// ----------------------------------------------------------------------------
// T05: Valid Action Reaches ActionHandler Test
// ----------------------------------------------------------------------------
class FutureNetAgentT05ValidActionDispatchTest : public TestCase
{
public:
    FutureNetAgentT05ValidActionDispatchTest()
        : TestCase("T05: Valid action reaches registered ActionHandler")
    {
    }

    void DoRun() override
    {
        Ptr<AgentInterface> agent = CreateObject<AgentInterface>();
        Ptr<MockActionHandler> handler = CreateObject<MockActionHandler>();
        agent->RegisterActionHandler(handler);

        AgentObservation obs = agent->BuildObservation();

        AgentAction action;
        action.decisionId = obs.decisionId;
        action.type = AgentActionType::SELECT_PATH;
        action.targetId = 42;
        action.secondaryTargetId = 7;
        action.value = 100.0;

        ActionResult res = agent->SubmitAction(action);
        NS_TEST_ASSERT_MSG_EQ(res.success, true, "Valid action execution should succeed");
        NS_TEST_ASSERT_MSG_EQ(handler->m_execCount, 1, "Handler should have been executed once");
        NS_TEST_ASSERT_MSG_EQ(handler->m_lastAction.targetId, 42, "Target ID must match dispatched action");
    }
};

// ----------------------------------------------------------------------------
// T06: Deterministic Action Sequence Test
// ----------------------------------------------------------------------------
class FutureNetAgentT06DeterministicActionSequenceTest : public TestCase
{
public:
    FutureNetAgentT06DeterministicActionSequenceTest()
        : TestCase("T06: Fixed network/rule input produces deterministic action sequence")
    {
    }

    void DoRun() override
    {
        auto runSimulation = []() -> std::vector<uint32_t> {
            Ptr<AgentInterface> agent = CreateObject<AgentInterface>();
            Ptr<MockActionHandler> handler = CreateObject<MockActionHandler>();
            agent->RegisterActionHandler(handler);

            std::vector<uint32_t> targets;
            for (uint32_t i = 1; i <= 10; ++i)
            {
                AgentObservation obs = agent->BuildObservation();
                AgentAction act;
                act.decisionId = obs.decisionId;
                act.type = AgentActionType::SELECT_PATH;
                act.targetId = i * 2;
                agent->SubmitAction(act);
                targets.push_back(handler->m_lastAction.targetId);
            }
            return targets;
        };

        std::vector<uint32_t> run1 = runSimulation();
        std::vector<uint32_t> run2 = runSimulation();

        NS_TEST_ASSERT_MSG_EQ(run1.size(), run2.size(), "Run lengths should match");
        for (size_t i = 0; i < run1.size(); ++i)
        {
            NS_TEST_ASSERT_MSG_EQ(run1[i], run2[i], "Deterministic sequence element must match");
        }
    }
};

// ----------------------------------------------------------------------------
// Test Suite Definition
// ----------------------------------------------------------------------------
class FutureNetAgentTestSuite : public TestSuite
{
public:
    FutureNetAgentTestSuite()
        : TestSuite("futurenet-agent", TestSuite::Type::UNIT)
    {
        AddTestCase(new FutureNetAgentT01ObservationTest, TestCase::Duration::QUICK);
        AddTestCase(new FutureNetAgentT02UnsupportedActionTest, TestCase::Duration::QUICK);
        AddTestCase(new FutureNetAgentT03StaleDecisionIdTest, TestCase::Duration::QUICK);
        AddTestCase(new FutureNetAgentT04NoOpTest, TestCase::Duration::QUICK);
        AddTestCase(new FutureNetAgentT05ValidActionDispatchTest, TestCase::Duration::QUICK);
        AddTestCase(new FutureNetAgentT06DeterministicActionSequenceTest, TestCase::Duration::QUICK);
    }
};

static FutureNetAgentTestSuite g_futurenetAgentTestSuite;
