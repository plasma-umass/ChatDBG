import os
import sys

import llm_utils
import openai

from . import chatdbg_utils


def why(self, arg):
    user_prompt = "Explain what the root cause of this error is, given the following source code and traceback, and generate code that fixes the error."
    user_prompt += "\n"
    user_prompt += "source code:\n```\n"
    stack_trace = ""
    stack_frames = len(self.stack)
    try:
        exception_name = sys.exc_info()[0].__name__
        exception_value = sys.exc_info()[1]
    except:
        print("The command 'why' only works when there is an uncaught exception.")
        print(" Try running 'python3 -m chatdbg -c continue'.")
        return
    for frame_lineno in self.stack:
        import inspect

        frame, lineno = frame_lineno
        # Only include frames for files in the same directory as the program being debugged.
        # TODO: add a --program-path option as in Scalene
        if not frame.f_code.co_filename.startswith(os.path.dirname(sys.argv[0])):
            stack_frames -= 1
            continue
        try:
            # user_prompt += '#' + '-' * 60 + '\n'
            lines = inspect.getsourcelines(frame)[0]
            for index, line in enumerate(lines, frame.f_code.co_firstlineno):
                user_prompt += "  "
                user_prompt += line.rstrip() + "\n"
                if index == lineno:
                    leading_spaces = len(line) - len(line.lstrip())
                    stack_trace += f"{stack_frames}: " + line.strip() + "\n"
                    # Degrade gracefully when using older Python versions that don't have column info.
                    try:
                        positions = inspect.getframeinfo(frame).positions
                    except:
                        positions = None
                    if positions:
                        stack_trace += (
                            " " * len(str(stack_frames))
                            + "  "
                            + " " * (positions.col_offset - leading_spaces)
                            + "^" * (positions.end_col_offset - positions.col_offset)
                            + "\n"
                        )
                if index >= lineno:
                    break
        except:
            pass
        stack_frames -= 1
    user_prompt += "```\n"
    user_prompt += "stack trace:\n"
    user_prompt += f"```\n{stack_trace}```\n"
    user_prompt += f"Exception: {exception_name} ({exception_value})\n"

    model = chatdbg_utils.get_model()
    if not model:
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
