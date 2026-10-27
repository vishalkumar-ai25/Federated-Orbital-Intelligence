/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: Abstract IPC Adapter interface.
 */

#ifndef FUTURENET_IPC_ADAPTER_H
#define FUTURENET_IPC_ADAPTER_H

#include "futurenet-agent-types.h"
#include "ns3/nstime.h"
#include "ns3/object.h"

namespace ns3 {
namespace futurenet {

/**
 * \ingroup futurenet-agent
 * \brief Abstract interface for communication between C++ AgentInterface and external Python agent.
 * Defined in SRS Section 3 and Section 8.
 */
class IpcAdapter : public Object
{
public:
    static TypeId GetTypeId();

    IpcAdapter();
    ~IpcAdapter() override;

    /**
     * \brief Initialize and start the IPC channel (e.g. open socket, bind, listen).
     * \return true if started successfully, false otherwise.
     */
    virtual bool Start() = 0;

    /**
     * \brief Shut down the IPC channel and release resources.
     */
    virtual void Stop() = 0;

    /**
     * \brief Transmit an observation snapshot to the external agent.
     * \param obs The observation snapshot to send.
     * \return true if successfully delivered or queued, false on error.
     */
    virtual bool SendObservation(const AgentObservation& obs) = 0;

    /**
     * \brief Attempt to receive an action decision from the external agent within a timeout.
     * \param action Output reference populated with received action.
     * \param timeout Maximum duration to wait before timing out.
     * \return true if an action was received, false on timeout or disconnect.
     */
    virtual bool ReceiveAction(AgentAction& action, Time timeout) = 0;

    /**
     * \brief Check whether an external agent client is currently connected.
     * \return true if client connected.
     */
    virtual bool IsConnected() const = 0;
};

} // namespace futurenet
} // namespace ns3

#endif // FUTURENET_IPC_ADAPTER_H
