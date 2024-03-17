import argparse
import textwrap
import time
from typing import Any, Callable, List, Optional, Tuple

import llm_utils
import openai
from rich.console import Console


class RichArgParser(argparse.ArgumentParser):
    def __init__(self, *args: Any, **kwargs: Any):
        self.console = Console(highlight=False)
        super().__init__(*args, **kwargs)

    def _print_message(self, message: Optional[str], file: Any = None) -> None:
        if message:
            self.console.print(message)


class ChatDBGArgumentFormatter(argparse.HelpFormatter):
    # RawDescriptionHelpFormatter.
    def _fill_text(self, text, width, indent):
        return "".join(indent + line for line in text.splitlines(keepends=True))

    # RawTextHelpFormatter.
    def _split_lines(self, text, width):
        return text.splitlines()

    # ArgumentDefaultsHelpFormatter.
    # Ignore if help message is multiline.
    def _get_help_string(self, action):
        help = action.help
        if "\n" not in help and "%(default)" not in action.help:
            if action.default is not argparse.SUPPRESS:
                defaulting_nargs = [argparse.OPTIONAL, argparse.ZERO_OR_MORE]
                if action.option_strings or action.nargs in defaulting_nargs:
                    help += " (default: %(default)s)"
        return help


def parse_known_args(argv: List[str]) -> Tuple[argparse.Namespace, List[str]]:
    description = textwrap.dedent(
        rf"""
            [b]ChatDBG[/b]: A Python debugger that uses AI to tell you `why`.
            [blue][link=https://github.com/plasma-umass/ChatDBG]https://github.com/plasma-umass/ChatDBG[/link][/blue]
        """
    ).strip()
    parser = RichArgParser(
        prog="chatdbg",
        usage=argparse.SUPPRESS,
        description=description,
        formatter_class=ChatDBGArgumentFormatter,
    )
    parser.add_argument(
        "--llm",
        type=str,
        default="gpt-4-turbo-preview",
        help="the language model to use, e.g., 'gpt-3.5-turbo' or 'gpt-4'",
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="when enabled, only print prompt and exit with `why`, and output to a log file with `chat`",
    )
    parser.add_argument(
        "--tool-call-max-result-tokens",
        default=512,  # Arbitrary.
        help="the maximum number of tokens to send in any tool call response",
    )
    parser.add_argument(
        "--fresh",
        action="store_true",
        default=False,
        help="in `chat` mode, reset the chat history to start a new conversation",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=60,
        help="the timeout for API calls in seconds",
    )

    return parser.parse_known_args(argv)


def explain(
    source_code: str,
    traceback: str,
    exception: str,
    args: argparse.Namespace,
    append_message: Callable[[str], None] = print,
    append_warning: Callable[[str], None] = print,
    set_error: Callable[[str], None] = print,
) -> None:
    user_prompt = f"""
Explain what the root cause of this error is, given the following source code
context for each stack frame and a traceback, and propose a fix. In your
response, never refer to the frames given below (as in, 'frame 0'). Instead,
always refer only to specific lines and filenames of source code.

Source code for each stack frame:
```
{source_code}
```

Traceback:
{traceback}

Stop reason: {exception}
    """.strip()

    input_tokens = llm_utils.count_tokens(args.llm, user_prompt)

    if args.debug:
        append_message(f"{user_prompt}\n\nTotal input tokens: {input_tokens}.")
        return

    start = time.time()

    try:
        client = openai.OpenAI(timeout=args.timeout)
    except openai.OpenAIError:
        append_warning("you need an OpenAI key to use this tool.")
        append_warning("You can get a key here: https://platform.openai.com/api-keys.")
        append_warning("set the environment variable OPENAI_API_KEY to your key value.")
        return

    try:
        completion = client.chat.completions.create(
            model=args.llm, messages=[{"role": "user", "content": user_prompt}]
        )
    except openai.NotFoundError:
        set_error(f"'{args.llm}' does not exist or you do not have access to it.")
        return
    except openai.RateLimitError:
        set_error("you have exceeded a rate limit or have no remaining funds.")
        return
    except openai.APITimeoutError:
        set_error("the OpenAI API timed out.")
        return

    text = completion.choices[0].message.content
    elapsed = time.time() - start
    input_tokens = completion.usage.prompt_tokens
    output_tokens = completion.usage.completion_tokens
    cost = llm_utils.calculate_cost(input_tokens, output_tokens, args.llm)

    append_message(
        llm_utils.word_wrap_except_code_blocks(text)
        + "\n\n"
        + f"Elapsed time: {elapsed:.2f} seconds"
        + "\n"
        + f"Total cost: {cost:.2f}$"
    )
