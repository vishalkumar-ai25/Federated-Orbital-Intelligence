/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: AgentInterface orchestrator.
 */

#ifndef FUTURENET_AGENT_INTERFACE_H
#define FUTURENET_AGENT_INTERFACE_H

#include "action-executor.h"
#include "action-handler.h"
#include "action-validator.h"
#include "futurenet-agent-types.h"
#include "ipc-adapter.h"
#include "observation-provider.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/traced-callback.h"
#include "ns3/uinteger.h"
#include <vector>

namespace ns3 {
namespace futurenet {

/**
 * \ingroup futurenet-agent
 * \brief Core orchestrator class coordinating observation gathering, IPC exchange with external agent,
 * validation, and safe action execution in ns-3.
 * Defined in SRS Section 4, 5, 8.
 */
class AgentInterface : public Object
{
public:
    static TypeId GetTypeId();

    AgentInterface();
    ~AgentInterface() override;

    /**
     * \brief Register a module observation provider.
     */
    void RegisterObservationProvider(Ptr<ObservationProvider> provider);

    /**
     * \brief Register a module action handler.
     */
    void RegisterActionHandler(Ptr<ActionHandler> handler);

    /**
     * \brief Set the IPC adapter used for communication with external Python process.
     */
    void SetIpcAdapter(Ptr<IpcAdapter> ipcAdapter);

    /**
     * \brief Get currently configured IPC adapter.
     */
    Ptr<IpcAdapter> GetIpcAdapter() const;

    /**
     * \brief Start the periodic decision-control loop.
     */
    void Start();

    /**
     * \brief Stop the control loop.
     */
    void Stop();

    /**
     * \brief Single control step: collect observation, communicate with agent, validate and execute action.
     */
    void Step();

    /**
     * \brief Assemble the current network observation across all registered ObservationProviders.
     */
    AgentObservation BuildObservation();

    /**
     * \brief Validate and apply an action to the appropriate ActionHandler.
     */
    ActionResult ApplyAction(const AgentAction& action);

    /**
     * \brief Direct execution method for testing/deterministic control.
     */
    ActionResult SubmitAction(const AgentAction& action);

    /**
     * \brief Get current decision ID.
     */
    uint64_t GetCurrentDecisionId() const;

private:
    void DoDispose() override;

    // Configuration Attributes (SRS Section 6)
    Time m_decisionInterval{Seconds(1.0)};              //!< DecisionInterval (default 1s)
    Time m_actionTimeout{MilliSeconds(500)};            //!< ActionTimeout (default 500ms)
    Time m_observationWindow{Seconds(1.0)};             //!< ObservationWindow (default 1s)
    uint32_t m_maxObservedEntities{100};                //!< MaxObservedEntities (default 100)
    bool m_normalizeObservations{false};                //!< NormalizeObservations (default false)
    bool m_enableReward{false};                         //!< EnableReward (default false)
    double m_rewardDelayWeight{0.4};                    //!< RewardDelayWeight (default 0.4)
    double m_rewardLossWeight{0.3};                     //!< RewardLossWeight (default 0.3)
    double m_rewardThroughputWeight{0.3};               //!< RewardThroughputWeight (default 0.3)
    uint32_t m_maxActionsPerDecision{1};                //!< MaxActionsPerDecision (default 1)
    bool m_strictActionValidation{true};                //!< StrictActionValidation (default true)
    uint8_t m_fallbackPolicyVal{0};                     //!< Fallback policy (0 = NO_OP, 1 = KEEP_CURRENT)

    // Internal State
    bool m_running{false};
    uint64_t m_decisionId{0};
    EventId m_stepEvent;
    AgentObservation m_lastObservation;

    // Components & Adapters
    std::vector<Ptr<ObservationProvider>> m_providers;
    std::vector<Ptr<ActionHandler>> m_handlers;
    Ptr<IpcAdapter> m_ipcAdapter;
    Ptr<ActionValidator> m_validator;
    Ptr<ActionExecutor> m_executor;

    // TraceSources (SRS Section 9)
    TracedCallback<uint64_t, Time> m_observationEmittedTrace;
    TracedCallback<uint64_t, std::string, uint32_t> m_actionReceivedTrace;
    TracedCallback<uint64_t, std::string> m_actionAcceptedTrace;
    TracedCallback<uint64_t, std::string> m_actionRejectedTrace;
    TracedCallback<uint64_t, std::string, std::string> m_actionExecutedTrace;
    TracedCallback<uint64_t> m_agentTimeoutTrace;
};

} // namespace futurenet
} // namespace ns3

#endif // FUTURENET_AGENT_INTERFACE_H
