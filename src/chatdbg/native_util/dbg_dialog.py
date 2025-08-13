import sys

from . import clangd_lsp_integration
from ..util.prompts import (
    build_followup_prompt,
    build_initial_prompt,
    initial_instructions,
)

from ..assistant.assistant import Assistant
from ..util.config import chatdbg_config
from ..util.history import CommandHistory
from ..util.log import ChatDBGLog
from .stacks import build_enriched_stacktrace


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
        self._prompt = prompt
        self._history = CommandHistory(self._prompt)
        self._unsafe_cmd = False

    def query_and_print(self, assistant, user_text, is_followup):
        prompt = self.build_prompt(user_text, is_followup)

        self._history.clear()
        print(assistant.query(prompt, user_text)["message"])
        if self._unsafe_cmd:
            self.warn(
                f"Warning: One or more debugger commands were blocked as potentially unsafe.\nWarning: You can disable sanitizing with `config --unsafe` and try again at your own risk."
            )
            self._unsafe_cmd = False

    def dialog(self, user_text):
        assistant = self._make_assistant()
        self.check_debugger_state()

        self.query_and_print(assistant, user_text, False)
        while True:
            try:
                command = input("(ChatDBG chatting) ").strip()
                if command in ["exit", "quit"]:
                    break
                if command in ["chat", "why"]:
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
                        if command != "test_prompt":
                            self._history.append(command, result)
                        print(result)
            except EOFError:
                # If it causes an error, break
                break

        assistant.close()

    # Return string for valid command.  None if the command is not valid.
    def _run_one_command(self, command):
        pass

    def _message_is_a_bad_command_error(self, message):
        pass

    def check_debugger_state(self):
        pass

    def _get_frame_summaries(self, max_entries: int = 20):
        pass

    def initial_prompt_instructions(self):
        functions = self._supported_functions()
        return initial_instructions(functions)

    def _initial_prompt_enchriched_stack_trace(self):
        return build_enriched_stacktrace(self._get_frame_summaries())

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

    def build_prompt(self, arg, conversing):
        if not conversing:
            return build_initial_prompt(
                self._initial_prompt_enchriched_stack_trace(),
                self._initial_prompt_error_message(),
                self._initial_prompt_error_details(),
                self._initial_prompt_command_line(),
                self._initial_prompt_input(),
                self._prompt_history(),
                user_text=arg,
            )
        else:
            return build_followup_prompt(
                self._prompt_history(), self._prompt_stack(), arg
            )

    def llm_debug(self, command: str) -> str:
        pass

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
        functions = [self.llm_debug, self.llm_get_code_surrounding]
        if clangd_lsp_integration.is_available():
            functions += [self.llm_find_definition]
        return functions

    def _make_assistant(self) -> Assistant:

        functions = self._supported_functions()
        instruction_prompt = self.initial_prompt_instructions()

        # gdb overwrites sys.stdin to be a file object that doesn't seem
        # to support colors or streaming.  So, just use the original stdout
        # here for all subclasses.
        printer = chatdbg_config.make_printer(sys.__stdout__, self._prompt, "   ", 80)

        assistant = Assistant(
            instruction_prompt,
            model=chatdbg_config.model,
            functions=functions,
            listeners=[
                printer,
                self._log,
            ],
        )

        return assistant

    def warn(self, message):
        print(message)

    def fail(self, message):
        raise DBGError(message)
