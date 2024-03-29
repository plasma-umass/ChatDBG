import argparse
from io import StringIO
import os
import json
import textwrap
from typing import Any, List, Optional, Tuple, Union

from chatdbg.util.log import ChatDBGLog
from chatdbg.util.printer import ChatDBGPrinter
import lldb

import llm_utils

from assistant.assistant import Assistant
import chatdbg_utils
import clangd_lsp_integration
from util.config import chatdbg_config

from lldb_utils.enriched_stack import *


# The file produced by the panic handler if the Rust program is using the chatdbg crate.
RUST_PANIC_LOG_FILENAME = "panic_log.txt"
PROMPT = "(ChatDBG lldb)"


def __lldb_init_module(debugger: lldb.SBDebugger, internal_dict: dict) -> None:
    debugger.HandleCommand(f"settings set prompt '{PROMPT} '")


def is_debug_build(debugger: lldb.SBDebugger) -> bool:
    """Returns False if not compiled with debug information."""
    target = debugger.GetSelectedTarget()
    if not target:
        return False
    for module in target.module_iter():
        for cu in module.compile_unit_iter():
            for line_entry in cu:
                if line_entry.GetLine() > 0:
                    return True
    return False


@lldb.command("print-test")
def print_test(
    debugger: lldb.SBDebugger,
    command: str,
    result: lldb.SBCommandReturnObject,
    internal_dict: dict,
) -> None:
    """print all variables in a run while recursing through pointers, keeping track of seen addresses"""

    args = command.split()
    recurse_max = 3
    help_string = "Usage: print-test [recurse_max]\n\nrecurse_max: The maximum number of times to recurse through nested structs or pointers to pointers. Default: 3"
    if len(args) > 1 or (len(args) == 1 and args[0] == "--help"):
        print(help_string)
        return
    elif len(args) == 1:
        try:
            recurse_max = int(args[0])
        except ValueError as e:
            print("recurse_max value could not be parsed: %s\n" % args[0])
            return
        if recurse_max < 1:
            print("recurse_max value must be at least 1.\n")
            return
    frame = (
        lldb.debugger.GetSelectedTarget()
        .GetProcess()
        .GetSelectedThread()
        .GetSelectedFrame()
    )

    all_vars = []
    addresses = {}
    for var in frame.get_all_variables():
        # Returns python dictionary for each variable, converts to JSON
        variable = _val_to_json(
            var, recurse_max, addresses
        )
        js = json.dumps(variable, indent=4)
        all_vars.append(js)

    # Print all addresses and JSON objects
    # print(addresses)
    for j in all_vars:
        print(j)
    return

def _val_to_json(
    var: lldb.SBValue,
    recurse_max: int,
    address_book: dict,
) -> dict:
    # Store address
    address_book.setdefault(str(var.GetAddress()), var.GetName())

    json = {}
    json["name"] = var.GetName()
    json["type"] = var.GetTypeName()
    # Dereference pointers
    if "*" in var.GetType().GetName():
        if var.GetValueAsUnsigned() != 0:
            value = "->"
            try:
                deref_val = var.Dereference()
                # If dereferenced value is "seen", then get name from address book
                if str(deref_val.GetAddress()) in address_book:
                    json["value"] = address_book[str(deref_val.GetAddress())]
                else:
                    # Recurse up to max_recurse times
                    for i in range(recurse_max - 1):
                        if "*" in deref_val.GetType().GetName():
                            value += "->"
                            deref_val = deref_val.Dereference()
                        elif len(deref_val.GetType().get_fields_array()) > 0:
                            value = _val_to_json(
                                deref_val,
                                recurse_max - i - 1,
                                address_book,
                            )
                            break
                        else:
                            break
                    # Append to -> string or not, depending on type of value
                    if isinstance(value, dict):
                        json["value"] = value
                    else:
                        json["value"] = (
                            value + str(deref_val)[str(deref_val).find("= ") + 2 :]
                        )
            except Exception as e:
                json["value"] = value + "Exception"
        else:
            json["value"] = "nullptr"
    # Recurse through struct fields
    elif len(var.GetType().get_fields_array()) > 0:
        fields = []
        for i in range(var.GetNumChildren()):
            f = var.GetChildAtIndex(i)
            fields.append(
                _val_to_json(
                    f,
                    recurse_max - 1,
                    address_book,
                )
            )
        json["value"] = fields
    else:
        json["value"] = str(var)[str(var).find("= ") + 2 :]
    return json


def _capture_onecmd(debugger, cmd):
    interpreter = debugger.GetCommandInterpreter()
    result = lldb.SBCommandReturnObject()
    interpreter.HandleCommand(cmd, result)

    if result.Succeeded():
        return result.GetOutput()
    else:
        return result.GetError()

def _instructions():
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

@lldb.command("debug")
def _function_lldb(
    debugger: lldb.SBDebugger,
    command: str,
    result: lldb.SBCommandReturnObject,
    internal_dict: dict,
) -> None:
    result.AppendMessage(_capture_onecmd(debugger, command))


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

