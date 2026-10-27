# M10 futurenet-agent — Project Plan

## Objective
Build `contrib/futurenet-agent`: a generic observation-action control interface
that lets an external Python AI/RL process observe ns-3 network state and issue
control actions at run-time, without modifying ns-3 core (`src/`).

---

## Key Requirements (from SRS M10 + Common Guidelines)

| ID | Requirement |
|----|-------------|
| R01 | Module lives entirely in `contrib/futurenet-agent/` — no changes to `src/` |
| R02 | `AgentInterface` orchestrates periodic observation → IPC → action → execution |
| R03 | IPC transport: TCP loopback, JSON-encoded (SocketIpcAdapter). ns3-ai is unavailable on this machine |
| R04 | All configuration through ns-3 Attribute system — no hard-coded constants |
| R05 | TraceSources for: observation sent, action received, action validated, action executed, step completed, error |
| R06 | `ActionValidator` checks stale node IDs, unsupported action types, NaN payloads |
| R07 | At least one Helper class (`AgentControlHelper`) for easy user installation |
| R08 | At least two examples: mock-control (no real routing) and network-control (real IPv4 rerouting) |
| R09 | Test suite passes: `./test.py -s futurenet-agent` (6 test cases T01–T06) |
| R10 | Python client library + rule-based agent in `examples/agent/` |
| R11 | All stochastic behavior via ns-3 random streams (CG-09 / CG-10) |
| R12 | CSV trace file written by helper (one row per control step) |

---

## Tech Stack

| Item | Value |
|------|-------|
| ns-3 version | 3.46.1 |
| IPC transport | TCP loopback (SocketIpcAdapter) — no ns3-ai |
| Build system | CMake + Ninja (via `./ns3`) |
| Language | C++17 (module) + Python 3.13 (agent client) |
| Platform | macOS arm64, Clang++ 21 |

---

## Environment & Commands

```bash
# Working directory for all build/test commands:
cd /Users/vishalkumar/ns-allinone-3.46.1/ns-3.46.1/

# Module source (symlinked into contrib/):
/Users/vishalkumar/Desktop/Lab Project/futurenet-agent/

# Build the module only:
./ns3 build futurenet-agent

# Run test suite:
./test.py -s futurenet-agent

# Run mock example:
./ns3 run futurenet-agent-mock-control

# Run network-control example:
./ns3 run futurenet-agent-network-control
```

---

## Architecture

```
AgentInterface (ns3::Application)
 ├── [0..N] ObservationProvider*   (abstract — user implements per scenario)
 ├── [0..N] ActionHandler*         (abstract — user implements per scenario)
 ├── ActionValidator               (validates incoming actions)
 ├── ActionExecutor                (fans out validated actions to handlers)
 └── IpcAdapter*                   (abstract transport)
       └── SocketIpcAdapter        (concrete: TCP loopback + JSON)

AgentControlHelper
 └── installs AgentInterface on a NodeContainer
 └── connects CSV TraceSource callback → file

examples/
 ├── futurenet-agent-mock-control.cc   (MockProvider + MockHandler, no real routing)
 └── futurenet-agent-network-control.cc (real Ipv4StaticRouting reroute on link fail)

examples/agent/
 ├── futurenet_agent_client.py    (TCP JSON client library)
 └── rule_based_agent.py          (deterministic rules: link fail → SELECT_PATH, queue overload → SELECT_EDGE)
```

---

## Key Architecture Decisions

| Decision | Choice | Reason |
|----------|--------|--------|
| IPC transport | TCP loopback + JSON | ns3-ai unavailable; SRS §3 approves socket fallback |
| IPC model | Blocking `poll()` with timeout per step | Avoids threads; safe in ns-3 single-thread sim loop |
| Action types | SELECT_PATH, SELECT_EDGE, ADJUST_RATE, NOP | Cover SRS §4 requirements |
| Stale-ID check | `AgentInterface::IsKnownNode()` | CG-11: validate all inputs |
| Test isolation | `MockProvider` + `MockHandler` inside test file | No dependency on real ns-3 apps |
| CMake layout | root `CMakeLists.txt` uses `build_lib()`; `examples/CMakeLists.txt` uses `build_lib_example()` separately | Required by ns-3.46.1 CMake system |

---

## Critical CMake / API Gotchas (Learned)

- `build_lib_example()` **must** be in `examples/CMakeLists.txt`, NOT root `CMakeLists.txt`
- `EventId` has no `IsRunning()` — use `IsPending()`
- `TestSuite::UNIT` → `TestSuite::Type::UNIT`; `TestCase::QUICK` → `TestCase::Duration::QUICK`
- Must `#include "ns3/object-factory.h"` explicitly

---

## Phase Breakdown

| Phase | Slices | Gate |
|-------|--------|------|
| **P0 Scaffold** | S01 | `./ns3 configure` detects module |
| **P1 Types** | S02 | `./ns3 build futurenet-agent` with types only |
| **P2 Abstractions** | S03 | Interfaces compile; test stubs RED |
| **P3 Validation** | S04, S05, S06 | T02, T03, T04 pass (TDD) |
| **P4 Core** | S07, S08 | T01, T05, T06 pass |
| **P5 IPC** | S09 | Python round-trip integration test |
| **P6 Helper** | S10 | CSV file written |
| **P7 Examples** | S11, S12 | Both examples run end-to-end |
| **P8 Docs** | S13 | README + doc/ complete |

---

## Risks & Mitigations

| Risk | Mitigation |
|------|-----------|
| TCP port conflict in CI | Use configurable port; default 9999 |
| Python not found at test time | IPC integration is in examples, not test suite; test suite uses mock only |
| ns3-ai temptation | Hard constraint: SocketIpcAdapter only |
| Context window pressure | One slice per conversation turn; build + commit each |
