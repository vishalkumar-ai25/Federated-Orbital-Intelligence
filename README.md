# futurenet-agent: Agentic Network Control and External AI Integration

**Module Name:** `futurenet-agent`  
**Target Environment:** ns-allinone-3.46.1 / ns-3.46.1  
**Project:** M10 - Future Network Extensions for ns-3.46.1  
**Version:** 1.0  
**License:** BSD-3-Clause

---

## 1. Purpose

`futurenet-agent` provides a generic, safe, observation-action control interface through which an external Python process observes an ns-3 network and issues high-level control decisions at configurable time intervals.

The module cleanly decouples the **ns-3 simulation mechanisms** from the **external decision policy**. The default implementation provides a deterministic rule-based controller (satisfying all Mandatory requirements), and the interface is designed to support future Reinforcement Learning (Gymnasium/PPO/DQN) and LLM-based agent controllers without modifying the underlying ns-3 networking code.

---

## 2. Architecture & Design Principles

```
  ns-3 Controlled Modules (Routing / Edge / Access / Handover)
                     |
         ObservationProvider Interface
                     |
                     v
             AgentInterface (Orchestrator)
                     |
         SocketIpcAdapter (TCP Socket, Line-Delimited JSON)
                     |
           ==================== Network Boundary (IPC) ====================
                     |
             External Python Agent Process (RuleBasedAgent)
                     |
           ================================================================
                     |
                     v
             ActionValidator (Integrity, Range, Stale-ID, Support Check)
                     |
             ActionExecutor (Safe Fan-Out)
                     |
           ActionHandler Interface
                     |
                     v
       ns-3 Network State Mutation (e.g., Ipv4StaticRouting update)
```

### Key Engineering Features:
- **Mechanism / Policy Separation:** Controlled modules expose only `ObservationProvider` and `ActionHandler` adapters. No internal class state is ever accessed directly.
- **Fail-Safe Operation:** Invalid, out-of-order, or late actions never crash or corrupt the simulation. Default fallback is `NO_OP` / keep-current-state.
- **High-Performance Non-Blocking IPC:** `SocketIpcAdapter` uses standard loopback TCP (`127.0.0.1:12345`) with structured JSON serialization, compatible with any Python environment without external C++ bindings.
- **Full Traceability:** Emits standard ns-3 `TracedCallback`s and generates machine-readable CSV logs complying with the project schema.

---

## 3. Directory Layout

```
contrib/futurenet-agent/
├── CMakeLists.txt                         # Top-level CMake module registration
├── README.md                              # This document
├── model/
│   ├── futurenet-agent-types.h            # Common schemas (AgentObservation, AgentAction, enums)
│   ├── observation-provider.h/.cc         # Abstract ObservationProvider base class
│   ├── action-handler.h/.cc               # Abstract ActionHandler base class
│   ├── action-validator.h/.cc             # Action validation & sanitization
│   ├── action-executor.h/.cc              # Action dispatch and execution
│   ├── ipc-adapter.h/.cc                  # Abstract IPC transport interface
│   ├── socket-ipc-adapter.h/.cc           # TCP socket IPC adapter implementation
│   └── agent-interface.h/.cc              # Core AgentInterface orchestrator
├── helper/
│   ├── agent-control-helper.h/.cc         # High-level installation & CSV trace helper
│   └── CMakeLists.txt
├── examples/
│   ├── CMakeLists.txt                     # Example targets definition
│   ├── futurenet-agent-mock-control.cc    # Large-scale mock demonstration (30 entities, 100+ steps)
│   ├── futurenet-agent-network-control.cc # Closed-loop IPv4 rerouting demonstration
│   └── agent/
│       ├── futurenet_agent_client.py      # Standalone Python IPC client library
│       └── rule_based_agent.py            # Reference deterministic Python rule agent
├── test/
│   └── futurenet-agent-test.cc            # Unit & integration test suite (T01 - T06)
└── doc/
    ├── architecture.md                    # In-depth architectural report
    ├── parameter-table.md                 # Complete configuration parameters
    └── validation.md                      # Validation experiments & evidence (V1, V2, V3)
```

---

## 4. Build Instructions

### Prerequisites
- macOS or Linux
- Clang++ or GCC with C++20/C++23 support
- CMake >= 3.16
- Ninja
- Python >= 3.8

### Building with ns-3
From your ns-3 installation directory (e.g. `~/ns-allinone-3.46.1/ns-3.46.1`):
```bash
./ns3 configure --enable-examples --enable-tests
./ns3 build futurenet-agent
```

To build all examples:
```bash
./ns3 build futurenet-agent-mock-control futurenet-agent-network-control
```

---

## 5. Running Automated Tests

Run the dedicated test suite:
```bash
./test.py -s futurenet-agent
```
Output:
```
[1/1] PASS: TestSuite futurenet-agent
1 of 1 tests passed (1 passed, 0 skipped, 0 failed, 0 crashed, 0 valgrind errors)
```

All 6 requirement tests (T01–T06) are verified in this single command.

---

## 6. Running Demonstrations

### Example 1: Mock Control Demonstration (30 Entities, 40s)
Terminal 1 (Start Simulator):
```bash
./ns3 run "futurenet-agent-mock-control --port=12345 --simTime=40 --csv=mock_trace.csv"
```

Terminal 2 (Start External Python Agent):
```bash
python3 contrib/futurenet-agent/examples/agent/rule_based_agent.py --port=12345 --max-decisions=40
```

### Example 2: Real Network Control Demonstration (Closed-Loop IPv4 Rerouting)
Terminal 1 (Start Simulator):
```bash
./ns3 run "futurenet-agent-network-control --port=12346 --simTime=25 --csv=network_trace.csv"
```

Terminal 2 (Start External Python Agent):
```bash
python3 contrib/futurenet-agent/examples/agent/rule_based_agent.py --port=12346 --max-decisions=25
```

At `t = 15s`, physical link 0->1 fails. The Python agent detects the failure within 1 control cycle, issues `SELECT_PATH(target=2)`, and ns-3 dynamically switches the `Ipv4StaticRouting` default route to backup Node 2.

---

## 7. Known Limitations & Extensions

- **Lockstep IPC:** `SocketIpcAdapter` currently operates in synchronized lockstep per decision interval. Asynchronous continuous action streams can be added using a worker thread.
- **Gymnasium Adapter:** `futurenet_agent_client.py` can be wrapped into a standard `gymnasium.Env` by defining observation and action Box/Discrete spaces (recommended extension M10-FR-10).
