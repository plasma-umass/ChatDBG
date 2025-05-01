import types
import ast
import atexit
import inspect
import linecache
import os
import pydoc
import sys
import textwrap
import traceback
from io import StringIO
from pathlib import Path

import IPython

import pdb

from chatdbg.pdb_util.sandbox import sandbox_eval
from chatdbg.util.prompts import (
    build_followup_prompt,
    build_initial_prompt,
    initial_instructions,
)

from chatdbg.assistant.assistant import Assistant, AssistantError
from chatdbg.pdb_util.capture import CaptureInput, CaptureOutput
from chatdbg.pdb_util.locals import print_locals
from chatdbg.util.text import strip_ansi, truncate_proportionally
from chatdbg.util.config import chatdbg_config
from chatdbg.util.log import ChatDBGLog
from chatdbg.util.history import CommandHistory
from chatdbg.util.exit_message import chatdbg_was_called, print_exit_message


def load_ipython_extension(ipython):
    global chatdbg_config
    from chatdbg.chatdbg_pdb import ChatDBG
    from chatdbg.util.config import ChatDBGConfig, chatdbg_config

    ipython.InteractiveTB.debugger_cls = ChatDBG
    chatdbg_config = ChatDBGConfig(config=ipython.config)
    print("*** Loaded ChatDBG ***")


_special_config = []
try:
    ipython = IPython.get_ipython()
    if ipython != None:
        from IPython.terminal.interactiveshell import TerminalInteractiveShell

        if isinstance(ipython, TerminalInteractiveShell):
            # ipython --pdb
            from IPython.terminal.debugger import TerminalPdb

            ChatDBGSuper = TerminalPdb
        else:
            # inside jupyter
            from IPython.core.debugger import InterruptiblePdb

            ChatDBGSuper = InterruptiblePdb
            _special_config += ["--format=jupyter"]
    else:
        # ichatpdb on command line
        from IPython.terminal.debugger import TerminalPdb

        ChatDBGSuper = TerminalPdb
except NameError as e:
    print(f"Error {e}: IPython not found. Defaulting to pdb plugin.")
    ChatDBGSuper = pdb.Pdb


