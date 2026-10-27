/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: ActionHandler implementation.
 */

#include "action-handler.h"
#include "ns3/log.h"

namespace ns3 {
namespace futurenet {

NS_LOG_COMPONENT_DEFINE("FutureNetActionHandler");

NS_OBJECT_ENSURE_REGISTERED(ActionHandler);

TypeId
ActionHandler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::futurenet::ActionHandler")
                            .SetParent<Object>()
                            .SetGroupName("FutureNet");
    return tid;
}

ActionHandler::ActionHandler()
{
    NS_LOG_FUNCTION(this);
}

ActionHandler::~ActionHandler()
{
    NS_LOG_FUNCTION(this);
}

} // namespace futurenet
} // namespace ns3
