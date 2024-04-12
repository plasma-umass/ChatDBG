# file src/chatdbg/native_util/dbg_dialog.py:151-174
# lines [151, 172, 173]
# branches []

import pytest
from unittest.mock import Mock

# Assuming the existence of DBGDialog in chatdbg.native_util.dbg_dialog
from chatdbg.native_util.dbg_dialog import DBGDialog

@pytest.fixture
def dbg_dialog(mocker):
    dbg_dialog_instance = DBGDialog(prompt=Mock())
    mocker.patch.object(dbg_dialog_instance, '_run_one_command')
    return dbg_dialog_instance

def test_llm_get_code_surrounding(dbg_dialog):
    file_name = "test_file.py"
    line_number = 10
    expected_result = f"code {file_name}:{line_number}"

    dbg_dialog._run_one_command.return_value = "mocked result"

    result = dbg_dialog.llm_get_code_surrounding(file_name, line_number)

    assert result == (expected_result, "mocked result")
    dbg_dialog._run_one_command.assert_called_once_with(expected_result)
