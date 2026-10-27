/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: ActionExecutor interface.
 */

#ifndef FUTURENET_ACTION_EXECUTOR_H
#define FUTURENET_ACTION_EXECUTOR_H

#include "action-handler.h"
#include "futurenet-agent-types.h"
#include "ns3/object.h"
#include <vector>

namespace ns3 {
namespace futurenet {

/**
 * \ingroup futurenet-agent
 * \brief Dispatches validated actions to the appropriate registered ActionHandler.
 * Defined in SRS Section 4.
 */
class ActionExecutor : public Object
{
public:
    static TypeId GetTypeId();

    ActionExecutor();
    ~ActionExecutor() override;

    /**
     * \brief Execute an action across the collection of registered handlers.
     * \param action The validated action to execute.
     * \param handlers The list of registered ActionHandlers.
     * \return ActionResult indicating whether execution succeeded.
     */
    ActionResult Execute(const AgentAction& action,
                         const std::vector<Ptr<ActionHandler>>& handlers);
};

} // namespace futurenet
} // namespace ns3

#endif // FUTURENET_ACTION_EXECUTOR_H
