/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: SocketIpcAdapter implementation.
 */

#include "socket-ipc-adapter.h"
#include "ns3/log.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <cmath>

namespace ns3 {
namespace futurenet {

NS_LOG_COMPONENT_DEFINE("FutureNetSocketIpcAdapter");

NS_OBJECT_ENSURE_REGISTERED(SocketIpcAdapter);

TypeId
SocketIpcAdapter::GetTypeId()
{
    static TypeId tid = TypeId("ns3::futurenet::SocketIpcAdapter")
                            .SetParent<IpcAdapter>()
                            .SetGroupName("FutureNet")
                            .AddConstructor<SocketIpcAdapter>()
                            .AddAttribute("Port",
                                          "TCP port to listen on for external Python agent",
                                          UintegerValue(12345),
                                          MakeUintegerAccessor(&SocketIpcAdapter::m_port),
                                          MakeUintegerChecker<uint16_t>())
                            .AddAttribute("Host",
                                          "Host address to bind",
                                          StringValue("127.0.0.1"),
                                          MakeStringAccessor(&SocketIpcAdapter::m_host),
                                          MakeStringChecker())
                            .AddAttribute("ConnectTimeout",
                                          "Timeout waiting for external agent to connect on first observation",
                                          TimeValue(Seconds(15.0)),
                                          MakeTimeAccessor(&SocketIpcAdapter::m_connectTimeout),
                                          MakeTimeChecker());
    return tid;
}

SocketIpcAdapter::SocketIpcAdapter()
{
    NS_LOG_FUNCTION(this);
}

SocketIpcAdapter::~SocketIpcAdapter()
{
    NS_LOG_FUNCTION(this);
    Stop();
}

bool
SocketIpcAdapter::Start()
{
    NS_LOG_FUNCTION(this << m_host << m_port);

    if (m_serverFd >= 0)
    {
        return true; // Already started
    }

    m_serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_serverFd < 0)
    {
        NS_LOG_ERROR("Failed to create socket: " << strerror(errno));
        return false;
    }