# The log file used by the listener on the Assistant
_log = ChatDBGLog(
    log_filename=chatdbg_config.log,
    config=chatdbg_config.to_json(),
    capture_streams=False,  # don't have access to target's stdout/stderr here.
)

class LLDBPrinter(ChatDBGPrinter):
    def __init__(self, debugger_prompt, chat_prefix, width, stream=False):
        super().__init__(StringIO(), debugger_prompt, chat_prefix, width, stream)

    def get_output_and_reset(self):
        result = self.out.getvalue()
        self.out = StringIO()
        return result

_lldb_printer = LLDBPrinter(
                PROMPT + ' ',   # must end with ' ' to match other tools
                '   ',
                80,
                stream=chatdbg_config.stream,
            )



def _make_assistant(
    debugger: lldb.SBDebugger,
    result: lldb.SBCommandReturnObject,
) -> Assistant:

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
        return command, _capture_onecmd(debugger, f"debug {command}")

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
        return f"code {filename}:{lineno}", _capture_onecmd(debugger, f"code {filename}:{lineno}")

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
        return f"definition {filename}:{lineno} {symbol}", _capture_onecmd(debugger, f"definition {filename}:{lineno} {symbol}")

    functions = [ llm_debug, llm_get_code_surrounding ]
    if not clangd_lsp_integration.is_available():
        result.AppendWarning(
            "`clangd` was not found. The `find_definition` function will not be made available."
        )
    else:
        functions += [ llm_find_definition ]

    instruction_prompt = _instructions()

    assistant = Assistant(
        instruction_prompt,
        model=chatdbg_config.model,
        debug=chatdbg_config.debug,
        functions=functions,
        stream=chatdbg_config.stream,
        listeners=[
            _lldb_printer,
            _log
        ],
    )

    return assistant


_assistant = None


@lldb.command("chat")
def chat(
    debugger: lldb.SBDebugger,
    command: str,
    result: lldb.SBCommandReturnObject,
    internal_dict: dict,
):
    if not debugger.GetSelectedTarget():
        result.SetError("must be attached to a program to use `chat`.")
        return
    if not is_debug_build(debugger):
        result.SetError(
            "your program must be compiled with debug information (`-g`) to use `chat`."
        )
        return
    thread = get_thread(debugger)
    if not thread:
        result.SetError("must run the code first to use `chat`.")
        return

    args, remaining = chatdbg_utils.parse_known_args(command.split())

    global _assistant

    parts = []

    if _assistant == None:
        error_message = get_error_message(debugger)
        if not error_message:
            result.AppendWarning("could not generate an error message.")
        else:
            parts.append(
                "Here is the reason the program stopped execution:\n```\n"
                + error_message
                + "\n```"
            )

        summaries = get_frame_summaries(debugger)
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
                [
                    s.count() if isinstance(s, SkippedFramesEntry) else 1
                    for s in summaries
                ]
            )

            if total_frames > 1000:
                parts.append(
                    "Note that there are over 1000 frames in the stack trace, hinting at a possible stack overflow error."
                )

        max_initial_locations_to_send = 3
        source_code_entries = []
        for summary in summaries:
            if isinstance(summary, FrameSummaryEntry):
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

        command_line_invocation = get_command_line_invocation(debugger)
        if command_line_invocation:
            parts.append(
                "Here is the command line invocation that started the program:\n```\n"
                + command_line_invocation
                + "\n```"
            )
        else:
            result.AppendWarning("could not retrieve the command line invocation.")

        input_path = get_input_path(debugger)
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

    user_text = " ".join(remaining) if remaining else "What's the problem? Provide code to fix the issue."

    parts.append(user_text)

    prompt = "\n\n".join(parts)

    if not _assistant: # or args.fresh:
        _assistant = _make_assistant(debugger, result)

    # TODO: make the 
    _assistant.query(prompt, user_text)
    result.AppendMessage(_lldb_printer.get_output_and_reset())



@lldb.command("repl")
def repl(
    debugger: lldb.SBDebugger,
    command: str,
    result: lldb.SBCommandReturnObject,
    internal_dict: dict,
):
    if command.strip():
        result.AppendWarning("`repl` does not take any arguments (arguments ignored).")

    while True:
        try:
            command = input(f"{PROMPT} ").strip()
        except EOFError:
            break

        if command == "exit":
            break
        result = _capture_onecmd(debugger, command)
        print("-----------------------------------")
        print(result, end="")
        print("-----------------------------------")


"""

def why() -> just goes to chat

def chat(line):
   make assistant
   run the query
   while True:
      line = input()
      if line is a nother why or chat line:
          run another query and pass in history as part of prompt and reset history
      elif line is done:
          break
      else:
          run it as a command
          match response:
             case "command not recognized" -> run as query
             case error -> report error   # check on what result objects look like...
             case anything else -> print it
                and record history (input, result output)
    



"""