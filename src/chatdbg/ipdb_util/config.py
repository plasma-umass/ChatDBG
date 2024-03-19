import os

from traitlets import Bool, Int, TraitError, Unicode
from traitlets.config import Configurable


def chat_get_env(option_name, default_value):
    env_name = "CHATDBG_" + option_name.upper()
    v = os.getenv(env_name, str(default_value))
    if type(default_value) == int:
        return int(v)
    elif type(default_value) == bool:
        return v.lower() == "true"
    else:
        return v


class Chat(Configurable):
    model = Unicode(
        chat_get_env("model", "gpt-4-1106-preview"), help="The OpenAI model"
    ).tag(config=True)
    # model = Unicode(default_value='gpt-3.5-turbo-1106', help="The OpenAI model").tag(config=True)
    debug = Bool(chat_get_env("debug", False), help="Log OpenAI calls").tag(config=True)
    log = Unicode(chat_get_env("log", "log.yaml"), help="The log file").tag(config=True)
    tag = Unicode(chat_get_env("tag", ""), help="Any extra info for log file").tag(
        config=True
    )
    rc_lines = Unicode(
        chat_get_env("rc_lines", "[]"), help="lines to run at startup"
    ).tag(config=True)

    context = Int(
        chat_get_env("context", 5),
        help="lines of source code to show when displaying stacktrace information",
    ).tag(config=True)
    show_locals = Bool(
        chat_get_env("show_locals", True), help="show local var values in stacktrace"
    ).tag(config=True)
    show_libs = Bool(
        chat_get_env("show_libs", False), help="show library frames in stacktrace"
    ).tag(config=True)
    show_slices = Bool(
        chat_get_env("show_slices", True), help="support the `slice` command"
    ).tag(config=True)
    take_the_wheel = Bool(
        chat_get_env("take_the_wheel", True), help="Let LLM take the wheel"
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
        }
