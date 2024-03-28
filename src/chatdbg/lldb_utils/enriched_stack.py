import argparse
import os
import json
import textwrap
from typing import Any, List, Optional, Tuple, Union

import lldb
import llm_utils

class _ArgumentEntry:
    def __init__(self, type: str, name: str, value: str):
        self._type = type
        self._name = name
        self._value = value

    def __str__(self):
        return f"({self._type}) {self._name} = {self._value if self._value else '[unknown]'}"

    def __repr__(self):
        return f"_ArgumentEntry({repr(self.type)}, {repr(self._name)}, {repr(self._value)})"


class FrameSummaryEntry:
    def __init__(
        self,
        index: int,
        name: str,
        arguments: List[_ArgumentEntry],
        file_path: str,
        lineno: int,
    ):
        self._index = index
        self._name = name
        self._arguments = arguments
        self._file_path = file_path
        self._lineno = lineno

    def index(self):
        return self._index

    def file_path(self):
        return self._file_path

    def lineno(self):
        return self._lineno

    def __str__(self):
        return f"{self._index}: {self._name}({', '.join([str(a) for a in self._arguments])}) at {self._file_path}:{self._lineno}"

    def __repr__(self):
        return f"FrameSummaryEntry({self._index}, {repr(self._name)}, {repr(self._arguments)}, {repr(self._file_path)}, {self._lineno})"


class SkippedFramesEntry:
    def __init__(self, count: int):
        self._count = count

    def count(self):
        return self._count

    def __str__(self):
        return f"[{self._count} skipped frame{'s' if self._count > 1 else ''}...]"

    def __repr__(self):
        return f"SkippedFramesEntry({self._count})"

def get_error_message(debugger: lldb.SBDebugger) -> Optional[str]:
    thread = get_thread(debugger)
    return thread.GetStopDescription(1024) if thread else None


def get_frame_summaries(
    debugger: lldb.SBDebugger, max_entries: int = 20
) -> Optional[List[Union[FrameSummaryEntry, SkippedFramesEntry]]]:
    thread = get_thread(debugger)
    if not thread:
        return None

    skipped = 0
    summaries: List[Union[FrameSummaryEntry, SkippedFramesEntry]] = []

    index = -1
    for frame in thread:
        index += 1
        if not frame.GetDisplayFunctionName():
            skipped += 1
            continue
        name = frame.GetDisplayFunctionName().split("(")[0]
        arguments: List[_ArgumentEntry] = []
        for j in range(
            frame.GetFunction().GetType().GetFunctionArgumentTypes().GetSize()
        ):
            arg = frame.FindVariable(frame.GetFunction().GetArgumentName(j))
            if not arg:
                arguments.append(_ArgumentEntry("[unknown]", "[unknown]", "[unknown]"))
                continue
            # TODO: Check if we should simplify / truncate types, e.g. std::unordered_map.
            arguments.append(
                _ArgumentEntry(arg.GetTypeName(), arg.GetName(), arg.GetValue())
            )

        line_entry = frame.GetLineEntry()
        file_path = line_entry.GetFileSpec().fullpath
        lineno = line_entry.GetLine()

        # If we are in a subdirectory, use a relative path instead.
        if file_path.startswith(os.getcwd()):
            file_path = os.path.relpath(file_path)

        # Skip frames for which we have no source -- likely system frames.
        if not os.path.exists(file_path):
            skipped += 1
            continue

        if skipped > 0:
            summaries.append(SkippedFramesEntry(skipped))
            skipped = 0

        summaries.append(FrameSummaryEntry(index, name, arguments, file_path, lineno))
        if len(summaries) >= max_entries:
            break

    if skipped > 0:
        summaries.append(SkippedFramesEntry(skipped))
        if len(summaries) > max_entries:
            summaries.pop(-2)

    total_summary_count = sum(
        [s.count() if isinstance(s, SkippedFramesEntry) else 1 for s in summaries]
    )

    if total_summary_count < len(thread):
        if isinstance(summaries[-1], SkippedFramesEntry):
            summaries[-1] = SkippedFramesEntry(
                len(thread) - total_summary_count + summaries[-1].count()
            )
        else:
            summaries.append(SkippedFramesEntry(len(thread) - total_summary_count + 1))
            if len(summaries) > max_entries:
                summaries.pop(-2)

    assert sum(
        [s.count() if isinstance(s, SkippedFramesEntry) else 1 for s in summaries]
    ) == len(thread)

    return summaries


def get_command_line_invocation(debugger: lldb.SBDebugger) -> Optional[str]:
    executable = debugger.GetSelectedTarget().GetExecutable()
    executable_path = os.path.join(executable.GetDirectory(), executable.GetFilename())
    if executable_path.startswith(os.getcwd()):
        executable_path = os.path.join(".", os.path.relpath(executable_path))

    command_line_arguments = [
        debugger.GetSelectedTarget().GetLaunchInfo().GetArgumentAtIndex(i)
        for i in range(debugger.GetSelectedTarget().GetLaunchInfo().GetNumArguments())
    ]

    return " ".join([executable_path, *command_line_arguments])


def get_input_path(debugger: lldb.SBDebugger) -> Optional[str]:
    stream = lldb.SBStream()
    debugger.GetSetting("target.input-path").GetAsJSON(stream)
    entry = json.loads(stream.GetData())
    return entry if entry else None


def get_process(debugger) -> Optional[lldb.SBProcess]:
    """
    Get the process that the current target owns.
    :return: An lldb object representing the process (lldb.SBProcess) that this target owns.
    """
    target = debugger.GetSelectedTarget()
    return target.process if target else None


def get_thread(debugger: lldb.SBDebugger) -> Optional[lldb.SBThread]:
    """
    Returns a currently stopped thread in the debugged process.
    :return: A currently stopped thread or None if no thread is stopped.
    """
    process = get_process(debugger)
    if not process:
        return None
    for thread in process:
        reason = thread.GetStopReason()
        if reason not in [lldb.eStopReasonNone, lldb.eStopReasonInvalid]:
            return thread
    return thread
