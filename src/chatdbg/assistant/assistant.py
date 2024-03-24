import json
import time
from typing import Callable

import litellm
import openai

def sandwhich_tokens(model, text: str, max_tokens: int, top_proportion: float) -> str:
    if max_tokens == None:
        return text
    tokens = litellm.encode(model, text)
    if len(tokens) <= max_tokens:
        return text
    else:
        total_len = max_tokens - 5   # some slop for the ...
        top_len = int(top_proportion * total_len)
        bot_len = int((1-top_proportion) * total_len)
        return litellm.decode(model, tokens[0:top_len]) + " [...] " + litellm.decode(model, tokens[-bot_len:])


class AssistantClient:
    def warn(text):
        pass
    def fail(text):
        pass
    def response(text):
        pass
    def function_call(name, args, result):
        pass



class Assistant:
    def __init__(
        self,
        instructions,
        model="gpt-4",
        timeout=30,
        max_call_response_tokens=None,
        debug=False,
    ):
        if debug:
            log_file = open(f"chatdbg.log", "w")
            self._logger = lambda model_call_dict: print(model_call_dict, file=log_file, flush=True)
        else:
            self._logger = None

        self._functions = {}
        self._model = model
        self._timeout = timeout
        self._conversation = [{"role": "system", "content": instructions}]
        self._max_call_response_tokens = max_call_response_tokens

    def add_function(self, function):
        """
        Add a new function to the list of function tools.
        The function should have the necessary json spec as its docstring, with
        this format:
            "schema": function schema,
            "format": format to print call,
        """
        schema = json.loads(function.__doc__)
        assert "name" in schema, "Bad JSON in docstring for function tool."
        self._functions[schema["name"]] = {
            "function": function,
            "schema": schema
        }

    def _make_call(self, tool_call) -> str:
        name = tool_call.function.name
        args = json.loads(tool_call.function.arguments)
        try:
            args = json.loads(args)
            function = self.functions[name]
            result = function(**args)
        except OSError as e:
            # function produced some error -- move this to client???
            result = f"Error: {e}"
        except Exception as e:
            result = f"Ill-formed function call: {e}"
        return result


    def query(
        self,
        prompt: str,
        client: XXX
    ) -> None:
        start = time.time()
        cost = 0

        try:
            self._conversation.append({"role": "user", "content": prompt})
            
            while True:
                self._conversation = litellm.trim_messages(self._conversation, self._model)
                completion = litellm.completion(
                    model=self._model,
                    messages=self._conversation,
                    tools=[
                        {"type": "function", "function": f["schema"]["schema"]}
                        for f in self._functions.values()
                    ],
                    timeout=self._timeout,
                    logger_fn=self._logger
                )

                cost += litellm.completion_cost(completion)

                choice = completion.choices[0]

                if choice.finish_reason == "tool_calls":
                    responses = []
                    try:
                        for tool_call in choice.message.tool_calls:
                            function_response = self._make_call(tool_call)
                            function_response = sandwhich_tokens(self._model, 
                                                                 function_response, 
                                                                 self._max_call_response_tokens,
                                                                 0.5)
                            response = {
                                "tool_call_id": tool_call.id,
                                "role": "tool",
                                "name": tool_call.function.name,
                                "content": function_response,
                            }
                            responses.append(response)
                        self._conversation.append(choice.message)
                        self._conversation.extend(responses)
                    except Exception as e:
                        # Warning: potential infinite loop.
                        client.warn(f"Error processing tool calls: {e}")
                elif choice.finish_reason == "stop":
                    break
                else:
                    client.fail(f"Completation reason not supported: {choice.finish_reason}")
                    return

            elapsed = time.time() - start
            return {
                "cost": cost,
                "time": elapsed,
                "model": self._model,
                "tokens": litellm.token_counter(self._conversation),
            }
        except openai.OpenAIError as e:
            client.fail(f"Internal Error: {e}")


# import atexit
# import textwrap
# import json
# import time
# import sys

# import llm_utils
# from openai import *
# from openai import AssistantEventHandler
# from pydantic import BaseModel

# class AssistantPrinter:
#     def begin_stream(self):
#         print('\n', flush=True)

#     def stream(self, text=''):
#         print(text, flush=True, end='')

#     def end_stream(self):
#         print('\n', flush=True)

#     def complete_message(self, text=''):
#         print(text, flush=True)

#     def log(self, json_obj):
#         pass

