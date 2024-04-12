# file src/chatdbg/native_util/dbg_dialog.py:73-74
# lines [73, 74]
# branches []

import pytest
from chatdbg.native_util.dbg_dialog import DBGDialog

# Assuming there are other methods in DBGDialog class that may not be shown here, including a constructor that requires a 'prompt' parameter.

class TestDBGDialog:
    def test_run_one_command(self, mocker):
        # Mocking the DBGDialog class constructor to not require the 'prompt' parameter
        mocker.patch.object(DBGDialog, '__init__', return_value=None)
        
        # Mocking the _run_one_command method
        mocker.patch.object(DBGDialog, '_run_one_command')
        dialog = DBGDialog()
        
        # Call the _run_one_command method
        dialog._run_one_command("some_command")
        
        # Assert that the method was called with the correct argument
        DBGDialog._run_one_command.assert_called_once_with("some_command")

        # Clean up is handled by pytest's fixture scoped mocker.patch.object
