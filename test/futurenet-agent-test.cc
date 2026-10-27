/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: Automated unit tests for ActionValidator and ActionExecutor (T02, T03, T04, T05).
 */

#include "ns3/action-executor.h"
#include "ns3/action-handler.h"
#include "ns3/action-validator.h"
#include "ns3/futurenet-agent-types.h"
#include "ns3/observation-provider.h"
#include "ns3/test.h"
#include <cmath>

using namespace ns3;
using namespace ns3::futurenet;

namespace {

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

// T02: Unsupported Action Rejected
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

        AgentAction unsupportedAction;
        unsupportedAction.decisionId = 5;
        unsupportedAction.type = AgentActionType::TRIGGER_REEVALUATION;
        unsupportedAction.targetId = 1;

        ValidationResult res = validator->Validate(unsupportedAction, obs, handlers);
        NS_TEST_ASSERT_MSG_EQ(res.valid, false, "Unsupported action must be rejected");
    }
};

// T03: Stale Decision ID Rejected
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

// T04: Numerical validation (NaN/Inf rejected, NO_OP accepted)
class FutureNetAgentT04ValidationSanityTest : public TestCase
{
public:
    FutureNetAgentT04ValidationSanityTest()
        : TestCase("T04: Numerical sanity and NO_OP validation")
    {
    }

    void DoRun() override
    {
        Ptr<ActionValidator> validator = CreateObject<ActionValidator>();
        Ptr<MockActionHandler> handler = CreateObject<MockActionHandler>();
        std::vector<Ptr<ActionHandler>> handlers = {handler};

        AgentObservation obs;
        obs.decisionId = 1;

        AgentAction nanAction;
        nanAction.decisionId = 1;
        nanAction.type = AgentActionType::SELECT_PATH;
        nanAction.value = std::nan("");

        ValidationResult nanRes = validator->Validate(nanAction, obs, handlers);
        NS_TEST_ASSERT_MSG_EQ(nanRes.valid, false, "NaN scalar value must be rejected");

        AgentAction noop;
        noop.decisionId = 1;
        noop.type = AgentActionType::NO_OP;

        ValidationResult noopRes = validator->Validate(noop, obs, handlers);
        NS_TEST_ASSERT_MSG_EQ(noopRes.valid, true, "NO_OP must always be validated successfully");
    }
};

// T05: Valid Action Reaches ActionHandler via ActionExecutor
class FutureNetAgentT05ValidActionDispatchTest : public TestCase
{
public:
    FutureNetAgentT05ValidActionDispatchTest()
        : TestCase("T05: Valid action reaches registered ActionHandler via ActionExecutor")
    {
    }

    void DoRun() override
    {
        Ptr<ActionExecutor> executor = CreateObject<ActionExecutor>();
        Ptr<MockActionHandler> handler = CreateObject<MockActionHandler>();
        std::vector<Ptr<ActionHandler>> handlers = {handler};

        AgentAction action;
        action.decisionId = 1;
        action.type = AgentActionType::SELECT_PATH;
        action.targetId = 42;
        action.secondaryTargetId = 7;
        action.value = 100.0;

        ActionResult res = executor->Execute(action, handlers);
        NS_TEST_ASSERT_MSG_EQ(res.success, true, "Valid action execution should succeed");
        NS_TEST_ASSERT_MSG_EQ(handler->m_execCount, 1, "Handler should have been executed once");
        NS_TEST_ASSERT_MSG_EQ(handler->m_lastAction.targetId, 42, "Target ID must match dispatched action");
    }
};

class FutureNetAgentTestSuite : public TestSuite
{
public:
    FutureNetAgentTestSuite()
        : TestSuite("futurenet-agent", TestSuite::Type::UNIT)
    {
        AddTestCase(new FutureNetAgentT02UnsupportedActionTest, TestCase::Duration::QUICK);
        AddTestCase(new FutureNetAgentT03StaleDecisionIdTest, TestCase::Duration::QUICK);
        AddTestCase(new FutureNetAgentT04ValidationSanityTest, TestCase::Duration::QUICK);
        AddTestCase(new FutureNetAgentT05ValidActionDispatchTest, TestCase::Duration::QUICK);
    }
};

static FutureNetAgentTestSuite g_futurenetAgentTestSuite;
