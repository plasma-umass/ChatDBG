# file src/chatdbg/chatdbg_pdb.py:387-395
# lines [387, 391, 392, 393, 394, 395]
# branches []

import pytest
from unittest.mock import MagicMock, mock_open, patch
from chatdbg.chatdbg_pdb import ChatDBG

@pytest.fixture
def chatdbg_instance():
    with patch('chatdbg.chatdbg_pdb.CaptureInput', mock_open(read_data="mock")):
        chatdbg = ChatDBG()
        chatdbg.message = MagicMock()  # Mock the message method to avoid actual print
        chatdbg._initial_prompt_instructions = MagicMock(return_value="Initial prompt instructions")
        chatdbg._build_prompt = MagicMock(return_value="Built prompt")
        yield chatdbg

def test_do_test_prompt(chatdbg_instance):
    test_arg = "test argument"
    chatdbg_instance.do_test_prompt(test_arg)
    chatdbg_instance.message.assert_any_call("Instructions:")
    chatdbg_instance.message.assert_any_call("Initial prompt instructions")
    chatdbg_instance.message.assert_any_call("-" * 80)
    chatdbg_instance.message.assert_any_call("Prompt:")
    chatdbg_instance.message.assert_any_call("Built prompt")
    chatdbg_instance._initial_prompt_instructions.assert_called_once()
    chatdbg_instance._build_prompt.assert_called_once_with(test_arg, False)
