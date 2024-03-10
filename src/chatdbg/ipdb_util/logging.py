from io import StringIO
from datetime import datetime
import uuid
import sys
import yaml
from .config import Chat

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

    def __init__(self, config : Chat):
        self.steps = [ ]
        self.meta = {
            'time' : datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            'command_line' : ' '.join(sys.argv),
            'uid' : str(uuid.uuid4()),
            'config' : config.to_json(),
            'mark': '?',
        }
        self.log = config.log
        self._instructions = ''
        self.stdout_wrapper = CopyingTextIOWrapper(sys.stdout)
        self.stderr_wrapper = CopyingTextIOWrapper(sys.stderr)
        sys.stdout = self.stdout_wrapper
        sys.stderr = self.stdout_wrapper
        self.chat_step = None
        self.mark = '?'


    def add_mark(self, value):
        if value not in [ 'Fix', 'Partial', 'None', '?' ]:
            print(f"answer must be in { ['Fix', 'Partial', 'None', '?'] }")
        else:
            self.meta['mark'] = value

    def total(self, key):
        return sum([ x['stats'][key] for x in self.steps if x['output']['type'] == 'chat'])

    def dump(self): 
        self.meta['total_tokens'] = self.total('tokens')
        self.meta['total_time'] = self.total('time')
        self.meta['total_cost'] = self.total('cost')

        full_json = [{
            'meta' : self.meta,
            'steps' : self.steps,
            'instructions' : self._instructions,
            'stdout' : self.stdout_wrapper.getvalue(),
            'stderr' : self.stderr_wrapper.getvalue()
        }]
        
        print(f'*** Write ChatDBG log to {self.log}')
        with open(self.log, 'a') as file:
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
            },
            'stats' : { 
                'tokens' : 0,
                'cost' : 0, 
                'time' : 0
            }
        }

    def pop_chat(self, stats):
        self.chat_step['stats'] = stats

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


# Custom representer for literal scalar representation
def literal_presenter(dumper, data):
    if '\n' in data:
        return dumper.represent_scalar('tag:yaml.org,2002:str', data, style='|')
    return dumper.represent_scalar('tag:yaml.org,2002:str', data)

yaml.add_representer(str, literal_presenter)
