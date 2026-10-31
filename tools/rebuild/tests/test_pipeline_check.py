"""Tests for the pipeline preflight check of the debugger control port.

Usage:
    python3 -m unittest discover -s tools/rebuild/tests
"""
from __future__ import annotations

import socket
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))

import pipeline_check  # noqa: E402


class PortTestCase(unittest.TestCase):
    """The preflight must tell a held debugger port from a free one."""

    def bound(self) -> tuple[socket.socket, int]:
        listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.addCleanup(listener.close)
        listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        listener.bind(("127.0.0.1", 0))
        listener.listen(1)
        return listener, listener.getsockname()[1]

    def test_a_held_port_is_not_free(self) -> None:
        _, port = self.bound()
        self.assertFalse(pipeline_check.debugger_port_free(port))

    def test_a_closed_port_is_free(self) -> None:
        listener, port = self.bound()
        listener.close()
        self.assertTrue(pipeline_check.debugger_port_free(port))


if __name__ == "__main__":
    unittest.main()
