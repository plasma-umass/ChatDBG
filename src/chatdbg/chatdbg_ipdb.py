"""
in ipython_config.py:

c.InteractiveShellApp.extensions = ['chatdbg.chatdbg_pdb', 'ipyflow']
"""

import yaml
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
import re
import IPython

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

def chat_get_env(option_name, default_value):
    env_name = 'CHATDBG_' + option_name.upper()
    t = type(default_value)
    return t(os.getenv(env_name, default_value))

def make_arrow(pad):
    """generate the leading arrow in front of traceback or debugger"""
    if pad >= 2:
        return '-'*(pad-2) + '> '
    elif pad == 1:
        return '>'
    return ''


class Chat(Configurable):
    model = Unicode(chat_get_env('model', 'gpt-4-1106-preview'), help="The OpenAI model").tag(config=True)
    # model = Unicode(default_value='gpt-3.5-turbo-1106', help="The OpenAI model").tag(config=True)
    debug = Bool(chat_get_env('debug',False), help="Log OpenAI calls").tag(config=True)
    log = Unicode(chat_get_env('log','log.yaml'), help="The log file").tag(config=True)
    tag = Unicode(chat_get_env('tag', ''), help="Any extra info for log file").tag(config=True)
    context = Int(chat_get_env('context', 5), help='lines of source code to show when displaying stacktrace information').tag(config=True)

    def to_json(self):
        """Serialize the object to a JSON string."""
        return {
            'model': self.model,
            'debug': self.debug,
            'log': self.log,
            'tag': self.tag,
            'context': self.context
        }

_config = None

def load_ipython_extension(ipython):
    # Create an instance of your configuration class with IPython's config
    global _config
    from chatdbg.chatdbg_ipdb import ChatDBG, Chat
    ipython.InteractiveTB.debugger_cls = ChatDBG
    _config = Chat(config=ipython.config)
    print("*** Loaded ChatDBG ***")

def strip_color(s):
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    return ansi_escape.sub('', s)

# Custom representer for literal scalar representation
def literal_presenter(dumper, data):
    if '\n' in data:
        return dumper.represent_scalar('tag:yaml.org,2002:str', data, style='|')
    return dumper.represent_scalar('tag:yaml.org,2002:str', data)

yaml.add_representer(str, literal_presenter)


_intro=f"""\
You are a debugging assistant.  You will be given a Python stack trace for an
error and answer questions related to the root cause of the error.
"""

_pbd_function=f"""\
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

Call `pdb` to print any variable value or expression that you believe may
contribute to the error.
"""

_info_function="""\
Call the `info` function to get the documentation and source code for any
function or method that is visible in the current frame.  The argument to
info can be the name of the function or an expression of the form `obj.method_name` 
to see the information for the method_name method of object obj.

Unless it is from a common, widely-used library, you MUST call `info` on any
function that is called in the code, that apppears in the argument list for a
function call in the code, or that appears on the call stack.  
"""

_how_function="""\
Call the `how` function to get the code used to produce
the value currently stored a variable.  
"""

