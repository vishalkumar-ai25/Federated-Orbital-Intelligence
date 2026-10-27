# Configuration Parameter Reference Table
## Module: futurenet-agent

| Parameter Name | Target Class | C++ Type | ns-3 Attribute Name | Default Value | Project Range | Unit | Description |
|---|---|---|---|---|---|---|---|
| Decision Interval | `AgentInterface` | `Time` | `DecisionInterval` | `Seconds(1.0)` | 10 ms – 60 s | s | Period at which observation snapshots are built and emitted to the agent. |
| Action Timeout | `AgentInterface` | `Time` | `ActionTimeout` | `MilliSeconds(500)` | 10 ms – 60 s | ms | Maximum wait time for an action response before triggering fallback. |
| Fallback Policy | `AgentInterface` | `uint8_t` | `FallbackAction` | `0` (NO_OP) | 0 (NO_OP) / 1 (KEEP_CURRENT) | — | Action taken upon timeout, rejection, or disconnect. |
| Observation Window | `AgentInterface` | `Time` | `ObservationWindow` | `Seconds(1.0)` | 100 ms – 60 s | s | Window duration for aggregating network metrics. |
| Max Observed Entities | `AgentInterface` | `uint32_t` | `MaxObservedEntities` | `100` | 1 – 10,000 | count | Maximum number of node and link entities in an observation snapshot. |
| Normalize Observations | `AgentInterface` | `bool` | `NormalizeObservations` | `false` | `true`, `false` | bool | Whether observation values are normalized to the [0, 1] range. |
| Enable Reward | `AgentInterface` | `bool` | `EnableReward` | `false` | `true`, `false` | bool | Whether to compute scalar reinforcement learning reward. |
| Reward Delay Weight | `AgentInterface` | `double` | `RewardDelayWeight` | `0.4` | 0.0 – 1.0 | — | Weight for delay penalty in RL reward computation. |
| Reward Loss Weight | `AgentInterface` | `double` | `RewardLossWeight` | `0.3` | 0.0 – 1.0 | — | Weight for packet loss penalty in RL reward computation. |
| Reward Throughput Weight | `AgentInterface` | `double` | `RewardThroughputWeight` | `0.3` | 0.0 – 1.0 | — | Weight for throughput reward in RL reward computation. |
| Max Actions Per Decision | `AgentInterface` | `uint32_t` | `MaxActionsPerDecision` | `1` | 1 – 100 | count | Maximum number of control actions dispatched per decision interval. |
| Strict Action Validation | `ActionValidator` / `AgentInterface` | `bool` | `StrictActionValidation` | `true` | `true`, `false` | bool | Enforce exact matching of decision IDs; rejects stale or out-of-order actions. |
| Port | `SocketIpcAdapter` | `uint16_t` | `Port` | `12345` | 1024 – 65535 | — | Local TCP port for Python agent communication. |
| Host | `SocketIpcAdapter` | `std::string` | `Host` | `"127.0.0.1"` | Valid IPv4 / hostname | — | IP address to bind the IPC server. |
| Connect Timeout | `SocketIpcAdapter` | `Time` | `ConnectTimeout` | `Seconds(15.0)` | 1.0 s – 60.0 s | s | Maximum time to wait for external agent client connection during initialization. |
