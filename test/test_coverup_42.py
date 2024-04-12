# file src/chatdbg/native_util/clangd_lsp_integration.py:79-94
# lines [85, 86, 87, 88, 89, 90, 91, 92, 94]
# branches []

import os
import subprocess
import pytest
from unittest.mock import Mock, patch
from chatdbg.native_util.clangd_lsp_integration import clangd

@pytest.fixture
def mock_popen(monkeypatch):
    mock_process = Mock()
    attrs = {
        'stdin': Mock(),
        'stdout': Mock(),
        'poll.return_value': None
    }
    mock_process.configure_mock(**attrs)
    mock_process.stdout.readline.return_value = 'Content-Length: 0\r\n\r\n'
    monkeypatch.setattr(subprocess, 'Popen', Mock(return_value=mock_process))
    return mock_process

def test_clangd_init_default_parameters(mock_popen):
    with patch('chatdbg.native_util.clangd_lsp_integration.clangd.initialize') as mock_initialize:
        instance = clangd()
        assert instance.id == 0
        subprocess.Popen.assert_called_once_with(
            ['clangd'],
            text=True,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            cwd=os.getcwd(),
        )
        assert instance.process is mock_popen
        mock_initialize.assert_called_once()

def test_clangd_init_custom_parameters(mock_popen, tmp_path):
    custom_executable = '/usr/bin/custom_clangd'
    custom_working_directory = tmp_path
    custom_stderr = subprocess.PIPE

    with patch('chatdbg.native_util.clangd_lsp_integration.clangd.initialize') as mock_initialize:
        instance = clangd(executable=custom_executable, working_directory=custom_working_directory, stderr=custom_stderr)
        assert instance.id == 0
        subprocess.Popen.assert_called_once_with(
            [custom_executable],
            text=True,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=custom_stderr,
            cwd=custom_working_directory,
        )
        assert instance.process is mock_popen
        mock_initialize.assert_called_once()
