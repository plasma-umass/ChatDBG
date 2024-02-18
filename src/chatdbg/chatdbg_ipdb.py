"""
c.InteractiveShellApp.exec_lines = [
     'from IPython.core.debugger import Pdb',
     'from chatdbg.chatdbg_ipdb import IChatDBG, Chat',
     'get_ipython().InteractiveTB.debugger_cls = IChatDBG',
     'print("Loaded ChatDBG.")'
     ]

c.InteractiveShellApp.extensions = ['chatdbg.chatdbg_ipdb']

"""

from IPython.terminal.debugger import TerminalPdb, Pdb
from IPython.core.getipython import get_ipython

from colors import strip_color
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

from traitlets.config import Configurable
from traitlets import Unicode, Bool, Int

_valid_models = [
    'gpt-4-turbo-preview', 
    'gpt-4-0125-preview', 
    'gpt-4-1106-preview', 
    'gpt-3.5-turbo-0125', 
    'gpt-3.5-turbo-1106',
    'gpt-4',         # no parallel calls
    'gpt-3.5-turbo'  # no parallel calls
]

class Chat(Configurable):
    model = Unicode(default_value='gpt-4-1106-preview', help="The OpenAI model").tag(config=True)
    debug = Bool(default_value=False, help="Log OpenAI calls").tag(config=True)
    log = Unicode(default_value='log.json', help="The log file").tag(config=True)
    tag = Unicode(default_value='', help="Any extra info for log file").tag(config=True)
    context = Int(default_value=5, help='lines of source code to show when displaying stacktrace information').tag(config=True)

    def to_json(self):
        """Serialize the object to a JSON string."""
        return {
            'model': self.model,
            'debug': self.debug,
            'log': self.log,
            'tag': self.tag,
            'context': self.context
        }


def load_ipython_extension(ipython):
    # Create an instance of your configuration class with IPython's config
    global _config
    _config = Chat(config=ipython.config)


