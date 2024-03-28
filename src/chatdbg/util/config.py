import os

from traitlets import Bool, Int, Unicode
from traitlets.config import Configurable


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
        _chatdbg_get_env("stream", False), help="Stream the response at it arrives"
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


chatdbg_config: ChatDBGConfig = ChatDBGConfig()
