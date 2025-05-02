import ast
import inspect
import itertools
import numbers
import textwrap

import numpy as np

from io import StringIO
from types import FrameType
from typing import Any, Union


class SymbolFinder(ast.NodeVisitor):
    def __init__(self):
        self.defined_symbols = set()

    def visit_Assign(self, node: ast.Assign) -> None:
        for target in node.targets:
            if isinstance(target, ast.Name):
                self.defined_symbols.add(target.id)
        self.generic_visit(node)

    def visit_For(self, node: ast.For) -> None:
        if isinstance(node.target, ast.Name):
            self.defined_symbols.add(node.target.id)
        self.generic_visit(node)

    def visit_comprehension(self, node: ast.Name) -> None:
        if isinstance(node.target, ast.Name):
            self.defined_symbols.add(node.target.id)
        self.generic_visit(node)


def _extract_locals(frame: FrameType) -> set[str]:
    try:
        source = textwrap.dedent(inspect.getsource(frame))
        tree = ast.parse(source)

        finder = SymbolFinder()
        finder.visit(tree)

        args, varargs, keywords, locals = inspect.getargvalues(frame)
        parameter_symbols = set(args + [varargs, keywords])
        parameter_symbols.discard(None)

        return (finder.defined_symbols | parameter_symbols) & locals.keys()
    except:
        # ipes
        return set()


def _is_iterable(obj: Any) -> bool:
    try:
        iter(obj)
        return True
    except TypeError:
        return False


def _repr_if_defined(obj: Any) -> bool:
    if obj.__class__ in [np.ndarray, dict, list, tuple]:
        # handle these at iterables to truncate reasonably
        return False
    result = (
        "__repr__" in dir(obj.__class__)
        and obj.__class__.__repr__ is not object.__repr__
    )
    return result


def _format_limited(
    value: Union[int, np.ndarray], limit: int = 10, depth: int = 3
) -> str:
    def format_tuple(t, depth):
        return tuple([helper(x, depth) for x in t])

    def format_list(list, depth):
        return [helper(x, depth) for x in list]

    def format_dict(items, depth):
        return {k: helper(v, depth) for k, v in items}

    def format_object(obj, depth):
        attributes = dir(obj)
        fields = {
            attr: getattr(obj, attr, None)
            for attr in attributes
            if not callable(getattr(obj, attr, None)) and not attr.startswith("__")
        }
        return format(
            f"{type(obj).__name__} object with fields {format_dict(fields.items(), depth)}"
        )

    def helper(value, depth):
        if depth == 0:
            return ...
        if value is Ellipsis:
            return ...
        if isinstance(value, dict):
            if len(value) > limit:
                return format_dict(
                    list(value.items())[: limit - 1] + [(..., ...)], depth - 1
                )
            else:
                return format_dict(value.items(), depth - 1)
        elif isinstance(value, (str, bytes)):
            if len(value) > 254:
                value = str(value)[0:253] + "..."
            return value
        elif isinstance(value, tuple):
            if len(value) > limit:
                return format_tuple(value[0 : limit - 1] + (...,), depth - 1)
            else:
                return format_tuple(value, depth - 1)
        elif value is None or isinstance(
            value, (int, float, bool, type, numbers.Number)
        ):
            return value
        elif isinstance(value, np.ndarray):
            with np.printoptions(threshold=limit):
                return np.array_repr(value)
        elif inspect.isclass(type(value)) and _repr_if_defined(value):
            return repr(value)
        elif _is_iterable(value):
            value = list(itertools.islice(value, 0, limit + 1))
            if len(value) > limit:
                return format_list(value[: limit - 1] + [...], depth - 1)
            else:
                return format_list(value, depth - 1)
        elif inspect.isclass(type(value)):
            return format_object(value, depth - 1)
        else:
            return value

    result = str(helper(value, depth=3)).replace("Ellipsis", "...")
    if len(result) > 1024 * 2:
        result = result[: 1024 * 2 - 3] + "..."
    if type(value) == str:
        return "'" + result + "'"
    else:
        return result


def print_locals(file: StringIO, frame: FrameType) -> None:
    locals = frame.f_locals
    in_global_scope = locals is frame.f_globals
    defined_locals = _extract_locals(frame)
    # Unclear benefit: possibly some benefit w/ stack only runs, but large context...
    # if in_global_scope and "In" in locals:  # in notebook
    #     defined_locals = defined_locals | extract_nb_globals(locals)
    if len(defined_locals) > 0:
        if in_global_scope:
            print(f"    Global variables:", file=file)
        else:
            print(f"    Variables in this frame:", file=file)
        for name in sorted(defined_locals):
            value = locals[name]
            t = type(value).__name__
            prefix = f"      {name}: {t} = "
            rep_list = _format_limited(value, limit=20).split("\n")
            if len(rep_list) > 1:
                rep = (
                    prefix
                    + rep_list[0]
                    + "\n"
                    + textwrap.indent("\n".join(rep_list[1:]), prefix=" " * len(prefix))
                )
            else:
                rep = prefix + rep_list[0]
            print(rep, file=file)
        print(file=file)
