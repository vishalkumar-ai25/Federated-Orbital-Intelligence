# M10 futurenet-agent — Task List

> **Process rule (mandatory):**
> - Each slice: write test FIRST (RED) → implement → build → test must pass (GREEN) → commit → report → wait for "yes" before next slice.
> - Never write more than ~100 lines before running `./ns3 build` + `./test.py`.
> - Never touch `src/` (ns-3 core).

---

## Status Legend
- `[ ]` Not started
- `[~]` In progress
- `[ ]` Done (build ✅ + tests ✅ + committed)

---

## Slice S01 — Scaffold (CMakeLists + directory skeleton)
`[x]`
**Deliverables:**
- `CMakeLists.txt` — `build_lib()` with empty source list placeholder
- `model/` `helper/` `examples/` `test/` `doc/` directories (with `.gitkeep`)
- `examples/CMakeLists.txt` — empty `build_lib_example()` list

**Verify:** `./ns3 configure` shows `futurenet-agent` in module list  
**Commit message:** `feat(scaffold): add CMakeLists and directory structure`

---

## Slice S02 — Types header
`[x]`
**Deliverables:**
- `model/futurenet-agent-types.h` — `AgentActionType` enum, `AgentObservation`, `AgentAction`, `NetworkState`, `LinkState`, `ValidationResult`, `ActionResult`

**TDD:** Add a compile-only test case (`T00`) to `test/futurenet-agent-test.cc` that includes the header and checks `AgentActionType::NOP` exists → must compile  
**Verify:** `./ns3 build futurenet-agent` clean; `./test.py -s futurenet-agent` passes T00  
**Commit message:** `feat(types): add AgentObservation/Action/NetworkState types`

---

## Slice S03 — Abstract interfaces
`[x]`
**Deliverables:**
- `model/observation-provider.h` + `.cc` — pure abstract `ObservationProvider`
- `model/action-handler.h` + `.cc` — pure abstract `ActionHandler`

**TDD:** Add `T00b` — instantiate a `MockProvider` (inline, inside test file) that derives from `ObservationProvider`; assert it compiles and is callable  
**Verify:** `./ns3 build futurenet-agent` clean; tests pass  
**Commit message:** `feat(interfaces): add ObservationProvider and ActionHandler abstractions`

---

## Slice S04 — ActionValidator + T02 (invalid action type)
`[x]`
**TDD cycle:**
1. Write `T02` (RED): validator rejects unknown `AgentActionType` value → `./test.py` must FAIL
2. Implement `model/action-validator.h` + `.cc` with type check only
3. `./test.py` must PASS (GREEN)

**Commit message:** `feat(validator): ActionValidator rejects invalid action type [T02 green]`

---

## Slice S05 — ActionValidator + T03 (stale node ID)
`[x]`
**TDD cycle:**
1. Write `T03` (RED): validator rejects node ID not in known-nodes set → FAIL
2. Extend `ActionValidator` with node-ID check
3. `./test.py` must PASS

**Commit message:** `feat(validator): ActionValidator rejects unknown node ID [T03 green]`

---

## Slice S06 — ActionValidator + T04 (NaN payload)
`[x]`
**TDD cycle:**
1. Write `T04` (RED): validator rejects `NaN` in `AgentAction::value` → FAIL
2. Extend `ActionValidator` with NaN check
3. `./test.py` must PASS

**Commit message:** `feat(validator): ActionValidator rejects NaN payload [T04 green]`

---

## Slice S07 — ActionExecutor + T05 (fan-out dispatch)
`[x]`
**TDD cycle:**
1. Write `T05` (RED): executor with two mock handlers — both receive the action → FAIL
2. Implement `model/action-executor.h` + `.cc`
3. `./test.py` must PASS

**Commit message:** `feat(executor): ActionExecutor fans out to all registered handlers [T05 green]`

---