#     def fail(self, message='Failed'):
#         print()
#         print(textwrap.wrap(message, width=70, initial_indent='*** '))  -- wrap then indent
#         sys.exit(1)
        
#     def warn(self, message='Warning'):
#         print()
#         print(textwrap.wrap(message, width=70, initial_indent='*** '))  -- wrap then indent
        

# class Assistant:
#     """
#     An Assistant is a wrapper around OpenAI's assistant API.  Example usage:

#         assistant = Assistant("Assistant Name", instructions,
#                               model='gpt-4-1106-preview', debug=True)
#         assistant.add_function(my_func)
#         response = assistant.run(user_prompt)

#     Name can be any name you want.

#     If debug is True, it will create a log of all messages and JSON responses in
#     json.txt.
#     """

#     def __init__(
#         self, name, instructions, model="gpt-3.5-turbo-1106", timeout=30,
#         printer = AssistantPrinter()):
#         self.printer = printer
#         try:
#             self.client = OpenAI(timeout=timeout)
#         except OpenAIError:
#             self.printer.fail("""\
#                 You need an OpenAI key to use this tool.
#                 You can get a key here: https://platform.openai.com/api-keys.
#                 Set the environment variable OPENAI_API_KEY to your key value.""")

#         self.assistants = self.client.beta.assistants
#         self.threads = self.client.beta.threads
#         self.functions = dict()

#         self.assistant = self.assistants.create(
#             name=name, instructions=instructions, model=model
#         )
#         self.thread = self.threads.create()

#         atexit.register(self._delete_assistant)

#     def _delete_assistant(self):
#         if self.assistant != None:
#             try:
#                 id = self.assistant.id
#                 response = self.assistants.delete(id)
#                 assert response.deleted
#             except OSError:
#                 raise
#             except Exception as e:
#                 self.printer.warn(f"Assistant {id} was not deleted ({e}).  You can do so at https://platform.openai.com/assistants.")

#     def add_function(self, function):
#         """
#         Add a new function to the list of function tools for the assistant.
#         The function should have the necessary json spec as its pydoc string.
#         """
#         function_json = json.loads(function.__doc__)
#         try:
#             name = function_json["name"]
#             self.functions[name] = function

#             tools = [
#                 {"type": "function", "function": json.loads(function.__doc__)}
#                 for function in self.functions.values()
#             ]

#             self.assistants.update(self.assistant.id, tools=tools)
#         except OpenAIError as e:
#             self.printer.fail(f"OpenAI Error: {e}")

#     def _make_call(self, tool_call):
#         name = tool_call.function.name
#         args = tool_call.function.arguments

#         # There is a sketchy case that happens occasionally because
#         # the API produces a bad call...
#         try:
#             args = json.loads(args)
#             function = self.functions[name]
#             result = function(**args)
#         except OSError as e:
#             result = f"Error: {e}"
#         except Exception as e:
#             result = f"Ill-formed function call: {e}"
#         return result

#     def drain_stream(self, stream):
#         run = None
#         for event in stream:
#             if event.event not in [ 'thread.message.delta', 'thread.run.step.delta' ]:
#                 print(event.event)
#             self.printer.log(event)
#             if event.event in [ 'thread.run.created', 'thread.run.in_progress' ]:
#                 run = event.data
#             elif event.event == 'thread.run.completed':
#                 self.printer.end_stream()
#                 return event.data
#             elif event.event == 'thread.message.delta':
#                 self.printer.stream(event.data.delta.content[0].text.value)
#             elif event.event == 'thread.message.completed':
#                 self.printer.complete_message(event.data.content[0].text.value)
#             elif event.event == 'thread.run.requires_action':
#                 r = event.data
#                 if r.status == "requires_action":
#                     outputs = []
#                     self.printer.end_stream()                        
#                     for tool_call in r.required_action.submit_tool_outputs.tool_calls:
#                         output = self._make_call(tool_call)
#                         outputs += [{"tool_call_id": tool_call.id, "output": output}]
#                     self.printer.begin_stream()                        
#                     try:
#                         with self.threads.runs.submit_tool_outputs(
#                             thread_id=self.thread.id, run_id=r.id, tool_outputs=outputs, stream=True
#                         ) as new_stream:
#                             _ = self.drain_stream(new_stream)
#                     except OSError as e:
#                         raise
#                     except Exception as e:
#                         # silent failure because the tool call submit biffed.  Not muchw e can do
#                         pass
#             elif event.event == 'thread.run.failed':
#                 run = event.data
#                 self.printer.fail(f"Internal Failure ({run.last_error.code}): {run.last_error.message}")
#             elif event.event == 'error':
#                 self.printer.fail(f"Internal Failure:** {event.data}")
#         print('***', run)
#         return run
        
