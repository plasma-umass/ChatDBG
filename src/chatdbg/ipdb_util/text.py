import re
import itertools

def make_arrow(pad):
    """generate the leading arrow in front of traceback or debugger"""
    if pad >= 2:
        return '-'*(pad-2) + '> '
    elif pad == 1:
        return '>'
    return ''

def strip_color(s):
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    return ansi_escape.sub('', s)

def format_limited(value, limit=10):

    def format_list(list):
        return format([ format_limited(x, limit) for x in list ])

    def format_dict(items):
        return format({ format_limited(k, limit) :
                        format_limited(v, limit) for k,v in items })

    def format(v):
        result = str(v)
        if len(result) > 512:
            result = result[:512-3] + '...'
        return result

    if isinstance(value, dict):
        if len(value) > limit:
            return format_dict(list(value.items())[:limit-1] + [('...', '...')])
        else:
            return format_dict(value.items())
    elif isinstance(value, (str,bytes)):
        return format(value)
    elif hasattr(value, '__iter__'):
        value = list(itertools.islice(value, 0, limit + 1))
        if len(value) > limit:
            return format_list(value[:limit-1] + [ '...' ])
        else:
            return format_list(value)
    else:
        return format(value)

def truncate_proportionally(text, maxlen=32000, top_proportion = 0.5):
    """Omit part of a string if needed to make it fit in a maximum length."""
    if len(text) > maxlen:
        pre = max(0, int((maxlen-3) * top_proportion))
        post = max(0, maxlen-3-pre)
        return text[:pre] + '...' + text[len(text)-post:]
    return text

