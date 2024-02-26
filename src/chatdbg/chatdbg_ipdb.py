"""
$ ipython profile create
$ echo "c.InteractiveShellApp.extensions = ['chatdbg.chatdbg_ipdb', 'ipyflow']" > ~/.ipython/profile_default/ipython_config.py

To get in ipython_config.py:

c.InteractiveShellApp.extensions = ['chatdbg.chatdbg_ipdb', 'ipyflow']
"""

import ast
import atexit
import inspect
import os
import pdb
import pydoc
import sys
import textwrap
import traceback
from io import StringIO
from pprint import pprint

import IPython
import llm_utils
from traitlets import TraitError

from .assistant.assistant import Assistant
from .ipdb_util.config import Chat
from .ipdb_util.logging import ChatDBGLog, CopyingTextIOWrapper
from .ipdb_util.prompts import instructions
from .ipdb_util.text import *

_valid_models = [
    'gpt-4-turbo-preview', 
    'gpt-4-0125-preview', 
    'gpt-4-1106-preview', 
    'gpt-3.5-turbo-0125', 
    'gpt-3.5-turbo-1106',
    'gpt-4',         # no parallel calls
    'gpt-3.5-turbo'  # no parallel calls
]

_config : Chat = None

def load_ipython_extension(ipython):
    # Create an instance of your configuration class with IPython's config
    global _config
    from chatdbg.chatdbg_ipdb import Chat, ChatDBG
    ipython.InteractiveTB.debugger_cls = ChatDBG
    _config = Chat(config=ipython.config)
    print("*** Loaded ChatDBG ***")

    
