import linecache
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
from pathlib import Path
from pprint import pprint

import IPython
import llm_utils
from traitlets import TraitError

from .assistant.assistant import Assistant
from .ipdb_util.config import Chat
from .ipdb_util.logging import ChatDBGLog, CopyingTextIOWrapper
from .ipdb_util.prompts import pdb_instructions
from .ipdb_util.text import *
from .ipdb_util.locals import *

_valid_models = [
    "gpt-4-turbo-preview",
    "gpt-4-0125-preview",
    "gpt-4-1106-preview",
    "gpt-3.5-turbo-0125",
    "gpt-3.5-turbo-1106",
    "gpt-4",  # no parallel calls
    "gpt-3.5-turbo",  # no parallel calls
]

chatdbg_config: Chat = None


def load_ipython_extension(ipython):
    # Create an instance of your configuration class with IPython's config
    global chatdbg_config
    from chatdbg.chatdbg_pdb import Chat, ChatDBG

    ipython.InteractiveTB.debugger_cls = ChatDBG
    chatdbg_config = Chat(config=ipython.config)
    print("*** Loaded ChatDBG ***")


try:
    ipython = IPython.get_ipython()
    if ipython != None:
        from IPython.terminal.interactiveshell import TerminalInteractiveShell

        if isinstance(ipython, TerminalInteractiveShell):
            # ipython --pdb
            from IPython.terminal.debugger import TerminalPdb

            ChatDBGSuper = TerminalPdb
            _user_file_prefixes = [os.getcwd(), "<ipython"]
        else:
            # inside jupyter
            from IPython.core.debugger import InterruptiblePdb

            ChatDBGSuper = InterruptiblePdb
            _user_file_prefixes = [os.getcwd(), IPython.paths.tempfile.gettempdir()]
    else:
        # ichatpdb on command line
        from IPython.terminal.debugger import TerminalPdb

        ChatDBGSuper = TerminalPdb
        _user_file_prefixes = [os.getcwd()]
except NameError as e:
    print(f"Error {e}:IPython not found. Defaulting to pdb plugin.")
    ChatDBGSuper = pdb.Pdb


