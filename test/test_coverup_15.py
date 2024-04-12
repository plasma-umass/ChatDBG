# file src/chatdbg/native_util/clangd_lsp_integration.py:9-16
# lines [9, 10, 11, 12, 14, 15, 16]
# branches ['11->12', '11->14']

import json
import pytest

# Assuming the module name is `clangd_lsp_integration` and it contains the `_to_lsp_request` function.
from chatdbg.native_util import clangd_lsp_integration

def test_to_lsp_request_with_params():
    request_id = 1
    method = "testMethod"
    params = {"param1": "value1", "param2": "value2"}

    result = clangd_lsp_integration._to_lsp_request(request_id, method, params)
    expected_content = json.dumps({
        "jsonrpc": "2.0",
        "id": request_id,
        "method": method,
        "params": params
    })
    expected_header = f"Content-Length: {len(expected_content)}\r\n\r\n"
    expected_result = expected_header + expected_content

    assert result == expected_result

def test_to_lsp_request_without_params():
    request_id = 2
    method = "testMethod"
    params = None

    result = clangd_lsp_integration._to_lsp_request(request_id, method, params)
    expected_content = json.dumps({
        "jsonrpc": "2.0",
        "id": request_id,
        "method": method
    })
    expected_header = f"Content-Length: {len(expected_content)}\r\n\r\n"
    expected_result = expected_header + expected_content

    assert result == expected_result
