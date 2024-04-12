# file src/chatdbg/native_util/stacks.py:51-62
# lines [51, 52, 53, 55, 56, 58, 59, 61, 62]
# branches []

import pytest
from chatdbg.native_util.stacks import _SkippedFramesEntry

def test_skipped_frames_entry_count():
    entry = _SkippedFramesEntry(5)
    assert entry.count() == 5

def test_skipped_frames_entry_str_singular():
    entry = _SkippedFramesEntry(1)
    assert str(entry) == "[1 skipped frame...]"

def test_skipped_frames_entry_str_plural():
    entry = _SkippedFramesEntry(2)
    assert str(entry) == "[2 skipped frames...]"

def test_skipped_frames_entry_repr():
    entry = _SkippedFramesEntry(3)
    assert repr(entry) == "_SkippedFramesEntry(3)"
