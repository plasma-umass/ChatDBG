import sys
import os

import json
import os
from typing import List, Optional, Union
import lldb

import llm_utils

from chatdbg.native_util import clangd_lsp_integration
from chatdbg.native_util.stacks import _ArgumentEntry, _FrameSummaryEntry, _SkippedFramesEntry
from chatdbg.util.config import chatdbg_config

from chatdbg.native_util.dbg_dialog import DBGDialog


# The file produced by the panic handler if the Rust program is using the chatdbg crate.
RUST_PANIC_LOG_FILENAME = "panic_log.txt"
PROMPT = "(ChatDBG lldb) "


def __lldb_init_module(debugger: lldb.SBDebugger, internal_dict: dict) -> None:
    debugger.HandleCommand(f"settings set prompt '{PROMPT}'")
    chatdbg_config.format = "text"


@lldb.command("code")
def _function_code(
    debugger: lldb.SBDebugger,
    command: str,
    result: lldb.SBCommandReturnObject,
    internal_dict: dict,
) -> None:
    parts = command.split(":")
    if len(parts) != 2:
        result.SetError("usage: code <filename>:<lineno>")
        return
    filename, lineno = parts[0], int(parts[1])
    try:
        lines, first = llm_utils.read_lines(filename, lineno - 7, lineno + 3)
    except FileNotFoundError:
        result.SetError(f"file '{filename}' not found.")
        return
    formatted = llm_utils.number_group_of_lines(lines, first)
    result.AppendMessage(formatted)


_clangd = None
if clangd_lsp_integration.is_available():
    _clangd = clangd_lsp_integration.clangd()


@lldb.command("definition")
def _function_definition(
    debugger: lldb.SBDebugger,
    command: str,
    result: lldb.SBCommandReturnObject,
    internal_dict: dict,
) -> None:
    if not clangd_lsp_integration.is_available():
        result.SetError(
            "`clangd` was not found. The `definition` function will not be made available."
        )
        return
    last_space_index = command.rfind(" ")
    if last_space_index == -1:
        result.SetError(
            "`clangd` was not found. The `definition` function will not be made available."
        )
        return
    filename_lineno = command[:last_space_index]
    symbol = command[last_space_index + 1 :]
    parts = filename_lineno.split(":")
    if len(parts) != 2:
        result.SetError("usage: definition <filename>:<lineno> <symbol>")
        return
    filename, lineno = parts[0], int(parts[1])

    try:
        with open(filename, "r") as file:
            lines = file.readlines()
    except FileNotFoundError:
        result.SetError(f"file '{filename}' not found.")
        return

    if lineno - 1 >= len(lines):
        result.SetError("symbol not found at that location.")
        return

    # We just return the first match here. Maybe we should find all definitions.
    character = lines[lineno - 1].find(symbol)

    # Now, some heuristics to make up for GPT's terrible math skills.
    if character == -1:
        symbol = symbol.lstrip("*")
        character = lines[lineno - 1].find(symbol)

    if character == -1:
        symbol = symbol.split("::")[-1]
        character = lines[lineno - 1].find(symbol)

    # Check five lines above and below.
    if character == -1:
        for i in range(-5, 6, 1):
            if lineno - 1 + i < 0 or lineno - 1 + i >= len(lines):
                continue
            character = lines[lineno - 1 + i].find(symbol)
            if character != -1:
                lineno += i
                break

    if character == -1:
        result.SetError("symbol not found at that location.")
        return

    global _clangd
    _clangd.didOpen(filename, "c" if filename.endswith(".c") else "cpp")
    definition = _clangd.definition(filename, lineno, character + 1)
    _clangd.didClose(filename)

    if "result" not in definition or not definition["result"]:
        result.SetError("No definition found.")
        return

    path = clangd_lsp_integration.uri_to_path(definition["result"][0]["uri"])
    start_lineno = definition["result"][0]["range"]["start"]["line"] + 1
    end_lineno = definition["result"][0]["range"]["end"]["line"] + 1
    lines, first = llm_utils.read_lines(path, start_lineno - 5, end_lineno + 5)
    content = llm_utils.number_group_of_lines(lines, first)
    line_string = (
        f"line {start_lineno}"
        if start_lineno == end_lineno
        else f"lines {start_lineno}-{end_lineno}"
    )
    result.AppendMessage(f"""File '{path}' at {line_string}:\n```\n{content}\n```""")

@lldb.command("chat")
@lldb.command("why")
def chat(
    debugger: lldb.SBDebugger,
    command: str,
    result: lldb.SBCommandReturnObject,
    internal_dict: dict,
):
    try:
        print(type(sys.stdout))
        print(type(sys.__stdout__))
        dialog = LLDBDialog(PROMPT, debugger)
        dialog.dialog(command)
    except Exception as e:
        result.SetError(str(e))

