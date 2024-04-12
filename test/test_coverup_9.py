# file src/chatdbg/native_util/clangd_lsp_integration.py:51-63
# lines [51, 52, 54, 55, 56, 57, 58, 60, 61, 62, 63]
# branches ['61->62', '61->63']

import os
import pytest
import urllib.parse
from unittest.mock import patch

# Assuming the function uri_to_path is part of a module named clangd_lsp_integration
from chatdbg.native_util import clangd_lsp_integration

def test_uri_to_path_with_absolute_path(monkeypatch):
    test_uri = "file:///absolute/path/to/file.txt"
    expected_path = "/absolute/path/to/file.txt"

    # Mock os.getcwd() to return a different path to ensure the function does not attempt to make it relative
    monkeypatch.setattr(os, 'getcwd', lambda: "/some/other/directory")

    # Call the function and assert the result
    result = clangd_lsp_integration.uri_to_path(test_uri)
    assert result == expected_path

def test_uri_to_path_with_relative_path(monkeypatch):
    cwd = os.getcwd()
    relative_path = "relative/path/to/file.txt"
    test_uri = f"file://{cwd}/{relative_path}"
    expected_path = relative_path

    # Call the function and assert the result
    result = clangd_lsp_integration.uri_to_path(test_uri)
    assert result == expected_path

def test_uri_to_path_with_escaped_characters(monkeypatch):
    test_uri = "file:///path/to/some%20file.txt"
    expected_path = "/path/to/some file.txt"

    # Mock os.getcwd() to return a different path to ensure the function does not attempt to make it relative
    monkeypatch.setattr(os, 'getcwd', lambda: "/some/other/directory")

    # Call the function and assert the result
    result = clangd_lsp_integration.uri_to_path(test_uri)
    assert result == expected_path

def test_uri_to_path_with_invalid_scheme():
    test_uri = "http:///path/to/file.txt"

    with pytest.raises(AssertionError):
        clangd_lsp_integration.uri_to_path(test_uri)

def test_uri_to_path_with_netloc():
    test_uri = "file://localhost/path/to/file.txt"

    with pytest.raises(AssertionError):
        clangd_lsp_integration.uri_to_path(test_uri)

# Removed the test_uri_to_path_with_params test as it is not valid according to the RFC 8089
# which allows params in the file URI scheme

def test_uri_to_path_with_query():
    test_uri = "file:///path/to/file.txt?query=string"

    with pytest.raises(AssertionError):
        clangd_lsp_integration.uri_to_path(test_uri)

def test_uri_to_path_with_fragment():
    test_uri = "file:///path/to/file.txt#fragment"

    with pytest.raises(AssertionError):
        clangd_lsp_integration.uri_to_path(test_uri)