    int opt = 1;
    ::setsockopt(m_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    if (::inet_pton(AF_INET, m_host.c_str(), &addr.sin_addr) <= 0)
    {
        NS_LOG_ERROR("Invalid host address: " << m_host);
        ::close(m_serverFd);
        m_serverFd = -1;
        return false;
    }

    if (::bind(m_serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        NS_LOG_ERROR("Failed to bind socket on " << m_host << ":" << m_port << ": " << strerror(errno));
        ::close(m_serverFd);
        m_serverFd = -1;
        return false;
    }

    if (::listen(m_serverFd, 1) < 0)
    {
        NS_LOG_ERROR("Failed to listen on socket: " << strerror(errno));
        ::close(m_serverFd);
        m_serverFd = -1;
        return false;
    }

    // Set server socket non-blocking so accept won't hang indefinitely
    int flags = ::fcntl(m_serverFd, F_GETFL, 0);
    ::fcntl(m_serverFd, F_SETFL, flags | O_NONBLOCK);

    NS_LOG_INFO("SocketIpcAdapter listening on " << m_host << ":" << m_port);
    return true;
}

void
SocketIpcAdapter::Stop()
{
    NS_LOG_FUNCTION(this);
    if (m_clientFd >= 0)
    {
        ::close(m_clientFd);
        m_clientFd = -1;
    }
    if (m_serverFd >= 0)
    {
        ::close(m_serverFd);
        m_serverFd = -1;
    }
}

bool
SocketIpcAdapter::IsConnected() const
{
    return (m_clientFd >= 0);
}

bool
SocketIpcAdapter::AcceptClient(int timeoutMs)
{
    if (m_serverFd < 0)
    {
        return false;
    }

    pollfd pfd{};
    pfd.fd = m_serverFd;
    pfd.events = POLLIN;

    int ret = ::poll(&pfd, 1, timeoutMs);
    if (ret > 0 && (pfd.revents & POLLIN))
    {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        m_clientFd = ::accept(m_serverFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (m_clientFd >= 0)
        {
            int flag = 1;
            ::setsockopt(m_clientFd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
            NS_LOG_INFO("Accepted external agent client connection");
            return true;
        }
    }
    return false;
}

std::string
SocketIpcAdapter::SerializeObservation(const AgentObservation& obs)
{
    std::ostringstream ss;
    ss << "{\"decisionId\":" << obs.decisionId
       << ",\"timestampMs\":" << obs.timestamp.GetMilliSeconds()
       << ",\"nodes\":[";

    for (size_t i = 0; i < obs.nodes.size(); ++i)
    {
        if (i > 0) ss << ",";
        const auto& n = obs.nodes[i];
        ss << "{\"nodeId\":" << n.nodeId
           << ",\"oneWayDelayMs\":" << (std::isnan(n.oneWayDelayMs) ? -1.0 : n.oneWayDelayMs)
           << ",\"throughputMbps\":" << n.throughputMbps
           << ",\"packetLossRatio\":" << n.packetLossRatio
           << ",\"availableBandwidthMbps\":" << n.availableBandwidthMbps
           << ",\"queuePackets\":" << n.queuePackets
           << ",\"nodeLoad\":" << n.nodeLoad
           << ",\"energyJ\":" << n.energyJ
           << ",\"energyValid\":" << (n.energyValid ? "true" : "false")
           << "}";
    }
    ss << "],\"links\":[";

    for (size_t i = 0; i < obs.links.size(); ++i)
    {
        if (i > 0) ss << ",";
        const auto& l = obs.links[i];
        ss << "{\"sourceNodeId\":" << l.sourceNodeId
           << ",\"destinationNodeId\":" << l.destinationNodeId
           << ",\"available\":" << (l.available ? "true" : "false")
           << ",\"dataRateMbps\":" << l.dataRateMbps
           << ",\"propagationDelayMs\":" << l.propagationDelayMs
           << ",\"lossRatio\":" << l.lossRatio
           << "}";
    }
    ss << "]}\n";
    return ss.str();
}

bool
SocketIpcAdapter::SendObservation(const AgentObservation& obs)
{
    NS_LOG_FUNCTION(this << obs.decisionId);

    // If client is not connected, wait for external agent to connect
    if (m_clientFd < 0)
    {
        int waitMs = static_cast<int>(m_connectTimeout.GetMilliSeconds());
        if (!AcceptClient(waitMs))
        {
            NS_LOG_WARN("No client connected within " << waitMs << " ms to receive observation");
            return false;
        }
    }

    std::string payload = SerializeObservation(obs);
    ssize_t sent = ::send(m_clientFd, payload.c_str(), payload.size(), 0);
    if (sent < 0 || static_cast<size_t>(sent) != payload.size())
    {
        NS_LOG_WARN("Failed to send observation to client: " << strerror(errno));
        ::close(m_clientFd);
        m_clientFd = -1;
        return false;
    }

    return true;
}

static bool ExtractJsonString(const std::string& line, const std::string& key, std::string& outVal)
{
    std::string search = "\"" + key + "\":";
    size_t pos = line.find(search);
    if (pos == std::string::npos) return false;
    pos += search.size();
    while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\"')) pos++;
    size_t end = pos;
    while (end < line.size() && line[end] != '\"' && line[end] != ',' && line[end] != '}') end++;
    outVal = line.substr(pos, end - pos);
    return true;
}

static bool ExtractJsonNumber(const std::string& line, const std::string& key, double& outVal)
{
    std::string search = "\"" + key + "\":";
    size_t pos = line.find(search);
    if (pos == std::string::npos) return false;
    pos += search.size();
    while (pos < line.size() && line[pos] == ' ') pos++;
    try
    {
        outVal = std::stod(line.substr(pos));
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool
SocketIpcAdapter::DeserializeAction(const std::string& line, AgentAction& action)
{
    double decId = 0, targetId = 0, secTargetId = 0, val = 0;
    std::string typeStr;

    if (!ExtractJsonNumber(line, "decisionId", decId)) return false;
    action.decisionId = static_cast<uint64_t>(decId);

    if (ExtractJsonString(line, "type", typeStr))
    {
        action.type = AgentActionTypeFromString(typeStr);
    }
    else if (ExtractJsonNumber(line, "type", val))
    {
        action.type = static_cast<AgentActionType>(static_cast<uint8_t>(val));
    }
    else
    {
        action.type = AgentActionType::NO_OP;
    }

    if (ExtractJsonNumber(line, "targetId", targetId))
    {
        action.targetId = static_cast<uint32_t>(targetId);
    }
    if (ExtractJsonNumber(line, "secondaryTargetId", secTargetId))
    {
        action.secondaryTargetId = static_cast<uint32_t>(secTargetId);
    }
    if (ExtractJsonNumber(line, "value", val))
    {
        action.value = val;
    }
    else
    {
        action.value = 0.0;
    }

    return true;
}

bool
SocketIpcAdapter::ReceiveAction(AgentAction& action, Time timeout)
{
    NS_LOG_FUNCTION(this << timeout.GetMilliSeconds());

    if (m_clientFd < 0)
    {
        if (!AcceptClient(static_cast<int>(timeout.GetMilliSeconds())))
        {
            return false;
        }
    }

    pollfd pfd{};
    pfd.fd = m_clientFd;
    pfd.events = POLLIN;

    int timeoutMs = static_cast<int>(timeout.GetMilliSeconds());
    int ret = ::poll(&pfd, 1, timeoutMs);

    if (ret <= 0)
    {
        NS_LOG_DEBUG("Timeout or poll error waiting for action (" << ret << ")");
        return false;
    }

    if (pfd.revents & (POLLERR | POLLHUP))
    {
        NS_LOG_WARN("Socket hung up or error");
        ::close(m_clientFd);
        m_clientFd = -1;
        return false;
    }

    std::string line;

    // Read until newline
    while (true)
    {
        char ch;
        ssize_t n = ::recv(m_clientFd, &ch, 1, 0);
        if (n <= 0)
        {
            ::close(m_clientFd);
            m_clientFd = -1;
            return false;
        }
        if (ch == '\n') break;
        line += ch;
        if (line.size() > 4096) break; // sanity limit
    }

    return DeserializeAction(line, action);
}

} // namespace futurenet
} // namespace ns3