## Slice S08 — AgentInterface skeleton + T01 + T06
`[x]`
**Deliverables:**
- `model/agent-interface.h` + `.cc`:
  - 13 Attributes (StepInterval, Port, MaxSteps, …)
  - 6 TraceSources (ObservationSent, ActionReceived, ActionValidated, ActionExecuted, StepCompleted, Error)
  - `StartApplication` / `StopApplication` / `DoStep` periodic loop
  - Uses mock IPC (no real socket yet — returns NOP when no adapter set)

**TDD cycle:**
1. Write `T01` (RED): `AgentInterface` created, starts, runs 3 steps, stops → FAIL
2. Write `T06` (RED): TraceSources fire on step → FAIL
3. Implement
4. `./test.py` must PASS (T01 + T06 + all previous GREEN)

**Commit message:** `feat(agent-interface): AgentInterface orchestrator with Attributes and TraceSources [T01, T06 green]`

---

## Slice S09 — SocketIpcAdapter (IPC layer)
`[x]`
**Deliverables:**
- `model/ipc-adapter.h` — abstract `IpcAdapter`
- `model/socket-ipc-adapter.h` + `.cc` — TCP loopback, JSON encode/decode, `poll()` timeout

**TDD:** Manual integration test (not in test suite — uses Python):
```bash
# Terminal 1 — start ns-3 example (mock-control)
./ns3 run futurenet-agent-mock-control

# Terminal 2 — start Python agent
python3 .../examples/agent/rule_based_agent.py
```
Verify: Python agent receives observations and sends NOP actions back; sim completes without crash.

**Commit message:** `feat(ipc): SocketIpcAdapter TCP loopback with JSON encoding`

---

## Slice S10 — AgentControlHelper + CSV tracing
`[x]`
**Deliverables:**
- `helper/agent-control-helper.h` + `.cc`
  - `Install(NodeContainer)` → creates and configures `AgentInterface`
  - `EnableCsvTracing(filename)` → connects TraceSource → CSV file

**Verify:** `./ns3 build futurenet-agent` clean; CSV file appears after mock-control run  
**Commit message:** `feat(helper): AgentControlHelper with CSV trace`

---

## Slice S11 — Example: mock-control
`[x]`
**Deliverables:**
- `examples/futurenet-agent-mock-control.cc`
  - 30 MockProvider entities, MockHandler, 40 s sim
  - Uses `AgentControlHelper`
- `examples/agent/futurenet_agent_client.py`
- `examples/agent/rule_based_agent.py`
- `examples/CMakeLists.txt` updated

**Verify:** `./ns3 run futurenet-agent-mock-control` + Python agent → 39 decisions, no crash  
**Commit message:** `feat(example): mock-control example with Python rule agent`

---

## Slice S12 — Example: network-control (real routing)
`[x]`
**Deliverables:**
- `examples/futurenet-agent-network-control.cc`
  - Real `Ipv4StaticRouting` reroute on link failure at t=15 s
  - `ActionHandler` implementation that modifies routing table

**Verify:** `./ns3 run futurenet-agent-network-control` → link failure → agent issues SELECT_PATH → routing updated → traffic resumes  
**Commit message:** `feat(example): network-control example with real IPv4 rerouting`

---

## Slice S13 — Documentation
`[ ]`
**Deliverables:**
- `README.md` (build, run, Python usage, attribute table)
- `doc/architecture.md`
- `doc/parameter-table.md`
- `doc/validation.md`

**Verify:** All files present and accurate  
**Commit message:** `docs: README, architecture, parameter table, validation guide`

---

## Final Checklist (before submission)
- `[ ]` `./ns3 build futurenet-agent` — zero warnings
- `[ ]` `./test.py -s futurenet-agent` — `1 of 1 tests passed` (T01–T06 all GREEN)
- `[ ]` Mock example runs end-to-end with Python agent
- `[ ]` Network-control example reroutes correctly
- `[ ]` CSV file written by helper
- `[ ]` All 13 Attributes accessible via command line (`--PrintHelp`)
- `[ ]` Git log shows one commit per slice
- `[ ]` No `src/` files modified
