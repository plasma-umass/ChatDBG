import argparse
import json
import textwrap

import lldb
import llm_utils

from .functions_lldb import LldbFunctions


# This is LLDB-only (for now?).
def converse(client, args: argparse.Namespace):
    fns = LldbFunctions(args)
    available_functions_names = [fn["function"]["name"] for fn in fns.as_tools()]
    system_message = textwrap.dedent(
        f"""
            You are an assistant debugger. The user is having an issue with their code, and you are trying to help them.
            A few functions exist to help with this process, namely: {", ".join(available_functions_names)}.
            Don't hesitate to call as many functions as needed to give the best possible answer.
            Once you have identified the problem, explain the diagnostic and provide a way to fix the issue if you can.
        """
    ).strip()
    user_message = f"""Here is the reason the program stopped execution:
```
{fns.get_error_message()}
```

Here is a summary of the stack frames:
```
{fns.get_frame_summary()}
```

What's the problem?"""
    conversation = [
        {"role": "system", "content": system_message},
        {"role": "user", "content": user_message},
    ]

    if args.show_prompt:
        return f"""System:\n{system_message}\n\nUser:\n{user_message}"""

    while True:
        completion = client.chat.completions.create(
            model=args.llm,
            messages=conversation,
            tools=fns.as_tools(),
        )

        choice = completion.choices[0]
        if choice.finish_reason == "tool_calls":
            for tool_call in choice.message.tool_calls:
                name = tool_call.function.name
                arguments = json.loads(tool_call.function.arguments)
                function_response = fns.dispatch(name, arguments)
                if function_response:
                    conversation.append(choice.message)
                    conversation.append(
                        {
                            "tool_call_id": tool_call.id,
                            "role": "tool",
                            "name": tool_call.function.name,
                            "content": function_response,
                        }
                    )
        elif choice.finish_reason == "stop":
            text = completion.choices[0].message.content
            return llm_utils.word_wrap_except_code_blocks(text)
        else:
            print(f"Not found: {choice.finish_reason}.")
