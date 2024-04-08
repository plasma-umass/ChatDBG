import argparse
import os

from traitlets import Bool, Int, Unicode
from traitlets.config import Configurable

from chatdbg.util.markdown import ChatDBGMarkdownPrinter
from chatdbg.util.printer import ChatDBGPrinter

from io import StringIO
from types import *
from typing import *

from chatdbg.util.jupyter import ChatDBGJupyterPrinter

def _chatdbg_get_env(option_name: str, default_value: Union[int, str, bool]) -> Union[int, str, bool]:
    env_name = "CHATDBG_" + option_name.upper()
    v = os.getenv(env_name, str(default_value))
    if type(default_value) == int:
        return int(v)
    elif type(default_value) == bool:
        return v.lower() == "true"
    else:
        return v


class DBGParser(argparse.ArgumentParser):
    def __init__(self, **kwargs) -> None:
        super().__init__(**kwargs)

    def error(self, message):
        raise Exception(f"Error: {message}\n")


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

    no_stream = Bool(
        _chatdbg_get_env("batch", False), help="Do not stream the LLM responses"
    ).tag(config=True)

    format = Unicode(
        _chatdbg_get_env("format", "md"),
        help="The output format (text or md or md:simple or jupyter).",
    ).tag(config=True)

    instructions = Unicode(
        _chatdbg_get_env("instructions", ""),
        help="the File for the instructions, or '' for the default version.",
    ).tag(config=True)

    _user_configurable = [debug, log, model, no_stream, format]

    def _parser(self):
        parser = DBGParser(add_help=False)

        for trait in self._user_configurable:
            name = f"--{trait.name}"
            value = self._trait_values[trait.name]
            t = type(value)
            if t == bool:
                parser.add_argument(name, default=value, action="store_true")
            else:
                parser.add_argument(name, default=value, type=t)

        return parser

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
            "no_stream": self.no_stream,
            "format": self.format,
            "instructions": self.instructions
        }

    def parse_user_flags(self, argv):

        args, unknown_args = self._parser().parse_known_args(argv)

        for x in self._user_configurable:
            self.set_trait(x.name, getattr(args, x.name))

        return unknown_args

    def user_flags_help(self):
        return "\n".join(
            [
                self.class_get_trait_help(x, self).replace("ChatDBGConfig.", "")
                for x in self._user_configurable
            ]
        )

    def user_flags(self):
        return "\n".join(
            [
                f"  --{x.name:10}{self._trait_values[x.name]}"
                for x in self._user_configurable
            ]
        )

    def parse_only_user_flags(self, args):
        try:
            unknown = chatdbg_config.parse_user_flags(args)
            if unknown:
                return (
                    f"Unrecognized arguments: {' '.join(unknown)}\n\n"
                    + f"ChatDBG arguments:\n\n{self.user_flags_help()}"
                )
            return chatdbg_config.user_flags()
        except Exception as e:
            return str(e) + f"\nChatDBG arguments:\n\n{self.user_flags_help()}"

    def make_printer(self, stdout, prompt, prefix, width):
        format = chatdbg_config.format
        split = format.split(":")
        if split[0] == "md":
            theme = split[1] if len(split) == 2 else None
            return ChatDBGMarkdownPrinter(
                stdout, prompt, prefix, width, theme=theme
            )
        elif format == 'text':
            return ChatDBGPrinter(stdout, prompt, prefix, width)
        elif format == 'jupyter':
            return ChatDBGJupyterPrinter(prompt, prefix, width)
        else:
            print("*** Unknown format '{format}'.  Defaulting to 'text'", file=stdout)
            return ChatDBGPrinter(stdout, prompt, prefix, width)


chatdbg_config: ChatDBGConfig = ChatDBGConfig()
