#!/usr/bin/env python3
"""
Copyright (c) 2026 FutureNet Team
SPDX-License-Identifier: BSD-3-Clause

M10: FutureNet Python IPC Client Library.
Connects to ns-3 AgentInterface SocketIpcAdapter over TCP socket.
"""

import json
import socket
import sys
import time
from typing import Any, Dict, Optional


class FutureNetAgentClient:
    """Client for communicating with the ns-3 futurenet-agent module."""

    def __init__(self, host: str = "127.0.0.1", port: int = 12345, timeout: float = 10.0):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.sock: Optional[socket.socket] = None
        self._buffer = ""

    def connect(self, retry_attempts: int = 20, retry_delay: float = 0.5) -> bool:
        """Connect to the ns-3 SocketIpcAdapter server with retry."""
        for attempt in range(1, retry_attempts + 1):
            try:
                self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                self.sock.settimeout(self.timeout)
                self.sock.connect((self.host, self.port))
                self.sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
                print(f"[Client] Connected to ns-3 futurenet-agent at {self.host}:{self.port}")
                return True
            except (ConnectionRefusedError, socket.timeout, OSError) as e:
                if attempt == retry_attempts:
                    print(f"[Client] Failed to connect after {retry_attempts} attempts: {e}", file=sys.stderr)
                    return False
                time.sleep(retry_delay)
        return False

    def close(self):
        """Close connection cleanly."""
        if self.sock:
            try:
                self.sock.close()
            except Exception:
                pass
            self.sock = None

    def receive_observation(self) -> Optional[Dict[str, Any]]:
        """Receive a single line-delimited JSON observation from ns-3."""
        if not self.sock:
            return None

        while "\n" not in self._buffer:
            try:
                chunk = self.sock.recv(4096).decode("utf-8")
                if not chunk:
                    return None  # Server closed
                self._buffer += chunk
            except socket.timeout:
                return None
            except Exception as e:
                print(f"[Client] Error receiving observation: {e}", file=sys.stderr)
                return None

        line, self._buffer = self._buffer.split("\n", 1)
        if not line.strip():
            return None

        try:
            return json.loads(line)
        except json.JSONDecodeError as e:
            print(f"[Client] JSON decode error: {e} for raw: {line}", file=sys.stderr)
            return None

    def send_action(
        self,
        decision_id: int,
        action_type: str = "NO_OP",
        target_id: int = 0,
        secondary_target_id: int = 0,
        value: float = 0.0,
    ) -> bool:
        """Send an action decision back to ns-3."""
        if not self.sock:
            return False

        payload = {
            "decisionId": int(decision_id),
            "type": action_type,
            "targetId": int(target_id),
            "secondaryTargetId": int(secondary_target_id),
            "value": float(value),
        }

        try:
            data = (json.dumps(payload) + "\n").encode("utf-8")
            self.sock.sendall(data)
            return True
        except Exception as e:
            print(f"[Client] Error sending action: {e}", file=sys.stderr)
            return False
