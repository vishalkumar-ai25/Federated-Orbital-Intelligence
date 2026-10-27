/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: Agentic Network Control and External AI Integration
 * Common observation and action structures.
 */

#ifndef FUTURENET_AGENT_TYPES_H
#define FUTURENET_AGENT_TYPES_H

#include "ns3/nstime.h"
#include <cstdint>
#include <string>
#include <vector>
#include <cmath>

namespace ns3 {
namespace futurenet {

/**
 * \ingroup futurenet-agent
 * \brief Common network state structure as defined in Guidelines Section 4.1.
 */
struct NetworkState
{
    Time timestamp{Seconds(0)};
    uint32_t nodeId{0};
    double oneWayDelayMs{std::nan("")};          //!< One-way delay in ms (NaN if unavailable)
    double throughputMbps{0.0};                  //!< Measured or estimated throughput in Mbps
    double packetLossRatio{0.0};                 //!< Loss ratio [0, 1]
    double availableBandwidthMbps{0.0};          //!< Estimated available bandwidth in Mbps (>= 0)
    uint32_t queuePackets{0};                    //!< Current queue length in packets
    double nodeLoad{0.0};                        //!< Normalized node load [0, 1]
    double energyJ{0.0};                         //!< Energy in Joules (valid only when energyValid=true)
    bool energyValid{false};                     //!< Whether energyJ contains a valid reading
};

/**
 * \ingroup futurenet-agent
 * \brief Common link state structure as defined in Guidelines Section 4.2.
 */
struct LinkState
{
    uint32_t sourceNodeId{0};
    uint32_t destinationNodeId{0};
    bool available{true};                        //!< Whether the link is currently up/available
    double dataRateMbps{0.0};                    //!< Link data rate in Mbps
    double propagationDelayMs{0.0};              //!< One-way propagation delay in ms
    double lossRatio{0.0};                       //!< Packet loss ratio [0, 1]
};

/**
 * \ingroup futurenet-agent
 * \brief Comprehensive observation snapshot presented to external agent.
 * Defined in SRS Section 7.
 */
struct AgentObservation
{
    uint64_t decisionId{0};                      //!< Monotonically increasing decision identifier
    Time timestamp{Seconds(0)};                  //!< Simulation time when observation was captured
    std::vector<NetworkState> nodes;             //!< State of observed nodes
    std::vector<LinkState> links;                //!< State of observed links
};

/**
 * \ingroup futurenet-agent
 * \brief Supported high-level network control action types.
 * Defined in SRS Section 7.
 */
enum class AgentActionType : uint8_t
{
    NO_OP = 0,                                   //!< No operation; keep current state
    SELECT_PATH = 1,                             //!< Switch route/path to specified path/node target
    SELECT_ACCESS = 2,                           //!< Switch radio/access point
    SELECT_RELAY = 3,                            //!< Switch relay node
    SELECT_EDGE = 4,                             //!< Select edge server target
    SET_RATE_OR_WEIGHT = 5,                      //!< Adjust traffic rate, scheduling weight, or bandwidth
    TRIGGER_REEVALUATION = 6                     //!< Force re-evaluation of routing or association
};

/**
 * \ingroup futurenet-agent
 * \brief High-level network action issued by external agent.
 * Defined in SRS Section 7.
 */
struct AgentAction
{
    uint64_t decisionId{0};                      //!< Decision ID matching observation
    AgentActionType type{AgentActionType::NO_OP}; //!< Action type
    uint32_t targetId{0};                        //!< Primary target ID (nodeId, pathId, interfaceId)
    uint32_t secondaryTargetId{0};               //!< Secondary target ID (destinationId, altPathId)
    double value{0.0};                           //!< Scalar value (rate in Mbps, weight, threshold)
};

/**
 * \ingroup futurenet-agent
 * \brief Fallback action policy when agent times out or action is invalid.
 */
enum class FallbackPolicy : uint8_t
{
    FALLBACK_NO_OP = 0,
    FALLBACK_KEEP_CURRENT = 1
};

/**
 * \ingroup futurenet-agent
 * \brief Result of an action execution by an ActionHandler.
 */
struct ActionResult
{
    bool success{false};
    std::string message;
};

/**
 * \ingroup futurenet-agent
 * \brief Result of action validation by ActionValidator.
 */
struct ValidationResult
{
    bool valid{false};
    std::string reason;

    static ValidationResult Ok()
    {
        return {true, "OK"};
    }

    static ValidationResult Reject(const std::string& msg)
    {
        return {false, msg};
    }
};

/**
 * \brief Helper function to convert AgentActionType to string.
 */
inline std::string AgentActionTypeToString(AgentActionType type)
{
    switch (type)
    {
        case AgentActionType::NO_OP: return "NO_OP";
        case AgentActionType::SELECT_PATH: return "SELECT_PATH";
        case AgentActionType::SELECT_ACCESS: return "SELECT_ACCESS";
        case AgentActionType::SELECT_RELAY: return "SELECT_RELAY";
        case AgentActionType::SELECT_EDGE: return "SELECT_EDGE";
        case AgentActionType::SET_RATE_OR_WEIGHT: return "SET_RATE_OR_WEIGHT";
        case AgentActionType::TRIGGER_REEVALUATION: return "TRIGGER_REEVALUATION";
        default: return "UNKNOWN";
    }
}

/**
 * \brief Helper function to parse AgentActionType from string.
 */
inline AgentActionType AgentActionTypeFromString(const std::string& str)
{
    if (str == "SELECT_PATH") return AgentActionType::SELECT_PATH;
    if (str == "SELECT_ACCESS") return AgentActionType::SELECT_ACCESS;
    if (str == "SELECT_RELAY") return AgentActionType::SELECT_RELAY;
    if (str == "SELECT_EDGE") return AgentActionType::SELECT_EDGE;
    if (str == "SET_RATE_OR_WEIGHT") return AgentActionType::SET_RATE_OR_WEIGHT;
    if (str == "TRIGGER_REEVALUATION") return AgentActionType::TRIGGER_REEVALUATION;
    return AgentActionType::NO_OP;
}

} // namespace futurenet
} // namespace ns3

#endif // FUTURENET_AGENT_TYPES_H