# @lldb.command("test_prompt")
# def test_prompt(
#     debugger: lldb.SBDebugger,
#     command: str,
#     result: lldb.SBCommandReturnObject,
#     internal_dict: dict,
# ):
#     try:
#         # new dialog object, so no history...
#         dialog = LLDBDialog(PROMPT, debugger)
#         result.AppendMessage(dialog.initial_prompt_instructions())
#         result.AppendMessage("-" * 80)
#         result.AppendMessage(dialog.build_prompt(command, False))
#     except Exception as e:
#         result.SetError(str(e))


@lldb.command("config")
def config(
    debugger: lldb.SBDebugger,
    command: str,
    result: lldb.SBCommandReturnObject,
    internal_dict: dict,
):
    args = command.split()
    message = chatdbg_config.parse_only_user_flags(args)
    result.AppendMessage(message)



####


class LLDBDialog(DBGDialog):

    def __init__(self, prompt, debugger) -> None:
        super().__init__(prompt)
        self._debugger = debugger

    def _message_is_a_bad_command_error(self, message):
        return message.strip().endswith("is not a valid command.")

    def _run_one_command(self, command):
        interpreter = self._debugger.GetCommandInterpreter()
        result = lldb.SBCommandReturnObject()
        interpreter.HandleCommand(command, result)

        if result.Succeeded():
            return result.GetOutput()
        else:
            return result.GetError()

    def _is_debug_build(self) -> bool:
        """Returns False if not compiled with debug information."""
        target = self._debugger.GetSelectedTarget()
        if not target:
            return False
        for module in target.module_iter():
            for cu in module.compile_unit_iter():
                for line_entry in cu:
                    if line_entry.GetLine() > 0:
                        return True
        return False

    def get_thread(self, debugger: lldb.SBDebugger) -> Optional[lldb.SBThread]:
        """
        Returns a currently stopped thread in the debugged process.
        :return: A currently stopped thread or None if no thread is stopped.
        """
        process = self._get_process(debugger)
        if not process:
            return None
        for thread in process:
            reason = thread.GetStopReason()
            if reason not in [lldb.eStopReasonNone, lldb.eStopReasonInvalid]:
                return thread
        return thread


    def check_debugger_state(self):
        if not self._debugger.GetSelectedTarget():
            self.fail("must be attached to a program to use `chat`.")

        elif not self._is_debug_build():
            self.fail(
                "your program must be compiled with debug information (`-g`) to use `chat`."
            )

        thread = self.get_thread(self._debugger)
        if not thread:
            self.fail("must run the code first to use `chat`.")

        if not clangd_lsp_integration.is_available():
            self.warn(
                "`clangd` was not found. The `find_definition` function will not be made available."
            )

        
    def _get_frame_summaries(self, max_entries: int = 20
    ) -> Optional[List[Union[_FrameSummaryEntry, _SkippedFramesEntry]]]:
        thread = self.get_thread(self._debugger)
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
            if file_path == None:
                file_path = "[unknown]"
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


    def _get_process(self, debugger) -> Optional[lldb.SBProcess]:
        """
        Get the process that the current target owns.
        :return: An lldb object representing the process (lldb.SBProcess) that this target owns.
        """
        target = debugger.GetSelectedTarget()
        return target.process if target else None

    def _initial_prompt_error_message(self):
        thread = self.get_thread(self._debugger)

        error_message = (thread.GetStopDescription(1024) if thread else None)
        if error_message:
            return error_message
        else:
            self.warn("could not generate an error message.")
            return None

    def _initial_prompt_command_line(self):
        executable = self._debugger.GetSelectedTarget().GetExecutable()
        executable_path = os.path.join(executable.GetDirectory(), executable.GetFilename())
        if executable_path.startswith(os.getcwd()):
            executable_path = os.path.join(".", os.path.relpath(executable_path))

        command_line_arguments = [
            self._debugger.GetSelectedTarget().GetLaunchInfo().GetArgumentAtIndex(i)
            for i in range(self._debugger.GetSelectedTarget().GetLaunchInfo().GetNumArguments())
        ]

        command_line_invocation = " ".join([executable_path, *command_line_arguments])
        if command_line_invocation:
            return command_line_invocation
        else:
            self.warn("could not retrieve the command line invocation.")
            return None

    def _initial_prompt_input(self):
        stream = lldb.SBStream()
        self._debugger.GetSetting("target.input-path").GetAsJSON(stream)
        entry = json.loads(stream.GetData())
    
        input_path = (entry if entry else None)
        if input_path:
            try:
                with open(input_path, "r", errors="ignore") as file:
                    input_contents = file.read()
                    return input_contents
            except FileNotFoundError:
                self.warn("could not retrieve the input data.")
                return None    

    def _initial_prompt_error_details(self):
        """Anything more beyond the initial error message to include."""
        return None

    def _prompt_stack(self):
        """
        Return a simple backtrace to show the LLM where we are on the stack
        in followup prompts.
        """
        return None


