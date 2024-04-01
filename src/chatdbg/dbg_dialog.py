import json
import sys
import textwrap

from chatdbg.util.log import ChatDBGLog
from chatdbg.util.printer import ChatDBGPrinter
import lldb

import llm_utils

from assistant.assistant import Assistant
import clangd_lsp_integration
from util.config import chatdbg_config

from lldb_utils.prompts import build_prompt, get_thread
from lldb_utils.chat_history import History
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
        self._history = []

    def dialog(self, user_text):
        assistant = self._make_assistant()
        debuggable = self.check_debugger_state()

        user_text = (
            command if command else "What's the problem? Provide code to fix the issue."
        )

        stats = assistant.query(initial_prompt=build_initial_prompt(), user_text)
        print(f"\n[Cost: ~${stats['cost']:.2f} USD]")
        while True:
            try:
                command = input(">>> " + self.prompt).strip()
                if command == "exit" or command == "quit":
                    break
                if command == "chat" or command == "why":
                    # TODO: Pass in the history as part of the followup prompt
                    followup_prompt = build_followup_prompt(user_text)
                    stats = assistant.query(followup_prompt, user_text)
                    print(f"\n[Cost: ~${stats['cost']:.2f} USD]")
                elif command == "history":
                    self.history
                else:
                    # Send the next input as an LLDB command
                    result = run_one_command(command)
                    # If result is not a recognized command, pass it as a query
                    history.make_entry(command, result)
                    if result == None:
                        followup_prompt = build_followup_prompt(command)
                        stats = assistant.query(followup_prompt, command)
                        print(f"\n[Cost: ~${stats['cost']:.2f} USD]")
                    else:
                        print(result)
            except EOFError:
                # If it causes an error, break
                break

        assistant.close()

    def run_one_command(self, command):
        pass

    def check_debugger_state(self):
        pass

    def build_enriched_stacktrace(self):
        pass

    def build_error(self):
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
            return command, self.run_one_command(command)

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
            return f"code {filename}:{lineno}", self.run_one_command(
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
            return f"definition {filename}:{lineno} {symbol}", self.run_one_command(
                f"definition {filename}:{lineno} {symbol}"
            )

        functions = [llm_debug, llm_get_code_surrounding]
        if clangd_lsp_integration.is_available():
            functions += [llm_find_definition]

        instruction_prompt = build_instructions()

        assistant = Assistant(
            instruction_prompt,
            model=chatdbg_config.model,
            debug=chatdbg_config.debug,
            functions=functions,
            stream=chatdbg_config.stream,
            listeners=[
                ChatDBGPrinter(
                    sys.stdout,
                    PROMPT,  # must end with ' ' to match other tools
                    "   ",
                    80,
                    stream=chatdbg_config.stream,
                ),
                _log,
            ],
        )

        return assistant
    
    def warn(self, message):
        print(message)

    def fail(self, message):
        raise DBGError(message)
    
    ###################################################################

class LLDBDialog(DBGDialog):

    def __init__(self, prompt, debugger) -> None:
        super().__init__(prompt)
        self._debugger = debugger

    def run_one_command(self, command):
        interpreter = self._debugger.GetCommandInterpreter()
        result = lldb.SBCommandReturnObject()
        interpreter.HandleCommand(cmd, result)

        if result.Succeeded():
            return result.GetOutput()
        else:
            return "Error: " + result.GetError()

    def check_debugger_state(self):
        if not self._debugger.GetSelectedTarget():
            self.fail("must be attached to a program to use `chat`.")

        elif not is_debug_build(self._debugger):
            self.fail(
                "your program must be compiled with debug information (`-g`) to use `chat`."
            )

        thread = get_thread(self._debugger)
        if not thread:
            self.fail("must run the code first to use `chat`.")

        if not clangd_lsp_integration.is_available():
            self.warn(
                "`clangd` was not found. The `find_definition` function will not be made available."
            )

    def build_enriched_stacktrace(self):
        pass

    def build_error(self):
        pass

    def build_inputs(self):
        pass

    