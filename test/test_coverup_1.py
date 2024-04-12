# file src/chatdbg/pdb_util/paths.py:5-16
# lines [5, 7, 8, 9, 11, 12, 13, 14, 16]
# branches ['8->9', '8->11', '11->12', '11->16', '12->11', '12->13', '13->11', '13->14']

import os
import sys
import pytest
from chatdbg.pdb_util.paths import is_library_file

def test_is_library_file_stdlib(monkeypatch):
    std_lib_path = os.path.dirname(os.__file__)
    file_path = os.path.join(std_lib_path, "os.py")
    assert is_library_file(file_path) == True

def test_is_library_file_site_packages(monkeypatch):
    site_packages_path = None
    for path in sys.path:
        if "site-packages" in path:
            site_packages_path = path
            break
    if site_packages_path is None:
        pytest.skip("No site-packages directory found in sys.path")
    file_path = os.path.join(site_packages_path, "example_package", "example_module.py")
    monkeypatch.setattr(sys, 'path', sys.path + [site_packages_path])
    assert is_library_file(file_path) == True

def test_is_library_file_dist_packages(monkeypatch):
    dist_packages_path = None
    for path in sys.path:
        if "dist-packages" in path:
            dist_packages_path = path
            break
    if dist_packages_path is None:
        pytest.skip("No dist-packages directory found in sys.path")
    file_path = os.path.join(dist_packages_path, "example_package", "example_module.py")
    monkeypatch.setattr(sys, 'path', sys.path + [dist_packages_path])
    assert is_library_file(file_path) == True

def test_is_library_file_user_file(tmp_path):
    user_file = tmp_path / "user_file.py"
    user_file.touch()
    assert is_library_file(str(user_file)) == False