#     def run(self, prompt):
#         """
#         Give the prompt to the assistant and get the response, which may included
#         intermediate function calls.
#         All output is printed to the given file.
#         """

#         if self.assistant == None:
#             return {
#                 "tokens": 0,
#                 "prompt_tokens": 0,
#                 "completion_tokens": 0,
#                 "model": self.assistant.model,
#                 "cost": 0,
#             }

#         start_time = time.perf_counter()
        
#         assert len(prompt) <= 32768
#         self.threads.messages.create(
#             thread_id=self.thread.id, role="user", content=prompt
#         )

#         class EventHandler(AssistantEventHandler):    
#             def on_event(self, event):
#                 print(event.event)

#         with self.threads.runs.create_and_stream(
#             thread_id=self.thread.id,
#             assistant_id=self.assistant.id,
#             # stream=True
#             event_handler=EventHandler(),
#         ) as stream:
#             self.drain_stream(stream)

#         end_time = time.perf_counter()
#         elapsed_time = end_time - start_time

#         cost = llm_utils.calculate_cost(
#             run.usage.prompt_tokens,
#             run.usage.completion_tokens,
#             self.assistant.model,
#         )
#         return {
#             "tokens": run.usage.total_tokens,
#             "prompt_tokens": run.usage.prompt_tokens,
#             "completion_tokens": run.usage.completion_tokens,
#             "model": self.assistant.model,
#             "cost": cost,
#             "time": elapsed_time,
#             "thread.id": self.thread.id,
#             "thread": self.thread,
#             "run.id": run.id,
#             "run": run,
#             "assistant.id": self.assistant.id,
#         }

#         return {}

# if __name__ == '__main__':
#     def weather(location):
#         """
#         {
#             "name": "get_weather",
#             "description": "Determine weather in my location",
#             "parameters": {
#                 "type": "object",
#                 "properties": {
#                 "location": {
#                     "type": "string",
#                     "description": "The city and state e.g. San Francisco, CA"
#                 },
#                 "unit": {
#                     "type": "string",
#                     "enum": [
#                     "c",
#                     "f"
#                     ]
#                 }
#                 },
#                 "required": [
#                 "location"
#                 ]
#             }
#         }
#         """
#         return "Sunny and 72 degrees."

#     a = Assistant("Test", "You generate text.")
#     a.add_function(weather)
#     x = a.run("What's the weather in Boston?")
#     print(x)
            

    #             def _print_message(
    #     self, message, indent, append_message: Callable[[str], None], wrap=120
    # ) -> None:
    #     def _format_message(indent) -> str:
    #         tool_calls = None
    #         if "tool_calls" in message:
    #             tool_calls = message["tool_calls"]
    #         elif hasattr(message, "tool_calls"):
    #             tool_calls = message.tool_calls

    #         content = None
    #         if "content" in message:
    #             content = message["content"]
    #         elif hasattr(message, "content"):
    #             content = message.content

    #         assert content != None or tool_calls != None

    #         # The longest role string is 'assistant'.
    #         max_role_length = 9
    #         # We add 3 characters for the brackets and space.
    #         subindent = indent + max_role_length + 3

    #         role = message["role"].upper()
    #         role_indent = max_role_length - len(role)

    #         output = ""

    #         if content != None:
    #             content = llm_utils.word_wrap_except_code_blocks(
    #                 content, wrap - len(role) - indent - 3
    #             )
    #             first, *rest = content.split("\n")
    #             output += f"{' ' * indent}[{role}]{' ' * role_indent} {first}\n"
    #             for line in rest:
    #                 output += f"{' ' * subindent}{line}\n"

    #         if tool_calls != None:
    #             if content != None:
    #                 output += f"{' ' * subindent} Function calls:\n"
    #             else:
    #                 output += (
    #                     f"{' ' * indent}[{role}]{' ' * role_indent} Function calls:\n"
    #                 )
    #             for tool_call in tool_calls:
    #                 arguments = json.loads(tool_call.function.arguments)
    #                 output += f"{' ' * (subindent + 4)}{tool_call.function.name}({', '.join([f'{k}={v}' for k, v in arguments.items()])})\n"
    #         return output

    #     append_message(_format_message(indent))
    #     if self._log:
    #         print(_format_message(0), file=self._log)


