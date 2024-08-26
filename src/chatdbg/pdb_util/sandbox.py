import ast
import re
from chatdbg.util.config import chatdbg_config


def _sandboxed_call(func, *args, **kwargs):
    """
    Check if the function is in the module whitelist before calling it.
    """
    allowed_modules = chatdbg_config.get_module_whitelist()

    # Get the module name of the function.
    # If the module name is None, use the __name__ attribute of the globals dictionary.
    module_name = func.__module__
    if module_name is None:
        module_name = func.__globals__.get("__name__", None)

    # Check if the function is in the module whitelist. If it is, call the function.
    # Otherwise, raise an ImportError.
    if any(
        re.fullmatch(allowed, f"{module_name}.{func.__name__}")
        for allowed in allowed_modules
    ):
        return func(*args, **kwargs)
    else:
        raise ImportError(
            f"Calling function {func.__name__} from module {module_name} is not allowed."
        )


class SandboxTransformer(ast.NodeTransformer):
    """
    Wrap all function calls in the expression with a call to _sandboxed_call.
    """

    def visit_Call(self, node):
        new_node = ast.Call(
            func=ast.Name(id="_sandboxed_call", ctx=ast.Load()),
            args=[node.func] + node.args,
            keywords=node.keywords,
        )
        return ast.copy_location(new_node, node)


def sandbox_eval(expression, globals, locals):
    """
    Wrap all function calls in the expression with a call to _sandboxed_call.
    This function will raise an ImportError if the function is not in the module whitelist.
    """
    tree = ast.parse(expression, mode="eval")
    tree = SandboxTransformer().visit(tree)
    ast.fix_missing_locations(tree)
    code = compile(tree, filename="<ast>", mode="eval")
    globals = globals.copy()
    globals["_sandboxed_call"] = _sandboxed_call
    return eval(code, globals, locals)
