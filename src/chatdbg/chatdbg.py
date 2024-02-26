#! /usr/bin/env python3

import importlib.metadata
import atexit
import inspect
import os
import pdb
import pydoc
import sys
import textwrap
import traceback
import json
from datetime import datetime
from io import StringIO

import llm_utils

from .assistant.assistant import Assistant

_config = {"model": "gpt-4-1106-preview", "debug": False, "log": "log.json", "tag": ""}

_basic_instructions = f"""\
You are a debugging assistant.  You will be given a Python stack trace for an
error and answer questions related to the root cause of the error.

Call the `pdb` function to run Pdb debugger commands on the stopped program. The
Pdb debugger keeps track of a current frame. You may call the `pdb` function
with the following strings:

    bt
            Print a stack trace, with the most recent frame at the bottom.
            An arrow indicates the "current frame", which determines the
            context of most commands. 

    up
            Move the current frame count one level up in the
            stack trace (to an older frame).
    down
            Move the current frame count one level down in the
            stack trace (to a newer frame).

    p expression
            Print the value of the expression.

    whatis expression
            Print the type of the expression.

    list
            List the source code for the current frame. 
            The current line in the current frame is indicated by "->".
            
Call the `info` function to get the documentation and source code for any
function that is visible in the current frame.

Call the `pdb` and `info` functions as many times as you would like.

Call `pdb` to print any variable value or expression that you believe may
contribute to the error.

Unless it is in a common, widely-used library, you MUST call `info` on any
function that is called in the code, that apppears in the argument list for a
function call in the code, or that appears on the call stack.  

The root cause of any error is likely due to a problem in the source code within
the {os.getcwd()} directory.

Keep your answers under about 8-10 sentences.  Conclude each response with
either a propopsed fix if you have identified the root cause or a bullet list of
1-3 suggestions for how to continue debugging.
"""


class CopyingTextIOWrapper:
    """
    File wrapper that will stash a copy of everything written.
    """

    def __init__(self, file):
        self.file = file
        self.buffer = StringIO()

    def write(self, data):
        self.buffer.write(data)
        return self.file.write(data)

    def getvalue(self):
        return self.buffer.getvalue()

    def getfile(self):
        return self.file

    def __getattr__(self, attr):
        # Delegate attribute access to the file object
        return getattr(self.file, attr)


class ChatDBGLog:
    def __init__(self):
        self.steps = []
        self.meta = {
            "time": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            "command_line": " ".join(sys.argv),
            "config": _config,
        }
        self._instructions = ""
        self.stdout_wrapper = CopyingTextIOWrapper(sys.stdout)
        self.stderr_wrapper = CopyingTextIOWrapper(sys.stderr)
        sys.stdout = self.stdout_wrapper
        sys.stderr = self.stdout_wrapper
        self.chat_step = None

    def dump(self):
        full_json = {
            "meta": self.meta,
            "steps": self.steps,
            "instructions": self._instructions,
            "stdout": self.stdout_wrapper.getvalue(),
            "stderr": self.stderr_wrapper.getvalue(),
        }
        with open(_config["log"], "w") as file:
            print(json.dumps(full_json, indent=2), file=file)

    def instructions(self, instructions):
        self._instructions = instructions

    def user_command(self, line, output):
        if self.chat_step != None:
            x = self.chat_step
            self.chat_step = None
        else:
            x = {"input": line, "output": {"type": "text", "output": output}}
        self.steps.append(x)

    def push_chat(self, line, full_prompt):
        self.chat_step = {
            "input": line,
            "full_prompt": full_prompt,
            "output": {"type": "chat", "outputs": []},
        }

    def pop_chat(self, tokens, cost, time):
        self.chat_step["stats"] = {"tokens": tokens, "cost": cost, "time": time}

    def message(self, text):
        self.chat_step["output"]["outputs"].append({"type": "text", "output": text})

    def function(self, line, output):
        x = {
            "type": "call",
            "input": line,
            "output": {"type": "text", "output": output},
        }
        self.chat_step["output"]["outputs"].append(x)