class ChatDBG(ChatDBGSuper):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        chatdbg_config.parse_only_user_flags(_special_config)

        self.prompt = "(ChatDBG) "
        self._chat_prefix = "   "
        self._text_width = 120
        self._assistant = None
        atexit.register(print_exit_message)
        atexit.register(lambda: self._close_assistant())

        self._history = CommandHistory(self.prompt)
        self._error_message = ""
        self._error_details = ""

        sys.stdin = CaptureInput(sys.stdin)

        self._supports_flow = self.can_support_flow()

        self.do_context(chatdbg_config.context)
        self.rcLines += ast.literal_eval(chatdbg_config.rc_lines)

        # set this to True ONLY AFTER we have had access to stack frames
        self._show_locals = False

        self._library_paths = [os.path.dirname(os.__file__)] + [
            path
            for path in sys.path
            if "site-packages" in path or "dist-packages" in path
        ]

        self._log = ChatDBGLog(
            log_filename=chatdbg_config.log,
            config=chatdbg_config.to_json(),
            capture_streams=True,
        )

    def _close_assistant(self):
        if self._assistant != None:
            self._assistant.close()

    def can_support_flow(self):
        # Only use flow when we are in jupyter or using stdin in ipython.   In both
        # cases, there will be no python file at the start of argv after the
        # ipython commands.
        if chatdbg_config.show_slices:
            if ChatDBGSuper is not IPython.core.debugger.InterruptiblePdb:
                for arg in sys.argv:
                    if arg.endswith("ipython") or arg.endswith("ipython3"):
                        continue
                    if arg.startswith("-"):
                        continue
                    if Path(arg).suffix in [".py", ".ipy"]:
                        return False
            return True
        else:
            return False

    def _is_user_frame(self, frame):
        if not self._is_user_file(frame.f_code.co_filename):
            return False
        name = frame.f_code.co_name
        return not name.startswith("<") or name == "<module>"

    def _is_user_file(self, file_name):
        if file_name.endswith(".pyx"):
            return False
        elif file_name == "<string>" or file_name.startswith("<frozen"):
            # synthetic entry point or frozen modules
            return False
        elif file_name.startswith("<ipython"):
            # stdin from ipython session
            return True

        for path in self._library_paths:
            if os.path.commonpath([file_name, path]) == path:
                return False

        return True

    def enriched_stack_trace(self, context=None):
        old_stdout = self.stdout
        buf = StringIO()
        self.stdout = buf
        try:
            self.print_stack_trace(context)
        finally:
            self.stdout = old_stdout
        return strip_ansi(buf.getvalue())

    def interaction(self, frame, tb_or_exc):
        if isinstance(tb_or_exc, BaseException):
            exception = tb_or_exc
        elif sys.exc_info()[1] != None:
            exception = sys.exc_info()[1]
        else:
            exception = None

        if exception != None:
            details = "".join(
                traceback.format_exception_only(type(exception), exception)
            ).rstrip()
            self._error_message = details

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
                self._error_details = f"The code `{current_line.strip()}` is correct and MUST remain unchanged in your fix."

    def execRcLines(self):
        # do before running rclines -- our stack should be set up by now.
        if not chatdbg_config.show_libs:
            self._hide_lib_frames()

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
            hist_file = CaptureOutput(self.stdout)
            self.stdout = hist_file
            try:
                self.was_chat_or_renew = False
                return super().onecmd(line)
            finally:
                self.stdout = hist_file.getfile()
                output = strip_ansi(hist_file.getvalue())
                if not self.was_chat_or_renew:
                    self._log.on_function_call(line, output)
                    if line.split()[0] not in [
                        "hist",
                        "test_prompt",
                        "c",
                        "cont",
                        "continue",
                        "config",
                    ]:
                        self._history.append(line, output)

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
            result = strip_ansi(result)
            return result
        finally:
            self.stdout = stdout
            self.lastcmd = lastcmd

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

    def _getval(self, arg):
        """
        Sandbox for evaluating expressions from the LLM.
        """
        try:
            if chatdbg_config.unsafe:
                return super()._getval(arg)
            else:
                return sandbox_eval(arg, self.curframe.f_globals, self.curframe_locals)
        except NameError as e:
            self.error(f"NameError: {e}")
            return None
        except ImportError as e:
            self.error(f"ImportError: {e}")
            return None

    def _getval_except(self, arg, frame=None):
        """
        Sandbox in case an LLM ever tries to use the display features...
        """
        try:
            if frame is None:
                return sandbox_eval(arg, self.curframe.f_globals, self.curframe_locals)
            else:
                return sandbox_eval(arg, frame.f_globals, frame.f_locals)
        except:
            exc_info = sys.exc_info()[:2]
            err = traceback.format_exception_only(*exc_info)[-1].strip()
            return "** raised %s **" % err

    def do_hist(self, arg):
        """hist
        Print the history of user-issued commands since the last chat.
        """
        self.message(self._history)

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

            if obj == None:
                # try again, using pydoc's logic...
                obj = pydoc.locate(arg)

            # didn't find anything
            if obj == None:
                self.message(f"No name `{arg}` is visible in the current frame.")
            elif isinstance(obj, types.BuiltinFunctionType) or isinstance(
                obj, types.BuiltinMethodType
            ):
                self.message(f"`{arg}` is a built-in.")
            elif self._is_user_file(inspect.getfile(obj)):
                self.message(f"Source from file {inspect.getfile(obj)}:")
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
        """
        slice
        Print the backwards slice for a variable used in the current cell but
        defined in an earlier cell.  [interactive IPython / Jupyter only]
        """
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
        self.message(self._initial_prompt_instructions())
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
        # override to print the skips into stdout instead of stderr...
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
                    print_locals(self.stdout, frame_lineno[0])
            if skipped:
                print(
                    f"{Colors.excName}    [... skipping {skipped} hidden frame(s)]{ColorsNormal}\n",
                    file=self.stdout,
                )
        except KeyboardInterrupt:
            pass

    def _prompt_stack(self):
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
                    f"""\
                This is the current stack.  The current frame is indicated by 
                an arrow '>' at the start of the line.
                ```"""
                )
                + f"\n{stack_frames}\n```"
            )
            return stack
        finally:
            self.stdout = stdout

    def _initial_prompt_instructions(self):
        functions = self._supported_functions()
        return initial_instructions(functions)

    def _initial_prompt_enchriched_stack_trace(self):
        return self.enriched_stack_trace()

    def _initial_prompt_error_message(self):
        return self._error_message

    def _initial_prompt_error_details(self):
        return self._error_details

    def _initial_prompt_command_line(self):
        return " ".join(sys.argv)

    def _initial_prompt_input(self):
        return sys.stdin.get_captured_input()

    def _prompt_history(self):
        return str(self._history)

    def _build_prompt(self, arg, conversing):
        if not conversing:
            return build_initial_prompt(
                self._initial_prompt_enchriched_stack_trace(),
                self._initial_prompt_error_message(),
                self._initial_prompt_error_details(),
                self._initial_prompt_command_line(),
                self._initial_prompt_input(),
                self._prompt_history(),
                user_text=arg,
            )
        else:
            return build_followup_prompt(
                self._prompt_history(), self._prompt_stack(), arg
            )

    def do_chat(self, arg):
        """chat
        Send a chat message.
        """
        chatdbg_was_called()
        self.was_chat_or_renew = True

        full_prompt = self._build_prompt(arg, self._assistant != None)
        full_prompt = strip_ansi(full_prompt)
        full_prompt = truncate_proportionally(full_prompt)

        self._history.clear()

        try:
            if self._assistant == None:
                self._make_assistant()

            stats = self._assistant.query(full_prompt, user_text=arg)
            self.message(stats["message"])
        except AssistantError as e:
            for line in str(e).split("\n"):
                self.error(line)

    def do_renew(self, arg):
        """renew
        End the current chat dialog and prepare to start a new one.
        """
        if self._assistant != None:
            self._assistant.close()
            self._assistant = None
        self.was_chat_or_renew = True
        self.message(f"Ready to start a new dialog.")

    def do_config(self, arg):
        """
        config
        Print out the ChatDBG config options.
        """
        args = arg.split()
        message = chatdbg_config.parse_only_user_flags(args)
        self.message(message)

    def _supported_functions(self):
        if chatdbg_config.take_the_wheel:
            functions = [self.debug, self.info]
            if self._supports_flow:
                functions += [self.slice]
        else:
            functions = []

        return functions

    def _make_assistant(self):
        instruction_prompt = self._initial_prompt_instructions()
        functions = self._supported_functions()

        self._assistant = Assistant(
            instruction_prompt,
            model=chatdbg_config.model,
            functions=functions,
            max_call_response_tokens=8192,
            listeners=[
                chatdbg_config.make_printer(
                    self.stdout, self.prompt, self._chat_prefix, self._text_width
                ),
                self._log,
            ],
        )

    ### Callbacks for LLM

    def info(self, value):
        """
        {
            "name": "info",
            "description": "Call the `info` function to get the documentation and source code for any variable, function, package, class, method reference, field reference, or dotted reference visible in the current frame.  Examples include: n, e.n where e is an expression, and t.n where t is a type. Unless it is from a common, widely-used library, you MUST call `info` exactly once on any symbol that is referenced in code leading up to the error.",
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
        return command, truncate_proportionally(result, top_proportion=1)

    def debug(self, command):
        """
        {
            "name": "debug",
            "description": "Call the `debug` function to run Pdb debugger commands on the stopped program. You may call the `pdb` function to run the following commands: `bt`, `up`, `down`, `p expression`, `list`.  Call `debug` to print any variable value or expression that you believe may contribute to the error.",
            "parameters": {
                "type": "object",
                "properties": {
                    "command": {
                        "type": "string",
                        "description": "The pdb command to run."
                    }
                },
                "required": [ "command" ]
            }
        }
        """
        cmd = command if command != "list" else "ll"
        # old_curframe = self.curframe
        result = self._capture_onecmd(cmd)

        # help the LLM know where it is...
        # if old_curframe != self.curframe:
        #     result += strip_color(self._stack_prompt())

        return command, truncate_proportionally(result, maxlen=8000, top_proportion=0.9)

    def slice(self, name):
        """
        {
            "name": "slice",
            "description": "Call the `slice` function to get the code used to produce the value currently stored a variable.  You MUST call `slice` exactly once on any variable used but not defined in the current frame's code.",
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
        return command, truncate_proportionally(result, top_proportion=0.5)
