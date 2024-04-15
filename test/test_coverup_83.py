# file src/chatdbg/util/jupyter.py:55-58
# lines [55, 56, 57, 58]
# branches ['56->57', '56->58']

import pytest
from IPython.display import HTML, DisplayHandle
from unittest.mock import Mock
from chatdbg.util.jupyter import ChatDBGJupyterPrinter

# Mocking the IPython display function
@pytest.fixture
def mock_display(monkeypatch):
    mock = Mock(return_value=DisplayHandle())
    monkeypatch.setattr("chatdbg.util.jupyter.display", mock)
    return mock

# Test to ensure that the display handle is created when _streamed is empty
def test_on_stream_delta_initializes_display_handle(mock_display):
    printer = ChatDBGJupyterPrinter(debugger_prompt='debug>', chat_prefix='chat>', width=80)
    printer._streamed = ""
    printer.on_stream_delta("Hello, world!")
    empty_html = HTML("")

    # The test must check that the call was made with an HTML object.
    # Since HTML objects are not directly comparable, we need to check the call with the isinstance function.
    mock_display.assert_called_once()
    args, kwargs = mock_display.call_args
    assert isinstance(args[0], HTML)  # This checks that the first argument was an HTML object
    assert kwargs == {'display_id': True}
    assert printer._display_handle is not None
    assert printer._streamed == "Hello, world!"

# Test to ensure that the display handle is not created again if _streamed is not empty
def test_on_stream_delta_does_not_initialize_display_handle_if_already_streamed(mock_display):
    printer = ChatDBGJupyterPrinter(debugger_prompt='debug>', chat_prefix='chat>', width=80)
    printer._streamed = "Previous content"
    printer._display_handle = DisplayHandle()
    printer.on_stream_delta("More content")

    mock_display.assert_not_called()
    assert printer._display_handle is not None
    assert printer._streamed == "Previous contentMore content"
