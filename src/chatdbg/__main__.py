import os

from . import chatdbg
    
def api_key() -> str:
    """
    Get the API key from the environment variable 'OPENAI_API_KEY'.
    
    :return: The value of the environment variable 'OPENAI_API_KEY'.
    :rtype: str
    """
    key = ''
    try:
        key = os.environ['OPENAI_API_KEY']
    except:
        pass
    return key

def main():
    chatdbg.main()

main()