_supports_flow = not (len(sys.argv) > 0 and (sys.argv[-1].endswith('.py') or sys.argv[-1].endswith('.ipynb')))
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
        self.rcLines += ast.literal_eval(_config.rc_lines)

        # set this to True ONLY AFTER we have had stack frames
        self._show_locals = False
        
        self.log = ChatDBGLog(_config)
        atexit.register(lambda: self.log.dump())

    def _is_user_frame(self, frame):
        if not self._is_user_file(frame.f_code.co_filename):
            return False
        name = frame.f_code.co_name
        return not name.startswith('<') or name == '<module>'

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
            exception = tb_or_exc
        elif sys.exception() != None:
            exception = sys.exception()
        else:
            exception = None
        
        if exception != None:
            details = "".join(traceback.format_exception_only(exception)).rstrip()
            self._error_specific_prompt = f"The program encountered the following error:\n```\n{details}\n```\n"

        super().interaction(frame, tb_or_exc)

 

    def _hide_lib_frames(self):
        # hide lib frames
        for s in self.stack:
            s[0].f_locals['__tracebackhide__'] = not self._is_user_frame(s[0])

        # truncate huge stacks
        for frame in self.stack[0:-30]:
            frame[0].f_locals['__tracebackhide__'] = True

        # go up until we are not in a library
        while self.curindex > 0 and self.curframe_locals.get('__tracebackhide__', False):
            self.curindex -= 1
            self.curframe, self.lineno = self.stack[self.curindex][0]
            self.curframe_locals = self.curframe.f_locals

    def execRcLines(self):

        # do before running rclines -- our stack should be set up by now.

        if not _config.show_libs:
            self._hide_lib_frames()
        self._error_stack_trace = f"The program has the following stack trace:\n```\n{self.format_stack_trace()}\n```\n"

        # finally safe to enable this.
        self._show_locals = _config.show_locals and not _config.show_libs
        
        return super().execRcLines()

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
                self.stdout = hist_file.getfile()
                if not line.startswith('config') and not line.startswith('mark'):
                    output = strip_color(hist_file.getvalue())
                    if line not in [ 'quit', 'EOF']:
                        self.log.user_command(line, output)
                    if line not in [ 'hist', 'test_prompt' ] and not self.was_chat:
                        self._history += [ (line, output) ]

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
 
    def _format_history_entry(self, entry, indent = ''):
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
        entry_strs = [ self._format_history_entry(x) for x in self._history ]
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

    def do_slice(self, arg):
        if not _supports_flow:
            self.message("*** `slice` is only supported in Jupyter notebooks")
            return
        
        try:
            from ipyflow import cells, singletons
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
            result = f"*** Bad frame for call to slice ({type(e).__name__}: {e})"

        self.message(result)

    def do_test_prompt(self, arg):
        """test_prompt
        [For debugging] Prints the prompts to be sent to the assistant.
        """
        self.message('Instructions:')
        self.message(instructions(_supports_flow, _config.take_the_wheel))
        self.message('-' * 80)
        self.message('Prompt:')
        self.message(self._build_prompt(arg, False))

    def print_stack_trace(self, context=None, locals=None):
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
        
        if locals is None:
            locals = self._show_locals
        else:
            locals = locals and self._show_locals

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
                if locals:
                    self._print_locals(frame_lineno[0])
            if skipped:
                print(
                    f"{Colors.excName}    [... skipping {skipped} hidden frame(s)]{ColorsNormal}\n",
                    file=self.stdout
                )
        except KeyboardInterrupt:
            pass


    def _get_defined_locals_and_params(self, frame):

        class SymbolFinder(ast.NodeVisitor):
            def __init__(self):
                self.defined_symbols = set()

            def visit_Assign(self, node):
                for target in node.targets:
                    if isinstance(target, ast.Name):
                        self.defined_symbols.add(target.id)
                self.generic_visit(node)

            def visit_For(self, node):
                if isinstance(node.target, ast.Name):
                    self.defined_symbols.add(node.target.id)
                self.generic_visit(node)

            def visit_comprehension(self, node):
                if isinstance(node.target, ast.Name):
                    self.defined_symbols.add(node.target.id)
                self.generic_visit(node)


        try:    
            source = textwrap.dedent(inspect.getsource(frame.f_code))
            tree = ast.parse(source)

            finder = SymbolFinder()
            finder.visit(tree)

            args, varargs, keywords, locals = inspect.getargvalues(frame)
            parameter_symbols = set(args + [ varargs, keywords ])
            parameter_symbols.discard(None)

            return (finder.defined_symbols | parameter_symbols) & locals.keys()
        except Exception as e:
            # yipes -silent fail...
            return set()

    def _print_locals(self, frame):
        locals = frame.f_locals
        defined_locals = self._get_defined_locals_and_params(frame)
        if locals is frame.f_globals:
            print(f'        Global variables:', file=self.stdout)
        else:
            print(f'        Variables in this frame:', file=self.stdout)                
        if len(defined_locals) > 0:
            for name in sorted(defined_locals):
                value = locals[name]
                print(f"          {name}= {format_limited(value, limit=20)}", file=self.stdout)
            print(file=self.stdout)

    def _stack_prompt(self):
        stdout = self.stdout
        buffer = StringIO()
        self.stdout = buffer
        try :            
            self.print_stack_trace(context=1,locals=False)
            stack_frames = buffer.getvalue()
            stack_frames = '\n'.join(line for line in stack_frames.splitlines() if line.strip())
            stack = textwrap.dedent(f"""
                This is the current stack.  The current frame is indicated by 
                an arrow '>' at the start of the line.
                ```""") + f'\n{stack_frames}\n```'
            return stack
        finally:
            self.stdout = stdout

    def _build_prompt(self, arg, conversing):
        prompt = ''

        if not conversing:
            stack_dump = f'The program has this stack trace:\n```\n{self.format_stack_trace()}\n```\n'
            prompt = '\n' + stack_dump + self._error_specific_prompt
        
        if len(self._history) > 0:
            hist = textwrap.indent(self._capture_onecmd('hist'), '')
            self._clear_history()
            hist = f"This is the history of some pdb commands I ran and the results.\n```\n{hist}\n```\n"
            prompt += hist

        if arg == 'why':
            arg = "Explain the root cause of the error."

        stack = self._stack_prompt()
        prompt += stack + '\n' + arg

        return prompt

    def do_chat(self, arg):
        """chat/:
        Send a chat message.
        """
        self.was_chat = True

        full_prompt = self._build_prompt(arg, self._assistant != None)

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
        full_prompt = truncate_proportionally(full_prompt)

        self.log.push_chat(arg, full_prompt)
        tokens, cost, time = self._assistant.run(full_prompt, client_print)
        self.log.pop_chat(tokens, cost, time)

    def do_mark(self, arg):
        marks = [ 'Fix', 'Partial', 'None', '?' ]
        if arg == None or arg == '':
            arg = input(f'mark? (one of {marks}): ')
            while arg not in marks:
                arg = input(f'mark? (one of {marks}): ')
        if arg not in marks:
            self.error(f"answer must be in { ['Fix', 'Partial', '?', 'None'] }")
        else:
            self.log.add_mark(arg)

    def do_config(self, arg):
        args = arg.split()
        if len(args) == 0:
            pprint(_config.to_json(), sort_dicts=True, stream=self.stdout)
            return
            
        if len(args) != 2:
            self.error("Usage: config <option> <value>")
            self.error("   or: config")
            return
        
        option, value = args
        try:
            _config.set_trait(option, value)
            pprint(_config.to_json(), sort_dicts=True, stream=self.stdout)                
        except TraitError as e:
            self.error(f'{e}')            


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
            self.message(self._format_history_entry((command, result), 
                                                   indent = self.chat_prefix))
            result = strip_color(result)        
            self.log.function(command, result)
            return truncate_proportionally(result, top_proportion=1)

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

            self.message(self._format_history_entry((command, result), 
                                                   indent = self.chat_prefix))

            result = strip_color(result)
            self.log.function(command, result)

            # help the LLM know where it is...
            result += strip_color(self._stack_prompt())
            return truncate_proportionally(result, top_proportion=0.9)

        def slice(name):
            """
            {
                "name": "slice",
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
            command = f'slice {name}'
            result = self._capture_onecmd(command)
            self.message(self._format_history_entry((command, result), 
                                                   indent = self.chat_prefix))
            result = strip_color(result)        
            self.log.function(command, result)
            return truncate_proportionally(result, top_proportion=0.5)


        self._clear_history()
        instruction_prompt = instructions(_supports_flow, _config.take_the_wheel)
        
        self.log.instructions(instruction_prompt)

        if not _config.model in _valid_models:
            print(f"'{_config.model}' is not a valid OpenAI model.  Choose from: {_valid_models}.")
            sys.exit(0)

        self._assistant = Assistant("ChatDBG", 
                                    instruction_prompt, 
                                    model=_config.model, 
                                    debug=_config.debug)
        
        if _config.take_the_wheel:
            self._assistant.add_function(pdb)
            self._assistant.add_function(info)

            if _supports_flow:
                self._assistant.add_function(slice)


    


def main():
    import ipdb
    ipdb.__main__._get_debugger_cls = lambda : ChatDBG
    ipdb.__main__.main()
