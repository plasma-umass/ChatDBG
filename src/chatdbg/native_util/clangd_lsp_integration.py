import json
import os
import subprocess
import urllib.parse

import llm_utils


def _to_lsp_request(id, method, params):
    request = {"jsonrpc": "2.0", "id": id, "method": method}
    if params:
        request["params"] = params

    content = json.dumps(request)
    header = f"Content-Length: {len(content)}\r\n\r\n"
    return header + content


# Same as a request, but without an id.
def _to_lsp_notification(method, params):
    request = {"jsonrpc": "2.0", "method": method}
    if params:
        request["params"] = params

    content = json.dumps(request)
    header = f"Content-Length: {len(content)}\r\n\r\n"
    return header + content


def _parse_lsp_response(id, file):
    # Ignore all messages until the response with the correct id is found.
    while True:
        header = {}
        while True:
            line = file.readline().strip()
            if not line:
                break
            key, value = line.split(":", 1)
            header[key.strip()] = value.strip()

        content = file.read(int(header["Content-Length"]))
        response = json.loads(content)
        if "id" in response and response["id"] == id:
            return response


def _path_to_uri(path):
    return "file://" + os.path.abspath(path)


def uri_to_path(uri):
    data = urllib.parse.urlparse(uri)

    assert data.scheme == "file"
    assert not data.netloc
    assert not data.params
    assert not data.query
    assert not data.fragment

    path = data.path
    if path.startswith(os.getcwd()):
        path = os.path.relpath(path, os.getcwd())
    return urllib.parse.unquote(path)  # clangd seems to escape paths.


def is_available(executable="clangd"):
    try:
        clangd = subprocess.run(
            [executable, "--version"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        return clangd.returncode == 0
    except FileNotFoundError:
        return False


class clangd:
    def __init__(
        self,
        executable="clangd",
        working_directory=os.getcwd(),
        stderr=subprocess.DEVNULL,
    ):
        self.id = 0
        self.process = subprocess.Popen(
            [executable],
            text=True,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=stderr,
            cwd=working_directory,
        )
        self.initialize()

    def __del__(self):
        self.process.terminate()

    def initialize(self):
        self.id += 1
        request = _to_lsp_request(self.id, "initialize", {"processId": os.getpid()})
        self.process.stdin.write(request)
        self.process.stdin.flush()
        return _parse_lsp_response(self.id, self.process.stdout)
        # TODO: Assert there is no error.

    def didOpen(self, filename, languageId):
        with open(filename, "r") as file:
            text = file.read()

        notification = _to_lsp_notification(
            "textDocument/didOpen",
            {
                "textDocument": {
                    "uri": _path_to_uri(filename),
                    "languageId": languageId,
                    "version": 1,
                    "text": text,
                }
            },
        )
        self.process.stdin.write(notification)
        self.process.stdin.flush()

    def didClose(self, filename):
        notification = _to_lsp_notification(
            "textDocument/didClose", {"textDocument": {"uri": _path_to_uri(filename)}}
        )
        self.process.stdin.write(notification)
        self.process.stdin.flush()

    def definition(self, filename, line, character):
        self.id += 1
        request = _to_lsp_request(
            self.id,
            "textDocument/definition",
            {
                "textDocument": {"uri": _path_to_uri(filename)},
                "position": {
                    # Things are 0-indexed in LSP.
                    "line": line - 1,
                    "character": character - 1,
                },
            },
        )
        self.process.stdin.write(request)
        self.process.stdin.flush()
        return _parse_lsp_response(self.id, self.process.stdout)


def native_definition(command):
    if not is_available():
        return "`clangd` was not found. The `definition` function will not be made available."
    last_space_index = command.rfind(" ")
    if last_space_index == -1:
        return "usage: definition <filename>:<lineno> <symbol>"
    filename_lineno = command[:last_space_index]
    symbol = command[last_space_index + 1 :]
    parts = filename_lineno.split(":")
    if len(parts) != 2:
        return "usage: definition <filename>:<lineno> <symbol>"
    filename, lineno = parts[0], int(parts[1])

    try:
        with open(filename, "r") as file:
            lines = file.readlines()
    except FileNotFoundError:
        return f"file '{filename}' not found."

    if lineno - 1 >= len(lines):
        return "symbol not found at that location."

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
        return "symbol not found at that location."

    _clangd = None
    if is_available():
        _clangd = clangd()

    _clangd.didOpen(filename, "c" if filename.endswith(".c") else "cpp")
    definition = _clangd.definition(filename, lineno, character + 1)
    _clangd.didClose(filename)

    if "result" not in definition or not definition["result"]:
        return "No definition found."

    path = uri_to_path(definition["result"][0]["uri"])
    start_lineno = definition["result"][0]["range"]["start"]["line"] + 1
    end_lineno = definition["result"][0]["range"]["end"]["line"] + 1
    lines, first = llm_utils.read_lines(path, start_lineno - 5, end_lineno + 5)
    content = llm_utils.number_group_of_lines(lines, first)
    line_string = (
        f"line {start_lineno}"
        if start_lineno == end_lineno
        else f"lines {start_lineno}-{end_lineno}"
    )
    return f"""File '{path}' at {line_string}:\n```\n{content}\n```"""
