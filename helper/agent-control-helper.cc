/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: AgentControlHelper implementation.
 */

#include "agent-control-helper.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3 {
namespace futurenet {

NS_LOG_COMPONENT_DEFINE("FutureNetAgentControlHelper");

void
AgentControlHelper::CsvTraceLogger::OnObservationEmitted(uint64_t decisionId, Time timestamp)
{
    currentDecisionId = decisionId;
}

void
AgentControlHelper::CsvTraceLogger::OnActionReceived(uint64_t decisionId, std::string actionType, uint32_t targetId)
{
    // Action received tracking
}

void
AgentControlHelper::CsvTraceLogger::OnActionAccepted(uint64_t decisionId, std::string actionType)
{
    if (file.is_open())
    {
        double nowS = Simulator::Now().GetSeconds();
        file << nowS << ","
             << decisionId << ","
             << actionType << ","
             << 0 << "," // target
             << 0 << "," // secondary target
             << 0.0 << "," // value
             << "true" << ","
             << "none" << ","
             << lastObsDelayMs << ","
             << lastObsLoss << ","
             << lastObsRateMbps << "\n";
        file.flush();
    }
}

void
AgentControlHelper::CsvTraceLogger::OnActionRejected(uint64_t decisionId, std::string reason)
{
    if (file.is_open())
    {
        double nowS = Simulator::Now().GetSeconds();
        file << nowS << ","
             << decisionId << ","
             << "REJECTED" << ","
             << 0 << ","
             << 0 << ","
             << 0.0 << ","
             << "false" << ","
             << "\"" << reason << "\"" << ","
             << lastObsDelayMs << ","
             << lastObsLoss << ","
             << lastObsRateMbps << "\n";
        file.flush();
    }
}

void
AgentControlHelper::CsvTraceLogger::OnActionExecuted(uint64_t decisionId, std::string actionType, std::string result)
{
    // Handled in accepted/rejected
}

void
AgentControlHelper::CsvTraceLogger::OnAgentTimeout(uint64_t decisionId)
{
    if (file.is_open())
    {
        double nowS = Simulator::Now().GetSeconds();
        file << nowS << ","
             << decisionId << ","
             << "TIMEOUT_NO_OP" << ","
             << 0 << ","
             << 0 << ","
             << 0.0 << ","
             << "false" << ","
             << "AgentTimeout" << ","
             << lastObsDelayMs << ","
             << lastObsLoss << ","
             << lastObsRateMbps << "\n";
        file.flush();
    }
}

AgentControlHelper::AgentControlHelper()
{
    NS_LOG_FUNCTION(this);
    m_factory.SetTypeId("ns3::futurenet::AgentInterface");
}

AgentControlHelper::~AgentControlHelper()
{
    NS_LOG_FUNCTION(this);
    if (m_csvLogger && m_csvLogger->file.is_open())
    {
        m_csvLogger->file.close();
    }
}

void
AgentControlHelper::SetInterfaceAttribute(const std::string& name, const AttributeValue& value)
{
    NS_LOG_FUNCTION(this << name);
    m_factory.Set(name, value);
}

void
AgentControlHelper::RegisterProvider(Ptr<ObservationProvider> provider)
{
    NS_LOG_FUNCTION(this << provider);
    if (m_interface)
    {
        m_interface->RegisterObservationProvider(provider);
    }
    else
    {
        m_pendingProviders.push_back(provider);
    }
}

void
AgentControlHelper::RegisterHandler(Ptr<ActionHandler> handler)
{
    NS_LOG_FUNCTION(this << handler);
    if (m_interface)
    {
        m_interface->RegisterActionHandler(handler);
    }
    else
    {
        m_pendingHandlers.push_back(handler);
    }
}

Ptr<AgentInterface>
AgentControlHelper::Install()
{
    NS_LOG_FUNCTION(this);
    m_interface = m_factory.Create<AgentInterface>();

    for (const auto& p : m_pendingProviders)
    {
        m_interface->RegisterObservationProvider(p);
    }
    m_pendingProviders.clear();

    for (const auto& h : m_pendingHandlers)
    {
        m_interface->RegisterActionHandler(h);
    }
    m_pendingHandlers.clear();

    return m_interface;
}

void
AgentControlHelper::EnableCsvTracing(const std::string& filename)
{
    NS_LOG_FUNCTION(this << filename);
    m_csvLogger = std::make_shared<CsvTraceLogger>();
    m_csvLogger->file.open(filename);

    if (m_csvLogger->file.is_open())
    {
        // Write CSV Header (SRS Section 9)
        m_csvLogger->file << "time_s,decision_id,action,target,secondary_target,value,accepted,reject_reason,observed_delay_ms,observed_loss,observed_rate_mbps\n";
        m_csvLogger->file.flush();
    }

    if (m_interface)
    {
        m_interface->TraceConnectWithoutContext("ObservationEmitted",
            MakeCallback(&CsvTraceLogger::OnObservationEmitted, m_csvLogger));
        m_interface->TraceConnectWithoutContext("ActionReceived",
            MakeCallback(&CsvTraceLogger::OnActionReceived, m_csvLogger));
        m_interface->TraceConnectWithoutContext("ActionAccepted",
            MakeCallback(&CsvTraceLogger::OnActionAccepted, m_csvLogger));
        m_interface->TraceConnectWithoutContext("ActionRejected",
            MakeCallback(&CsvTraceLogger::OnActionRejected, m_csvLogger));
        m_interface->TraceConnectWithoutContext("ActionExecuted",
            MakeCallback(&CsvTraceLogger::OnActionExecuted, m_csvLogger));
        m_interface->TraceConnectWithoutContext("AgentTimeout",
            MakeCallback(&CsvTraceLogger::OnAgentTimeout, m_csvLogger));
    }
}

Ptr<AgentInterface>
AgentControlHelper::GetInterface() const
{
    return m_interface;
}

} // namespace futurenet
} // namespace ns3
