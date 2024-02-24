import json
import os
import subprocess
import urllib.parse


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


def _uri_to_path(uri):
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
