import lldb

import llm_utils

import clangd_lsp_integration
from util.config import chatdbg_config

from dbg_dialog import LLDBDialog, DBGError


# The file produced by the panic handler if the Rust program is using the chatdbg crate.
RUST_PANIC_LOG_FILENAME = "panic_log.txt"
PROMPT = "(ChatDBG lldb) "


def __lldb_init_module(debugger: lldb.SBDebugger, internal_dict: dict) -> None:
    debugger.HandleCommand(f"settings set prompt '{PROMPT}'")


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
        dialog = LLDBDialog(PROMPT, debugger)
        dialog.dialog(command)
    except Exception as e:
        result.setError(e.message)


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
