# file src/chatdbg/assistant/assistant.py:77-82
# lines [77, 78, 80, 81, 82]
# branches []

import pytest
from chatdbg.assistant.assistant import Assistant

class MockAssistant(Assistant):
    def __init__(self):
        self.error_messages = []

    def _broadcast(self, event_type, message):
        if event_type == "on_error":
            self.error_messages.append(message)

@pytest.fixture
def mock_assistant():
    return MockAssistant()

def test_warn_about_exception(mock_assistant):
    try:
        raise ValueError("Test exception")
    except Exception as e:
        mock_assistant._warn_about_exception(e, "Test Exception Message")

    assert len(mock_assistant.error_messages) == 1
    assert "Test Exception Message" in mock_assistant.error_messages[0]
    assert "Test exception" in mock_assistant.error_messages[0]
    assert "ValueError: Test exception" in mock_assistant.error_messages[0]
    assert "traceback" in mock_assistant.error_messages[0].lower()

def test_warn_about_exception_default_message(mock_assistant):
    try:
        raise ValueError("Test exception with default message")
    except Exception as e:
        mock_assistant._warn_about_exception(e)

    assert len(mock_assistant.error_messages) == 1
    assert "Unexpected Exception" in mock_assistant.error_messages[0]
    assert "Test exception with default message" in mock_assistant.error_messages[0]
    assert "ValueError: Test exception with default message" in mock_assistant.error_messages[0]
    assert "traceback" in mock_assistant.error_messages[0].lower()
