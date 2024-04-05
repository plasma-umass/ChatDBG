import sys

import llm_utils
import native_util.clangd_lsp_integration as clangd_lsp_integration
from util.prompts import (build_followup_prompt, build_initial_prompt,
                          initial_instructions)

from ..assistant.assistant import Assistant
from ..util.config import chatdbg_config
from ..util.history import CommandHistory
from ..util.log import ChatDBGLog
from ..util.printer import ChatDBGPrinter
from .stacks import _FrameSummaryEntry, _SkippedFramesEntry


class DBGError(Exception):

    def __init__(self, message):
        self.message = message
        super().__init__(self.message)


class DBGDialog:
    # The log file used by the listener on the Assistant
    _log = ChatDBGLog(
        log_filename=chatdbg_config.log,
        config=chatdbg_config.to_json(),
        capture_streams=False,  # don't have access to target's stdout/stderr here.
    )

    def __init__(self, prompt) -> None:
        self.prompt = prompt
        self._history = CommandHistory()

    def query_and_print(self, assistant, user_text, is_followup):
        prompt = self._build_prompt(user_text, is_followup)
        print(assistant.query(prompt, user_text)["message"])

    def dialog(self, user_text):
        assistant = self._make_assistant()
        self.check_debugger_state()

        self.query_and_print(assistant, user_text, False)
        while True:
            try:
                command = input(">>> " + self.prompt).strip()
                
                if command in [ "exit", "quit" ]:
                    break
                if command in [ "chat", "why" ]:
                    self.query_and_print(assistant, command, True)
                elif command == "history":
                    print(self._history)
                else:
                    # Send the next input as an LLDB command
                    result = self._run_one_command(command)
                    if self._message_is_a_bad_command_error(result):
                        # If result is not a recognized command, pass it as a query
                        self.query_and_print(assistant, command, True)
                    else:
                        self._history.append(command, result)
                        print(result)
            except EOFError:
                # If it causes an error, break
                break

        assistant.close()

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

    def _initial_prompt_instructions(self):
        functions = self._supported_functions()
        return initial_instructions(functions)

    def _initial_prompt_enchriched_stack_trace(self):
        return self._build_enriched_stacktrace()

    def _initial_prompt_error_message(self):
        return None

    def _initial_prompt_error_details(self):
        """Anything more beyond the initial error message to include."""
        return None

    def _initial_prompt_command_line(self):
        return None

    def _initial_prompt_input(self):
        return None

    def _prompt_stack(self):
        """
        Return a simple backtrace to show the LLM where we are on the stack
        in followup prompts.
        """
        return None

    def _prompt_history(self):
        return str(self._history)

    def _build_prompt(self, arg, conversing):
        if not conversing:
            return build_initial_prompt(self._initial_prompt_enchriched_stack_trace(),
                                 self._initial_prompt_error_message(),
                                 self._initial_prompt_error_details(),
                                 self._initial_prompt_command_line(),
                                 self._initial_prompt_input(),
                                 self._prompt_history(),
                                 None,
                                 arg)
        else:
            return build_followup_prompt(self._prompt_history(), 
                                         self._prompt_stack(), 
                                         arg)


    # TODO: Factor out the name of the debugger that's embedded in the doc string...
    def llm_debug(self, command: str) -> str:
        """
        {
            "name": "debug",
            "description": "The `debug` function runs an LLDB command on the stopped program and gets the response.",
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

    def llm_get_code_surrounding(self, filename: str, line_number: int) -> str:
        """
        {
            "name": "get_code_surrounding",
            "description": "The `get_code_surrounding` function returns the source code in the given file surrounding and including the provided line number.",
            "parameters": {
                "type": "object",
                "properties": {
                    "filename": {
                        "type": "string",
                        "description": "The filename to read from."
                    },
                    "line_number": {
                        "type": "integer",
                        "description": "The line number to focus on. Some context before and after that line will be provided."
                    }
                },
                "required": [ "filename", "line_number" ]
            }
        }
        """
        return f"code {filename}:{line_number}", self._run_one_command(
            f"code {filename}:{line_number}"
        )

    def llm_find_definition(self, filename: str, line_number: int, symbol: str) -> str:
        """
        {
            "name": "find_definition",
            "description": "The `find_definition` function returns the source code for the definition for the given symbol at the given source line number.  Call `find_definition` on every symbol that could be linked to the issue.",
            "parameters": {
                "type": "object",
                "properties": {
                    "filename": {
                        "type": "string",
                        "description": "The filename the symbol is from."
                    },
                    "line_number": {
                        "type": "integer",
                        "description": "The line number where the symbol is present."
                    },
                    "symbol": {
                        "type": "string",
                        "description": "The symbol to lookup."
                    }
                },
                "required": [ "filename", "line_number", "symbol" ]
            }
        }
        """
        return f"definition {filename}:{line_number} {symbol}", self._run_one_command(
            f"definition {filename}:{line_number} {symbol}"
        )

    def _supported_functions(self):
        functions = [ self.llm_debug, self.llm_get_code_surrounding]
        if clangd_lsp_integration.is_available():
            functions += [ self.llm_find_definition ]
        return functions

    def _make_assistant(self) -> Assistant:

        functions = self._supported_functions()
        instruction_prompt = self._initial_prompt_instructions()

        assistant = Assistant(
            instruction_prompt,
            model=chatdbg_config.model,
            debug=chatdbg_config.debug,
            functions=functions,
            stream=not chatdbg_config.no_stream,
            listeners=[
                ChatDBGPrinter(
                    sys.stdout,
                    self.prompt,  # must end with ' ' to match other tools
                    "   ",
                    80,
                    stream=not chatdbg_config.no_stream,
                ),
                self._log,
            ],
        )

        return assistant
        
    def warn(self, message):
        print(message)

    def fail(self, message):
        raise DBGError(message)
