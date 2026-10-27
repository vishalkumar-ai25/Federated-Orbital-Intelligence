/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: AgentInterface implementation.
 */

#include "agent-interface.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {
namespace futurenet {

NS_LOG_COMPONENT_DEFINE("FutureNetAgentInterface");

NS_OBJECT_ENSURE_REGISTERED(AgentInterface);

TypeId
AgentInterface::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::futurenet::AgentInterface")
            .SetParent<Object>()
            .SetGroupName("FutureNet")
            .AddConstructor<AgentInterface>()
            .AddAttribute("DecisionInterval",
                          "Control evaluation period",
                          TimeValue(Seconds(1.0)),
                          MakeTimeAccessor(&AgentInterface::m_decisionInterval),
                          MakeTimeChecker())
            .AddAttribute("ActionTimeout",
                          "Maximum wait time for external agent action",
                          TimeValue(MilliSeconds(500)),
                          MakeTimeAccessor(&AgentInterface::m_actionTimeout),
                          MakeTimeChecker())
            .AddAttribute("ObservationWindow",
                          "Window duration for metric aggregation",
                          TimeValue(Seconds(1.0)),
                          MakeTimeAccessor(&AgentInterface::m_observationWindow),
                          MakeTimeChecker())
            .AddAttribute("MaxObservedEntities",
                          "Maximum number of entities included in observation",
                          UintegerValue(100),
                          MakeUintegerAccessor(&AgentInterface::m_maxObservedEntities),
                          MakeUintegerChecker<uint32_t>(1, 10000))
            .AddAttribute("NormalizeObservations",
                          "Whether to normalize observation metrics to [0,1]",
                          BooleanValue(false),
                          MakeBooleanAccessor(&AgentInterface::m_normalizeObservations),
                          MakeBooleanChecker())
            .AddAttribute("EnableReward",
                          "Compute and include RL scalar reward in observation",
                          BooleanValue(false),
                          MakeBooleanAccessor(&AgentInterface::m_enableReward),
                          MakeBooleanChecker())
            .AddAttribute("RewardDelayWeight",
                          "Delay penalty weight for reward calculation",
                          DoubleValue(0.4),
                          MakeDoubleAccessor(&AgentInterface::m_rewardDelayWeight),
                          MakeDoubleChecker<double>(0.0, 1.0))
            .AddAttribute("RewardLossWeight",
                          "Loss penalty weight for reward calculation",
                          DoubleValue(0.3),
                          MakeDoubleAccessor(&AgentInterface::m_rewardLossWeight),
                          MakeDoubleChecker<double>(0.0, 1.0))
            .AddAttribute("RewardThroughputWeight",
                          "Throughput reward weight",
                          DoubleValue(0.3),
                          MakeDoubleAccessor(&AgentInterface::m_rewardThroughputWeight),
                          MakeDoubleChecker<double>(0.0, 1.0))
            .AddAttribute("MaxActionsPerDecision",
                          "Maximum actions processed in one decision cycle",
                          UintegerValue(1),
                          MakeUintegerAccessor(&AgentInterface::m_maxActionsPerDecision),
                          MakeUintegerChecker<uint32_t>(1, 100))
            .AddAttribute("StrictActionValidation",
                          "Strictly reject stale or out-of-order decision IDs",
                          BooleanValue(true),
                          MakeBooleanAccessor(&AgentInterface::m_strictActionValidation),
                          MakeBooleanChecker())
            .AddAttribute("FallbackAction",
                          "Fallback policy on timeout/rejection (0=NO_OP, 1=KEEP_CURRENT)",
                          UintegerValue(0),
                          MakeUintegerAccessor(&AgentInterface::m_fallbackPolicyVal),
                          MakeUintegerChecker<uint8_t>(0, 1))
            .AddTraceSource("ObservationEmitted",
                            "Fired when an observation snapshot is captured and emitted",
                            MakeTraceSourceAccessor(&AgentInterface::m_observationEmittedTrace),
                            "ns3::futurenet::AgentInterface::ObservationEmittedCallback")
            .AddTraceSource("ActionReceived",
                            "Fired when an action is received from the agent",
                            MakeTraceSourceAccessor(&AgentInterface::m_actionReceivedTrace),
                            "ns3::futurenet::AgentInterface::ActionReceivedCallback")
            .AddTraceSource("ActionAccepted",
                            "Fired when an action passes validation",
                            MakeTraceSourceAccessor(&AgentInterface::m_actionAcceptedTrace),
                            "ns3::futurenet::AgentInterface::ActionAcceptedCallback")
            .AddTraceSource("ActionRejected",
                            "Fired when an action is rejected by validator",
                            MakeTraceSourceAccessor(&AgentInterface::m_actionRejectedTrace),
                            "ns3::futurenet::AgentInterface::ActionRejectedCallback")
            .AddTraceSource("ActionExecuted",
                            "Fired when an action has finished execution by an ActionHandler",
                            MakeTraceSourceAccessor(&AgentInterface::m_actionExecutedTrace),
                            "ns3::futurenet::AgentInterface::ActionExecutedCallback")
            .AddTraceSource("AgentTimeout",
                            "Fired when external agent fails to return an action within ActionTimeout",
                            MakeTraceSourceAccessor(&AgentInterface::m_agentTimeoutTrace),
                            "ns3::futurenet::AgentInterface::AgentTimeoutCallback");
    return tid;
}

AgentInterface::AgentInterface()
{
    NS_LOG_FUNCTION(this);
    m_validator = CreateObject<ActionValidator>();
    m_executor = CreateObject<ActionExecutor>();
}

AgentInterface::~AgentInterface()
{
    NS_LOG_FUNCTION(this);
}

