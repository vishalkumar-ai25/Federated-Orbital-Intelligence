/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: ActionHandler abstract interface.
 */

#ifndef FUTURENET_ACTION_HANDLER_H
#define FUTURENET_ACTION_HANDLER_H

#include "futurenet-agent-types.h"
#include "ns3/object.h"

namespace ns3 {
namespace futurenet {

/**
 * \ingroup futurenet-agent
 * \brief Abstract interface implemented by any FutureNet module or adapter
 * that can execute high-level control actions issued by the agent.
 * Defined in SRS Section 8.
 */
class ActionHandler : public Object
{
public:
    static TypeId GetTypeId();

    ActionHandler();
    ~ActionHandler() override;

    /**
     * \brief Query whether this handler supports executing the specified action.
     * \param action The proposed action.
     * \return true if the handler can execute it, false otherwise.
     */
    virtual bool Supports(const AgentAction& action) const = 0;

    /**
     * \brief Execute the action within the underlying network module.
     * \param action The validated action to execute.
     * \return ActionResult indicating success/failure and status message.
     */
    virtual ActionResult Execute(const AgentAction& action) = 0;
};

} // namespace futurenet
} // namespace ns3

#endif // FUTURENET_ACTION_HANDLER_H
