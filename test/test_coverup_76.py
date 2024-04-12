# file src/chatdbg/native_util/stacks.py:20-48
# lines [29, 30, 31, 32, 33, 36, 39, 42, 45, 48]
# branches []

import pytest
from chatdbg.native_util.stacks import _FrameSummaryEntry

class TestFrameSummaryEntry:
    @pytest.fixture
    def frame_summary_entry(self):
        class ArgumentEntryStub:
            def __init__(self, name, value):
                self._name = name
                self._value = value
            
            def __str__(self):
                return f"{self._name}='{self._value}'"
                
            def __repr__(self):
                return f"_ArgumentEntry('{self._name}', '{self._value}')"

        arg_entry = ArgumentEntryStub('arg', 'value')
        return _FrameSummaryEntry(
            index=1,
            name='function_name',
            arguments=[arg_entry],
            file_path='/path/to/file.py',
            lineno=10
        )

    def test_frame_summary_entry_init(self, frame_summary_entry):
        assert frame_summary_entry._index == 1
        assert frame_summary_entry._name == 'function_name'
        assert frame_summary_entry._arguments[0]._name == 'arg'
        assert frame_summary_entry._arguments[0]._value == 'value'
        assert frame_summary_entry._file_path == '/path/to/file.py'
        assert frame_summary_entry._lineno == 10

    def test_frame_summary_entry_index(self, frame_summary_entry):
        assert frame_summary_entry.index() == 1

    def test_frame_summary_entry_file_path(self, frame_summary_entry):
        assert frame_summary_entry.file_path() == '/path/to/file.py'

    def test_frame_summary_entry_lineno(self, frame_summary_entry):
        assert frame_summary_entry.lineno() == 10

    def test_frame_summary_entry_str(self, frame_summary_entry):
        expected_str = "1: function_name(arg='value') at /path/to/file.py:10"
        assert str(frame_summary_entry) == expected_str

    def test_frame_summary_entry_repr(self, frame_summary_entry):
        expected_repr = "_FrameSummaryEntry(1, 'function_name', [_ArgumentEntry('arg', 'value')], '/path/to/file.py', 10)"
        assert repr(frame_summary_entry) == expected_repr
