# file src/chatdbg/assistant/assistant.py:74-75
# lines [74, 75]
# branches []

import pytest
from unittest.mock import MagicMock
from chatdbg.assistant.assistant import Assistant

# Test to ensure the close method broadcasts "on_end_dialog"
def test_assistant_close_broadcasts_on_end_dialog(monkeypatch):
    mock_instructions = MagicMock()
    assistant = Assistant(mock_instructions)
    mock_broadcast = MagicMock()
    monkeypatch.setattr(assistant, '_broadcast', mock_broadcast)

    assistant.close()

    mock_broadcast.assert_called_once_with("on_end_dialog")

# Clean up is handled by the monkeypatch fixture automatically, no additional teardown is needed.