_general_instructions="""\
Call the provided functions as many times as you would like.

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
        full_json = [{
            'meta' : self.meta,
            'steps' : self.steps,
            'instructions' : self._instructions,
            'stdout' : self.stdout_wrapper.getvalue(),
            'stderr' : self.stderr_wrapper.getvalue()
        }]
        
        with open(_config.log, 'a') as file:
            yaml.dump(full_json, file, default_flow_style=False)

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

    
_supports_flow = False
try:
    ipython = IPython.get_ipython()
    if ipython != None:
        if isinstance(ipython, IPython.terminal.interactiveshell.TerminalInteractiveShell):
            # ipython --pdb
            from IPython.terminal.debugger import TerminalPdb
            ChatDBGSuper = TerminalPdb
            _user_file_prefixes = [ os.getcwd(), '<ipython'  ]

        else:
            # inside jupyter
            from IPython.core.debugger import InterruptiblePdb
            ChatDBGSuper = InterruptiblePdb
            _user_file_prefixes = [ os.getcwd(), IPython.paths.tempfile.gettempdir() ]
            _supports_flow = True
    else: 
        # ichatpdb on command line
        from IPython.terminal.debugger import TerminalPdb
        ChatDBGSuper = TerminalPdb
        _user_file_prefixes = [ os.getcwd() ]
except NameError as e:
    print(f'{e}')
    ChatDBGSuper = pdb.Pdb

class ChatDBG(ChatDBGSuper):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        
        self.prompt = '(ChatDBG ipdb) '
        self.chat_prefix = '   '
        self.text_width = 80
        self._assistant = None
        self._history = []
        self._error_specific_prompt = ''

        global _config
        if _config == None:
            _config = Chat()

        self.do_context(_config.context)

        self.log = ChatDBGLog()
        atexit.register(lambda: self.log.dump())
        

    def _is_user_file(self, file_name):
        if file_name.endswith('.pyx'):
            return False
        for prefix in _user_file_prefixes:
            if file_name.startswith(prefix):
                return True
        return False

    def format_stack_trace(self, context=None):
        old_stdout = self.stdout
        buf = StringIO()
        self.stdout = buf
        try:
            self.print_stack_trace(context)
        finally:
            self.stdout = old_stdout
        return strip_color(buf.getvalue())

    def interaction(self, frame, tb_or_exc):
        if isinstance(tb_or_exc, BaseException):
            details = "".join(traceback.format_exception_only(tb_or_exc)).rstrip()
            self._error_specific_prompt = f"The program encountered the following error:\n```\n{details}\n```\n"
        else:
            if sys.exception() != None:
                details = "".join(traceback.format_exception_only(sys.exception())).rstrip()
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
            entry = f"{self.prompt} {line}\n{output}"
        else:
            entry = f"{self.prompt} {line}"
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
        if ChatDBGSuper != pdb.Pdb:
            line = super(IPython.core.debugger.Pdb, self).precmd(line)
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

    def do_how(self, arg):
        if not _supports_flow:
            self.message("*** `how` is only supported in Jupyter notebooks")
            return
        
        try:
            from ipyflow import singletons   
            from ipyflow import cells 
            from ipyflow.models import statements

            index = self.curindex
            _x = None
            while index > 0:
                # print(index)
                pos, _ = singletons.flow().get_position(self.stack[index][0])
                if pos >= 0: 
                    cell = cells().at_counter(pos)
                    # print(cell.used_symbols)
                    _x = next((x for x in cell.used_symbols if x.name == arg), None)
                if _x != None: 
                    break
                index -= 1
            if _x != None:
                # print('found it')
                # print(_x)
                # print(_x.__dict__)
                # print(_x._get_timestamps_for_version(version=-1))
                # print(code(_x))
                time_stamps = _x._get_timestamps_for_version(version=-1)
                time_stamps = [ts for ts in time_stamps if ts.cell_num > -1]
                result = str(statements().format_multi_slice(time_stamps,
                                                        blacken=True,
                                                        format_type=None)).rstrip()
            else:
                result = f"*** No information avaiable for {arg}, only {cell.used_symbols}.  Run the command `p {arg}` to see its value."
        except Exception as e:
            # traceback.print_exc()
            result = f"*** Bad frame for call to how ({e})"

        self.message(result)

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
        how_fn = _how_function if _supports_flow else ''
        instructions = f"{_intro}\n{_pbd_function}\n{_info_function}\n{how_fn}\n{_general_instructions}"

        stack_dump = f'The program has this stack trace:\n```\n{self.format_stack_trace()}\n```\n'
        return instructions + '\n' + stack_dump + self._error_specific_prompt

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
                "description": "Get the documentation and source code (if available) for any function or method visible in the current frame.  The argument to info can be the name of the function or an expression of the form `obj.method_name`  to see the information for the method_name method of object obj.",
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

        def how(name):
            """
            {
                "name": "how",
                "description": "Return the code to compute a global variable used in the current frame",
                "parameters": {
                    "type": "object",
                    "properties": {
                        "name": {
                            "type": "string",
                            "description": "The variable to look at."
                        }
                    },
                    "required": [ "name"  ]
                }
            }

            """
            command = f'how {name}'
            result = self._capture_onecmd(command)
            self.message(self.format_history_entry((command, result), 
                                                   indent = self.chat_prefix))
            result = strip_color(result)        
            self.log.function(command, result)
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
        self._assistant.add_function(how)

        if _supports_flow:
            self._assistant.add_function(info)


    # def format_stack_entry(self, frame_lineno, lprefix=': ', context=None):
    #     from IPython.utils import coloransi, py3compat 
    #     import linecache

    #     if context is None:
    #         context = self.context
    #     try:
    #         context = int(context)
    #         if context <= 0:
    #             print("Context must be a positive integer", file=self.stdout)
    #     except (TypeError, ValueError):
    #             print("Context must be a positive integer", file=self.stdout)

    #     import reprlib

    #     ret = []

    #     Colors = self.color_scheme_table.active_colors
    #     ColorsNormal = Colors.Normal
    #     tpl_link = "%s%%s%s" % (Colors.filenameEm, ColorsNormal)
    #     tpl_call = "%s%%s%s%%s%s" % (Colors.vName, Colors.valEm, ColorsNormal)
    #     tpl_line = "%%s%s%%s %s%%s" % (Colors.lineno, ColorsNormal)
    #     tpl_line_em = "%%s%s%%s %s%%s%s" % (Colors.linenoEm, Colors.line, ColorsNormal)

    #     frame, lineno = frame_lineno

    #     return_value = ''
    #     loc_frame = self._get_frame_locals(frame)
    #     if "__return__" in loc_frame:
    #         rv = loc_frame["__return__"]
    #         # return_value += '->'
    #         return_value += reprlib.repr(rv) + "\n"
    #     ret.append(return_value)

    #     #s = filename + '(' + `lineno` + ')'
    #     filename = self.canonic(frame.f_code.co_filename)
    #     link = tpl_link % py3compat.cast_unicode(filename)

    #     if frame.f_code.co_name:
    #         func = frame.f_code.co_name
    #     else:
    #         func = "<lambda>"

    #     call = ""
    #     if func != "?":
    #         if "__args__" in loc_frame:
    #             args = reprlib.repr(loc_frame["__args__"])
    #         else:
    #             args = '()'
    #         call = tpl_call % (func, args)

    #     # The level info should be generated in the same format pdb uses, to
    #     # avoid breaking the pdbtrack functionality of python-mode in *emacs.
    #     if frame is self.curframe:
    #         ret.append('> ')
    #     else:
    #         ret.append("  ")
    #     ret.append("%s(%s)%s\n" % (link, lineno, call))

    #     try:
    #         ilines, istart = inspect.getsourcelines(frame.f_code)
    #         iend = istart + len(ilines) - 1
    #     except Exception as e:
    #         print(e)
    #         istart, iend = 0, 99999

    #     start = max(istart, lineno - 1 - context//2)
    #     lines = linecache.getlines(filename)
    #     start = min(start, len(lines) - context)
    #     start = max(start, 0)
    #     end = min(iend, start + context)
    #     lines = lines[start : end]

    #     for i, line in enumerate(lines):
    #         show_arrow = start + 1 + i == lineno
    #         linetpl = (frame is self.curframe or show_arrow) and tpl_line_em or tpl_line
    #         ret.append(
    #             self.__format_line(
    #                 linetpl, filename, start + 1 + i, line, arrow=show_arrow
    #             )
    #         )
    #     return "".join(ret)


    # def __format_line(self, tpl_line, filename, lineno, line, arrow=False):
    #     bp_mark = ""
    #     bp_mark_color = ""

    #     new_line, err = self.parser.format2(line, 'str')
    #     if not err:
    #         line = new_line

    #     bp = None
    #     if lineno in self.get_file_breaks(filename):
    #         bps = self.get_breaks(filename, lineno)
    #         bp = bps[-1]

    #     if bp:
    #         Colors = self.color_scheme_table.active_colors
    #         bp_mark = str(bp.number)
    #         bp_mark_color = Colors.breakpoint_enabled
    #         if not bp.enabled:
    #             bp_mark_color = Colors.breakpoint_disabled

    #     numbers_width = 7
    #     if arrow:
    #         # This is the line with the error
    #         pad = numbers_width - len(str(lineno)) - len(bp_mark)
    #         num = '%s%s' % (make_arrow(pad), str(lineno))
    #     else:
    #         num = '%*s' % (numbers_width - len(bp_mark), str(lineno))

    #     return tpl_line % (bp_mark_color + bp_mark, num, line)
    