class ChatDBG(ChatDBGSuper):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self.prompt = "(ChatDBG) "
        self._chat_prefix = "   "
        self._text_width = 80
        self._assistant = None
        self._history = []
        self._error_specific_prompt = ""

        global chatdbg_config
        if chatdbg_config == None:
            chatdbg_config = Chat()

        # Only use flow when we are in jupyter or using stdin in ipython.   In both
        # cases, there will be no python file at the start of argv after the
        # ipython commands.
        self._supports_flow = chatdbg_config.show_slices
        if self._supports_flow:
            if ChatDBGSuper is not IPython.core.debugger.InterruptiblePdb:
                for arg in sys.argv:
                    if arg.endswith("ipython") or arg.endswith("ipython3"):
                        continue
                    if arg.startswith("-"):
                        continue
                    if Path(arg).suffix in [".py", ".ipy"]:
                        self._supports_flow = False
                    break

        self.do_context(chatdbg_config.context)
        self.rcLines += ast.literal_eval(chatdbg_config.rc_lines)

        # set this to True ONLY AFTER we have had stack frames
        self._show_locals = False

        self._log = ChatDBGLog(chatdbg_config)
        atexit.register(lambda: self._log.dump())

    def _is_user_frame(self, frame):
        if not self._is_user_file(frame.f_code.co_filename):
            return False
        name = frame.f_code.co_name
        return not name.startswith("<") or name == "<module>"

    def _is_user_file(self, file_name):
        if file_name.endswith(".pyx"):
            return False
        if file_name == "<string>":
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
        elif sys.exc_info()[1] != None:
            exception = sys.exc_info()[1]
        else:
            exception = None

        if exception != None:
            details = "".join(traceback.format_exception_only(exception)).rstrip()
            self._error_specific_prompt = (
                f"The program encountered the following error:\n```\n{details}\n```\n"
            )

        super().interaction(frame, tb_or_exc)

    def _hide_lib_frames(self):
        # hide lib frames
        for s in self.stack:
            s[0].f_locals["__tracebackhide__"] = not self._is_user_frame(s[0])

        # truncate huge stacks
        for frame in self.stack[0:-30]:
            frame[0].f_locals["__tracebackhide__"] = True

        # go up until we are not in a library
        while self.curindex > 0 and self.curframe_locals.get(
            "__tracebackhide__", False
        ):
            self.curindex -= 1
            self.curframe, self.lineno = self.stack[self.curindex]
            self.curframe_locals = self.curframe.f_locals

        # Assume assertions are correct and the code leading to them is not!
        if self.curframe.f_lineno != None:
            current_line = linecache.getline(
                self.curframe.f_code.co_filename, self.curframe.f_lineno
            )
            if current_line.strip().startswith("assert"):
                self._error_specific_prompt += f"The code `{current_line.strip()}` is correct and MUST remain unchanged in your fix.\n"

    def execRcLines(self):

        # do before running rclines -- our stack should be set up by now.

        if not chatdbg_config.show_libs:
            self._hide_lib_frames()
        self._error_stack_trace = f"The program has the following stack trace:\n```\n{self.format_stack_trace()}\n```\n"

        # finally safe to enable this.
        self._show_locals = chatdbg_config.show_locals and not chatdbg_config.show_libs

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
                if not line.startswith("config") and not line.startswith("mark"):
                    output = strip_color(hist_file.getvalue())
                    if line not in ["quit", "EOF"]:
                        self._log.user_command(line, output)
                    if (
                        line not in ["hist", "test_prompt", "c", "continue"]
                        and not self.was_chat
                    ):
                        self._history += [(line, output)]

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
        # return super().error('If the name is undefined, be sure you are in the right frame.  Use up and down to do that, and then print the variable again'.expandtabs())

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

    def _format_history_entry(self, entry, indent=""):
        line, output = entry
        if output:
            entry = f"{self.prompt}{line}\n{output}"
        else:
            entry = f"{self.prompt}{line}"
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

    def precmd(self, line):
        # skip TerminalPdf's ? and ?? replacement
        if ChatDBGSuper != pdb.Pdb:
            line = super(IPython.core.debugger.Pdb, self).precmd(line)
        return line

    def do_hist(self, arg):
        """hist
        Print the history of user-issued commands since the last chat.
        """
        entry_strs = [self._format_history_entry(x) for x in self._history]
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
            # try both given and unqualified form incase LLM biffs
            args_to_try = [arg, arg.split(".")[-1]]
            obj = None
            for x in args_to_try:
                try:
                    obj = eval(x, self.curframe.f_globals, self.curframe_locals)
                    break  # found something so we're good
                except:
                    # fail silently, try the next name
                    pass

            # didn't find anything
            if obj == None:
                self.message(f"No name `{arg}` is visible in the current frame.")
                return

            if self._is_user_file(inspect.getfile(obj)):
                self.do_source(x)
            else:
                self.do_pydoc(x)
                self.message(
                    f"You MUST assume that `{x}` is specified and implemented correctly."
                )
        except OSError:
            raise
        except NameError:
            # alread handled
            pass
        except Exception:
            self.do_pydoc(x)
            self.message(
                f"You MUST assume that `{x}` is specified and implemented correctly."
            )

    def do_slice(self, arg):
        if not self._supports_flow:
            self.message("*** `slice` is only supported in Jupyter notebooks")
            return

        try:
            from ipyflow import cells, singletons
            from ipyflow.models import statements

            index = self.curindex
            _x = None
            cell = None
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
                result = str(
                    statements().format_multi_slice(
                        time_stamps, blacken=True, format_type=None
                    )
                ).rstrip()
            else:
                used_symbols = (
                    set() if cell == None else set([str(x) for x in cell.used_symbols])
                )
                defined = (
                    f", only for these symbols: {', '.join(used_symbols)}"
                    if len(used_symbols) > 0
                    else ""
                )

                result = f"*** No information avaiable for {arg}{defined}.  Run the command `p {arg}` to see its value."
        except OSError:
            raise
        except Exception as e:
            # traceback.print_exc()
            result = f"*** Bad frame for call to slice ({type(e).__name__}: {e})"

        self.message(result)

    def do_test_prompt(self, arg):
        """test_prompt
        [For debugging] Prints the prompts to be sent to the assistant.
        """
        self.message("Instructions:")
        self.message(
            pdb_instructions(self._supports_flow, chatdbg_config.take_the_wheel)
        )
        self.message("-" * 80)
        self.message("Prompt:")
        self.message(self._build_prompt(arg, False))

    def _hidden_predicate(self, frame):
        """
        Given a frame return whether it it should be hidden or not by IPython.
        """

        if self._predicates["readonly"]:
            fname = frame.f_code.co_filename
            # we need to check for file existence and interactively define
            # function would otherwise appear as RO.
            if os.path.isfile(fname) and not os.access(fname, os.W_OK):
                return True

        if self._predicates["tbhide"]:
            if frame in (self.curframe, getattr(self, "initial_frame", None)):
                return False
            fname = frame.f_code.co_filename

            # Hack because the locals for this frame are shared with
            # the first user frame, so we can't rely on the flag
            # in frame_locals to be set properly.
            if fname == "<string>":
                return True

            frame_locals = self._get_frame_locals(frame)
            if "__tracebackhide__" not in frame_locals:
                return False
            return frame_locals["__tracebackhide__"]
        return False

    def print_stack_trace(self, context=None, locals=None):
        # override to print the skips into stdout...
        Colors = self.color_scheme_table.active_colors
        ColorsNormal = Colors.Normal
        if context is None:
            context = self.context
        try:
            context = int(context)
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
                        file=self.stdout,
                    )
                    skipped = 0
                self.print_stack_entry(frame_lineno, context=context)
                if locals:
                    self._print_locals(frame_lineno[0])
            if skipped:
                print(
                    f"{Colors.excName}    [... skipping {skipped} hidden frame(s)]{ColorsNormal}\n",
                    file=self.stdout,
                )
        except KeyboardInterrupt:
            pass

    def _print_locals(self, frame):
        locals = frame.f_locals
        in_global_scope = locals is frame.f_globals
        defined_locals = extract_locals(frame)
        # if in_global_scope and "In" in locals:  # in notebook
        #     defined_locals = defined_locals | extract_nb_globals(locals)
        if len(defined_locals) > 0:
            if in_global_scope:
                print(f"    Global variables:", file=self.stdout)
            else:
                print(f"    Variables in this frame:", file=self.stdout)
            for name in sorted(defined_locals):
                value = locals[name]
                t = type(value).__name__
                prefix = f"      {name}: {t} = "
                rep = format_limited(value, limit=20).split("\n")
                if len(rep) > 1:
                    rep = (
                        prefix
                        + rep[0]
                        + "\n"
                        + textwrap.indent("\n".join(rep[1:]), prefix=" " * len(prefix))
                    )
                else:
                    rep = prefix + rep[0]
                print(rep, file=self.stdout)
            print(file=self.stdout)

    def _stack_prompt(self):
        stdout = self.stdout
        buffer = StringIO()
        self.stdout = buffer
        try:
            self.print_stack_trace(context=1, locals=False)
            stack_frames = buffer.getvalue()
            stack_frames = "\n".join(
                line for line in stack_frames.splitlines() if line.strip()
            )
            stack = (
                textwrap.dedent(
                    f"""
                This is the current stack.  The current frame is indicated by 
                an arrow '>' at the start of the line.
                ```"""
                )
                + f"\n{stack_frames}\n```"
            )
            return stack
        finally:
            self.stdout = stdout

    def _build_prompt(self, arg, conversing):
        prompt = ""

        if not conversing:
            stack_dump = f"The program has this stack trace:\n```\n{self.format_stack_trace()}\n```\n\n"
            prompt = "\n" + stack_dump + self._error_specific_prompt

        if len(self._history) > 0:
            hist = textwrap.indent(self._capture_onecmd("hist"), "")
            self._clear_history()
            hist = f"\nThis is the history of some pdb commands I ran and the results.\n```\n{hist}\n```\n"
            prompt += hist

        if arg == "why":
            arg = "Explain the root cause of the error."

        stack = self._stack_prompt()
        prompt += stack + "\n" + arg

        return prompt

    def do_chat(self, arg):
        """chat/:
        Send a chat message.
        """
        self.was_chat = True

        full_prompt = self._build_prompt(arg, self._assistant != None)

        if self._assistant == None:
            self._make_assistant()

        def client_print(line=""):
            line = llm_utils.word_wrap_except_code_blocks(line, self._text_width - 10)
            self._log.message(line)
            line = textwrap.indent(line, self._chat_prefix, lambda _: True)
            print(line, file=self.stdout, flush=True)

        full_prompt = strip_color(full_prompt)
        full_prompt = truncate_proportionally(full_prompt)

        self._log.push_chat(arg, full_prompt)
        stats = self._assistant.run(full_prompt, client_print)
        self._log.pop_chat(stats)

    def do_mark(self, arg):
        marks = ["Full", "Partial", "Wrong", "None", "?"]
        if arg == None or arg == "":
            arg = input(f"mark? (one of {marks}): ")
            while arg not in marks:
                arg = input(f"mark? (one of {marks}): ")
        if arg not in marks:
            self.error(
                f"answer must be in { ['Full', 'Partial', 'Wrong', '?', 'None'] }"
            )
        else:
            self._log.add_mark(arg)

    def do_config(self, arg):
        args = arg.split()
        if len(args) == 0:
            pprint(chatdbg_config.to_json(), sort_dicts=True, stream=self.stdout)
            return

        if len(args) != 2:
            self.error("Usage: config <option> <value>")
            self.error("   or: config")
            return

        option, value = args
        try:
            chatdbg_config.set_trait(option, value)
            pprint(chatdbg_config.to_json(), sort_dicts=True, stream=self.stdout)
        except TraitError as e:
            self.error(f"{e}")

            # Get the documentation and source code (if available) for any function or method visible in the current frame.  The argument to info can be the name of the function or an expression of the form `obj.method_name`  to see the information for the method_name method of object obj.",

    def _make_assistant(self):
        def info(value):
            """
            {
                "name": "info",
                "description": "Get the documentation and source code for a reference, which may be a variable, function, method reference, field reference, or dotted reference visible in the current frame.  Examples include n, e.n where e is an expression, and t.n where t is a type.",
                "parameters": {
                    "type": "object",
                    "properties": {
                        "value": {
                            "type": "string",
                            "description": "The reference to get the information for."
                        }
                    },
                    "required": [ "value"  ]
                }
            }
            """
            command = f"info {value}"
            result = self._capture_onecmd(command)
            self.message(
                self._format_history_entry((command, result), indent=self._chat_prefix)
            )
            result = strip_color(result)
            self._log.function(command, result)
            return truncate_proportionally(result, top_proportion=1)

        def debug(command):
            """
            {
                "name": "debug",
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

            self.message(
                self._format_history_entry((command, result), indent=self._chat_prefix)
            )

            result = strip_color(result)
            self._log.function(command, result)

            # help the LLM know where it is...
            result += strip_color(self._stack_prompt())
            return truncate_proportionally(result, maxlen=8000, top_proportion=0.9)

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
            command = f"slice {name}"
            result = self._capture_onecmd(command)
            self.message(
                self._format_history_entry((command, result), indent=self._chat_prefix)
            )
            result = strip_color(result)
            self._log.function(command, result)
            return truncate_proportionally(result, top_proportion=0.5)

        self._clear_history()
        instruction_prompt = pdb_instructions(
            self._supports_flow, chatdbg_config.take_the_wheel
        )

        self._log.instructions(instruction_prompt)

        if not chatdbg_config.model in _valid_models:
            print(
                f"'{chatdbg_config.model}' is not a valid OpenAI model.  Choose from: {_valid_models}."
            )
            sys.exit(0)

        self._assistant = Assistant(
            "ChatDBG",
            instruction_prompt,
            model=chatdbg_config.model,
            debug=chatdbg_config.debug,
        )

        if chatdbg_config.take_the_wheel:
            self._assistant.add_function(debug)
            self._assistant.add_function(info)

            if self._supports_flow:
                self._assistant.add_function(slice)