_basic_instructions=f"""\
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

    list
            List the source code for the current frame. 
            The current line in the current frame is indicated by "->".
            
Call the `info` function to get the documentation and source code for any
function that is visible in the current frame.

Call the `pdb` and `info` functions as many times as you would like.

Call `pdb` to print any variable value or expression that you believe may
contribute to the error.

Unless it is from a common, widely-used library, you MUST call `info` on any
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
        self.steps = [ ]
        self.meta = {
            'time' : datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            'command_line' : ' '.join(sys.argv),
            'config' : _config.to_json(),
        }
        self._instructions = ''
        self.stdout_wrapper = CopyingTextIOWrapper(sys.stdout)
        self.stderr_wrapper = CopyingTextIOWrapper(sys.stderr)
        sys.stdout = self.stdout_wrapper
        sys.stderr = self.stdout_wrapper
        self.chat_step = None

    def dump(self): 
        full_json = {
            'meta' : self.meta,
            'steps' : self.steps,
            'instructions' : self._instructions,
            'stdout' : self.stdout_wrapper.getvalue(),
            'stderr' : self.stderr_wrapper.getvalue()
        }
        with open(_config.log, 'w') as file:
            print(json.dumps(full_json, indent=2), file=file)

    def instructions(self, instructions):
        self._instructions = instructions

    def user_command(self, line, output): 
        if self.chat_step != None:
            x = self.chat_step
            self.chat_step = None
        else:
            x = {
                'input' : line,
                'output' : {
                    'type' : 'text',
                    'output' : output
                }
            }
        self.steps.append(x)

    def push_chat(self, line, full_prompt):
        self.chat_step = {
            'input' : line,
            'full_prompt' : full_prompt,
            'output' : {
                'type' : 'chat',
                'outputs' : [ ]
            }
        }

    def pop_chat(self, tokens, cost, time):
        self.chat_step['stats'] = {
            'tokens' : tokens,
            'cost' : cost, 
            'time' : time
        }

    def message(self, text):
        self.chat_step['output']['outputs'].append(
        {
            'type' : 'text',
            'output' : text
        })

    def function(self, line, output):
        x = {
            'type' : 'call',
            'input' : line,
            'output' : {
                'type' : 'text',
                'output' : output
            }
        }
        self.chat_step['output']['outputs'].append(x)

    


class IChatDBG(TerminalPdb):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        
        self.prompt = '(ChatDBG pdb) '
        self.chat_prefix = '   '
        self.text_width = 80
        self._assistant = None
        self._history = []
        self._error_specific_prompt = ''

        self.do_context(_config.context)

        self.log = ChatDBGLog()
        atexit.register(lambda: self.log.dump())
        

    def _is_user_file(self, file_name):
        return (file_name.startswith(os.getcwd()) and not file_name.endswith('.pyx')) \
                or file_name.startswith('<ipython')

    def format_stack_trace(self, context=None):
        old_stdout = self.stdout
        buf = StringIO()
        self.stdout = buf
        try:
            self.print_stack_trace(context)
        finally:
            self.stdout = old_stdout
        return buf.getvalue()

    def interaction(self, frame, tb_or_exc):
        if isinstance(tb_or_exc, BaseException):
            details = "".join(traceback.format_exception_only(tb_or_exc)).rstrip()
            self._error_specific_prompt = f"The program encountered the following error:\n```\n{details}\n```\n"

        super().interaction(frame, tb_or_exc)
 
    def setup(self, f, tb):

        super().setup(f, tb)

        # hide lib frames
        for t in traceback.walk_tb(tb):
            file_name = t[0].f_code.co_filename
            if not self._is_user_file(file_name):
                t[0].f_locals['__tracebackhide__'] = True

        # go up until we are not in a library
        while self.curindex > 0 and self.curframe_locals.get('__tracebackhide__', False):
            self.curindex -= 1
            self.curframe = self.stack[self.curindex][0]
            self.curframe_locals = self.curframe.f_locals
            self.lineno = None

        self._error_stack_trace = f"The program has the following stack trace:\n```\n{self.format_stack_trace()}\n```\n"

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
                output = strip_color(hist_file.getvalue())
                if line not in [ 'quit', 'EOF']:
                    self.log.user_command(line, output)
                if line not in [ 'hist', 'test_prompt' ] and not self.was_chat:
                    self._history += [ (line, output) ]
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
 
    def format_history_entry(self, entry, indent = ''):
        line, output = entry
        if output:
            entry = f"(ChatDBG pdb) {line}\n{output}"
        else:
            entry = f"(ChatDBG pdb) {line}"
        return textwrap.indent(entry, indent, lambda _ : True) 

    def _clear_history(self):
        self._history = [ ]

    def default(self, line):
        if line[:1] == '!': 
            super().default(line)
        else:
            if line[:1] == ':': 
                line = line[1:].strip()
            self.do_chat(line)

    def precmd(self, line):
        # skip TerminalPdf's ? and ?? replacement
        line = super(Pdb, self).precmd(line)
        return line

    def do_hist(self, arg):  
        """hist
        Print the history of user-issued commands since the last chat.
        """
        entry_strs = [ self.format_history_entry(x) for x in self._history ]
        history_str = "\n".join(entry_strs)
        self.message(history_str)

    def do_pydoc(self, arg):
        """pydoc name
        Print the pydoc string for a name.
        """
        try:
            obj = self._getval(arg)
            if obj.__doc__ != None:
                pydoc.doc(obj, output = self.stdout)
            else:
                self.message(f'No documentation is available.')
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
                self.message(f'You MUST assume that `{arg}` is specified and implemented correctly.')
        except NameError:
            # message already printed in _getval
            pass
        except Exception:
            self.do_pydoc(arg)
            self.message(f'You MUST assume that `{arg}` is specified and implemented correctly.')

    def do_test_prompt(self, arg):
        """test_prompt
        [For debugging] Prints the prompts to be sent to the assistant.
        """
        self.message('Instructions:')
        self.message(self._instructions())
        self.message('-' * 80)
        self.message('Prompt:')
        self.message(self._get_prompt(arg))

    def _instructions(self):
        stack_dump = f'The program has this stack trace:\n```\n{self.format_stack_trace()}\n```\n'
        return _basic_instructions + '\n' + stack_dump + self._error_specific_prompt

    def print_stack_trace(self, context=None):
        # override to print the skips into stdout...
        Colors = self.color_scheme_table.active_colors
        ColorsNormal = Colors.Normal
        if context is None:
            context = self.context
        try:
            context=int(context)
            if context <= 0:
                raise ValueError("Context must be a positive integer")
        except (TypeError, ValueError):
                raise ValueError("Context must be a positive integer")
        try:
            skipped = 0
            for hidden, frame_lineno in zip(self.hidden_frames(self.stack), self.stack):
                if hidden and self.skip_hidden:
                    skipped += 1
                    continue
                if skipped:
                    print(
                        f"{Colors.excName}    [... skipping {skipped} hidden frame(s)]{ColorsNormal}\n",
                        file=self.stdout
                    )
                    skipped = 0
                self.print_stack_entry(frame_lineno, context=context)
            if skipped:
                print(
                    f"{Colors.excName}    [... skipping {skipped} hidden frame(s)]{ColorsNormal}\n",
                    file=self.stdout
                )
        except KeyboardInterrupt:
            pass


    def _stack_prompt(self):
        stack_frames = textwrap.indent(self._capture_onecmd('bt'), '')
        stack = textwrap.dedent(f"""
            This is the current stack.  The current frame is indicated by 
            an arrow '>' at the start of the line.
            ```""") + f'\n{stack_frames}\n```'
        return stack

    def _get_prompt(self, arg):
        if arg == 'why':
            arg = "Explain the root cause of the error."

        user_prompt = ''
        if len(self._history) > 0:
            hist = textwrap.indent(self._capture_onecmd('hist'), '')
            self._clear_history()
            hist = f"This is the history of some pdb commands I ran and the results.\n```\n{hist}\n```\n"
            user_prompt += hist

        stack = self._stack_prompt()
        user_prompt += stack + '\n' + arg

        return user_prompt

    def do_chat(self, arg):
        """chat/:
        Send a chat message.
        """
        self.was_chat = True

        full_prompt = self._get_prompt(arg)

        if self._assistant == None:
            self._make_assistant()

        def client_print(line=''):
            line = llm_utils.word_wrap_except_code_blocks(line, 
                                                          self.text_width - 10)
            self.log.message(line)
            line = textwrap.indent(line, 
                                   self.chat_prefix, 
                                   lambda _ : True)
            print(line, file=self.stdout, flush=True)

        full_prompt = strip_color(full_prompt)
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
            command = f'info {name}'
            result = self._capture_onecmd(command)
            self.message(self.format_history_entry((command, result), 
                                                   indent = self.chat_prefix))
            result = strip_color(result)        
            self.log.function(command, result)
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
            cmd = command if command != 'list' else 'll'
            result = self._capture_onecmd(cmd)

            self.message(self.format_history_entry((command, result), 
                                                   indent = self.chat_prefix))

            result = strip_color(result)
            self.log.function(command, result)

            # help the LLM know where it is...
            result += strip_color(self._stack_prompt())
            return result

        self._clear_history()
        instructions = self._instructions()
        
        self.log.instructions(instructions)

        if not _config.model in _valid_models:
            print(f"'{_config.model}' is not a valid OpenAI model.  Choose from: {_valid_models}.")
            sys.exit(0)

        self._assistant = Assistant("ChatDBG", 
                                    self._instructions(), 
                                    model=_config.model, 
                                    debug=_config.debug)
        self._assistant.add_function(pdb)
        self._assistant.add_function(info)




