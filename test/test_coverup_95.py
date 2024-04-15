# file src/chatdbg/native_util/dbg_dialog.py:85-87
# lines [85, 86, 87]
# branches []

import pytest
from unittest.mock import patch
from chatdbg.native_util.dbg_dialog import DBGDialog

# Assuming initial_instructions is a standalone function that we can mock
from chatdbg.native_util.dbg_dialog import initial_instructions

class TestDBGDialog:
    @pytest.fixture(autouse=True)
    def setup_class(self, mocker):
        self.dbg_dialog = DBGDialog(prompt="mock_prompt")
        self.mock_initial_instructions = mocker.patch(
            'chatdbg.native_util.dbg_dialog.initial_instructions', return_value="Mocked instructions"
        )

    def test_initial_prompt_instructions(self, mocker):
        # We assume _supported_functions is a method of DBGDialog that we can mock
        supported_functions_mock = ['function1', 'function2']
        with patch.object(self.dbg_dialog, '_supported_functions', return_value=supported_functions_mock):
            instructions = self.dbg_dialog.initial_prompt_instructions()

        # Verify that initial_instructions was called with the mocked _supported_functions
        self.mock_initial_instructions.assert_called_once_with(supported_functions_mock)
        # Verify that the instructions returned are from the mock
        assert instructions == "Mocked instructions"
