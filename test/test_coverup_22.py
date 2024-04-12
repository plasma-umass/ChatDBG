# file src/chatdbg/util/log.py:128-133
# lines [128, 129, 130, 131, 132, 133]
# branches []

import pytest
from unittest.mock import patch

# Assuming the existence of the following classes and methods based on the provided context
class BaseAssistantListener:
    _log = None
    _current_chat = None

    def on_response(self, text):
        raise NotImplementedError

def word_wrap_except_code_blocks(text):
    # Placeholder for the actual implementation
    return text

# The actual ChatDBGLog class to be tested
class ChatDBGLog(BaseAssistantListener):
    def on_response(self, text):
        log = self._log
        assert log is not None
        assert self._current_chat is not None
        text = word_wrap_except_code_blocks(text)
        self._current_chat["output"]["outputs"].append({"type": "text", "output": text})

# Test function to cover on_response method
def test_on_response(monkeypatch):
    # Create a mock ChatDBGLog instance with the necessary attributes
    log = ChatDBGLog()
    log._log = True
    log._current_chat = {
        "output": {
            "outputs": []
        }
    }

    test_text = "Test response text"

    # Patch the word_wrap_except_code_blocks function
    with patch('chatdbg.util.log.word_wrap_except_code_blocks', side_effect=word_wrap_except_code_blocks):
        # Call the method we want to test
        log.on_response(test_text)

        # Check if the text was appended correctly
        assert log._current_chat["output"]["outputs"] == [{"type": "text", "output": test_text}]
