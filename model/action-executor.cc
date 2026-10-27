/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: ActionExecutor implementation.
 */

#include "action-executor.h"
#include "ns3/log.h"

namespace ns3 {
namespace futurenet {

NS_LOG_COMPONENT_DEFINE("FutureNetActionExecutor");

NS_OBJECT_ENSURE_REGISTERED(ActionExecutor);

TypeId
ActionExecutor::GetTypeId()
{
    static TypeId tid = TypeId("ns3::futurenet::ActionExecutor")
                            .SetParent<Object>()
                            .SetGroupName("FutureNet")
                            .AddConstructor<ActionExecutor>();
    return tid;
}

ActionExecutor::ActionExecutor()
{
    NS_LOG_FUNCTION(this);
}

ActionExecutor::~ActionExecutor()
{
    NS_LOG_FUNCTION(this);
}

ActionResult
ActionExecutor::Execute(const AgentAction& action,
                        const std::vector<Ptr<ActionHandler>>& handlers)
{
    NS_LOG_FUNCTION(this << "action=" << AgentActionTypeToString(action.type));

    // NO_OP requires no execution on any handler and guarantees no state mutation (T04)
    if (action.type == AgentActionType::NO_OP)
    {
        return {true, "NO_OP executed successfully"};
    }

    // Dispatch to first handler that claims support
    for (const auto& handler : handlers)
    {
        if (handler && handler->Supports(action))
        {
            return handler->Execute(action);
        }
    }

    return {false, "No handler available to execute action"};
}

} // namespace futurenet
} // namespace ns3
