# file src/chatdbg/pdb_util/paths.py:19-33
# lines [19, 20, 21, 23, 24, 27, 29, 30, 31, 33]
# branches ['30->31', '30->33']

import os
import sys
from unittest.mock import patch
import pytest

# Assuming the function `is_library_file` is defined somewhere in the module
from chatdbg.pdb_util.paths import is_library_file, main

@pytest.fixture
def clean_sys_path(monkeypatch):
    # Backup original sys.path
    original_sys_path = sys.path.copy()
    yield
    # Restore original sys.path after the test
    monkeypatch.setattr(sys, 'path', original_sys_path)

def test_main_with_library_file(clean_sys_path, monkeypatch, capsys):
    # Mock sys.path to contain known library paths
    monkeypatch.setattr(sys, 'path', ['/usr/lib/python3.8', '/usr/local/lib/python3.8/site-packages'])
    # Mock os.__file__ to a known location
    monkeypatch.setattr(os, '__file__', '/usr/lib/python3.8/os.py')
    # Mock is_library_file to return True
    monkeypatch.setattr('chatdbg.pdb_util.paths.is_library_file', lambda x: True)

    main()
    captured = capsys.readouterr()

    assert "*** user path: /usr/lib/python3.8 ***" in captured.out
    assert "/usr/local/lib/python3.8/site-packages" in captured.out
    assert "/path/to/your/file.py is likely a library file." in captured.out

def test_main_with_user_written_file(clean_sys_path, monkeypatch, capsys):
    # Mock sys.path to contain no library paths
    monkeypatch.setattr(sys, 'path', ['/home/user/projects'])
    # Mock os.__file__ to a known location
    monkeypatch.setattr(os, '__file__', '/usr/lib/python3.8/os.py')
    # Mock is_library_file to return False
    monkeypatch.setattr('chatdbg.pdb_util.paths.is_library_file', lambda x: False)

    main()
    captured = capsys.readouterr()

    assert "*** user path: /home/user/projects ***" in captured.out
    assert "/path/to/your/file.py is likely a user-written file." in captured.out
