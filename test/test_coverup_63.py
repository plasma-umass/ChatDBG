# file src/chatdbg/util/config.py:112-128
# lines [112, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127]
# branches []

import pytest
from chatdbg.util.config import ChatDBGConfig

# Test to_json method
def test_to_json():
    config = ChatDBGConfig()
    config.model = 'test_model'
    config.debug = True
    config.log = 'test_log'
    config.tag = 'test_tag'
    config.rc_lines = '42'  # Adjusted to be a string since the error shows rc_lines is expected to be a unicode string
    config.context = 10  # Adjusted to be an int since the error shows context is expected to be an int
    config.show_locals = True
    config.show_libs = False
    config.show_slices = True
    config.take_the_wheel = False
    config.no_stream = True
    config.format = 'test_format'
    config.instructions = 'test_instructions'

    expected_json = {
        "model": 'test_model',
        "debug": True,
        "log": 'test_log',
        "tag": 'test_tag',
        "rc_lines": '42',  # Adjusted to be a string
        "context": 10,  # Adjusted to be an int
        'module_whitelist': '',
        "show_locals": True,
        "show_libs": False,
        "show_slices": True,
        "take_the_wheel": False,
        "no_stream": True,
        "format": 'test_format',
        "instructions": 'test_instructions',
    }

    assert config.to_json() == expected_json, "to_json method did not return the expected data"
