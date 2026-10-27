/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: ActionValidator implementation.
 */

#include "action-validator.h"
#include "ns3/log.h"
#include <cmath>

namespace ns3 {
namespace futurenet {

NS_LOG_COMPONENT_DEFINE("FutureNetActionValidator");

NS_OBJECT_ENSURE_REGISTERED(ActionValidator);

TypeId
ActionValidator::GetTypeId()
{
    static TypeId tid = TypeId("ns3::futurenet::ActionValidator")
                            .SetParent<Object>()
                            .SetGroupName("FutureNet")
                            .AddConstructor<ActionValidator>()
                            .AddAttribute("StrictActionValidation",
                                          "Enforce exact matching of decisionId between action and observation",
                                          BooleanValue(true),
                                          MakeBooleanAccessor(&ActionValidator::m_strictValidation),
                                          MakeBooleanChecker());
    return tid;
}

ActionValidator::ActionValidator()
{
    NS_LOG_FUNCTION(this);
}

ActionValidator::~ActionValidator()
{
    NS_LOG_FUNCTION(this);
}

void
ActionValidator::SetStrictValidation(bool strict)
{
    NS_LOG_FUNCTION(this << strict);
    m_strictValidation = strict;
}

bool
ActionValidator::GetStrictValidation() const
{
    return m_strictValidation;
}

ValidationResult
ActionValidator::Validate(const AgentAction& action,
                          const AgentObservation& currentObs,
                          const std::vector<Ptr<ActionHandler>>& handlers) const
{
    NS_LOG_FUNCTION(this << "decisionId=" << action.decisionId
                         << " type=" << AgentActionTypeToString(action.type));

    // 1. Check for NO_OP action: NO_OP is universally valid and safe
    if (action.type == AgentActionType::NO_OP)
    {
        return ValidationResult::Ok();
    }

    // 2. Strict decision-ID check: reject stale/mismatched actions (T03)
    if (m_strictValidation && (action.decisionId != currentObs.decisionId))
    {
        std::string reason = "Stale or mismatched decision ID (expected=" +
                             std::to_string(currentObs.decisionId) +
                             ", received=" + std::to_string(action.decisionId) + ")";
        NS_LOG_WARN("Validation rejected: " << reason);
        return ValidationResult::Reject(reason);
    }

    // 3. Numerical sanity check on scalar value
    if (std::isnan(action.value) || std::isinf(action.value))
    {
        std::string reason = "Malformed scalar value (NaN or Inf)";
        NS_LOG_WARN("Validation rejected: " << reason);
        return ValidationResult::Reject(reason);
    }

    // 4. Verify that at least one registered handler supports this action (T02)
    bool supported = false;
    for (const auto& handler : handlers)
    {
        if (handler && handler->Supports(action))
        {
            supported = true;
            break;
        }
    }

    if (!supported)
    {
        std::string reason = "Unsupported action type: " +
                             AgentActionTypeToString(action.type) +
                             " (no registered ActionHandler supports it)";
        NS_LOG_WARN("Validation rejected: " << reason);
        return ValidationResult::Reject(reason);
    }

    return ValidationResult::Ok();
}

} // namespace futurenet
} // namespace ns3
