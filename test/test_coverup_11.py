# file src/chatdbg/native_util/code.py:4-13
# lines [4, 5, 6, 7, 8, 9, 10, 11, 12, 13]
# branches ['6->7', '6->8']

import pytest
from unittest.mock import MagicMock

# Assuming llm_utils is a module that needs to be mocked
import llm_utils

# Mocking llm_utils.read_lines and llm_utils.number_group_of_lines
@pytest.fixture
def mock_llm_utils(monkeypatch):
    mock_read_lines = MagicMock(return_value=(["line1", "line2", "line3"], 1))
    mock_number_group_of_lines = MagicMock(return_value="numbered lines")
    monkeypatch.setattr(llm_utils, 'read_lines', mock_read_lines)
    monkeypatch.setattr(llm_utils, 'number_group_of_lines', mock_number_group_of_lines)
    return mock_read_lines, mock_number_group_of_lines

def test_code_usage_message():
    from chatdbg.native_util.code import code
    result = code("incorrect_usage")
    assert result == "usage: code <filename>:<lineno>"

def test_code_file_not_found(mock_llm_utils):
    from chatdbg.native_util.code import code
    mock_read_lines, _ = mock_llm_utils
    mock_read_lines.side_effect = FileNotFoundError
    result = code("nonexistent.py:10")
    assert result == "file 'nonexistent.py' not found."

def test_code_success(mock_llm_utils):
    from chatdbg.native_util.code import code
    mock_read_lines, mock_number_group_of_lines = mock_llm_utils
    result = code("existent.py:10")
    assert result == "numbered lines"
    mock_read_lines.assert_called_once_with("existent.py", 3, 13)
    mock_number_group_of_lines.assert_called_once_with(["line1", "line2", "line3"], 1)
