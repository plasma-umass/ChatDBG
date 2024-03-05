import ast
import inspect
import textwrap

class SymbolFinder(ast.NodeVisitor):
    def __init__(self):
        self.defined_symbols = set()

    def visit_Assign(self, node):
        for target in node.targets:
            if isinstance(target, ast.Name):
                self.defined_symbols.add(target.id)
        self.generic_visit(node)

    def visit_For(self, node):
        if isinstance(node.target, ast.Name):
            self.defined_symbols.add(node.target.id)
        self.generic_visit(node)

    def visit_comprehension(self, node):
        if isinstance(node.target, ast.Name):
            self.defined_symbols.add(node.target.id)
        self.generic_visit(node)

def extract_locals(frame):
    try: 
        source = textwrap.dedent(inspect.getsource(frame))
        tree = ast.parse(source)

        finder = SymbolFinder()
        finder.visit(tree)

        args, varargs, keywords, locals = inspect.getargvalues(frame)
        parameter_symbols = set(args + [ varargs, keywords ])
        parameter_symbols.discard(None)

        return (finder.defined_symbols | parameter_symbols) & locals.keys()
    except:
        # ipes
        return set()

def extract_nb_globals(globals):
    result = set()
    for source in globals["In"]:
        try:
            tree = ast.parse(source)
            finder = SymbolFinder()
            finder.visit(tree)
            result = result | (finder.defined_symbols & globals.keys())
        except Exception as e:
            pass
    return result