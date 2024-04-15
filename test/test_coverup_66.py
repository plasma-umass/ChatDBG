# file src/chatdbg/native_util/clangd_lsp_integration.py:107-123
# lines [108, 109, 111, 112, 113, 114, 115, 116, 117, 118, 122, 123]
# branches []

import pytest
from unittest.mock import Mock, mock_open
from chatdbg.native_util.clangd_lsp_integration import clangd

def _path_to_uri(path):
    # This is a placeholder for the actual _path_to_uri function since it's not provided
    return f"file://{path}"

# Assume the _to_lsp_notification function is defined elsewhere
def _to_lsp_notification(method, params):
    # This is a placeholder for the actual function since it's not provided
    return f"{method} - {params}"

@pytest.fixture
def mock_process(monkeypatch):
    process_mock = Mock()
    stdout_mock = Mock()
    stdout_mock.readline = Mock(return_value='\n')  # simulate an empty response header
    process_mock.stdout = stdout_mock
    monkeypatch.setattr('subprocess.Popen', lambda *args, **kwargs: process_mock)
    return process_mock

def test_didOpen_reads_file_and_sends_notification(monkeypatch, mock_process):
    # Mock the 'open' function
    m = mock_open(read_data='file content')
    monkeypatch.setattr("builtins.open", m)

    # Mock the _to_lsp_notification function
    monkeypatch.setattr("chatdbg.native_util.clangd_lsp_integration._to_lsp_notification", _to_lsp_notification)
    monkeypatch.setattr("chatdbg.native_util.clangd_lsp_integration._path_to_uri", _path_to_uri)

    # Mock the clangd class constructor to prevent it from initializing
    monkeypatch.setattr(clangd, "__init__", lambda self: None)

    # Create an instance of the clangd class
    cl = clangd()
    cl.process = mock_process  # Assign the mock process to the clangd instance

    # Call didOpen method
    cl.didOpen("testfile.c", "c")

    # Verify that the file was read correctly
    m.assert_called_once_with("testfile.c", "r")

    # Verify that the notification was written to process.stdin
    expected_notification = _to_lsp_notification(
        "textDocument/didOpen",
        {
            "textDocument": {
                "uri": _path_to_uri("testfile.c"),
                "languageId": "c",
                "version": 1,
                "text": 'file content',
            }
        },
    )
    mock_process.stdin.write.assert_called_once_with(expected_notification)
    mock_process.stdin.flush.assert_called_once()

    # Close the mock to avoid state pollution
    m().close()
