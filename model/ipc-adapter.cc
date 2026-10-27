/*
 * Copyright (c) 2026 FutureNet Team
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * M10: Abstract IPC Adapter implementation.
 */

#include "ipc-adapter.h"
#include "ns3/log.h"

namespace ns3 {
namespace futurenet {

NS_LOG_COMPONENT_DEFINE("FutureNetIpcAdapter");

NS_OBJECT_ENSURE_REGISTERED(IpcAdapter);

TypeId
IpcAdapter::GetTypeId()
{
    static TypeId tid = TypeId("ns3::futurenet::IpcAdapter")
                            .SetParent<Object>()
                            .SetGroupName("FutureNet");
    return tid;
}

IpcAdapter::IpcAdapter()
{
    NS_LOG_FUNCTION(this);
}

IpcAdapter::~IpcAdapter()
{
    NS_LOG_FUNCTION(this);
}

} // namespace futurenet
} // namespace ns3
