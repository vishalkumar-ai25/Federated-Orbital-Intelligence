/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: ObservationProvider abstract interface.
 */

#ifndef FUTURENET_OBSERVATION_PROVIDER_H
#define FUTURENET_OBSERVATION_PROVIDER_H

#include "futurenet-agent-types.h"
#include "ns3/object.h"

namespace ns3 {
namespace futurenet {

/**
 * \ingroup futurenet-agent
 * \brief Abstract interface implemented by any FutureNet module or adapter
 * that supplies network state observations to the AgentInterface.
 * Defined in SRS Section 8.
 */
class ObservationProvider : public Object
{
public:
    static TypeId GetTypeId();

    ObservationProvider();
    ~ObservationProvider() override;

    /**
     * \brief Append observation metrics for this provider's domain into the shared observation object.
     * \param obs Reference to the current AgentObservation being assembled.
     */
    virtual void AppendObservation(AgentObservation& obs) const = 0;
};

} // namespace futurenet
} // namespace ns3

#endif // FUTURENET_OBSERVATION_PROVIDER_H
