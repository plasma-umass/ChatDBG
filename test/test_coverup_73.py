# file src/chatdbg/native_util/dbg_dialog.py:17-21
# lines [17, 19, 20, 21]
# branches []

import pytest
from chatdbg.native_util.dbg_dialog import DBGError

def test_DBGError_message():
    # Test that the DBGError exception correctly stores the message
    message = "This is a debug error message"
    try:
        raise DBGError(message)
    except DBGError as e:
        assert e.message == message

def test_DBGError_inheritance():
    # Test that DBGError is a subclass of Exception
    assert issubclass(DBGError, Exception)

def test_DBGError_instance():
    # Test that DBGError is an instance of Exception
    error = DBGError("Error")
    assert isinstance(error, Exception)
