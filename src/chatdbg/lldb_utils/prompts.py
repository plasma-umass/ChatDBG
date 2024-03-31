import os
import json
from typing import List, Optional, Union

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


class _FrameSummaryEntry:
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
        return f"_FrameSummaryEntry({self._index}, {repr(self._name)}, {repr(self._arguments)}, {repr(self._file_path)}, {self._lineno})"


class _SkippedFramesEntry:
    def __init__(self, count: int):
        self._count = count

    def count(self):
        return self._count

    def __str__(self):
        return f"[{self._count} skipped frame{'s' if self._count > 1 else ''}...]"

    def __repr__(self):
        return f"_SkippedFramesEntry({self._count})"


def _get_error_message(debugger: lldb.SBDebugger) -> Optional[str]:
    thread = get_thread(debugger)
    return thread.GetStopDescription(1024) if thread else None


def _initial_prompt_error(debugger, result):
    error_message = _get_error_message(debugger)
    if error_message:
        return (
            "Here is the reason the program stopped execution:\n```\n"
            + error_message
            + "\n```"
        )
    else:
        result.AppendWarning("could not generate an error message.")
        return ""


def _get_frame_summaries(
    debugger: lldb.SBDebugger, max_entries: int = 20
) -> Optional[List[Union[_FrameSummaryEntry, _SkippedFramesEntry]]]:
    thread = get_thread(debugger)
    if not thread:
        return None

    skipped = 0
    summaries: List[Union[_FrameSummaryEntry, _SkippedFramesEntry]] = []

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
            summaries.append(_SkippedFramesEntry(skipped))
            skipped = 0

        summaries.append(_FrameSummaryEntry(index, name, arguments, file_path, lineno))
        if len(summaries) >= max_entries:
            break

    if skipped > 0:
        summaries.append(_SkippedFramesEntry(skipped))
        if len(summaries) > max_entries:
            summaries.pop(-2)

    total_summary_count = sum(
        [s.count() if isinstance(s, _SkippedFramesEntry) else 1 for s in summaries]
    )

    if total_summary_count < len(thread):
        if isinstance(summaries[-1], _SkippedFramesEntry):
            summaries[-1] = _SkippedFramesEntry(
                len(thread) - total_summary_count + summaries[-1].count()
            )
        else:
            summaries.append(_SkippedFramesEntry(len(thread) - total_summary_count + 1))
            if len(summaries) > max_entries:
                summaries.pop(-2)

    assert sum(
        [s.count() if isinstance(s, _SkippedFramesEntry) else 1 for s in summaries]
    ) == len(thread)

    return summaries


def _initial_prompt_enriched_stack_trace(debugger, result):
    parts = []
    summaries = _get_frame_summaries(debugger)
    if not summaries:
        result.AppendWarning("could not generate any frame summary.")
    else:
        frame_summary = "\n".join([str(s) for s in summaries])
        parts.append(
            "Here is a summary of the stack frames, omitting those not associated with user source code:\n```\n"
            + frame_summary
            + "\n```"
        )

        total_frames = sum(
            [s.count() if isinstance(s, _SkippedFramesEntry) else 1 for s in summaries]
        )

        if total_frames > 1000:
            parts.append(
                "Note that there are over 1000 frames in the stack trace, hinting at a possible stack overflow error."
            )

    max_initial_locations_to_send = 3
    source_code_entries = []
    for summary in summaries:
        if isinstance(summary, _FrameSummaryEntry):
            file_path, lineno = summary.file_path(), summary.lineno()
            lines, first = llm_utils.read_lines(file_path, lineno - 10, lineno + 9)
            block = llm_utils.number_group_of_lines(lines, first)
            source_code_entries.append(
                f"Frame #{summary.index()} at {file_path}:{lineno}:\n```\n{block}\n```"
            )

            if len(source_code_entries) == max_initial_locations_to_send:
                break

    if source_code_entries:
        parts.append(
            f"Here is the source code for the first {len(source_code_entries)} frames:\n\n"
            + "\n\n".join(source_code_entries)
        )
    else:
        result.AppendWarning("could not retrieve source code for any frames.")

    return "\n\n".join(parts)


def _get_command_line_invocation(debugger: lldb.SBDebugger) -> Optional[str]:
    executable = debugger.GetSelectedTarget().GetExecutable()
    executable_path = os.path.join(executable.GetDirectory(), executable.GetFilename())
    if executable_path.startswith(os.getcwd()):
        executable_path = os.path.join(".", os.path.relpath(executable_path))

    command_line_arguments = [
        debugger.GetSelectedTarget().GetLaunchInfo().GetArgumentAtIndex(i)
        for i in range(debugger.GetSelectedTarget().GetLaunchInfo().GetNumArguments())
    ]

    return " ".join([executable_path, *command_line_arguments])


def _get_input_path(debugger: lldb.SBDebugger) -> Optional[str]:
    stream = lldb.SBStream()
    debugger.GetSetting("target.input-path").GetAsJSON(stream)
    entry = json.loads(stream.GetData())
    return entry if entry else None


def _initial_prompt_inputs(debugger, result):
    parts = []
    command_line_invocation = _get_command_line_invocation(debugger)
    if command_line_invocation:
        parts.append(
            "Here is the command line invocation that started the program:\n```\n"
            + command_line_invocation
            + "\n```"
        )
    else:
        result.AppendWarning("could not retrieve the command line invocation.")

    input_path = _get_input_path(debugger)
    if input_path:
        try:
            with open(input_path, "r", errors="ignore") as file:
                input_contents = file.read()
                if len(input_contents) > 512:
                    input_contents = input_contents[:512] + "\n\n[...]"
                parts.append(
                    "Here is the input data that was used:\n```\n"
                    + input_contents
                    + "\n```"
                )
        except FileNotFoundError:
            result.AppendWarning("could not retrieve the input data.")

    return "\n\n".join(parts)


def _get_process(debugger) -> Optional[lldb.SBProcess]:
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
    process = _get_process(debugger)
    if not process:
        return None
    for thread in process:
        reason = thread.GetStopReason()
        if reason not in [lldb.eStopReasonNone, lldb.eStopReasonInvalid]:
            return thread
    return thread


def _build(*args):
    return "\n".join(args)


def build_initial_prompt(debugger, user_text) -> str:
    result = lldb.SBCommandReturnObject()
    prompt = _build(
        _initial_prompt_error(debugger, result),
        _initial_prompt_enriched_stack_trace(debugger, result),
        _initial_prompt_inputs(debugger, result),
        user_text,
    )
    return prompt


def build_followup_prompt(debugger, user_text) -> str:
    result = lldb.SBCommandReturnObject()
    prompt = _build(user_text)
    return prompt
