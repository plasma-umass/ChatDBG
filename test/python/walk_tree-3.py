import inspect
import ast

def get_defined_symbols(func):
    source = inspect.getsource(func)
    tree = ast.parse(source)

    defined_symbols = set()

    for node in ast.walk(tree):
        if isinstance(node, ast.FunctionDef):
            # Skip over function definitions
            continue
        elif isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name):
                    defined_symbols.add(target.id)

    # Symbols that are defined in the code and not imported
    return defined_symbols

def print_locals():
    frame = inspect.currentframe()

    # Move up to the frame of the caller
    caller_frame = frame.f_back

    my_locals = get_defined_symbols(caller_frame)
    for l in my_locals:
        print(f'{l} = {caller_frame.f_locals[l]}')




# Example usage
def example_function():
    a = 1
    b = 2
    import math
    c = math.sqrt(a + b)
    def inner_function():
        d = 4
        e = 5
    print_locals()


example_function()