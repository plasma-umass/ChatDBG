# file src/chatdbg/native_util/dbg_dialog.py:205-209
# lines [205, 206, 207, 208, 209]
# branches ['207->208', '207->209']

import pytest
from unittest.mock import MagicMock

# Assuming the structure of the module `chatdbg.native_util.dbg_dialog` with the class `DBGDialog`
# and the module `clangd_lsp_integration` with the function `is_available` exists.
# Also assuming that DBGDialog requires a 'prompt' argument for initialization.

# Here is a pytest test script designed to test the missing lines/branches of the provided method `_supported_functions`.

# Import the necessary modules and classes
from chatdbg.native_util.dbg_dialog import DBGDialog
import chatdbg.native_util.clangd_lsp_integration as clangd_lsp_integration

# Test when clangd lsp integration is not available
def test_supported_functions_without_clangd(mocker):
    # Mock the `is_available` function to return False
    mocker.patch.object(clangd_lsp_integration, 'is_available', return_value=False)

    # Mock the DBGDialog methods that are used in the _supported_functions method
    mocker.patch.object(DBGDialog, 'llm_debug')
    mocker.patch.object(DBGDialog, 'llm_get_code_surrounding')
    mocker.patch.object(DBGDialog, 'llm_find_definition')

    # Instantiate the DBGDialog class with a mock prompt
    dialog = DBGDialog(prompt="MockPrompt")

    # Call the _supported_functions method
    functions = dialog._supported_functions()

    # Check that the llm_find_definition is not included
    assert dialog.llm_debug in functions
    assert dialog.llm_get_code_surrounding in functions
    assert dialog.llm_find_definition not in functions

# Test when clangd lsp integration is available
def test_supported_functions_with_clangd(mocker):
    # Mock the `is_available` function to return True
    mocker.patch.object(clangd_lsp_integration, 'is_available', return_value=True)

    # Mock the DBGDialog methods that are used in the _supported_functions method
    mocker.patch.object(DBGDialog, 'llm_debug')
    mocker.patch.object(DBGDialog, 'llm_get_code_surrounding')
    mocker.patch.object(DBGDialog, 'llm_find_definition')

    # Instantiate the DBGDialog class with a mock prompt
    dialog = DBGDialog(prompt="MockPrompt")

    # Call the _supported_functions method
    functions = dialog._supported_functions()

    # Check that the llm_find_definition is included
    assert dialog.llm_debug in functions
    assert dialog.llm_get_code_surrounding in functions
    assert dialog.llm_find_definition in functions
