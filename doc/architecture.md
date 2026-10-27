# Technical Architecture & Software Design Report
## Module: futurenet-agent (M10)

---

## 1. Executive Summary

The `futurenet-agent` module provides an extensible, safe, and decoupled interface between ns-3 simulations and external Python AI controllers. Designed for future network scenarios (e.g. NTN, UAV relays, Edge offloading, Resilient routing), it acts as an intelligent supervisor capable of steering network state transitions without requiring intrusive changes to ns-3 core models.

---

## 2. Core Design Patterns

### 2.1 Mechanism-Policy Separation
Following Guidelines Section 1.3, this module isolates:
- **Networking Mechanism:** Running inside ns-3 (e.g. NetDevices, queue managers, routing protocols).
- **Control Policy:** Running externally in Python (e.g. rule-based logic, reinforcement learning policies, or LLMs).

Communication is handled through two pure abstract interfaces:
1. `ObservationProvider`: Exposes read-only telemetry (`NetworkState`, `LinkState`) to `AgentInterface`.
2. `ActionHandler`: Implements actuation logic for supported `AgentAction` commands.

```
       +---------------------------------------------+
       |             ns-3 Simulation Engine          |
       |                                             |
       |  +--------------------+                     |
       |  | Controlled Module  |                     |
       |  +---------+----------+                     |
       |            |                                |
       |     implements                              |
       |            v                                |
       |  +-------------------------+                |
       |  |   ObservationProvider   |                |
       |  +------------+------------+                |
       |               | AppendObservation()         |
       |               v                             |
       |  +-------------------------+                |
       |  |      AgentInterface     |                |
       |  +------------+------------+                |
       |               |                             |
       |         SendObservation() / ReceiveAction() |
       |               v                             |
       |  +-------------------------+                |
       |  |      IpcAdapter         |                |
       |  +------------+------------+                |
       +---------------|-----------------------------+
                       |  IPC boundary (TCP Loopback)
       +---------------|-----------------------------+
       |               v                             |
       |      Python Agent Process                   |
       |      (RuleBasedAgent / RL / LLM)            |
       |               |                             |
       +---------------|-----------------------------+
                       |  AgentAction JSON
       +---------------|-----------------------------+
       |               v                             |
       |  +-------------------------+                |
       |  |     ActionValidator     |                |
       |  +------------+------------+                |
       |               | validated action            |
       |               v                             |
       |  +-------------------------+                |
       |  |     ActionExecutor      |                |
       |  +------------+------------+                |
       |               | Execute()                   |
       |               v                             |
       |  +-------------------------+                |
       |  |      ActionHandler      |                |
       |  +------------+------------+                |
       |               |                             |
       |     mutates state                           |
       |               v                             |
       |  +-------------------------+                |
       |  |  ns-3 Routing / Devices |                |
       |  +-------------------------+                |
       +---------------------------------------------+
```

---

## 3. Class Responsibilities

| Class | Base Class | Responsibility |
|---|---|---|
| `AgentObservation` | Struct | Immutable snapshot representing telemetry across nodes and links. |
| `AgentAction` | Struct | Formatted control command containing action type, targets, and value. |
| `ObservationProvider` | `ns3::Object` | Pure virtual base class implemented by any provider module. |
| `ActionHandler` | `ns3::Object` | Pure virtual base class declaring `Supports()` and `Execute()`. |
| `ActionValidator` | `ns3::Object` | Verifies decision ID freshness, range bounds, and handler support. |
| `ActionExecutor` | `ns3::Object` | Safe fan-out dispatcher routing validated actions to supporting handlers. |
| `IpcAdapter` | `ns3::Object` | Transport abstraction for external agent communication. |
| `SocketIpcAdapter` | `IpcAdapter` | High-efficiency loopback TCP socket server transmitting line-delimited JSON. |
| `AgentInterface` | `ns3::Object` | Central orchestrator scheduling control steps and invoking traces. |
| `AgentControlHelper` | C++ class | Convenient user API for configuration and automatic CSV tracing. |

---

## 4. Safety & Fault Tolerance

The system enforces strict resilience requirements:
1. **Decision ID Freshness:** When `StrictActionValidation` is enabled, any action with a decision ID mismatch is rejected immediately with a logged reason.
2. **Unsupported Action Rejection:** If no registered handler supports the action, the validator rejects it safely without affecting simulation state.
3. **Timeout Fallback:** If the external agent fails to respond within `ActionTimeout`, the interface logs `AgentTimeout` and automatically executes `NO_OP`, preserving simulation stability.
4. **NaN/Inf Sanitization:** Scalar values are verified before execution to prevent floating-point poisoning.

---

## 5. Trace Sources & Observability

`AgentInterface` defines six standard `TracedCallback` sources:
- `ObservationEmitted(uint64_t decisionId, Time timestamp)`
- `ActionReceived(uint64_t decisionId, std::string actionType, uint32_t targetId)`
- `ActionAccepted(uint64_t decisionId, std::string actionType)`
- `ActionRejected(uint64_t decisionId, std::string reason)`
- `ActionExecuted(uint64_t decisionId, std::string actionType, std::string result)`
- `AgentTimeout(uint64_t decisionId)`

The `AgentControlHelper::EnableCsvTracing()` method hooks into all six callbacks and writes the standardized CSV format specified in SRS Section 9.
