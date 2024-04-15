# file src/chatdbg/util/jupyter.py:26-43
# lines [26, 27, 28, 41, 42, 43]
# branches []

import pytest
from unittest.mock import MagicMock
from chatdbg.util.jupyter import ChatDBGJupyterPrinter

@pytest.fixture
def mock_console(monkeypatch):
    mock_console = MagicMock()
    mock_console.export_html.return_value = "<p>Test HTML</p>"
    monkeypatch.setattr("chatdbg.util.jupyter.Console", MagicMock(return_value=mock_console))
    return mock_console

def test_export_html(mock_console):
    debugger_prompt = MagicMock()
    chat_prefix = MagicMock()
    width = MagicMock()
    printer = ChatDBGJupyterPrinter(debugger_prompt, chat_prefix, width)
    result = printer._export_html()

    mock_console.export_html.assert_called_once_with(clear=True, inline_styles=True)
    assert "<style>" in result
    assert ".rich-text pre,code,div" in result
    assert "line-height: normal !important;" in result
    assert "margin-bottom: 0 !important;" in result
    assert "font-size: 14px !important;" in result
    assert ".rich-text .jp-RenderedHTMLCommon pre, .jp-RenderedHTMLCommon code" in result
    assert "white-space: pre;" in result
    assert "<div class=\"rich-text\"><p>Test HTML</p></div>" in result