class ChatDBG(pdb.Pdb):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.prompt = "(ChatDBG pdb) "
        self.chat_prefix = "   "
        self.text_width = 80
        self._assistant = None
        self._history = []
        self._error_specific_prompt = ""

        self.log = ChatDBGLog()
        atexit.register(lambda: self.log.dump())

    def _is_user_file(self, file_name):
        return file_name.startswith(os.getcwd())

    def grab_active_call_from_frame(self, tb):
        """
        Extract the text for the function call currently running in the
        top-most frame in tb.
        """
        frame = tb.tb_frame
        lineno = pdb.lasti2lineno(frame.f_code, tb.tb_lasti)
        lines = inspect.getsourcelines(frame)[0]
        for index, line in enumerate(lines, frame.f_code.co_firstlineno):
            if index == lineno:
                leading_spaces = len(line) - len(line.lstrip())
                # Degrade gracefully when using older Python versions that don't have column info.
                try:
                    positions = inspect.getframeinfo(frame).positions
                    return line[positions.col_offset : positions.end_col_offset]
                except:
                    return line
        assert False

    def _hide_lib_calls(self, tb):
        """
        Remove all frames from the stack that are not part of
        the user code.  Return a tuple (tb,lib_entry) where
          - tb is the new traceback
          - lib_entry is the most recent user frame calling into
            a library.
        """
        head = tb
        if head != None:
            while head != None and not self._is_user_file(
                head.tb_frame.f_code.co_filename
            ):
                head = head.tb_next
            if head == None:
                return None, None
            tail = head
            lib_entry = None
            while tail.tb_next != None:
                tail_file_name = tail.tb_next.tb_frame.f_code.co_filename
                if self._is_user_file(tail_file_name):
                    tail = tail.tb_next
                    lib_entry = None
                else:
                    if lib_entry == None:
                        lib_entry = tail.tb_next
                    tail.tb_next = tail.tb_next.tb_next
        return head, tail if lib_entry != None else None

    def interaction(self, frame, tb):
        """
        Override to remove all lib code from the stack and create more
        precise details about where to look for the error.
        """
        tb_str = None
        if tb != None:
            tb, lib_entry_point = self._hide_lib_calls(tb)
            if lib_entry_point != None:
                tb_str = "".join(traceback.format_tb(tb))
                details = textwrap.dedent(
                    f"""\
                    An exception was raised during the call to
                    {self.grab_active_call_from_frame(lib_entry_point)}. The
                    root cause is most likely related to the arguments passed
                    into that function. You MUST look at the values passed in as
                    arguments and the specification for the function. You MUST
                    consider the order that the arguments are listed.\n"""
                )
            else:
                # Use last_type and last_value in this context -- see
                # docs for the sys package.
                if hasattr(sys, "last_type"):
                    exc_type, exc_value = sys.last_type, sys.last_value
                    tb_str = "".join(
                        traceback.format_exception(exc_type, exc_value, tb)
                    )
                    details = ""
                else:
                    tb_str = "".join(traceback.format_tb(tb))
                    details = ""
            prompt = f"Here is the stack trace for the error:\n{tb_str}\n{details}\n"
            self._error_specific_prompt = prompt
        else:
            # In this case, the tb will be None, and it will Pdb will populate the stack
            # with the full trace, including library functions, etc.  I don't see an
            # easy to remove them.  We could take a different approach and hide the
            # lib frames in Pdb's printing code, but that means they still exist if
            # we issue up/down commands...
            self._error_specific_prompt = ""

        # eventually make it so the log starts a new section each time we
        # do an interaction.
        super().interaction(frame, tb)

    # def _check_about_command(self, str):
    #     all_commands = self.completenames('')
    #     words = str.split(' ')
    #     word = words[0]
    #     if word in all_commands:
    #         if word in self._warned_keywords:
    #             return True
    #         else:
    #             self.message(f"It looks like you may be issuing a Pdb command.")
    #             self.message(f"If you did not mean to, start your line with :{word} instead of {word}.")
    #             self._warned_keywords.add(words[0])
    #             return True
    #     else:
    #         return True

    def onecmd(self, line: str) -> bool:
        """
        Override to stash the results in our history.
        """
        if not line:
            # blank -- let super call back to into onecmd
            return super().onecmd(line)
        else:
            hist_file = CopyingTextIOWrapper(self.stdout)
            self.stdout = hist_file
            try:
                self.was_chat = False
                return super().onecmd(line)
            finally:
                output = hist_file.getvalue()
                if line not in ["quit", "EOF"]:
                    self.log.user_command(line, output)
                if line not in ["hist", "test_prompt"] and not self.was_chat:
                    self._history += [(line, output)]
                self.stdout = hist_file.getfile()

    def message(self, msg) -> None:
        """
        Override to remove tabs for messages so we can indent them.
        """
        return super().message(str(msg).expandtabs())

    def error(self, msg) -> None:
        """
        Override to remove tabs for messages so we can indent them.
        """
        return super().error(str(msg).expandtabs())

    def _capture_onecmd(self, line):
        """
        Run one Pdb command, but capture and return stdout.
        """
        stdout = self.stdout
        lastcmd = self.lastcmd
        try:
            self.stdout = StringIO()
            super().onecmd(line)
            result = self.stdout.getvalue().rstrip()
            return result
        finally:
            self.stdout = stdout
            self.lastcmd = lastcmd

    def format_history_entry(self, entry, indent=""):
        line, output = entry
        # output = llm_utils.word_wrap_except_code_blocks(output,
        #                                                 self.text_width)
        if output:
            entry = f"(ChatDBG pdb) {line}\n{output}"
        else:
            entry = f"(ChatDBG pdb) {line}"
        return textwrap.indent(entry, indent, lambda _: True)

    def _clear_history(self):
        self._history = []

    def default(self, line):
        if line[:1] == "!":
            super().default(line)
        else:
            if line[:1] == ":":
                line = line[1:].strip()
            self.do_chat(line)

    def do_hist(self, arg):
        """hist
        Print the history of user-issued commands since the last chat.
        """
        entry_strs = [self.format_history_entry(x) for x in self._history]
        history_str = "\n".join(entry_strs)
        self.message(history_str)

    def do_pydoc(self, arg):
        """pydoc name
        Print the pydoc string for a name.
        """
        try:
            obj = self._getval(arg)
            if obj.__doc__ != None:
                pydoc.doc(obj, output=self.stdout)
            else:
                self.message(f"No documentation is available.")
        except NameError:
            # message already printed in _getval
            pass

    def do_info(self, arg):
        """info name
        Print the pydoc string (and source code, if available) for a name.
        """
        try:
            obj = self._getval(arg)
            if self._is_user_file(inspect.getfile(obj)):
                self.do_source(arg)
            else:
                self.do_pydoc(arg)
                self.message(
                    f"You MUST assume that `{arg}` is specified and implemented correctly."
                )
        except NameError:
            # message already printed in _getval
            pass
        except Exception:
            self.do_pydoc(arg)
            self.message(
                f"You MUST assume that `{arg}` is specified and implemented correctly."
            )

    def do_test_prompt(self, arg):
        """test_prompt
        [For debugging] Prints the prompts to be sent to the assistant.
        """
        self.message("Instructions:")
        self.message(self._instructions())
        self.message("-" * 80)
        self.message("Prompt:")
        self.message(self._prompt(arg))

    def _instructions(self):
        return _basic_instructions + "\n" + self._error_specific_prompt

    def _stack_prompt(self):
        stack_frames = textwrap.indent(self._capture_onecmd("bt"), "")
        stack = (
            textwrap.dedent(
                f"""
            This is the current stack.  
            The current frame is indicated by an arrow '>' at 
            the start of the line.
            ```"""
            )
            + f"\n{stack_frames}\n```"
        )
        return stack

    def _prompt(self, arg):
        if arg == "why":
            arg = "Explain the root cause of the error."

        user_prompt = ""
        if len(self._history) > 0:
            hist = textwrap.indent(self._capture_onecmd("hist"), "")
            self._clear_history()
            hist = f"This is the history of some pdb commands I ran and the results.\n```\n{hist}\n```\n"
            user_prompt += hist

        stack = self._stack_prompt()
        user_prompt += stack + "\n" + arg

        return user_prompt

    def do_chat(self, arg):
        """chat/:
        Send a chat message.
        """
        self.was_chat = True

        full_prompt = self._prompt(arg)

        if self._assistant == None:
            self._make_assistant()

        def client_print(line=""):
            line = llm_utils.word_wrap_except_code_blocks(line, self.text_width - 10)
            self.log.message(line)
            line = textwrap.indent(line, self.chat_prefix, lambda _: True)
            print(line, file=self.stdout, flush=True)

        self.log.push_chat(arg, full_prompt)
        tokens, cost, time = self._assistant.run(full_prompt, client_print)
        self.log.pop_chat(tokens, cost, time)

    def _make_assistant(self):
        def info(name):
            """
            {
                "name": "info",
                "description": "Get the documentation and source code (if available) for any function visible in the current frame",
                "parameters": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The name of the function to get the information for"
                        }
                    },
                    "required": [ "name"  ]
                }
            }
            """
            command = f"info {name}"
            result = self._capture_onecmd(command)
            self.log.function(command, result)
            self.message(
                self.format_history_entry((command, result), indent=self.chat_prefix)
            )
            return result

        def pdb(command):
            """
            {
                "name": "pdb",
                "description": "Run a pdb command and get the response.",
                "parameters": {
                    "type": "object",
                    "properties": {
                        "command": {
                            "type": "string",
                            "description": "The pdb command to run."
                        }
                    },
                    "required": [ "command"  ]
                }
            }
            """
            cmd = command if command != "list" else "ll"
            result = self._capture_onecmd(cmd)
            self.log.function(command, result)

            self.message(
                self.format_history_entry((command, result), indent=self.chat_prefix)
            )

            # help the LLM know where it is...
            result += self._stack_prompt()

            return result

        self._clear_history()
        instructions = self._instructions()

        self.log.instructions(instructions)

        self._assistant = Assistant(
            "ChatDBG",
            self._instructions(),
            model=_config["model"],
            debug=_config["debug"],
        )
        self._assistant.add_function(pdb)
        self._assistant.add_function(info)


