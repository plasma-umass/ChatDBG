# file src/chatdbg/util/prompts.py:59-68
# lines [59, 60, 61, 63, 65, 66, 67, 68]
# branches ['60->61', '60->63']

import json
import os
import pytest
from unittest.mock import MagicMock

# Assuming the chatdbg_config is part of the same module
from chatdbg.util.prompts import initial_instructions, chatdbg_config


# Mock function with docstring in JSON format
def mock_function():
    """
    {"description": "This is a test function."}
    """
    pass

# Mock function without docstring
def mock_function_no_doc():
    pass

# Test to cover chatdbg_config.instructions not being an empty string
def test_initial_instructions_with_config_path(monkeypatch, tmp_path):
    # Set up a temporary instructions file
    test_instruction_content = "Test instructions: {functions}"
    test_instruction_file = tmp_path / "instructions.txt"
    test_instruction_file.write_text(test_instruction_content)

    # Mock chatdbg_config to return the path of the temporary file
    monkeypatch.setattr(chatdbg_config, 'instructions', str(test_instruction_file))

    # Call the function to test
    instructions = initial_instructions([mock_function])

    # Verify the output
    assert "This is a test function." in instructions

# Test to cover chatdbg_config.instructions being an empty string
def test_initial_instructions_without_config_path(monkeypatch, tmp_path):
    # Set up a temporary instructions file in the same directory as this test file
    test_instruction_content = "Default instructions: {functions}"
    test_instruction_file = tmp_path / "instructions" / "default.txt"
    os.mkdir(tmp_path / "instructions")
    test_instruction_file.write_text(test_instruction_content)

    # Mock os.path.dirname to return the directory of the temporary file
    monkeypatch.setattr(os.path, 'dirname', lambda _: str(tmp_path))

    # Set chatdbg_config.instructions to an empty string
    monkeypatch.setattr(chatdbg_config, 'instructions', '')

    # Call the function to test
    instructions = initial_instructions([mock_function])

    # Verify the output
    assert "Default instructions: This is a test function." in instructions

# Test to cover the case where a function has no docstring
def test_initial_instructions_with_function_without_docstring(monkeypatch, tmp_path):
    # Set up a temporary instructions file
    test_instruction_content = "Instructions with missing function description: {functions}"
    test_instruction_file = tmp_path / "instructions.txt"
    test_instruction_file.write_text(test_instruction_content)

    # Mock chatdbg_config to return the path of the temporary file
    monkeypatch.setattr(chatdbg_config, 'instructions', str(test_instruction_file))

    # Call the function to test with a function that has no docstring
    # Expecting a TypeError because json.loads will receive None
    with pytest.raises(TypeError):
        initial_instructions([mock_function_no_doc])

# Test to cover the case when a function's docstring is not JSON
def test_initial_instructions_with_function_with_non_json_docstring(monkeypatch, tmp_path):
    # Mock function with non-JSON docstring
    def mock_function_non_json():
        """
        This is not a JSON docstring.
        """
        pass

    # Set up a temporary instructions file
    test_instruction_content = "Instructions with invalid function description: {functions}"
    test_instruction_file = tmp_path / "instructions.txt"
    test_instruction_file.write_text(test_instruction_content)

    # Mock chatdbg_config to return the path of the temporary file
    monkeypatch.setattr(chatdbg_config, 'instructions', str(test_instruction_file))

    # Call the function to test with a function that has a non-JSON docstring
    with pytest.raises(json.decoder.JSONDecodeError):
        initial_instructions([mock_function_non_json])
