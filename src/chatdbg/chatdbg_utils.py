import os
import textwrap

import openai

import llm_utils

import argparse
from typing import Any, Optional
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


def use_argparse(full_command):
    description = textwrap.dedent(
        rf"""
                [b]ChatDBG[/b]: A Python debugger that uses AI to tell you `why`.
                [blue][link=https://github.com/plasma-umass/ChatDBG]https://github.com/plasma-umass/ChatDBG[/link][/blue]

                usage:
                [b]chatdbg [-c command] ... [-m module | pyfile] [arg] ...[/b]

                Debug the Python program given by pyfile. Alternatively,
                an executable module or package to debug can be specified using
                the -m switch.

                Initial commands are read from .pdbrc files in your home directory
                and in the current directory, if they exist.  Commands supplied with
                -c are executed after commands from .pdbrc files.

                To let the script run until an exception occurs, use "-c continue".
                You can then type `why` to get an explanation of the root cause of
                the exception, along with a suggested fix. NOTE: you must have an
                OpenAI key saved as the environment variable OPENAI_API_KEY.
                You can get a key here: https://openai.com/api/

                To let the script run up to a given line X in the debugged file, use
                "-c 'until X'".
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
        help=textwrap.dedent(
            """
                the language model to use, e.g., 'gpt-3.5-turbo' or 'gpt-4'
                the default mode tries gpt-4-turbo-preview and falls back to gpt-4
            """
        ).strip(),
    )
    parser.add_argument(
        "-p",
        "--show-prompt",
        action="store_true",
        help="when enabled, only print prompt and exit (for debugging purposes)",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=60,
        help="the timeout for API calls in seconds",
    )
    # This is only used in the conversation mode.
    parser.add_argument(
        "--max-error-tokens",
        type=int,
        default=1920,
        help="the maximum number of tokens from the error message to send in the prompt",
    )

    args = parser.parse_args(full_command)
    return args


def get_model() -> str:
    all_models = [
        "gpt-3.5-turbo-1106",
        "gpt-3.5-turbo",
        "gpt-3.5-turbo-0613",
        "gpt-3.5-turbo-0301",
        "gpt-3.5-turbo-16k",
        "gpt-3.5-turbo-16k-0613",
        "gpt-4-1106-preview",
        "gpt-4",
        "gpt-4-0314",
        "gpt-4-32k",
        "gpt-4-32k-0314",
    ]
    if not "OPENAI_API_MODEL" in os.environ:
        model = "gpt-4-1106-preview"
    else:
        model = os.environ["OPENAI_API_MODEL"]
        if model not in all_models:
            print(
                f'The environment variable OPENAI_API_MODEL is currently set to "{model}".'
            )
            print(f"The only valid values are {all_models}.")
            return ""

    return model


def explain(source_code: str, traceback: str, exception: str, really_run=True) -> None:
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

    model = get_model()
    if not model:
        return

    input_tokens = llm_utils.count_tokens(model, user_prompt)

    if not really_run:
        print(user_prompt)
        print(f"Total input tokens: {input_tokens}")
        return

    try:
        client = openai.OpenAI(timeout=30)
    except openai.OpenAIError:
        print("You need an OpenAI key to use this tool.")
        print("You can get a key here: https://platform.openai.com/api-keys")
        print("Set the environment variable OPENAI_API_KEY to your key value.")
        return

    try:
        completion = client.chat.completions.create(
            model=model, messages=[{"role": "user", "content": user_prompt}]
        )
    except openai.NotFoundError:
        print(f"'{model}' either does not exist or you do not have access to it.")
        return
    except openai.RateLimitError:
        print("You have exceeded a rate limit or have no remaining funds.")
        return
    except openai.APITimeoutError:
        print("The OpenAI API timed out.")
        return

    text = completion.choices[0].message.content
    print(llm_utils.word_wrap_except_code_blocks(text))

    input_tokens = completion.usage.prompt_tokens
    output_tokens = completion.usage.completion_tokens
    cost = llm_utils.calculate_cost(input_tokens, output_tokens, model)
    print(f"\n(Total cost: approximately ${cost:.2f} USD.)")
