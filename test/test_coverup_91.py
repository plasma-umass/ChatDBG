# file src/chatdbg/util/markdown.py:146-147
# lines [146, 147]
# branches []

import pytest
from chatdbg.util.markdown import ChatDBGMarkdownPrinter

class MockChatDBGMarkdownPrinter(ChatDBGMarkdownPrinter):
    def __init__(self):
        self.messages = []

    def _message(self, text, level):
        self.messages.append((text, level))

@pytest.fixture
def mock_printer():
    return MockChatDBGMarkdownPrinter()

def test_on_warn(mock_printer):
    warning_message = "This is a warning"
    mock_printer.on_warn(warning_message)
    assert mock_printer.messages == [(warning_message, "warning")]
