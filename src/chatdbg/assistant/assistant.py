import json
import time
import sys
import litellm
import openai
import textwrap

import textwrap
import sys

class AbstractAssistantClient:

    def begin_dialog(self, instructions):
        pass

    def end_dialog(self):
        pass

    def begin_query(self, prompt, **kwargs):
        pass

    def end_query(self, stats):
        pass

    def warn(self, text):
        pass

    def fail(self, text):
        pass

    def begin_stream(self):
        pass
    def stream_delta(self, text):
        pass
    def end_stream(self):
        pass

    def response(self, text):
        pass

    def function_call(self, call, result):
        pass


class PrintingAssistantClient(AbstractAssistantClient):
    def __init__(self, out=sys.stdout):
        self.out = out

    def warn(self, text):
        print(textwrap.indent(text, '*** '), file=self.out)

    def fail(self, text):
        print(textwrap.indent(text, '*** '), file=self.out)
        sys.exit(1)

    def begin_stream(self):
        pass

    def stream_delta(self, text):
        print(text, end='', file=self.out, flush=True)

    def end_stream(self):
        pass

    def begin_query(self, prompt, **kwargs):
        pass

    def end_query(self, stats):
        pass

    def response(self, text):
        if text != None:
            print(text, file=self.out)
        
    def function_call(self, call, result):
        if result and len(result) > 0:
            entry = f"{call}\n{result}"
        else:
            entry = f"{call}"
        print(entry, file=self.out)
        

class StreamingAssistantClient(PrintingAssistantClient):
    def __init__(self, out=sys.stdout):
        super().__init__(out)

    def begin_stream(self):
        print('', flush=True)

    def stream_delta(self, text):
        print(text, end='', file=self.out, flush=True)

    def end_stream(self):
        print('', flush=True)

    def response(self, text):
        pass

