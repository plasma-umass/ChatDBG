# file src/chatdbg/util/config.py:19-29
# lines [19, 20, 21, 22, 23, 24, 25, 26, 27, 29]
# branches ['24->25', '24->26', '26->27', '26->29']

import os
import pytest
from typing import Union

# Assuming the code in question is part of a module named `chatdbg.util.config`
# If it's not, adjust the import statement accordingly
from chatdbg.util.config import _chatdbg_get_env

@pytest.fixture
def env_cleanup():
    # Keep track of the original environment variables to restore them after the test
    original_env = dict(os.environ)
    yield
    # Restore the original environment after the test runs
    for var in os.environ.keys() - original_env.keys():
        del os.environ[var]
    os.environ.update(original_env)

def test_chatdbg_get_env_int(env_cleanup):
    os.environ["CHATDBG_TEST_INT"] = "42"
    assert _chatdbg_get_env("test_int", 1) == 42

def test_chatdbg_get_env_int_default(env_cleanup):
    assert _chatdbg_get_env("test_int_nonexistent", 2) == 2

def test_chatdbg_get_env_bool_true(env_cleanup):
    os.environ["CHATDBG_TEST_BOOL"] = "true"
    assert _chatdbg_get_env("test_bool", False) is True

def test_chatdbg_get_env_bool_false(env_cleanup):
    os.environ["CHATDBG_TEST_BOOL"] = "false"
    assert _chatdbg_get_env("test_bool", True) is False

def test_chatdbg_get_env_bool_default(env_cleanup):
    assert _chatdbg_get_env("test_bool_nonexistent", True) is True

def test_chatdbg_get_env_str(env_cleanup):
    os.environ["CHATDBG_TEST_STR"] = "hello"
    assert _chatdbg_get_env("test_str", "default") == "hello"

def test_chatdbg_get_env_str_default(env_cleanup):
    assert _chatdbg_get_env("test_str_nonexistent", "default") == "default"

def test_chatdbg_get_env_str_with_non_int_default(env_cleanup):
    # Set an environment variable with a string value while the default is an integer
    os.environ["CHATDBG_TEST_STR_INT_DEFAULT"] = "testvalue"
    # Since the default value is an integer, the function is supposed to convert the env value to int
    # As "testvalue" cannot be converted to int, the test should expect an exception
    with pytest.raises(ValueError):
        _chatdbg_get_env("test_str_int_default", 10)
