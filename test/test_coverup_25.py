# file src/chatdbg/assistant/assistant.py:20-22
# lines [20, 21, 22]
# branches []

import pytest
from chatdbg.assistant.assistant import AssistantError

def test_assistant_error():
    with pytest.raises(AssistantError) as exc_info:
        raise AssistantError("An error occurred")

    assert str(exc_info.value) == "An error occurred"
