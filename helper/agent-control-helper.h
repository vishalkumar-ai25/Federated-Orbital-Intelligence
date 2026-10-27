/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: AgentControlHelper interface.
 */

#ifndef FUTURENET_AGENT_CONTROL_HELPER_H
#define FUTURENET_AGENT_CONTROL_HELPER_H

#include "ns3/agent-interface.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include <fstream>
#include <memory>
#include <string>

namespace ns3 {
namespace futurenet {

/**
 * \ingroup futurenet-agent
 * \brief High-level helper for configuring and deploying AgentInterface within simulation scripts.
 * Defined in SRS Section 8.
 */
class AgentControlHelper
{
public:
    AgentControlHelper();
    ~AgentControlHelper();

    /**
     * \brief Set an attribute on the underlying AgentInterface.
     */
    void SetInterfaceAttribute(const std::string& name, const AttributeValue& value);

    /**
     * \brief Register an ObservationProvider.
     */
    void RegisterProvider(Ptr<ObservationProvider> provider);

    /**
     * \brief Register an ActionHandler.
     */
    void RegisterHandler(Ptr<ActionHandler> handler);

    /**
     * \brief Create and install the AgentInterface.
     * \return Ptr to configured AgentInterface.
     */
    Ptr<AgentInterface> Install();

    /**
     * \brief Enable machine-readable CSV output for actions and observations.
     * Generates CSV following SRS Section 9 schema.
     * \param filename Path to output CSV file.
     */
    void EnableCsvTracing(const std::string& filename);

    /**
     * \brief Get the installed AgentInterface instance.
     */
    Ptr<AgentInterface> GetInterface() const;

private:
    struct CsvTraceLogger
    {
        std::ofstream file;
        uint64_t currentDecisionId{0};
        double lastObsDelayMs{0.0};
        double lastObsLoss{0.0};
        double lastObsRateMbps{0.0};

        void OnObservationEmitted(uint64_t decisionId, Time timestamp);
        void OnActionReceived(uint64_t decisionId, std::string actionType, uint32_t targetId);
        void OnActionAccepted(uint64_t decisionId, std::string actionType);
        void OnActionRejected(uint64_t decisionId, std::string reason);
        void OnActionExecuted(uint64_t decisionId, std::string actionType, std::string result);
        void OnAgentTimeout(uint64_t decisionId);
    };

    ObjectFactory m_factory;
    Ptr<AgentInterface> m_interface;
    std::vector<Ptr<ObservationProvider>> m_pendingProviders;
    std::vector<Ptr<ActionHandler>> m_pendingHandlers;
    std::shared_ptr<CsvTraceLogger> m_csvLogger;
};

} // namespace futurenet
} // namespace ns3

#endif // FUTURENET_AGENT_CONTROL_HELPER_H
