import sys
import uuid
from datetime import datetime

import yaml

from ..assistant.listeners import BaseAssistantListener
from ..pdb_util.capture import CaptureOutput
from .wrap import word_wrap_except_code_blocks


class ChatDBGLog(BaseAssistantListener):

    def __init__(self, log_filename, config, capture_streams=True):
        self._log_filename = log_filename
        self.config = config
        if capture_streams:
            self._stdout_wrapper = CaptureOutput(sys.stdout)
            self._stderr_wrapper = CaptureOutput(sys.stderr)
            sys.stdout = self._stdout_wrapper
            sys.stderr = self._stdout_wrapper
        else:
            self._stderr_wrapper = None
            self._stderr_wrapper = None

        self._log = self._make_log()
        self._current_chat = None

    def _make_log(self):
        meta = {
            "time": datetime.now(),
            "command_line": " ".join(sys.argv),
            "uid": str(uuid.uuid4()),
            "config": self.config,
        }
        return {
            "steps": [],
            "meta": meta,
            "instructions": None,
            "stdout": (
                None
                if self._stderr_wrapper == None
                else self._stdout_wrapper.getvalue()
            ),
            "stderr": (
                None
                if self._stderr_wrapper == None
                else self._stderr_wrapper.getvalue()
            ),
        }

    def _dump(self):
        log = self._log

        def total(key):
            return sum(
                x["stats"][key]
                for x in log["steps"]
                if x["output"]["type"] == "chat" and "stats" in x["output"]
            )

        log["meta"]["total_tokens"] = total("tokens")
        log["meta"]["total_time"] = total("time")
        log["meta"]["total_cost"] = total("cost")

        print(f"*** Writing ChatDBG dialog log to {self._log_filename}")

        with open(self._log_filename, "a") as file:

            def literal_presenter(dumper, data):
                if "\n" in data:
                    return dumper.represent_scalar(
                        "tag:yaml.org,2002:str", data, style="|"
                    )
                else:
                    return dumper.represent_scalar("tag:yaml.org,2002:str", data)

            yaml.add_representer(str, literal_presenter)
            yaml.dump([log], file, default_flow_style=False, indent=2)

    def on_begin_dialog(self, instructions):
        log = self._log
        assert log != None
        log["instructions"] = instructions

    def on_end_dialog(self):
        if self._log != None:
            self._dump()
        self._log = self._make_log()

    def on_begin_query(self, prompt, extra):
        log = self._log
        assert log != None
        assert self._current_chat == None
        self._current_chat = {
            "input": extra,
            "prompt": prompt,
            "output": {"type": "chat", "outputs": []},
        }

    def on_end_query(self, stats):
        log = self._log
        assert log != None
        assert self._current_chat != None
        log["steps"] += [self._current_chat]
        log["stats"] = stats
        self._current_chat = None

    def _post(self, text, kind):
        log = self._log
        assert log != None
        if self._current_chat != None:
            self._current_chat["output"]["outputs"].append(
                {"type": "text", "output": f"*** {kind}: {text}"}
            )
        else:
            log["steps"].append(
                {
                    "type": "call",
                    "input": f"*** {kind}",
                    "output": {"type": "text", "output": text},
                }
            )

    def on_warn(self, text):
        self._post(text, "Warning")

    def on_response(self, text):
        log = self._log
        assert log != None
        assert self._current_chat != None
        text = word_wrap_except_code_blocks(text)
        self._current_chat["output"]["outputs"].append({"type": "text", "output": text})

    def on_function_call(self, call, result):
        log = self._log
        assert log != None
        if self._current_chat != None:
            self._current_chat["output"]["outputs"].append(
                {
                    "type": "call",
                    "input": call,
                    "output": {"type": "text", "output": result},
                }
            )
        else:
            log["steps"].append(
                {
                    "type": "call",
                    "input": call,
                    "output": {"type": "text", "output": result},
                }
            )