_usage = f"""\
usage: chatdbg [-c command] ... [-m module | pyfile] [arg] ...

A Python debugger that uses AI to tell you `why`.
(version {importlib.metadata.metadata('chatdbg')['Version']})

https://github.com/plasma-umass/ChatDBG

Debug the Python program given by pyfile. Alternatively,
an executable module or package to debug can be specified using
the -m switch.

Initial commands are read from .pdbrc files in your home directory
and in the current directory, if they exist.  Commands supplied with
-c are executed after commands from .pdbrc files.

To let the script run until an exception occurs, use "-c continue".
You can then type `:why` to get an explanation of the root cause of
the exception, along with a suggested fix. NOTE: you must have an
OpenAI key saved as the environment variable OPENAI_API_KEY.
You can get a key here: https://openai.com/api/

You may also ask any other question that starts with the word `:`.

To let the script run up to a given line X in the debugged file, use
"-c 'until X'".

ChatDBG supports the following configuration flags before the 
pyfile or -m switch:
    --chat.model=<OpenAPI model>
    --chat.debug 
"""

_valid_models = [
    "gpt-4-turbo-preview",
    "gpt-4-0125-preview",
    "gpt-4-1106-preview",
    "gpt-3.5-turbo-0125",
    "gpt-3.5-turbo-1106",
    "gpt-4",  # no parallel calls
    "gpt-3.5-turbo",  # no parallel calls
]


def main():
    import getopt

    opts, args = getopt.getopt(
        sys.argv[1:], "mhc:", ["help", "command=", "chat.model=", "chat.debug"]
    )

    if any(opt in ["-h", "--help"] for opt, _ in opts):
        print(_usage)
        sys.exit()

    if not args:
        print(_usage)
        sys.exit(2)

    for o, a in opts:
        if o == "--chat.model":
            if a not in _valid_models:
                print(f"{a} is not supported.")
                print(f"The supported models are {_valid_models}.")
            _config["model"] = a
        elif o == "--chat.debug":
            _config["debug"] = True
        elif o in ["--chat.log", "--chat.tag"]:
            _config[o.split(".")[1]] = a
        elif o.startswith("--chat."):
            print(f"{o} not defined.")
            print(_usage)
            sys.exit(2)

    # drop all --chat options
    sys.argv[:] = [x for x in sys.argv if not x.startswith("--chat.")]

    pdb.Pdb = ChatDBG
    pdb.main()