class Assistant:
    def __init__(
        self,
        instructions,
        model="gpt-3.5-turbo-1106",
        timeout=30,
        clients = [ PrintingAssistantClient() ],
        functions=[],
        max_call_response_tokens=4096,
        debug=False,
        stream_response=False
    ):
        if debug:
            log_file = open(f"chatdbg.log", "w")
            self._logger = lambda model_call_dict: print(model_call_dict, file=log_file, flush=True)
        else:
            self._logger = None

        self._clients = clients
        
        self._functions = {}
        for f in functions:
            self._add_function(f)

        self._model = model
        self._timeout = timeout
        self._conversation = [{"role": "system", "content": instructions}]
        self._max_call_response_tokens = max_call_response_tokens
        self._stream_response = stream_response

        self._check_model()
        self._broadcast('begin_dialog', instructions)

    def close(self):
        self._broadcast('end_dialog')

    def _broadcast(self, method_name, *args, **kwargs):
        for client in self._clients:
            method = getattr(client, method_name, None)
            if callable(method):
                method(*args, **kwargs)

    def _check_model(self):
        result = litellm.validate_environment(self._model)
        missing_keys = result["missing_keys"]
        if missing_keys != []:
            _, provider, _, _ = litellm.get_llm_provider(self._model)
            if provider == 'openai':
                self._broadcast('fail', textwrap.dedent(f"""\
                    You need an OpenAI key to use the {self._model} model.
                    You can get a key here: https://platform.openai.com/api-keys.
                    Set the environment variable OPENAI_API_KEY to your key value."""))
                sys.exit(1)
            else:
                self._broadcast('fail', textwrap.dedent(f"""\
                    You need to set the following environment variables
                    to use the {self._model} model: {', '.join(missing_keys)}"""))
                sys.exit(1)

        if not litellm.supports_function_calling(self._model):
            self._broadcast('fail', textwrap.dedent(f"""\
                The {self._model} model does not support function calls.
                You must use a model that does, eg. gpt-4."""))
            sys.exit(1)

    def _sandwhich_tokens(self, text: str, max_tokens: int, top_proportion: float) -> str:
        model = self._model
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

    def _add_function(self, function):
        """
        Add a new function to the list of function tools.
        The function should have the necessary json spec as its docstring
        """
        schema = json.loads(function.__doc__)
        assert "name" in schema, "Bad JSON in docstring for function tool."
        self._functions[schema["name"]] = {
            "function": function,
            "schema": schema
        }

    def _make_call(self, tool_call) -> str:
        name = tool_call.function.name
        try:
            args = json.loads(tool_call.function.arguments)
            function = self._functions[name]
            call, result = function["function"](**args)
            self._broadcast('function_call', call, result)
        except OSError as e:
            # function produced some error -- move this to client???
            result = f"Error: {e}"
        except Exception as e:
            result = f"Ill-formed function call: {e}"
        return result

    def query(
        self,
        prompt: str,
        **kwargs
    ):
        if self._stream_response:
            return self._streamed_query(prompt=prompt, **kwargs)
        else:
            return self._batch_query(prompt=prompt, **kwargs)


    def _batch_query(
        self,
        prompt: str,
        **kwargs
    ):
        start = time.time()
        cost = 0

        try:
            self._broadcast("begin_query", prompt, **kwargs)
            self._conversation.append({"role": "user", "content": prompt})

            while True:
                self._conversation = litellm.utils.trim_messages(self._conversation, self._model)

                completion = self.completion()

                cost += litellm.completion_cost(completion)

                response_message = completion.choices[0].message
                self._conversation.append(response_message)
                
                if response_message.content:
                    self._broadcast('response', '(Message) ' + response_message.content)

                if completion.choices[0].finish_reason == 'tool_calls':
                    self._add_function_results_to_conversation(response_message)
                else:
                    break

            elapsed = time.time() - start
            stats = {
                "cost": cost,
                "time": elapsed,
                "model": self._model,
                "tokens": completion.usage.total_tokens,
                "prompt_tokens": completion.usage.prompt_tokens,
                "completion_tokens": completion.usage.completion_tokens,
            }
            self._broadcast("end_query", stats)
            return stats
        except openai.OpenAIError as e:
            self._broadcast('fail', f"Internal Error: {e.__dict__}")
            sys.exit(1)

    def _streamed_query(
        self,
        prompt: str,
        **kwargs
    ):
        start = time.time()
        cost = 0

        try:
            self._broadcast("begin_query", prompt, **kwargs)
            self._conversation.append({"role": "user", "content": prompt})

            while True:
                self._conversation = litellm.utils.trim_messages(self._conversation, self._model)
                # print("\n".join([str(x) for x in self._conversation]))

                stream = self.completion(stream=True)

                # litellm is broken for new GPT models that have content before calls, so...

                # stream the response, collecting the tool_call parts separately from the content
                self._broadcast('begin_stream')
                chunks = []
                tool_chunks = []
                for chunk in stream:
                    chunks.append(chunk)
                    if chunk.choices[0].delta.content != None:
                        self._broadcast('stream_delta', chunk.choices[0].delta.content)
                    else:
                        tool_chunks.append(chunk)
                self._broadcast('end_stream')

                # compute for the part that litellm gives back.
                completion = litellm.stream_chunk_builder(chunks, messages=self._conversation)
                cost += litellm.completion_cost(completion)

                # add content to conversation, but if there is no content, then the message
                # has only tool calls, and skip this step
                response_message = completion.choices[0].message
                if response_message.content != None:
                    self._conversation.append(response_message)
                
                if response_message.content != None:
                    self._broadcast('response', '(Message) ' + response_message.content)

                if completion.choices[0].finish_reason == 'tool_calls':
                    # create a message with just the tool calls, append that to the conversation, and generate the responses.
                    tool_completion = litellm.stream_chunk_builder(tool_chunks,self._conversation)

                    # this part wasn't counted above...
                    cost += litellm.completion_cost(tool_completion)

                    tool_message = tool_completion.choices[0].message
                    cost += litellm.completion_cost(tool_completion)
                    self._conversation.append(tool_message)
                    self._add_function_results_to_conversation(tool_message)
                else:
                    break

            elapsed = time.time() - start
            stats = {
                "cost": cost,
                "time": elapsed,
                "model": self._model,
                "tokens": completion.usage.total_tokens,
                "prompt_tokens": completion.usage.prompt_tokens,
                "completion_tokens": completion.usage.completion_tokens,
            }
            self._broadcast("end_query", stats)
            return stats
        except openai.OpenAIError as e:
            self._broadcast('fail', f"Internal Error: {e.__dict__}")
            sys.exit(1)


    def completion(self, stream=False):
        return litellm.completion(
            model=self._model,
            messages=self._conversation,
            tools=[
                {"type": "function", "function": f["schema"]}
                for f in self._functions.values()
            ],
            timeout=self._timeout,
            logger_fn=self._logger,
            stream=stream
        )

    def _add_function_results_to_conversation(self, response_message):
        response_message['role'] = 'assistant'
        tool_calls = response_message.tool_calls
        try:
            for tool_call in tool_calls:
                function_response = self._make_call(tool_call)
                function_response = self._sandwhich_tokens(
                                                                 function_response, 
                                                                 self._max_call_response_tokens,
                                                                 0.5)
                response = {
                                "tool_call_id": tool_call.id,
                                "role": "tool",
                                "name": tool_call.function.name,
                                "content": function_response,
                            }
                self._conversation.append(response)
        except Exception as e:
                        # Warning: potential infinite loop.
            self._broadcast('warn', f"Error processing tool calls: {e}")

if __name__ == '__main__':
    def weather(location,unit='f'):
        """
        {
            "name": "get_weather",
            "description": "Determine weather in my location",
            "parameters": {
                "type": "object",
                "properties": {
                "location": {
                    "type": "string",
                    "description": "The city and state e.g. San Francisco, CA"
                },
                "unit": {
                    "type": "string",
                    "enum": [
                    "c",
                    "f"
                    ]
                }
                },
                "required": [
                "location"
                ]
            }
        }
        """
        return f"weather(location, unit)", "Sunny and 72 degrees."



    a = Assistant("You generate text.", clients=[ StreamingAssistantClient() ], functions=[weather])
    x = a.query("tell me what model you are before making any function calls.  And what's the weather in Boston?", stream=True)
    print(x)

