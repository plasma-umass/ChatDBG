import os
import shutil
import textwrap

from rich import box
from rich.console import Console
from rich.live import Live
from rich.markdown import Markdown
from rich.panel import Panel
from rich.theme import Theme
from rich.table import Table

from chatdbg.util.text import fill_to_width, wrap_long_lines

from ..assistant.listeners import BaseAssistantListener


_theme = Theme(
        {
            "markdown.block": "black on light_steel_blue1",
            "markdown.paragraph": "black on light_steel_blue1",
            "markdown.text": "black on light_steel_blue1",
            "markdown.code": "blue",
            "markdown.code_block": "blue",
            "markdown.item.bullet": "bold blue",
            "markdown.item.number": "bold blue",
            "markdown.h1": "bold black",
            "markdown.h2": "bold black",
            "markdown.h3": "bold black",
            "markdown.h4": "bold black",
            "markdown.h5": "bold black",
            "command": "bold gray11 on wheat1",
            "result": "grey35 on wheat1",
        }
    )

_simple_box = Box = box.Box(
    "    \n"
    "    \n"
    "    \n"
    "    \n"
    "    \n"
    "    \n"
    "    \n"
    "    \n",
    ascii=True
)


class ChatDBGMarkdownPrinter(BaseAssistantListener):

    def __init__(
        self, out, debugger_prompt, chat_prefix, width
    ):
        self._out = out
        self._debugger_prompt = debugger_prompt
        self._chat_prefix = chat_prefix
        self._left_indent = 4
        self._width = shutil.get_terminal_size(fallback=(width, 24)).columns
        self._theme = _theme
        self._code_theme = "default"
        # used to keep track of streaming
        self._streamed = ""

        self._console = self._make_console(out)


    def _make_console(self, out):
        return Console(soft_wrap=False, file=out, theme=self._theme, width=self._width)
        
    # Call backs

    def on_begin_query(self, prompt, user_text):
        pass

    def on_end_query(self, stats):
        pass

    def _print(self, renderable, end=""):
        self._console.print(renderable, end=end)

    def _wrap_in_panel(self, rich_element):

        left_panel = Panel("", box=_simple_box, style="on default")
        right_panel = Panel(rich_element, box=_simple_box, style="black on light_steel_blue1")

        # Create a table to hold the panels side by side
        table = Table.grid(padding=0)
        table.add_column(justify="left", width=self._left_indent - 2)
        table.add_column(justify="left")
        table.add_row(left_panel, right_panel)
        return table

    def on_warn(self, text):
        self._print(textwrap.indent(text + "\n\n", "*** ", lambda _: True))

    def _stream_append(self, text):
        self._streamed += text
        m = self._wrap_in_panel(Markdown(self._streamed, code_theme=self._code_theme))
        self._live.update(m)

    def on_begin_stream(self):
        self._streamed = ""

    def on_stream_delta(self, text):
        if self._streamed == "":
            self._live = Live(vertical_overflow="visible", console=self._console)
            self._live.start(True)
        self._stream_append(text)

    def on_end_stream(self):
        if self._streamed != "":
            self._live.stop()

    def on_response(self, text):
        if self._streamed == "" and text != None:
            m = self._wrap_in_panel(Markdown(text, code_theme=self._code_theme))
            self._print(m, end="\n")
        self._streamed = ""

    def on_function_call(self, call, result):
        
        prefix=self._chat_prefix
        line = fill_to_width(f"\n{prefix}{self._debugger_prompt}{call}", self._width)
        entry = f"[command]{line}[/]\n"

        line_width = self._width - len(prefix) - self._left_indent - 2
        result = wrap_long_lines(result.expandtabs()+"\n", line_width, subsequent_indent='    ')
        result = fill_to_width(result, line_width)
        result = textwrap.indent(result, prefix, lambda _: True)
        full_response = f"[result]{result}[/]"
        entry += full_response
            
        m = self._wrap_in_panel(entry) 
        self._print(m, end='')
