/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: SocketIpcAdapter header.
 */

#ifndef FUTURENET_SOCKET_IPC_ADAPTER_H
#define FUTURENET_SOCKET_IPC_ADAPTER_H

#include "ipc-adapter.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include <string>

namespace ns3 {
namespace futurenet {

/**
 * \ingroup futurenet-agent
 * \brief TCP/IPC adapter for communicating with an external Python agent process.
 * Serializes observations to line-delimited JSON and parses incoming JSON actions.
 */
class SocketIpcAdapter : public IpcAdapter
{
public:
    static TypeId GetTypeId();

    SocketIpcAdapter();
    ~SocketIpcAdapter() override;

    bool Start() override;
    void Stop() override;
    bool SendObservation(const AgentObservation& obs) override;
    bool ReceiveAction(AgentAction& action, Time timeout) override;
    bool IsConnected() const override;

    /**
     * \brief Serialize AgentObservation to a JSON string.
     */
    static std::string SerializeObservation(const AgentObservation& obs);

    /**
     * \brief Deserialize a JSON string to an AgentAction.
     */
    static bool DeserializeAction(const std::string& line, AgentAction& action);

private:
    bool AcceptClient(int timeoutMs);

    uint16_t m_port{12345};                 //!< Port number to listen on
    std::string m_host{"127.0.0.1"};        //!< Host address to bind
    Time m_connectTimeout{Seconds(15.0)};   //!< Timeout waiting for agent to connect on first step
    int m_serverFd{-1};                     //!< Server listening socket descriptor
    int m_clientFd{-1};                     //!< Connected client socket descriptor
};

} // namespace futurenet
} // namespace ns3

#endif // FUTURENET_SOCKET_IPC_ADAPTER_H
