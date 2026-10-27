# Validation Methodology & Experimental Results
## Module: futurenet-agent (M10)

This document provides the validation methodology and empirical evidence for the three validation scenarios specified in SRS Section 12 and Guidelines Section 10.

---

## Validation Case V1: Forced Link Failure & Dynamic Path Selection

### Objective
Demonstrate that when a primary communication link fails, the external rule-based agent observes the failure and commands the simulator to reroute traffic through an available alternative path without simulator crash.

### Setup
- **Topology:** 4 nodes (`futurenet-agent-network-control.cc`). Node 0 sends 5 Mbps UDP traffic to Node 3.
  - Primary path: Node 0 -> Node 1 -> Node 3 (Link delay: 2ms).
  - Backup path: Node 0 -> Node 2 -> Node 3 (Link delay: 4ms).
- **Control Interval:** 1.0 s.
- **Fault Injection:** At `t = 15.0 s`, physical Link 0->1 is disabled.

### Observed Behavior
1. From `t = 1.0 s` to `14.0 s`: Both links are UP. The agent receives nominal observations and issues `NO_OP`. Node 3 continuously receives packets via Node 1.
2. At `t = 15.0 s`: Physical link 0->1 goes down.
3. Decision 15 (`t = 15.0 s`): The Python agent receives `available=False` on Link 0->1.
   - Rule 1 triggers: `SELECT_PATH(targetId=2)`.
   - `NetworkRoutingActionHandler` modifies `Ipv4StaticRouting` default route on Node 0 to Gateway 10.1.2.2 (Node 2).
4. From `t = 15.0 s` to `25.0 s`: Traffic flows through Node 2 without packet drops.

### Output Log Excerpt
```text
>>> [SIMULATOR EVENT at 15s]: Physical Link 0 -> 1 is failing! Disabling link devices... <<<

[15000.0ms Decision 15] Rule triggered: Link 0->1 is DOWN -> SELECT_PATH target=2

========================================================
 [AGENT CONTROL ACTION APPLIED at 15s]
 Dynamic Route Update: Node 0 default route switched to BACKUP (Node 2)
 Gateway: 10.1.2.2 via Interface 2
 Traffic successfully rerouted around failed link!
========================================================
```

---

## Validation Case V2: Queue Overload & Edge Selection

### Objective
Demonstrate that when a node's local queue exceeds a critical congestion threshold, the agent issues a `SELECT_EDGE` command to redistribute load to a secondary computing/forwarding node.

### Setup
- **Scenario:** `futurenet-agent-mock-control.cc` with 15 nodes and 15 links (30 entities total).
- **Threshold:** Node queue threshold = 10 packets. Node 4 is initialized with `queuePackets = 15`.
- **Fault Recovery:** At `t = 60.0 s`, queue length on Node 4 is cleared to 1 packet.

### Observed Behavior
1. From `t = 1.0 s` to `29.0 s`: Node 4 queue is 15 packets (> 10 threshold).
   - Agent triggers Rule 2: `SELECT_EDGE(targetId=5, secondaryTargetId=4, value=15)`.
   - Action is validated and executed by `ScalableMockActionHandler`.
2. At `t = 30.0 s`: Link 0->1 goes down. Link failure has higher precedence, and agent triggers `SELECT_PATH(targetId=2)`.

### CSV Trace Excerpt (`mock_trace.csv`)
```csv
time_s,decision_id,action,target,secondary_target,value,accepted,reject_reason,observed_delay_ms,observed_loss,observed_rate_mbps
1,1,SELECT_EDGE,0,0,0,true,none,0,0,0
2,2,SELECT_EDGE,0,0,0,true,none,0,0,0
...
29,29,SELECT_EDGE,0,0,0,true,none,0,0,0
30,30,SELECT_PATH,0,0,0,true,none,0,0,0
31,31,SELECT_PATH,0,0,0,true,none,0,0,0
```

---

## Validation Case V3: Agent Disconnect / Timeout Stability

### Objective
Demonstrate that if the external Python agent terminates, crashes, or is disconnected mid-simulation, the simulator does not block or crash, but applies the safe `FallbackAction` (`NO_OP`) for all remaining steps.

### Setup
- Start `futurenet-agent-mock-control` with `--simTime=20`.
- External Python agent connects, executes 5 decisions, and abruptly closes its socket.

### Observed Behavior
1. Decisions 1–5: Processed normally with external Python agent.
2. Decision 6 onwards: Python client is disconnected.
3. `SocketIpcAdapter::ReceiveAction` times out after `ActionTimeout` (500 ms).
4. `AgentInterface` fires `AgentTimeout` trace callback.
5. `ActionExecutor` applies fallback `NO_OP`.
6. Simulation successfully advances to `t = 20.0 s` and terminates cleanly with exit code 0.
