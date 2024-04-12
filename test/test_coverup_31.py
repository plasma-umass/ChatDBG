# file src/chatdbg/util/log.py:125-126
# lines [125, 126]
# branches []

import pytest
from chatdbg.util.log import ChatDBGLog

class MockAssistantListener(ChatDBGLog):
    def __init__(self):
        self.messages = []

    def _post(self, text, level):
        self.messages.append((text, level))

@pytest.fixture
def mock_listener():
    return MockAssistantListener()

def test_on_warn(mock_listener):
    warning_text = "This is a warning"
    mock_listener.on_warn(warning_text)
    assert mock_listener.messages == [(warning_text, "Warning")]
