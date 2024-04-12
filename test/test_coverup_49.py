# file src/chatdbg/pdb_util/paths.py:5-16
# lines [14]
# branches ['13->14']

import os
import sys
import pytest
from chatdbg.pdb_util.paths import is_library_file

def test_is_library_file_stdlib(monkeypatch):
    std_lib_path = os.path.dirname(os.__file__)
    file_path = os.path.join(std_lib_path, "os.py")
    assert is_library_file(file_path) == True

def test_is_library_file_site_packages(monkeypatch, tmp_path):
    # Create a fake site-packages directory and file
    site_packages = tmp_path / "site-packages"
    site_packages.mkdir()
    fake_lib_file = site_packages / "fake_lib.py"
    fake_lib_file.touch()

    # Add the fake site-packages to sys.path
    monkeypatch.setattr(sys, "path", [str(site_packages)] + sys.path)

    assert is_library_file(str(fake_lib_file)) == True

def test_is_library_file_dist_packages(monkeypatch, tmp_path):
    # Create a fake dist-packages directory and file
    dist_packages = tmp_path / "dist-packages"
    dist_packages.mkdir()
    fake_lib_file = dist_packages / "fake_lib.py"
    fake_lib_file.touch()

    # Add the fake dist-packages to sys.path
    monkeypatch.setattr(sys, "path", [str(dist_packages)] + sys.path)

    assert is_library_file(str(fake_lib_file)) == True

def test_is_library_file_user_written(monkeypatch, tmp_path):
    # Create a user-written file outside of stdlib and site/dist-packages
    user_file = tmp_path / "user_file.py"
    user_file.touch()

    # Ensure the fake file is not in stdlib or site/dist-packages
    std_lib_path = os.path.dirname(os.__file__)
    monkeypatch.setattr(sys, "path", [p for p in sys.path if "site-packages" not in p and "dist-packages" not in p])

    assert is_library_file(str(user_file)) == False
