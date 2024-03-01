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

# Example usage
def example_function():
    a = 1
    b = 2
    import math
    c = math.sqrt(a + b)
    def inner_function():
        d = 4
        e = 5

    my_locals = get_defined_symbols(example_function)
    for l in my_locals:
        print(f'{l} = {locals()[l]}')

example_function()