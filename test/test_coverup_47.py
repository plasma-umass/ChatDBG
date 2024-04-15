# file src/chatdbg/util/log.py:14-27
# lines [23, 24]
# branches ['17->23']

import sys
from unittest.mock import Mock
import pytest

# Assuming the existence of the following classes and methods based on the provided code snippet
from chatdbg.util.log import ChatDBGLog, BaseAssistantListener, CaptureOutput

# Test function to cover lines 23-24
def test_chatdbglog_without_capture_streams(monkeypatch):
    # Setup
    mock_sys_stdout = Mock()
    mock_sys_stderr = Mock()
    monkeypatch.setattr(sys, 'stdout', mock_sys_stdout)
    monkeypatch.setattr(sys, 'stderr', mock_sys_stderr)

    # Test without capturing streams
    log = ChatDBGLog(log_filename='test.log', config={}, capture_streams=False)

    # Assertions to ensure that the _stderr_wrapper is None
    assert log._stderr_wrapper is None

    # Cleanup
    monkeypatch.undo()
