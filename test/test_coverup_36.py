# file src/chatdbg/native_util/clangd_lsp_integration.py:30-44
# lines [32, 33, 34, 35, 36, 37, 38, 39, 41, 42, 43, 44]
# branches ['32->33', '34->35', '36->37', '36->38', '43->32', '43->44']

import json
import pytest
from unittest.mock import MagicMock

# Assuming the module name is `clangd_lsp_integration` and the function is `_parse_lsp_response`
from chatdbg.native_util.clangd_lsp_integration import _parse_lsp_response

@pytest.fixture
def mock_file():
    mock_file = MagicMock()
    mock_file.readline = MagicMock()
    mock_file.read = MagicMock()
    return mock_file

def test_parse_lsp_response_with_correct_id(mock_file):
    # Prepare the mock file with a response containing the correct id
    response_id = 42
    response_content = json.dumps({"id": response_id, "result": "success"})
    content_length = len(response_content)
    mock_file.readline.side_effect = [
        f"Content-Length: {content_length}\r\n",
        "\r\n"
    ]
    mock_file.read.return_value = response_content

    # Call the function with the mock file and the correct id
    response = _parse_lsp_response(response_id, mock_file)

    # Assert that the response is as expected
    assert response == {"id": response_id, "result": "success"}

def test_parse_lsp_response_with_incorrect_id(mock_file):
    # Prepare the mock file with a response containing an incorrect id first, then the correct one
    incorrect_response_content = json.dumps({"id": 999, "result": "failure"})
    correct_response_id = 42
    correct_response_content = json.dumps({"id": correct_response_id, "result": "success"})
    incorrect_content_length = len(incorrect_response_content)
    correct_content_length = len(correct_response_content)
    mock_file.readline.side_effect = [
        f"Content-Length: {incorrect_content_length}\r\n",
        "\r\n",
        f"Content-Length: {correct_content_length}\r\n",
        "\r\n"
    ]
    mock_file.read.side_effect = [
        incorrect_response_content,
        correct_response_content
    ]

    # Call the function with the mock file and the correct id
    response = _parse_lsp_response(correct_response_id, mock_file)

    # Assert that the response is the correct one
    assert response == {"id": correct_response_id, "result": "success"}
