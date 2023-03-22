# ChatDBG

by [Emery Berger](https://emeryberger.com)

ChatDBG is an experimental Python debugger that integrates large language models to help debug your code. With ChatDBG, you can ask your debugger "why" your program failed, and it will provide a suggested fix. As far as we are aware, ChatDBG is the first debugger to automatically perform root cause analysis and to provide suggested fixes. This is an alpha release; we greatly welcome feedback and suggestions!

## Installation

You can install ChatDBG using `pip`:

```
python3 -m pip install chatdbg
```


## Usage

To use ChatDBG, simply run your Python script with the `-m` flag:

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

