import argparse
import os
import textwrap

from traitlets import Bool, Int, Unicode
from traitlets.config import Configurable, Config


def _chatdbg_get_env(option_name, default_value):
    env_name = "CHATDBG_" + option_name.upper()
    v = os.getenv(env_name, str(default_value))
    if type(default_value) == int:
        return int(v)
    elif type(default_value) == bool:
        return v.lower() == "true"
    else:
        return v


class ChatDBGConfig(Configurable):
    model = Unicode(
        _chatdbg_get_env("model", "gpt-4-1106-preview"), help="The LLM model"
    ).tag(config=True)

    debug = Bool(_chatdbg_get_env("debug", False), help="Log LLM calls").tag(
        config=True
    )

    log = Unicode(_chatdbg_get_env("log", "log.yaml"), help="The log file").tag(
        config=True
    )

    tag = Unicode(_chatdbg_get_env("tag", ""), help="Any extra info for log file").tag(
        config=True
    )
    rc_lines = Unicode(
        _chatdbg_get_env("rc_lines", "[]"), help="lines to run at startup"
    ).tag(config=True)

    context = Int(
        _chatdbg_get_env("context", 10),
        help="lines of source code to show when displaying stacktrace information",
    ).tag(config=True)

    show_locals = Bool(
        _chatdbg_get_env("show_locals", True),
        help="show local var values in stacktrace",
    ).tag(config=True)

    show_libs = Bool(
        _chatdbg_get_env("show_libs", False), help="show library frames in stacktrace"
    ).tag(config=True)

    show_slices = Bool(
        _chatdbg_get_env("show_slices", True), help="support the `slice` command"
    ).tag(config=True)

    take_the_wheel = Bool(
        _chatdbg_get_env("take_the_wheel", True), help="Let LLM take the wheel"
    ).tag(config=True)

    stream = Bool(
        _chatdbg_get_env("stream", True), help="Stream the response at it arrives"
    ).tag(config=True)

    def to_json(self):
        """Serialize the object to a JSON string."""
        return {
            "model": self.model,
            "debug": self.debug,
            "log": self.log,
            "tag": self.tag,
            "rc_lines": self.rc_lines,
            "context": self.context,
            "show_locals": self.show_locals,
            "show_libs": self.show_libs,
            "show_slices": self.show_slices,
            "take_the_wheel": self.take_the_wheel,
            "stream": self.stream,
        }


    def parse_user_flags(self, argv):
        parser = argparse.ArgumentParser(add_help=False)
        parser.add_argument('--debug', help="dump the LLM messages to a chatdbg.log", default=self.debug, action='store_true')
        parser.add_argument('--log',  help="where to write the log of the debugging session", default=self.log, type=str)
        parser.add_argument('--model', help="the LLM model to use", default=self.model, type=str)
        # parser.add_argument('--stream', help="stream responses from the LLM", default=self.stream, action='store_true')

        args, unknown_args = parser.parse_known_args(argv)
        
        self.debug = args.debug
        self.log = args.log
        self.model = args.model
        # self.stream = args.stream

        return unknown_args

    def user_flags_help(self):
        return textwrap.indent(textwrap.dedent(f"""\
              --debug         dump the LLM messages to a chatdbg.log
              --log=file      where to write the log of the debugging session
              --model=model   the LLM model to use
            """), '  ')

    def user_flags(self):
        return textwrap.indent(textwrap.dedent(f"""\
                debug:  {self.debug}
                log:    {self.log}
                model:  {self.model}
                """), '  ')



chatdbg_config: ChatDBGConfig = ChatDBGConfig()
