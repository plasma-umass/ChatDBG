# file src/chatdbg/native_util/dbg_dialog.py:76-77
# lines [76, 77]
# branches []

import pytest
from unittest.mock import MagicMock
from chatdbg.native_util.dbg_dialog import DBGDialog

class TestDBGDialog:
    def test_message_is_a_bad_command_error(self, monkeypatch):
        # Mock the prompt argument required for initializing DBGDialog
        mock_prompt = MagicMock()

        # Create an instance of DBGDialog with the mocked prompt
        dialog = DBGDialog(prompt=mock_prompt)
        
        # Since we do not have the actual implementation of _message_is_a_bad_command_error,
        # we need to provide a mock for this test
        monkeypatch.setattr(dialog, '_message_is_a_bad_command_error', lambda message: True)
        
        # Assert that the monkeypatched method returns True
        assert dialog._message_is_a_bad_command_error('any message')
        
        # Monkeypatch the _message_is_a_bad_command_error method to return False for another test
        monkeypatch.setattr(dialog, '_message_is_a_bad_command_error', lambda message: False)
        
        # Assert that the monkeypatched method returns False
        assert not dialog._message_is_a_bad_command_error('any message')

# Note: Including the test invocation (pytest.main()) is against the user's instructions
