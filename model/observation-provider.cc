/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: ObservationProvider implementation.
 */

#include "observation-provider.h"
#include "ns3/log.h"

namespace ns3 {
namespace futurenet {

NS_LOG_COMPONENT_DEFINE("FutureNetObservationProvider");

NS_OBJECT_ENSURE_REGISTERED(ObservationProvider);

TypeId
ObservationProvider::GetTypeId()
{
    static TypeId tid = TypeId("ns3::futurenet::ObservationProvider")
                            .SetParent<Object>()
                            .SetGroupName("FutureNet");
    return tid;
}

ObservationProvider::ObservationProvider()
{
    NS_LOG_FUNCTION(this);
}

ObservationProvider::~ObservationProvider()
{
    NS_LOG_FUNCTION(this);
}

} // namespace futurenet
} // namespace ns3
