import openai
import openai_async
import os
import sys

async def why(self, arg):
    user_prompt = 'Explain what the root cause of this error is, given the following source code and traceback, and propose a fix.'
    user_prompt += '\n'
    user_prompt += 'source code:\n```\n'

    stack_trace = ''
    
    stack_frames = len(self.stack)
    import sys
    exception_name = sys.exc_info()[0].__name__
    exception_value = sys.exc_info()[1]

    # print(dir(self))
    for frame_lineno in self.stack:
        import inspect
        frame, lineno = frame_lineno
        if not frame.f_code.co_filename.startswith(os.getcwd()):
            stack_frames -= 1
            continue
        positions = inspect.getframeinfo(frame).positions
        try:
            user_prompt += '#' + '-' * 60 + '\n'
            lines = inspect.getsourcelines(frame)[0]
            for index, line in enumerate(lines, frame.f_code.co_firstlineno):
                user_prompt += '  '
                user_prompt += line.rstrip() + '\n'
                if index == lineno:
                    leading_spaces = len(line) - len(line.lstrip())
                    stack_trace += f'{stack_frames}: ' + line.strip() + '\n'
                    stack_trace += ' ' * len(str(stack_frames)) + '  ' + ' ' * (positions.col_offset - leading_spaces) + '^' * (positions.end_col_offset - positions.col_offset) + '\n'
                if index >= lineno:
                    break
        except:
            pass
        stack_frames -= 1
    user_prompt += '```\n'
    user_prompt += 'stack trace:\n'
    user_prompt += f'```\n{stack_trace}```\n'
    user_prompt += f'Exception: {exception_name} ({exception_value})\n'

    # print(user_prompt)
    import httpx
    text = ''
    try:
        completion = await openai_async.chat_complete(openai.api_key, timeout=30, payload={'model': 'gpt-3.5-turbo', 'messages': [{'role': 'user', 'content': user_prompt}]})
        json_payload = completion.json()
        text = json_payload['choices'][0]['message']['content']
    except (openai.error.AuthenticationError, httpx.LocalProtocolError):
        print()
        print('You need an OpenAI key to use commentator. You can get a key here: https://openai.com/api/')
        print('Set the environment variable OPENAI_API_KEY to your key value.')
        import sys
        sys.exit(1)
    except Exception as e:
        print(f"EXCEPTION {e}")
        pass
    print(text)
