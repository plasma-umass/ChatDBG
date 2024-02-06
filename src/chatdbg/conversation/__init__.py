import textwrap

import llm_utils

from . import functions


def get_truncated_error_message(args, diagnostic) -> str:
    """
    Alternate taking front and back lines until the maximum number of tokens.
    """
    front: list[str] = []
    back: list[str] = []
    diagnostic_lines = diagnostic.splitlines()
    n = len(diagnostic_lines)

    def build_diagnostic_string():
        return "\n".join(front) + "\n\n[...]\n\n" + "\n".join(reversed(back)) + "\n"

    for i in range(n):
        if i % 2 == 0:
            line = diagnostic_lines[i // 2]
            list = front
        else:
            line = diagnostic_lines[n - i // 2 - 1]
            list = back
        list.append(line)
        count = llm_utils.count_tokens(args.llm, build_diagnostic_string())
        if count > args.max_error_tokens:
            list.pop()
            break

    if len(front) + len(back) == n:
        return diagnostic
    return build_diagnostic_string()


def converse(client, args, diagnostic):
    fns = functions.Functions(args)
    available_functions_names = [fn["function"]["name"] for fn in fns.as_tools()]
    system_message = textwrap.dedent(
        f"""
            You are an assistant debugger. The user is having an issue with their code, and you are trying to help them.
            A few functions exist to help with this process, namely: {", ".join(available_functions_names)}.
            Don't hesitate to call as many functions as needed to give the best possible answer.
            Once you have identified the problem, explain the diagnostic and provide a way to fix the issue if you can.
        """
    ).strip()
    user_message = f"Here is my error message:\n\n```\n{get_truncated_error_message(args, diagnostic)}\n```\n\nWhat's the problem?"
    conversation = [
        {"role": "system", "content": system_message},
        {"role": "user", "content": user_message},
    ]

    if args.show_prompt:
        print("System message:", system_message)
        print("User message:", user_message)
        return

    while True:
        completion = client.chat.completions.create(
            model=args.llm,
            messages=conversation,
            tools=fns.as_tools(),
        )

        choice = completion.choices[0]
        if choice.finish_reason == "tool_calls":
            for tool_call in choice.message.tool_calls:
                function_response = fns.dispatch(tool_call.function)
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
