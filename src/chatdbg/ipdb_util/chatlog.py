import atexit
from io import StringIO
from datetime import datetime
import uuid
import sys
import yaml
from pydantic import BaseModel
from typing import List, Union, Optional

class Output(BaseModel):
    type: str
    output: Optional[str] = None

class FunctionOutput(Output):
    type: str = "call"

class TextOutput(Output):
    type: str = "text"

class ChatOutput(BaseModel):
    type: str = "chat"
    outputs: List[Output] = []

class Stats(BaseModel):
    tokens: int = 0
    cost: float = 0.0
    time: float = 0.0

    class Config:
        extra = 'allow'


class Step(BaseModel):
    input: str
    output: Union[TextOutput, ChatOutput]
    full_prompt: Optional[str] = None
    stats: Optional[Stats] = None

class Meta(BaseModel):
    time: datetime
    command_line: str
    uid: str
    config: str
    mark: str = "?"
    total_tokens: int = 0
    total_time: float = 0.0
    total_cost: float = 0.0

class Log(BaseModel):
    meta: Meta
    steps: List[Step]
    instructions: str
    stdout: Optional[str]
    stderr: Optional[str]

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


class ChatDBGLog:
    def __init__(self, log_filename, config, capture_streams=True):
        self.meta = Meta(
            time=datetime.now(),
            command_line=" ".join(sys.argv),
            uid=str(uuid.uuid4()),
            config=config
        )
        self.steps = []
        self.log_filename = log_filename
        self._instructions = ""
        if capture_streams:
            self.stdout_wrapper = CopyingTextIOWrapper(sys.stdout)
            self.stderr_wrapper = CopyingTextIOWrapper(sys.stderr)
            sys.stdout = self.stdout_wrapper
            sys.stderr = self.stdout_wrapper
        else:
            self.stderr_wrapper = None
            self.stderr_wrapper = None
        self.chat_step = None
        atexit.register(lambda: self.dump())

    def total(self, key):
        return sum(
            getattr(x.stats, key) for x in self.steps if x.output.type == "chat" and x.stats is not None
        )

    def dump(self):
        self.meta.total_tokens = self.total("tokens")
        self.meta.total_time = self.total("time")
        self.meta.total_cost = self.total("cost")

        full_log = Log(
            meta=self.meta,
            steps=self.steps,
            instructions=self._instructions,
            stdout=self.stdout_wrapper.getvalue(),
            stderr=self.stderr_wrapper.getvalue(),
            events=self.events
        )

        print(f"*** Write ChatDBG log to {self.log_filename}")
        with open(self.log_filename, "a") as file:
            def literal_presenter(dumper, data):
                return dumper.represent_scalar("tag:yaml.org,2002:str", data, style="|")

            yaml.add_representer(str, literal_presenter)
            yaml.dump(full_log, file, default_flow_style=False, indent=4)

    def instructions(self, instructions):
        self._instructions = instructions

    def user_command(self, line, output):
        if self.chat_step is not None:
            x = self.chat_step
            self.chat_step = None
        else:
            x = Step(input=line, output=TextOutput(output=output))
        self.steps.append(x)

    def push_chat(self, line, full_prompt):
        self.chat_step = Step(
            input=line,
            full_prompt=full_prompt,
            output=ChatOutput()
        )

    def pop_chat(self, stats):
        if self.chat_step is not None:
            self.chat_step.stats = Stats(**stats)