void
AgentInterface::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Stop();
    m_providers.clear();
    m_handlers.clear();
    m_ipcAdapter = nullptr;
    m_validator = nullptr;
    m_executor = nullptr;
    Object::DoDispose();
}

void
AgentInterface::RegisterObservationProvider(Ptr<ObservationProvider> provider)
{
    NS_LOG_FUNCTION(this << provider);
    if (provider)
    {
        m_providers.push_back(provider);
    }
}

void
AgentInterface::RegisterActionHandler(Ptr<ActionHandler> handler)
{
    NS_LOG_FUNCTION(this << handler);
    if (handler)
    {
        m_handlers.push_back(handler);
    }
}

void
AgentInterface::SetIpcAdapter(Ptr<IpcAdapter> ipcAdapter)
{
    NS_LOG_FUNCTION(this << ipcAdapter);
    m_ipcAdapter = ipcAdapter;
}

Ptr<IpcAdapter>
AgentInterface::GetIpcAdapter() const
{
    return m_ipcAdapter;
}

void
AgentInterface::Start()
{
    NS_LOG_FUNCTION(this);
    if (m_running)
    {
        return;
    }
    m_running = true;

    if (m_ipcAdapter)
    {
        m_ipcAdapter->Start();
    }

    if (m_validator)
    {
        m_validator->SetStrictValidation(m_strictActionValidation);
    }

    // Schedule initial decision step
    m_stepEvent = Simulator::Schedule(m_decisionInterval, &AgentInterface::Step, this);
}

void
AgentInterface::Stop()
{
    NS_LOG_FUNCTION(this);
    if (!m_running)
    {
        return;
    }
    m_running = false;

    if (m_stepEvent.IsPending())
    {
        m_stepEvent.Cancel();
    }

    if (m_ipcAdapter)
    {
        m_ipcAdapter->Stop();
    }
}

uint64_t
AgentInterface::GetCurrentDecisionId() const
{
    return m_decisionId;
}

AgentObservation
AgentInterface::BuildObservation()
{
    NS_LOG_FUNCTION(this);
    AgentObservation obs;
    obs.decisionId = ++m_decisionId;
    obs.timestamp = Simulator::Now();

    for (const auto& provider : m_providers)
    {
        if (provider)
        {
            provider->AppendObservation(obs);
        }
    }

    // Enforce MaxObservedEntities cap if configured
    if (obs.nodes.size() > m_maxObservedEntities)
    {
        obs.nodes.resize(m_maxObservedEntities);
    }
    if (obs.links.size() > m_maxObservedEntities)
    {
        obs.links.resize(m_maxObservedEntities);
    }

    m_lastObservation = obs;
    return obs;
}

ActionResult
AgentInterface::ApplyAction(const AgentAction& action)
{
    NS_LOG_FUNCTION(this << action.decisionId << AgentActionTypeToString(action.type));

    // 1. Validate action (M10-FR-05)
    ValidationResult valResult = m_validator->Validate(action, m_lastObservation, m_handlers);

    if (!valResult.valid)
    {
        m_actionRejectedTrace(action.decisionId, valResult.reason);

        // Fallback policy (M10-FR-06): execute safe NO_OP
        AgentAction fallbackAction;
        fallbackAction.decisionId = m_lastObservation.decisionId;
        fallbackAction.type = AgentActionType::NO_OP;
        ActionResult fbResult = m_executor->Execute(fallbackAction, m_handlers);
        m_actionExecutedTrace(fallbackAction.decisionId, "NO_OP", fbResult.message);
        return {false, "Validation failed: " + valResult.reason};
    }

    // 2. Action accepted
    m_actionAcceptedTrace(action.decisionId, AgentActionTypeToString(action.type));

    // 3. Dispatch to executor
    ActionResult execResult = m_executor->Execute(action, m_handlers);
    m_actionExecutedTrace(action.decisionId,
                          AgentActionTypeToString(action.type),
                          execResult.success ? "SUCCESS" : "FAILED");

    return execResult;
}

ActionResult
AgentInterface::SubmitAction(const AgentAction& action)
{
    m_actionReceivedTrace(action.decisionId, AgentActionTypeToString(action.type), action.targetId);
    return ApplyAction(action);
}

void
AgentInterface::Step()
{
    NS_LOG_FUNCTION(this);
    if (!m_running)
    {
        return;
    }

    // 1. Build and emit observation snapshot (M10-FR-01, M10-FR-02)
    AgentObservation obs = BuildObservation();
    m_observationEmittedTrace(obs.decisionId, obs.timestamp);

    // 2. Transmit observation and await action via IPC adapter if connected
    if (m_ipcAdapter)
    {
        bool sent = m_ipcAdapter->SendObservation(obs);
        AgentAction action;
        bool received = false;

        if (sent)
        {
            received = m_ipcAdapter->ReceiveAction(action, m_actionTimeout);
        }

        if (received)
        {
            m_actionReceivedTrace(action.decisionId,
                                  AgentActionTypeToString(action.type),
                                  action.targetId);
            ApplyAction(action);
        }
        else
        {
            // Timeout / disconnect fallback (M10-FR-06, AC-04)
            m_agentTimeoutTrace(obs.decisionId);
            AgentAction fallbackAction;
            fallbackAction.decisionId = obs.decisionId;
            fallbackAction.type = AgentActionType::NO_OP;
            ApplyAction(fallbackAction);
        }
    }

    // 3. Schedule next periodic decision step
    if (m_running)
    {
        m_stepEvent = Simulator::Schedule(m_decisionInterval, &AgentInterface::Step, this);
    }
}

} // namespace futurenet
} // namespace ns3
