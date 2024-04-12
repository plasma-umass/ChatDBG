# file src/chatdbg/native_util/clangd_lsp_integration.py:66-75
# lines [66, 67, 68, 69, 70, 71, 73, 74, 75]
# branches []

import subprocess
import pytest
from unittest.mock import patch

# Assuming the module name is chatdbg.native_util.clangd_lsp_integration
from chatdbg.native_util.clangd_lsp_integration import is_available

@pytest.fixture
def mock_subprocess_run_success():
    with patch('subprocess.run') as mock_run:
        mock_run.return_value.returncode = 0
        yield mock_run

@pytest.fixture
def mock_subprocess_run_failure():
    with patch('subprocess.run') as mock_run:
        mock_run.return_value.returncode = 1
        yield mock_run

@pytest.fixture
def mock_subprocess_run_file_not_found_error():
    with patch('subprocess.run') as mock_run:
        mock_run.side_effect = FileNotFoundError
        yield mock_run

def test_is_available_success(mock_subprocess_run_success):
    assert is_available() == True
    mock_subprocess_run_success.assert_called_once_with(
        ["clangd", "--version"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

def test_is_available_failure(mock_subprocess_run_failure):
    assert is_available() == False
    mock_subprocess_run_failure.assert_called_once_with(
        ["clangd", "--version"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

def test_is_available_file_not_found_error(mock_subprocess_run_file_not_found_error):
    assert is_available() == False
    mock_subprocess_run_file_not_found_error.assert_called_once_with(
        ["clangd", "--version"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

def test_is_available_custom_executable_success(mock_subprocess_run_success):
    assert is_available(executable="custom_clangd") == True
    mock_subprocess_run_success.assert_called_once_with(
        ["custom_clangd", "--version"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

def test_is_available_custom_executable_failure(mock_subprocess_run_failure):
    assert is_available(executable="custom_clangd") == False
    mock_subprocess_run_failure.assert_called_once_with(
        ["custom_clangd", "--version"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

def test_is_available_custom_executable_file_not_found_error(mock_subprocess_run_file_not_found_error):
    assert is_available(executable="custom_clangd") == False
    mock_subprocess_run_file_not_found_error.assert_called_once_with(
        ["custom_clangd", "--version"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
