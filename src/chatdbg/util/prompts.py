import json
import os
from chatdbg.util.config import chatdbg_config
from pdb_util.text import truncate_proportionally

def _wrap_it(before, x, after = "", maxlen=2048):
    if x:
        x = truncate_proportionally(x, maxlen, 0.5)
        if before:
            before += ':\n'
        if after:
            after += '\n'
        return f"{before}```\n{x}\n```\n{after}"
    else:
        return ''

def _concat_prompt(*args):
    args = [a for a in args if a]
    return "\n".join(args)

def _user_text_it(user_text):
    return user_text if user_text else "What's the error, and give me a fix."

def build_initial_prompt(stack, error, details, command_line, inputs, history, extra='', user_text=''):
    return _concat_prompt(
        _wrap_it("The program has this stack trace", stack),
        _wrap_it("The program encountered the following error", error, details),
        _wrap_it("This was the command line", command_line),
        _wrap_it("This was the program's input", inputs),
        _wrap_it("This is the history of some debugger commands I ran", history),
        extra,
        _user_text_it(user_text)
    )
def build_followup_prompt(history, extra, user_text):
    return _concat_prompt(
        _wrap_it("This is the history of some debugger commands I ran", history),
        extra,
        _user_text_it(user_text)
    )


def initial_instructions(functions):
    if chatdbg_config.instructions == '':
        file_path = os.path.join(os.path.dirname(__file__), 'instructions.txt')
    else:
        file_path = chatdbg_config.instructions

    function_instructions = [ json.loads(f.__doc__)['description'] for f in functions ]
    with open(file_path, 'r') as file:
        template = file.read()
        return template.format_map({'functions' : "\n\n".join(function_instructions)})
