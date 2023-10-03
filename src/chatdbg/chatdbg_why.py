import os
import sys
import textwrap

import chatdbg_utils
import openai


def why(self, arg):
    user_prompt = "Explain what the root cause of this error is, given the following source code and traceback, and generate code that fixes the error."
    user_prompt += "\n"
    user_prompt += "source code:\n```\n"
    stack_trace = ""
    stack_frames = len(self.stack)
    try:
        import sys

        exception_name = sys.exc_info()[0].__name__
        exception_value = sys.exc_info()[1]
    except:
        print(
            "The command 'why' only works when there is an uncaught exception. Try running 'python3 -m chatdbg -c continue'."
        )
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
        completion = openai.ChatCompletion.create(
            model=model,
            request_timeout=30,
            messages=[{"role": "user", "content": user_prompt}],
        )
        text = completion.choices[0].message.content
        print(chatdbg_utils.word_wrap_except_code_blocks(text))
    except openai.error.AuthenticationError:
        print(
            "You need a valid OpenAI key to use ChatDBG. You can get a key here: https://openai.com/api/"
        )
        print("Set the environment variable OPENAI_API_KEY to your key value.")
