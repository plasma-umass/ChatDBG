import os
import json
import sys
import textwrap
from typing import List, Optional, Union

from chatdbg.util.log import ChatDBGLog
from chatdbg.util.printer import ChatDBGPrinter
import lldb

import llm_utils

from assistant.assistant import Assistant, AssistantError
import clangd_lsp_integration
from util.config import chatdbg_config


class DBGError(Exception):

    def __init__(self, message):
        self.message = message
        super().__init__(self.message)


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


class DBGDialog:
    # The log file used by the listener on the Assistant
    _log = ChatDBGLog(
        log_filename=chatdbg_config.log,
        config=chatdbg_config.to_json(),
        capture_streams=False,  # don't have access to target's stdout/stderr here.
    )

    def __init__(self, prompt) -> None:
        self.prompt = prompt
        self._history = []

    def _report(self, stats):
        if stats["completed"]:
            print(f"\n[Cost: ~${stats['cost']:.2f} USD]")
        else:
            print(f"\n[Chat Interrupted]")

    def dialog(self, command):
        try:
            assistant = self._make_assistant()
            self.check_debugger_state()

            user_text = (
                command if command else "What's the problem? Provide code to fix the issue."
            )

            initial_prompt = self.build_prompt(user_text, False)
            self._report(assistant.query(initial_prompt, user_text))
            while True:
                try:
                    command = input(">>> " + self.prompt).strip()
                    
                    if command in [ "exit", "quit" ]:
                        break
                    if command in [ "chat", "why" ]:
                        followup_prompt = self.build_prompt(user_text, True)
                        self._report(assistant.query(followup_prompt, user_text))
                    elif command == "history":
                        print(self._do_history())
                    else:
                        # Send the next input as an LLDB command
                        result = self._run_one_command(command)
                        if self._message_is_a_bad_command_error(result):
                            # If result is not a recognized command, pass it as a query
                            followup_prompt = self.build_prompt(command, True)
                            self._report(assistant.query(followup_prompt, user_text))
                        else:
                            self._history += [(command, result)]
                            print(result)
                except EOFError:
                    # If it causes an error, break
                    break

            assistant.close()
        except AssistantError as e:


    def _format_history_entry(self, entry, indent=""):
        line, output = entry
        if output:
            entry = f"{self.prompt}{line}\n{output}"
        else:
            entry = f"{self.prompt}{line}"
        return textwrap.indent(entry, indent, lambda _: True)
    
    def _do_history(self):
        """
        Returns the formatted history of user-issued commands since the last chat.
        """
        entry_strs = [self._format_history_entry(x) for x in self._history]
        history_str = "\n".join(entry_strs)
        return history_str


    def _get_history(self):
            if len(self._history) > 0:
                hist = textwrap.indent(self._do_history(), "")
                hist = f"\nThis is the history of some commands I ran and the results.\n```\n{hist}\n```\n"
                return hist
            else:
                return ""

    def _build_enriched_stacktrace(self):
        parts = []
        summaries = self._get_frame_summaries()
        if not summaries:
            self.warn("could not generate any frame summary.")
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
            self.warn("could not retrieve source code for any frames.")

        return "\n\n".join(parts)

    # Return string for valid command.  None if the command is not valid.
    def _run_one_command(self, command):
        pass

    def _message_is_a_bad_command_error(self, message):        
        pass

    def check_debugger_state(self):
        pass

    def _get_frame_summaries(self, max_entries: int = 20):
        pass

    def _build_error(self):
        pass

    def build_inputs(self):
        pass

    def _build_history(self):
        pass

    def build_instructions(self):
        return textwrap.dedent(
            """
                You are an assistant debugger.
                The user is having an issue with their code, and you are trying to help them find the root cause.
                They will provide a short summary of the issue and a question to be answered.

                Call the `debug` function to run lldb debugger commands on the stopped program.
                Call the `get_code_surrounding` function to retrieve user code and give more context back to the user on their problem.
                Call the `find_definition` function to retrieve the definition of a particular symbol.
                You should call `find_definition` on every symbol that could be linked to the issue.

                Don't hesitate to use as many function calls as needed to give the best possible answer.
                Once you have identified the root cause of the problem, explain it and provide a way to fix the issue if you can.
            """
        ).strip()

    def _make_assistant(self) -> Assistant:

        # TODO: Move these functions to the toplevel and use functools.partial
        def llm_debug(command: str) -> str:
            """
            {
                "name": "debug",
                "description": "Run an LLDB command and get the response.",
                "parameters": {
                    "type": "object",
                    "properties": {
                        "command": {
                            "type": "string",
                            "description": "The LLDB command to run, possibly with arguments."
                        }
                    },
                    "required": [ "command" ]
                }
            }
            """
            return command, self._run_one_command(command)

        def llm_get_code_surrounding(filename: str, lineno: int) -> str:
            """
            {
                "name": "get_code_surrounding",
                "description": "Returns the code in the given file surrounding and including the provided line number.",
                "parameters": {
                    "type": "object",
                    "properties": {
                        "filename": {
                            "type": "string",
                            "description": "The filename to read from."
                        },
                        "lineno": {
                            "type": "integer",
                            "description": "The line number to focus on. Some context before and after that line will be provided."
                        }
                    },
                    "required": [ "filename", "lineno" ]
                }
            }
            """
            return f"code {filename}:{lineno}", self._run_one_command(
                f"code {filename}:{lineno}"
            )

        def llm_find_definition(filename: str, lineno: int, symbol: str) -> str:
            """
            {
                "name": "find_definition",
                "description": "Returns the definition for the given symbol at the given source line number.",
                "parameters": {
                    "type": "object",
                    "properties": {
                        "filename": {
                            "type": "string",
                            "description": "The filename the symbol is from."
                        },
                        "lineno": {
                            "type": "integer",
                            "description": "The line number where the symbol is present."
                        },
                        "symbol": {
                            "type": "string",
                            "description": "The symbol to lookup."
                        }
                    },
                    "required": [ "filename", "lineno", "symbol" ]
                }
            }
            """
            return f"definition {filename}:{lineno} {symbol}", self._run_one_command(
                f"definition {filename}:{lineno} {symbol}"
            )

        functions = [llm_debug, llm_get_code_surrounding]
        if clangd_lsp_integration.is_available():
            functions += [llm_find_definition]

        instruction_prompt = self.build_instructions()

        assistant = Assistant(
            instruction_prompt,
            model=chatdbg_config.model,
            debug=chatdbg_config.debug,
            functions=functions,
            stream=chatdbg_config.stream,
            listeners=[
                ChatDBGPrinter(
                    sys.stdout,
                    self.prompt,  # must end with ' ' to match other tools
                    "   ",
                    80,
                    stream=chatdbg_config.stream,
                ),
                self._log,
            ],
        )

        return assistant
    
    def build_prompt(self, user_text, conversing) -> str:
        if not conversing:
            prompt = "\n".join([
            self._build_error(),
            self._build_enriched_stacktrace(),
            self.build_inputs(),
            self._get_history(),
            user_text
            ])
        else: 
            prompt = "\n".join(
            [self._get_history(),
            user_text]
            )
            self._history = []
        return prompt
    
    def warn(self, message):
        print(message)

    def fail(self, message):
        raise DBGError(message)
    
    ##########################################################################################################

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




    def _build_error(self):
        thread = self.get_thread(self._debugger)

        error_message = (thread.GetStopDescription(1024) if thread else None)
        if error_message:
            return (
                "Here is the reason the program stopped execution:\n```\n"
                + error_message
                + "\n```"
            )
        else:
            self.warn("could not generate an error message.")
            return ""


    def build_inputs(self):
        parts = []

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
            parts.append(
                "Here is the command line invocation that started the program:\n```\n"
                + command_line_invocation
                + "\n```"
            )
        else:
            self.warn("could not retrieve the command line invocation.")

        stream = lldb.SBStream()
        self._debugger.GetSetting("target.input-path").GetAsJSON(stream)
        entry = json.loads(stream.GetData())
    
        input_path = (entry if entry else None)
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
                self.warn("could not retrieve the input data.")

        return "\n\n".join(parts)
    

