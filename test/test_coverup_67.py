# file src/chatdbg/chatdbg_pdb.py:426-465
# lines [426, 428, 429, 430, 431, 432, 433, 434, 435, 436, 437, 439, 440, 442, 444, 445, 446, 447, 448, 449, 450, 451, 452, 453, 455, 456, 457, 458, 459, 460, 461, 462, 464, 465]
# branches ['430->431', '430->432', '434->435', '434->439', '439->440', '439->442', '446->447', '446->459', '447->448', '447->450', '450->451', '450->456', '457->446', '457->458', '459->exit', '459->460']

from chatdbg.util.config import *
import pytest
from unittest.mock import Mock, MagicMock, patch
from chatdbg.chatdbg_pdb import ChatDBG, ChatDBGSuper

@pytest.fixture
def chat_dbg_instance(monkeypatch):
    with patch("chatdbg.chatdbg_pdb.CaptureInput") as capture_input_mock:
        capture_input_mock.return_value = MagicMock()
        chat_dbg = ChatDBG()
        chat_dbg.stdout = Mock()
        chat_dbg.color_scheme_table = Mock()
        chat_dbg.color_scheme_table.active_colors = Mock()
        chat_dbg.color_scheme_table.active_colors.Normal = ''
        chat_dbg.color_scheme_table.active_colors.excName = ''
        chat_dbg.context = 1
        chat_dbg._show_locals = False
        chat_dbg.stack = [(Mock(), 0)]
        chat_dbg.hidden_frames = lambda stack: [False]
        chat_dbg.print_stack_entry = Mock()
        chat_dbg.skip_hidden = False
        return chat_dbg


def test_print_stack_trace_with_valid_context(chat_dbg_instance):
    chat_dbg_instance.print_stack_trace()
    chat_dbg_instance.print_stack_entry.assert_called()
    chat_dbg_instance.stdout.write.assert_not_called()


def test_print_stack_trace_with_invalid_context_type(chat_dbg_instance):
    with pytest.raises(ValueError) as exc_info:
        chat_dbg_instance.print_stack_trace(context="invalid")
    assert str(exc_info.value) == "Context must be a positive integer"


def test_print_stack_trace_with_invalid_context_value(chat_dbg_instance):
    with pytest.raises(ValueError) as exc_info:
        chat_dbg_instance.print_stack_trace(context=-1)
    assert str(exc_info.value) == "Context must be a positive integer"


def test_print_stack_trace_with_skipped_frames(chat_dbg_instance):
    chat_dbg_instance.hidden_frames = lambda stack: [True]
    chat_dbg_instance.skip_hidden = True
    chat_dbg_instance.print_stack_trace()
    calls = chat_dbg_instance.stdout.write.call_args_list
    assert len(calls) == 2
    assert "skipping 1 hidden frame(s)" in calls[0][0][0]
    assert calls[1][0][0] == '\n'


def test_print_stack_trace_locals_true(chat_dbg_instance, monkeypatch):
    chat_dbg_instance.hidden_frames = lambda stack: [False]
    chat_dbg_instance._show_locals = True
    print_locals_mock = Mock()
    monkeypatch.setattr('chatdbg.chatdbg_pdb.print_locals', print_locals_mock)
    chat_dbg_instance.print_stack_trace(locals=True)
    print_locals_mock.assert_called_once()


def test_print_stack_trace_catches_keyboard_interrupt(chat_dbg_instance):
    with patch.object(chat_dbg_instance, 'print_stack_entry', side_effect=KeyboardInterrupt):
        # No assertion needed as we just want to ensure KeyboardInterrupt is caught
        chat_dbg_instance.print_stack_trace()
