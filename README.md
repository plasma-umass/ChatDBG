# ChatDBG

by [Emery Berger](https://emeryberger.com)

ChatDBG is an experimental debugger (for Python *and* (new) native code) that integrates large language models to help debug your code. With ChatDBG, you can ask your debugger "why" your program failed, and it will provide a suggested fix. As far as we are aware, ChatDBG is the first debugger to automatically perform root cause analysis and to provide suggested fixes. This is an alpha release; we greatly welcome feedback and suggestions!

[![PyPI Latest Release](https://img.shields.io/pypi/v/chatdbg.svg)](https://pypi.org/project/chatdbg/)[![Downloads](https://pepy.tech/badge/chatdbg)](https://pepy.tech/project/chatdbg) [![Downloads](https://pepy.tech/badge/chatdbg/month)](https://pepy.tech/project/chatdbg) ![Python versions](https://img.shields.io/pypi/pyversions/chatdbg.svg?style=flat-square)


## Installation

Install ChatDBG using `pip`:

```
python3 -m pip install chatdbg
```

## Usage (Python)

To use ChatDBG to debug Python programs, simply run your Python script with the `-m` flag:

```
python3 -m chatdbg -c continue yourscript.py
```

or just

```
chatdbg -c continue yourscript.py
```

ChatDBG is an extension of the standard Python debugger `pdb`. Like
`pdb`, when your script encounters an uncaught exception, ChatDBG will
enter post mortem debugging mode.

Unlike other debuggers, you can then use the `why` command to ask
ChatDBG why your program failed and get a suggested fix.

For example:

```
Traceback (most recent call last):
  File "yourscript.py", line 9, in <module>
    print(tryme(100))
  File "yourscript.py", line 4, in tryme
    if x / i > 2:
ZeroDivisionError: division by zero
Uncaught exception. Entering post mortem debugging
Running 'cont' or 'step' will restart the program
> yourscript.py(4)tryme()
-> if x / i > 2:
(Pdb) why
```


ChatDBG will then provide a helpful explanation of why your program failed and a suggested fix:

```
The root cause of the error is that the code is attempting to
divide by zero in the line "if x / i > 2". As i ranges from 0 to 99,
it will eventually reach the value of 0, causing a ZeroDivisionError.

A possible fix for this would be to add a check for i being equal to
zero before performing the division. This could be done by adding an
additional conditional statement, such as "if i == 0: continue", to
skip over the iteration when i is zero. The updated code would look
like this:

def tryme(x):
    count = 0
    for i in range(100):
        if i == 0:
            continue
        if x / i > 2:
            count += 1
    return count

if __name__ == '__main__':
    print(tryme(100))
```

## Usage (lldb)

Install ChatDBG into the `lldb` debugger by running the following command:

### Linux

```
python3 -m pip install ChatDBG
python3 -c 'import chatdbg; print(f"command script import {chatdbg.__path__[0]}/chatdbg_lldb.py")' >> ~/.lldbinit
```

### Mac

```
xcrun python3 -m pip install ChatDBG
xcrun python3 -c 'import chatdbg; print(f"command script import {chatdbg.__path__[0]}/chatdbg_lldb.py")' >> ~/.lldbinit
```

This will install ChatDBG as an LLVM extension.

You can now run native code (compiled with `-g` for debugging symbols) with `lldb`; when it crashes, ask `why`.

<details>
<summary>
<B>Example of using `why` in LLDB</B>
</summary>

```
(lldb) run
Process 91113 launched: '/Users/emery/git/chatdbg/test/a.out' (arm64)
TEST 1
TEST -422761288
TEST 0
TEST 0
TEST 0
TEST 0
TEST 0
TEST 0
Process 91113 stopped
* thread #1, queue = 'com.apple.main-thread', stop reason = EXC_BAD_ACCESS (code=1, address=0x100056200)
    frame #0: 0x0000000100002f68 a.out`foo(n=8) at test.cpp:7:22
   4     int x[] = { 1, 2, 3, 4, 5 };
   5     
   6     void foo(int n) {
-> 7       cout << "TEST " << x[n * 10000] << endl;
   8     }
   9     
   10    int main()
Target 0: (a.out) stopped.
```

Now you can ask `why`:

```
(lldb) why
The root cause of this error is an out-of-bounds memory access. The
program is trying to access an element of the `x` array that is beyond
its allocated size. Specifically, when `n` is large enough (greater
than or equal to 1), the expression `n * 10000` causes the program to
access memory beyond the end of the `x` array.

To fix this error, we can check that the index is within bounds before
accessing the array. One way to do this is to compare `n * 10000` with
the size of the array before accessing the element:

    ```
    void foo(int n) {
      if (n * 10000 < sizeof(x)/sizeof(int)) {
        cout << "TEST " << x[n * 10000] << endl;
      } else {
        cout << "ERROR: index out of bounds" << endl;
      }
    }
    ```

This code first computes `sizeof(x)/sizeof(int)`, which gives the
number of elements in the `x` array. It then checks whether `n *
10000` is less than this size before accessing the `x` array. If `n *
10000` is greater than or equal to the size of the array, it prints an
error message instead of accessing the `x` array.
```
</details>



## Usage (gdb)

Install ChatDBG into the `gdb` debugger by running the following command:

```
python3 -m pip install ChatDBG
python3 -c 'import chatdbg; print(f"source {chatdbg.__path__[0]}/chatdbg_gdb.py")' >> ~/.gdbinit
```

This will install ChatDBG as a GDB extension. You can now run native code (compiled with `-g` for debugging symbols) with `gdb`; when it crashes, ask `why`.

