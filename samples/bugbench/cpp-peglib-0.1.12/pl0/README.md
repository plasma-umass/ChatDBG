PL/0 language example
=====================

  https://en.wikipedia.org/wiki/PL/0

  * PL/0 PEG syntax
  * AST generation with symbol scope
  * Interpreter (slow...)
  * LLVM Code generation
  * LLVM JIT execution (fast!)

Build
-----

```
brew install llvm
export PATH="$PATH:/usr/local/opt/llvm/bin"
make
```

Usage
-----

```
pl0 PATH [--ast] [--llvm] [--jit]

  --ast: Show AST tree
  --llvm: Dump LLVM IR
  --jit: LLVM JIT execution
```
