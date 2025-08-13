# ChatDBG

by [Emery Berger](https://emeryberger.com), [Stephen Freund](https://www.cs.williams.edu/~freund/index.html), [Kyla Levin](https://khlevin.github.io/KylaHLevin/index.html), [Nicolas van Kempen](https://nvankempen.com/) (ordered alphabetically)

[![PyPI Latest Release](https://img.shields.io/pypi/v/chatdbg.svg)](https://pypi.org/project/chatdbg/)
[![Downloads](https://static.pepy.tech/badge/chatdbg)](https://pepy.tech/project/chatdbg)
[![Downloads](https://static.pepy.tech/badge/chatdbg/month)](https://pepy.tech/project/chatdbg)

[Read the paper!](https://raw.githubusercontent.com/plasma-umass/ChatDBG/main/ChatDBG.pdf)

ChatDBG is an AI-based debugging assistant for C/C++/Python/Rust code that integrates large language models into a standard debugger (`pdb`, `lldb`, `gdb`) to help debug your code. With ChatDBG, you can engage in a dialog with your debugger, asking open-ended questions about your program, like `why is x null?`. ChatDBG will _take the wheel_ and steer the debugger to answer your queries. ChatDBG can provide error diagnoses and suggest fixes.

As far as we are aware, ChatDBG is the _first_ debugger to automatically perform root cause analysis and to provide suggested fixes.

**Watch ChatDBG in action!**
| LLDB on [test-overflow.cpp](https://github.com/plasma-umass/ChatDBG/blob/main/samples/cpp/test-overflow.cpp) | GDB on [test-overflow.cpp](https://github.com/plasma-umass/ChatDBG/blob/main/samples/cpp/test-overflow.cpp) | Pdb on [bootstrap.py](https://github.com/plasma-umass/ChatDBG/blob/main/samples/python/bootstrap.py) |
|:-------------------------:|:-------------------------:|:-------------------------:|
| <a href="https://asciinema.org/a/RsAGFFmsicIvMW8xgvPP6PW2f" target="_blank"><img src="https://raw.githubusercontent.com/plasma-umass/ChatDBG/main/media/lldb.svg" /></a>| <a href="https://asciinema.org/a/bMWOyyrh7WXWsTCFboyKpqwTq" target="_blank"><img src="https://raw.githubusercontent.com/plasma-umass/ChatDBG/main/media/gdb.svg" /></a>|<a href="https://asciinema.org/a/qulxiJTqwVRJPaMZ1hcBs6Clu" target="_blank"><img src="https://raw.githubusercontent.com/plasma-umass/ChatDBG/main/media/pdb.svg" /></a>|

For technical details and a complete evaluation, see our FSE'25 paper, [_ChatDBG: An AI-Powered Debugging Assistant_](https://dl.acm.org/doi/10.1145/3729355) ([PDF](https://raw.githubusercontent.com/plasma-umass/ChatDBG/main/ChatDBG.pdf)).

> [!NOTE]
>
> ChatDBG for `pdb`, `lldb`, and `gdb` are feature-complete; we are currently backporting features for these debuggers into the other debuggers.

## Installation

> [!IMPORTANT]
>
> ChatDBG currently needs to be connected to an [OpenAI account](https://openai.com/api/). _Your account will need to have a positive balance for this to work_ ([check your balance](https://platform.openai.com/account/usage)). If you have never purchased credits, you will need to purchase at least \$1 in credits (if your API account was created before August 13, 2023) or \$0.50 (if you have a newer API account) in order to have access to GPT-4, which ChatDBG uses. [Get a key here.](https://platform.openai.com/account/api-keys)
>
> Once you have an API key, set it as an environment variable called `OPENAI_API_KEY`.
>
> ```bash
> export OPENAI_API_KEY=<your-api-key>
> ```

Install ChatDBG using `pip` (you need to do this whether you are debugging Python, C, or C++ code):

```bash
python3 -m pip install chatdbg
```

If you are using ChatDBG to debug Python programs, you are done. If you want to use ChatDBG to debug native code with `gdb` or `lldb`, follow the installation instructions below.

### Installing as an `lldb` extension

<details>
<summary>
<B><TT>lldb</TT> installation instructions</B>
</summary>

Install ChatDBG into the `lldb` debugger by running the following command:

#### Linux

```bash
python3 -m pip install ChatDBG
python3 -c 'import chatdbg; print(f"command script import {chatdbg.__path__[0]}/chatdbg_lldb.py")' >> ~/.lldbinit
```

If you encounter an error, you may be using an older version of LLVM. Update to the latest version as follows:

```
sudo apt install -y lsb-release wget software-properties-common gnupg
curl -sSf https://apt.llvm.org/llvm.sh | sudo bash -s -- 18 all
# LLDB now available as `lldb-18`.
```

#### Mac

```bash
xcrun python3 -m pip install ChatDBG
xcrun python3 -c 'import chatdbg; print(f"command script import {chatdbg.__path__[0]}/chatdbg_lldb.py")' >> ~/.lldbinit
```

This will install ChatDBG as an LLVM extension.

</details>

### Installing as a `gdb` extension

<details>
<summary>
<B><TT>gdb</TT> installation instructions</B>
</summary>

Install ChatDBG into the `gdb` debugger by running the following command:

```bash
python3 -m pip install ChatDBG
python3 -c 'import chatdbg; print(f"source {chatdbg.__path__[0]}/chatdbg_gdb.py")' >> ~/.gdbinit
```

This will install ChatDBG as a GDB extension.

</details>

## Usage

### Debugging Python

To use ChatDBG to debug Python programs, simply run your Python script as follows:

```bash
chatdbg -c continue yourscript.py
```

ChatDBG is an extension of the standard Python debugger `pdb`. Like
`pdb`, when your script encounters an uncaught exception, ChatDBG will
enter post mortem debugging mode.

Unlike other debuggers, you can then use the `why` command to ask
ChatDBG why your program failed and get a suggested fix. After the LLM responds,
you may issue additional debugging commands or continue the conversation by entering
any other text.

#### IPython and Jupyter Support

To use ChatDBG as the default debugger for IPython or inside Jupyter Notebooks,
create a IPython profile and then add the necessary exensions on startup. (Modify
these lines as necessary if you already have a customized profile file.)

```bash
ipython profile create
echo "c.InteractiveShellApp.extensions = ['chatdbg.chatdbg_pdb', 'ipyflow']" >> ~/.ipython/profile_default/ipython_config.py
```

On the command line, you can then run:

```bash
ipython --pdb yourscript.py
```

Inside Jupyter, run your notebook with the [ipyflow kernel](https://github.com/ipyflow/ipyflow) and include this line magic at the top of the file.

```
%pdb
```

### Debugging native code (C, C++, or Rust with <TT>lldb</TT> / <TT>gdb</TT>)

To use ChatDBG with `lldb` or `gdb`, just run native code (compiled with `-g` for debugging symbols) with your choice of debugger; when it crashes, ask `why`. This also works for post mortem debugging (when you load a core with the `-c` option).

The native debuggers work slightly differently than Pdb. After the debugger responds to your question, you will enter into ChatDBG's command loop, as indicated by the `(ChatDBG chatting)` prompt. You may continue issuing debugging commands and you may send additional messages to the LLM by starting those messages with "chat". When you are done, type `quit` to return to the debugger's main command loop.

<details>
<summary>
<B>Debugging Rust programs</B>
</summary>

To use ChatDBG with Rust, you need to do two steps: modify your
`Cargo.toml` file and add one line to your source program.

1. Add this to your `Cargo.toml` file:

```toml
[dependencies]
chatdbg = "0.6.2"

[profile.dev]
panic = "abort"

[profile.release]
panic = "abort"
```

2. In your program, apply the `#[chatdbg::main]` attribute to your `main`
   function:

```rust
#[chatdbg::main]
fn main() {
```

Now you can debug your Rust code with `gdb` or `lldb`.

</details>

### Examples

<details>
<summary>
<B>ChatDBG example in <TT>lldb</TT></B>
</summary>

```gdb
(ChatDBG lldb) run
Process 85494 launched: '/Users/emery/git/ChatDBG/test/a.out' (arm64)
TEST 1
TEST -422761288
TEST 0
TEST 0
TEST 0
TEST 0
TEST 0
TEST 0
Process 85494 stopped
* thread #1, queue = 'com.apple.main-thread', stop reason = EXC_BAD_ACCESS (code=1, address=0x100056200)
    frame #0: 0x0000000100002f64 a.out`foo(n=8, b=1) at test.cpp:7:22
   4     int x[] = { 1, 2, 3, 4, 5 };
   5
   6     void foo(int n, float b) {
-> 7       cout << "TEST " << x[n * 10000] << endl;
   8     }
   9
   10    int main()
Target 0: (a.out) stopped.
```

Ask `why` to have ChatDBG provide a helpful explanation why this program failed, and suggest a fix:

````gdb
(ChatDBG lldb) why
The root cause of this error is accessing an index of the array `x`
that is out of bounds. In `foo()`, the index is calculated as `n *
10000`, which can be much larger than the size of the array `x` (which
is only 5 elements). In the given trace, the program is trying to
access the memory address `0x100056200`, which is outside of the range
of allocated memory for the array `x`.

To fix this error, we need to ensure that the index is within the
bounds of the array. One way to do this is to check the value of `n`
before calculating the index, and ensure that it is less than the size
of the array divided by the size of the element. For example, we can
modify `foo()` as follows:

    ```
    void foo(int n, float b) {
      if (n < 0 || n >= sizeof(x)/sizeof(x[0])) {
        cout << "ERROR: Invalid index" << endl;
        return;
      }
      cout << "TEST " << x[n] << endl;
    }
    ```

This code checks if `n` is within the valid range, and prints an error
message if it is not. If `n` is within the range, the function prints
the value of the element at index `n` of `x`. With this modification,
the program will avoid accessing memory outside the bounds of the
array, and will print the expected output for valid indices.
````

</details>

<details>
<summary>
<B>ChatDBG example in Python (<TT>pdb</TT>)</B>
</summary>

```python
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
```

Ask `why` to have ChatDBG provide a helpful explanation why this program failed, and suggest a fix:

```python
(ChatDBG Pdb) why
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

</details>
