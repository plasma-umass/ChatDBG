# file src/chatdbg/native_util/clangd_lsp_integration.py:20-27
# lines [20, 21, 22, 23, 25, 26, 27]
# branches ['22->23', '22->25']

import json
import pytest

# Assuming the file structure is as follows:
# chatdbg/
# └── native_util/
#     └── clangd_lsp_integration.py

from chatdbg.native_util.clangd_lsp_integration import _to_lsp_notification

def test_to_lsp_notification_with_params():
    method = "textDocument/didOpen"
    params = {"textDocument": {"uri": "file://path/to/file.py", "languageId": "python", "version": 1, "text": "print('Hello, world!')"}}
    
    result = _to_lsp_notification(method, params)
    
    content = json.dumps({"jsonrpc": "2.0", "method": method, "params": params})
    expected_header = f"Content-Length: {len(content)}\r\n\r\n"
    expected_result = expected_header + content
    
    assert result == expected_result

def test_to_lsp_notification_without_params():
    method = "workspace/didChangeConfiguration"
    params = None
    
    result = _to_lsp_notification(method, params)
    
    content = json.dumps({"jsonrpc": "2.0", "method": method})
    expected_header = f"Content-Length: {len(content)}\r\n\r\n"
    expected_result = expected_header + content
    
    assert result == expected_result
