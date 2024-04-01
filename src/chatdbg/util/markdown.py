
import textwrap
from ..assistant.listeners import BaseAssistantListener
from rich.console import Console
from rich.live import Live
from rich.markdown import *
from rich.panel import Panel
from rich.theme import Theme
from rich.style import Style
from rich.text import Text
from rich import box
import os

_theme = Theme({
    "markdown.paragraph": 'bright_cyan',
    "markdown.text": 'bright_cyan',
    # "markdown.em": Style(italic=True),
    # "markdown.emph": Style(italic=True),  # For commonmark backwards compatibility
    # "markdown.strong": Style(bold=True),
    "markdown.code": "white",
    "markdown.code_block": Style(color="cyan"),
    # "markdown.block_quote": Style(color="magenta"),
    # "markdown.list": Style(color="cyan"),
    # "markdown.item": Style(),
    "markdown.item.bullet": Style(color="cyan", bold=True),
    "markdown.item.number": Style(color="cyan", bold=True),
    # "markdown.hr": Style(color="yellow"),
    # "markdown.h1.border": Style(),
    "markdown.h1": Style(color="bright_cyan", bold=True),
    "markdown.h2": Style(color="bright_cyan", bold=True),
    "markdown.h3": Style(color="bright_cyan", bold=True),
    "markdown.h4": Style(color="bright_cyan", bold=True),
    "markdown.h5": Style(color="bright_cyan", bold=True),
    # "markdown.link": Style(color="bright_blue"),
    # "markdown.link_url": Style(color="blue", underline=True),
    # "markdown.s": Style(strike=True)
})

class Heading(TextElement):
    """A heading."""

    @classmethod
    def create(cls, markdown: "Markdown", token: Token) -> "Heading":
        return cls(token.tag)

    def on_enter(self, context: "MarkdownContext") -> None:
        self.text = Text()
        context.enter_style(self.style_name)

    def __init__(self, tag: str) -> None:
        self.tag = tag
        self.style_name = f"markdown.{tag}"
        super().__init__()

    def __rich_console__(
        self, console: Console, options: ConsoleOptions
    ) -> RenderResult:
        text = self.text
        if self.tag == "h2":
            yield Text("")
        yield text


class CodeBlock(TextElement):
    """A code block with syntax highlighting."""

    style_name = "markdown.code_block"

    @classmethod
    def create(cls, markdown: "Markdown", token: Token) -> "CodeBlock":
        node_info = token.info or ""
        lexer_name = node_info.partition(" ")[0]
        return cls(lexer_name or "text", markdown.code_theme)

    def __init__(self, lexer_name: str, theme: str) -> None:
        self.lexer_name = lexer_name

        self.theme = theme

    def __rich_console__(
        self, console: Console, options: ConsoleOptions
    ) -> RenderResult:
        code = str(self.text).rstrip()
        syntax = Syntax(
            code, self.lexer_name, theme=self.theme, word_wrap=True, padding=0
        )
        yield syntax

class ChatDBGMarkdownPrinter(BaseAssistantListener):
    def __init__(self, out, debugger_prompt, chat_prefix, width, stream=False):
        self._out = out
        self._debugger_prompt = debugger_prompt
        self._chat_prefix = chat_prefix
        self._width = min(width, os.get_terminal_size().columns - len(chat_prefix))
        self._stream = stream
        self._console = Console(soft_wrap=False, file=out, theme=_theme)
        Markdown.elements['fence'] = CodeBlock
        Markdown.elements['code_block'] = CodeBlock
        Markdown.elements["heading_open"] = Heading


    # Call backs

    def on_begin_query(self, prompt, user_text):
        pass

    def on_end_query(self, stats):
        pass

    def _print(self, text, **kwargs):
        self._console.print(
            text, end=''
        )

    def _wrap_in_panel(self, rich_element):
        return Panel(rich_element, box=box.MINIMAL, padding=(0, 0, 0, len(self._chat_prefix)-1))

    def on_warn(self, text):
        self._print(textwrap.indent(text, "*** "))

    def on_fail(self, text):
        self._print(textwrap.indent(text, "*** "))

    def on_begin_stream(self):
        self._live = Live(vertical_overflow='visible', console=self._console)
        self._live.start(True)
        self._streamed = ''

    def _stream_append(self, text):
        self._streamed += text
        m = self._wrap_in_panel(Markdown(self._streamed))
        self._live.update(m)

    def on_stream_delta(self, text):
        if self._streamed == '':
           text = "\n" + text
        self._stream_append(text)

    def on_end_stream(self):
        self._live.stop()

    def on_response(self, text):
        if not self._stream and text != None:
            m = self._wrap_in_panel(Markdown(text))
            self._console.print(m)

    def on_function_call(self, call, result):
        entry = f"[bold bright_yellow]{self._debugger_prompt}{call}[/]"
        if result and len(result) > 0:
            entry += f"\n[yellow]{result}[/]"
        m = textwrap.indent(entry.rstrip()+'\n', prefix=self._chat_prefix)
        self._print(m)
