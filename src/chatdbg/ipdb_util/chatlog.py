import atexit
from io import StringIO
from datetime import datetime
import uuid
import sys
import yaml
from pydantic import BaseModel
from typing import List, Union, Optional
from ..assistant.assistant import AbstractAssistantClient



class CopyingTextIOWrapper:
    """
    File wrapper that will stash a copy of everything written.
    """

    def __init__(self, file):
        self.file = file
        self.buffer = StringIO()

    def write(self, data):
        self.buffer.write(data)
        return self.file.write(data)

    def getvalue(self):
        return self.buffer.getvalue()

    def getfile(self):
        return self.file

    def __getattr__(self, attr):
        # Delegate attribute access to the file object
        return getattr(self.file, attr)


class ChatDBGLog(AbstractAssistantClient):
    def __init__(self, log_filename, config, capture_streams=True):
        self._log_filename = log_filename
        self.config = config
        if capture_streams:
            self._stdout_wrapper = CopyingTextIOWrapper(sys.stdout)
            self._stderr_wrapper = CopyingTextIOWrapper(sys.stderr)
            sys.stdout = self._stdout_wrapper
            sys.stderr = self._stdout_wrapper
        else:
            self._stderr_wrapper = None
            self._stderr_wrapper = None

        meta = {
                'time': datetime.now(),
                'command_line':  " ".join(sys.argv),
                'uid': str(uuid.uuid4()),
                'config': self.config
        }
        log = {
            'steps':[],
            'meta':meta,
            'instructions':None,
            'stdout':self._stdout_wrapper.getvalue(),
            'stderr':self._stderr_wrapper.getvalue(),
        }
        self._current_chat = None
        self._log = log

    def _dump(self):
        log = self._log

        def total(key):
            return sum(
                x['stats'][key] for x in log['steps'] if x['output']['type'] == 'chat' and 'stats' in x['output']
            )

        log['meta']['total_tokens'] = total("tokens")
        log['meta']['total_time'] = total("time")
        log['meta']['total_cost'] = total("cost")

        print(f"*** Writing ChatDBG dialog log to {self._log_filename}")

        with open(self._log_filename, "a") as file:
            def literal_presenter(dumper, data):
                if "\n" in data:
                    return dumper.represent_scalar("tag:yaml.org,2002:str", data, style="|")
                else:
                    return dumper.represent_scalar("tag:yaml.org,2002:str", data)

            yaml.add_representer(str, literal_presenter)
            yaml.dump([ log ], file, default_flow_style=False, indent=2)

    def begin_dialog(self, instructions):
        log = self._log
        assert log != None
        log['instructions'] = instructions

    def end_dialog(self):
        if self._log != None:
            self._dump()
        self._log = None

    def begin_query(self, prompt, user_text):
        log = self._log
        assert log != None
        assert self._current_chat == None
        self._current_chat = {
            'input':user_text,
            'prompt':prompt,
            'output': { 'type': 'chat', 'outputs': []}
        }

    def end_query(self, stats):
        log = self._log
        assert log != None
        assert self._current_chat != None
        log['steps'] += [ self._current_chat ]
        self._current_chat = None

    def _post(self, text, kind):
        log = self._log
        assert log != None
        if self._current_chat != None:
            self._current_chat['output']['outputs'].append({ 'type': 'text', 'output': f"*** {kind}: {text}"})
        else:
            log['steps'].append({ 'type': 'call', 'input': f"*** {kind}", 'output': { 'type': 'text', 'output': text } })

    def warn(self, text):
        self._post(text, "Warning")

    def fail(self, text):
        self._post(text, "Failure")

    def response(self, text):
        log = self._log
        assert log != None
        assert self._current_chat != None
        self._current_chat['output']['outputs'].append({ 'type': 'text', 'output': text})

    def function_call(self, call, result):
        log = self._log
        assert log != None
        if self._current_chat != None:
            self._current_chat['output']['outputs'].append({ 'type': 'call', 'input': call, 'output': { 'type': 'text', 'output': result }})
        else:
            log['steps'].append({ 'type': 'call', 'input': call, 'output': { 'type': 'text', 'output': result }})
