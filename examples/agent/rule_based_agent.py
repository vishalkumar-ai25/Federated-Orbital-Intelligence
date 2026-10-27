#!/usr/bin/env python3
"""
Copyright (c) 2026 FutureNet Team
SPDX-License-Identifier: BSD-3-Clause

M10: Mandatory Rule-Based Control Agent.
Implements deterministic policies specified in SRS Section 10:
  - Policy 1 (Path Resilience): Reroutes traffic upon link failure (SELECT_PATH).
  - Policy 2 (Queue/Load Balancing): Shifts tasks upon queue overload (SELECT_EDGE).
  - Default: NO_OP when network is healthy.
"""

import argparse
import sys
from futurenet_agent_client import FutureNetAgentClient


class RuleBasedAgent:
    """Deterministic rule-based agent for network control."""

    def __init__(self, queue_threshold: int = 10, verbose: bool = True):
        self.queue_threshold = queue_threshold
        self.verbose = verbose
        self.decision_count = 0

    def decide(self, obs: dict) -> dict:
        """Evaluate observation and return an action dictionary."""
        decision_id = obs.get("decisionId", 0)
        timestamp_ms = obs.get("timestampMs", 0.0)
        nodes = obs.get("nodes", [])
        links = obs.get("links", [])
        self.decision_count += 1

        # Rule 1: Link Failure Detection & Path Selection (SRS Section 10)
        # Check if primary link (destination 1) is unavailable
        for link in links:
            if not link.get("available", True):
                src = link.get("sourceNodeId", 0)
                failed_dst = link.get("destinationNodeId", 0)
                # Find best available alternate link
                alt_dst = 2 if failed_dst == 1 else 1
                if self.verbose:
                    print(
                        f"[{timestamp_ms:.1f}ms Decision {decision_id}] Rule triggered: "
                        f"Link {src}->{failed_dst} is DOWN -> SELECT_PATH target={alt_dst}"
                    )
                return {
                    "decisionId": decision_id,
                    "type": "SELECT_PATH",
                    "targetId": alt_dst,
                    "secondaryTargetId": src,
                    "value": 0.0,
                }

        # Rule 2: Edge Queue Overload Detection (SRS Section 10)
        for node in nodes:
            queue_len = node.get("queuePackets", 0)
            node_id = node.get("nodeId", 0)
            if queue_len > self.queue_threshold:
                alt_node = node_id + 1
                if self.verbose:
                    print(
                        f"[{timestamp_ms:.1f}ms Decision {decision_id}] Rule triggered: "
                        f"Node {node_id} queue={queue_len} > {self.queue_threshold} -> SELECT_EDGE target={alt_node}"
                    )
                return {
                    "decisionId": decision_id,
                    "type": "SELECT_EDGE",
                    "targetId": alt_node,
                    "secondaryTargetId": node_id,
                    "value": float(queue_len),
                }

        # Default Policy: Safe NO_OP when state is nominal
        if self.verbose and (self.decision_count % 10 == 1 or self.decision_count <= 5):
            print(f"[{timestamp_ms:.1f}ms Decision {decision_id}] Nominal network state -> NO_OP")

        return {
            "decisionId": decision_id,
            "type": "NO_OP",
            "targetId": 0,
            "secondaryTargetId": 0,
            "value": 0.0,
        }


def main():
    parser = argparse.ArgumentParser(description="FutureNet M10 Rule-Based Control Agent")
    parser.add_argument("--host", default="127.0.0.1", help="ns-3 host address")
    parser.add_argument("--port", type=int, default=12345, help="ns-3 TCP port")
    parser.add_argument("--max-decisions", type=int, default=1000, help="Max decisions before exit")
    parser.add_argument("--queue-threshold", type=int, default=10, help="Queue length threshold")
    parser.add_argument("--quiet", action="store_true", help="Suppress verbose logging")
    args = parser.parse_args()

    client = FutureNetAgentClient(host=args.host, port=args.port)
    if not client.connect():
        sys.exit(1)

    agent = RuleBasedAgent(queue_threshold=args.queue_threshold, verbose=not args.quiet)
    print(f"[Agent] Starting control loop (max decisions: {args.max_decisions})...")

    decisions_processed = 0
    try:
        while decisions_processed < args.max_decisions:
            obs = client.receive_observation()
            if obs is None:
                print("[Agent] Disconnected from simulation or simulation finished.")
                break

            action = agent.decide(obs)
            success = client.send_action(
                decision_id=action["decisionId"],
                action_type=action["type"],
                target_id=action["targetId"],
                secondary_target_id=action["secondaryTargetId"],
                value=action["value"],
            )
            if not success:
                break
            decisions_processed += 1

    except KeyboardInterrupt:
        print("\n[Agent] Interrupted by user.")
    finally:
        client.close()
        print(f"[Agent] Finished. Processed {decisions_processed} control decisions.")


if __name__ == "__main__":
    main()
