import os

_intro = f"""\
You are a debugging assistant. You will be given a Python stack trace for an
error and answer questions related to the root cause of the error.
"""

_pdb_function = f"""\
Call the `pdb` function to run Pdb debugger commands on the stopped program. You 
may call the `pdb` function to run the following commands: `bt`, `up`, `down`, 
`p expression`, `list`.

Call `pdb` to print any variable value or expression that you believe may
contribute to the error.
"""


_info_function = """\
Call the `info` function to get the documentation and source code for any
variable, function, package, class, method reference, field reference, or 
dotted reference visible in the current frame.  Examples include: n, e.n  
where e is an expression, and t.n where t is a type.

Unless it is from a common, widely-used library, you MUST call `info` exactly once on any
symbol that is referenced in code leading up to the error.  
"""


_slice_function = """\
Call the `slice` function to get the code used to produce
the value currently stored a variable.  You MUST call `slice` exactly once on any
variable used but not defined in the current frame's code.
"""

_take_the_wheel_instructions = """\
Call the provided functions as many times as you would like.
"""

_general_instructions = f"""\
The root cause of any error is likely due to a problem in the source code from the user.  

Explain why each variable contributing to the error has been set 
to the value that it has.

Continue with your explanations until you reach the root cause of the error. Your answer may be as long as necessary.

End your answer with a section titled "##### Recommendation\\n" that contains one of:
* a fix if you have identified the root cause
* a numbered list of 1-3 suggestions for how to continue debugging if you have not
"""


_wheel_and_slice = f"""\
{_intro}
{_pdb_function}
{_info_function}
{_slice_function}
{_take_the_wheel_instructions}
{_general_instructions}
"""

_wheel_no_slice = f"""\
{_intro}
{_pdb_function}
{_info_function}
{_take_the_wheel_instructions}
{_general_instructions}
"""

_no_wheel = f"""\
{_intro}
{_general_instructions}
"""


def pdb_instructions(supports_flow, take_the_wheel):
    if take_the_wheel:
        if supports_flow:
            return _wheel_and_slice
        else:
            return _wheel_no_slice
    else:
        return _no_wheel

####

def wrap_it(before, x, after = ""):
    if before:
        before += ':\n'
    if after:
        after += '\n'
    if x:
        return f"{before}```\n{x}\n```\n{after}"
    else:
        return ''

def concat_prompt(*args):
    args = [a for a in args if a]
    return "\n".join(args)

def build_initial_prompt(stack, error, details, command_line, inputs, history, extra='', user_text=''):
    return concat_prompt(
        wrap_it("The program has this stack trace", stack),
        wrap_it("The program encountered the following error", error, details),
        wrap_it("These were the command line options", command_line),
        wrap_it("This was the program's input", inputs),
        wrap_it("This is the history of some debugger commands I ran", history),
        extra,
        user_text
    )
def build_followup_prompt(history, extra, user_text):
    return concat_prompt(
        wrap_it("This is the history of some debugger commands I ran", history),
        extra,
        wrap_it(user_text)
    )
