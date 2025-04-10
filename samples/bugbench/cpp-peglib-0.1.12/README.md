cpp-peglib
==========

[![Build Status](https://travis-ci.org/yhirose/cpp-peglib.svg?branch=master)](https://travis-ci.org/yhirose/cpp-peglib)
[![Bulid Status](https://ci.appveyor.com/api/projects/status/github/yhirose/cpp-peglib?branch=master&svg=true)](https://ci.appveyor.com/project/yhirose/cpp-peglib)

C++11 header-only [PEG](http://en.wikipedia.org/wiki/Parsing_expression_grammar) (Parsing Expression Grammars) library. You can start using it right away just by including `peglib.h` in your project.

You can also try the online version, PEG Playground at https://yhirose.github.io/cpp-peglib.

The PEG syntax is well described on page 2 in the [document](http://www.brynosaurus.com/pub/lang/peg.pdf). *cpp-peglib* also supports the following additional syntax for now:

  * `'...'i` (Case-insensitive literal operator)
  * `[^...]` (Negated character class operator)
  * `{2,5}` (Regex-like repetition operator)
  * `<` ... `>` (Token boundary operator)
  * `~` (Ignore operator)
  * `\x20` (Hex number char)
  * `%whitespace` (Automatic whitespace skipping)
  * `%word` (Word expression)
  * `$name(` ... `)` (Capture scope operator)
  * `$name<` ... `>` (Named capture operator)
  * `$name` (Backreference operator)
  * `|` (Dictionary operator)
  * `MACRO_NAME(` ... `)` (Parameterized rule or Macro)
  * `{ precedence L - + L / * }` (Parsing infix expression)

This library supports the linear-time parsing known as the [*Packrat*](http://pdos.csail.mit.edu/~baford/packrat/thesis/thesis.pdf) parsing.

  IMPORTANT NOTE for some Linux distributions such as Ubuntu and CentOS: Need `-pthread` option when linking. See [#23](https://github.com/yhirose/cpp-peglib/issues/23#issuecomment-261126127), [#46](https://github.com/yhirose/cpp-peglib/issues/46#issuecomment-417870473) and [#62](https://github.com/yhirose/cpp-peglib/issues/62#issuecomment-492032680).

How to use
----------

This is a simple calculator sample. It shows how to define grammar, associate samantic actions to the grammar, and handle semantic values.

```cpp
// (1) Include the header file
#include <peglib.h>
#include <assert.h>
#include <iostream>

using namespace peg;
using namespace std;

int main(void) {
    // (2) Make a parser
    parser parser(R"(
        # Grammar for Calculator...
        Additive    <- Multitive '+' Additive / Multitive
        Multitive   <- Primary '*' Multitive / Primary
        Primary     <- '(' Additive ')' / Number
        Number      <- < [0-9]+ >
        %whitespace <- [ \t]*
    )");

    assert((bool)parser == true);

    // (3) Setup actions
    parser["Additive"] = [](const SemanticValues& sv) {
        switch (sv.choice()) {
        case 0:  // "Multitive '+' Additive"
            return any_cast<int>(sv[0]) + any_cast<int>(sv[1]);
        default: // "Multitive"
            return any_cast<int>(sv[0]);
        }
    };

    parser["Multitive"] = [](const SemanticValues& sv) {
        switch (sv.choice()) {
        case 0:  // "Primary '*' Multitive"
            return any_cast<int>(sv[0]) * any_cast<int>(sv[1]);
        default: // "Primary"
            return any_cast<int>(sv[0]);
        }
    };

    parser["Number"] = [](const SemanticValues& sv) {
        return stoi(sv.token(), nullptr, 10);
    };

    // (4) Parse
    parser.enable_packrat_parsing(); // Enable packrat parsing.

    int val;
    parser.parse(" (1 + 2) * 3 ", val);

    assert(val == 9);
}
```

To show syntax errors in grammar text:

```cpp
auto grammar = R"(
    # Grammar for Calculator...
    Additive    <- Multitive '+' Additive / Multitive
    Multitive   <- Primary '*' Multitive / Primary
    Primary     <- '(' Additive ')' / Number
    Number      <- < [0-9]+ >
    %whitespace <- [ \t]*
)";

parser parser;

parser.log = [](size_t line, size_t col, const string& msg) {
    cerr << line << ":" << col << ": " << msg << "\n";
};

auto ok = parser.load_grammar(grammar);
assert(ok);
```

There are four semantic actions available:

```cpp
[](const SemanticValues& sv, any& dt)
[](const SemanticValues& sv)
[](SemanticValues& sv, any& dt)
[](SemanticValues& sv)
```

`SemanticValues` value contains the following information:

 - Semantic values
 - Matched string information
 - Token information if the rule is literal or uses a token boundary operator
 - Choice number when the rule is 'prioritized choise'

`any& dt` is a 'read-write' context data which can be used for whatever purposes. The initial context data is set in `peg::parser::parse` method.

`peg::any` is a simpler implementatin of std::any. If the compiler in use supports C++17, by default `peg::any` is defined as an alias to `std::any`.

To force using the simpler `any` implementation that comes with `cpp-peglib`, define `PEGLIB_USE_STD_ANY` as 0 before including `peglib.h`:
```cpp
#define PEGLIB_USE_STD_ANY 0
#include <peglib.h>
[...]
```

A semantic action can return a value of arbitrary data type, which will be wrapped by `peg::any`. If a user returns nothing in a semantic action, the first semantic value in the `const SemanticValues& sv` argument will be returned. (Yacc parser has the same behavior.)

Here shows the `SemanticValues` structure:

```cpp
struct SemanticValues : protected std::vector<any>
{
    // Input text
    const char* path;
    const char* ss;

    // Matched string
    std::string str() const;    // Matched string
    const char* c_str() const;  // Matched string start
    size_t      length() const; // Matched string length

    // Line number and column at which the matched string is
    std::pair<size_t, size_t> line_info() const;

    // Tokens
    std::vector<
        std::pair<
            const char*, // Token start
            size_t>>     // Token length
        tokens;

    std::string token(size_t id = 0) const;

    // Choice number (0 based index)
    size_t      choice() const;

    // Transform the semantic value vector to another vector
    template <typename T> vector<T> transform(size_t beg = 0, size_t end = -1) const;
}
```

The following example uses `<` ... ` >` operator, which is *token boundary* operator.

```cpp
auto syntax = R"(
    ROOT  <- _ TOKEN (',' _ TOKEN)*
    TOKEN <- < [a-z0-9]+ > _
    _     <- [ \t\r\n]*
)";

peg pg(syntax);

pg["TOKEN"] = [](const SemanticValues& sv) {
    // 'token' doesn't include trailing whitespaces
    auto token = sv.token();
};

auto ret = pg.parse(" token1, token2 ");
```

We can ignore unnecessary semantic values from the list by using `~` operator.

```cpp
peg::parser parser(R"(
    ROOT  <-  _ ITEM (',' _ ITEM _)*
    ITEM  <-  ([a-z])+
    ~_    <-  [ \t]*
)");

parser["ROOT"] = [&](const SemanticValues& sv) {
    assert(sv.size() == 2); // should be 2 instead of 5.
};

auto ret = parser.parse(" item1, item2 ");
```

The following grammar is same as the above.

```cpp
peg::parser parser(R"(
    ROOT  <-  ~_ ITEM (',' ~_ ITEM ~_)*
    ITEM  <-  ([a-z])+
    _     <-  [ \t]*
)");
```

*Semantic predicate* support is available. We can do it by throwing a `peg::parse_error` exception in a semantic action.

```cpp
peg::parser parser("NUMBER  <-  [0-9]+");

parser["NUMBER"] = [](const SemanticValues& sv) {
    auto val = stol(sv.str(), nullptr, 10);
    if (val != 100) {
        throw peg::parse_error("value error!!");
    }
    return val;
};

long val;
auto ret = parser.parse("100", val);
assert(ret == true);
assert(val == 100);

ret = parser.parse("200", val);
assert(ret == false);
```

*enter* and *leave* actions are also avalable.

```cpp
parser["RULE"].enter = [](const char* s, size_t n, any& dt) {
    std::cout << "enter" << std::endl;
};

parser["RULE"] = [](const SemanticValues& sv, any& dt) {
    std::cout << "action!" << std::endl;
};

parser["RULE"].leave = [](const char* s, size_t n, size_t matchlen, any& value, any& dt) {
    std::cout << "leave" << std::endl;
};
```

Ignoring Whitespaces
--------------------

As you can see in the first example, we can ignore whitespaces between tokens automatically with `%whitespace` rule.

`%whitespace` rule can be applied to the following three conditions:

  * trailing spaces on tokens
  * leading spaces on text
  * trailing spaces on literal strings in rules

These are valid tokens:

```
KEYWORD   <- 'keyword'
KEYWORDI  <- 'case_insensitive_keyword'
WORD      <-  < [a-zA-Z0-9] [a-zA-Z0-9-_]* >    # token boundary operator is used.
IDNET     <-  < IDENT_START_CHAR IDENT_CHAR* >  # token boundary operator is used.
```

The following grammar accepts ` one, "two three", four `.

```
ROOT         <- ITEM (',' ITEM)*
ITEM         <- WORD / PHRASE
WORD         <- < [a-z]+ >
PHRASE       <- < '"' (!'"' .)* '"' >

%whitespace  <-  [ \t\r\n]*
```

Word expression
---------------

```cpp
peg::parser parser(R"(
    ROOT         <-  'hello' 'world'
    %whitespace  <-  [ \t\r\n]*
    %word        <-  [a-z]+
)");

parser.parse("hello world"); // OK
parser.parse("helloworld");  // NG
```

Capture/Backreference
---------------------

```cpp
peg::parser parser(R"(
    ROOT      <- CONTENT
    CONTENT   <- (ELEMENT / TEXT)*
    ELEMENT   <- $(STAG CONTENT ETAG)
    STAG      <- '<' $tag< TAG_NAME > '>'
    ETAG      <- '</' $tag '>'
    TAG_NAME  <- 'b' / 'u'
    TEXT      <- TEXT_DATA
    TEXT_DATA <- ![<] .
)");

parser.parse("This is <b>a <u>test</u> text</b>."); // OK
parser.parse("This is <b>a <u>test</b> text</u>."); // NG
parser.parse("This is <b>a <u>test text</b>.");     // NG
```

Dictionary
----------

`|` operator allows us to make a word dictionary for fast lookup by using Trie structure internally. We don't have to worry about the order of words.

```peg
START <- 'This month is ' MONTH '.'
MONTH <- 'Jan' | 'January' | 'Feb' | 'February' | '...'
```

Parameterized Rule or Macro
---------------------------

```peg
# Syntax
Start      ← _ Expr
Expr       ← Sum
Sum        ← List(Product, SumOpe)
Product    ← List(Value, ProOpe)
Value      ← Number / T('(') Expr T(')')

# Token
SumOpe     ← T('+' / '-')
ProOpe     ← T('*' / '/')
Number     ← T([0-9]+)
~_         ← [ \t\r\n]*

# Macro
List(I, D) ← I (D I)*
T(x)       ← < x > _
```

Parsing infix expression by Precedence climbing
-----------------------------------------------

Regarding the *precedence climbing algorithm*, please see [this article](https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing).

```cpp
parser parser(R"(
    EXPRESSION               <-  INFIX_EXPRESSION(ATOM, OPERATOR)
    ATOM                     <-  NUMBER / '(' EXPRESSION ')'
    OPERATOR                 <-  < [-+/*] >
    NUMBER                   <-  < '-'? [0-9]+ >
    %whitespace              <-  [ \t]*

    # Declare order of precedence
    INFIX_EXPRESSION(A, O) <-  A (O A)* {
      precedence
        L + -
        L * /
    }
)");

parser["INFIX_EXPRESSION"] = [](const SemanticValues& sv) -> long {
    auto result = any_cast<long>(sv[0]);
    if (sv.size() > 1) {
        auto ope = any_cast<char>(sv[1]);
        auto num = any_cast<long>(sv[2]);
        switch (ope) {
            case '+': result += num; break;
            case '-': result -= num; break;
            case '*': result *= num; break;
            case '/': result /= num; break;
        }
    }
    return result;
};
parser["OPERATOR"] = [](const SemanticValues& sv) { return *sv.c_str(); };
parser["NUMBER"] = [](const SemanticValues& sv) { return atol(sv.c_str()); };

long val;
parser.parse(" -1 + (1 + 2) * 3 - -1", val);
assert(val == 9);
```

*precedence* instruction can be applied only to the following 'list' style rule.

```
Rule <- Atom (Operator Atom)* {
  precedence
    L - +
    L / *
    R ^
}
```

*precedence* instruction contains precedence info entries. Each entry starts with *associativity* which is 'L' (left) or 'R' (right), then operator tokens follow. The first entry has the highest order level.

AST generation
--------------

*cpp-peglib* is able to generate an AST (Abstract Syntax Tree) when parsing. `enable_ast` method on `peg::parser` class enables the feature.

```
peg::parser parser("...");

parser.enable_ast();

shared_ptr<peg::Ast> ast;
if (parser.parse("...", ast)) {
  cout << peg::ast_to_s(ast);

  std::vector<std::string> exceptions = { "defenition1", "defenition2 };
  ast = peg::AstOptimizer(true, exceptions).optimize(ast);

  cout << peg::ast_to_s(ast);
}
```

`peg::AstOptimizer` removes redundant nodes to make a AST simpler. You can make your own AST optimizers to fit your needs.

See actual usages in the [AST calculator example](https://github.com/yhirose/cpp-peglib/blob/master/example/calc3.cc) and [PL/0 language example](https://github.com/yhirose/cpp-peglib/blob/master/pl0/pl0.cc).

Make a parser with parser combinators
-------------------------------------

Instead of makeing a parser by parsing PEG syntax text, we can also construct a parser by hand with *parser combinatorss*. Here is an example:

```cpp
using namespace peg;
using namespace std;

vector<string> tags;

Definition ROOT, TAG_NAME, _;
ROOT     <= seq(_, zom(seq(chr('['), TAG_NAME, chr(']'), _)));
TAG_NAME <= oom(seq(npd(chr(']')), dot())), [&](const SemanticValues& sv) {
                tags.push_back(sv.str());
            };
_        <= zom(cls(" \t"));

auto ret = ROOT.parse(" [tag1] [tag:2] [tag-3] ");
```

The following are available operators:

| Operator |     Description                 |
| :------- | :------------------------------ |
| seq      | Sequence                        |
| cho      | Prioritized Choice              |
| zom      | Zero or More                    |
| oom      | One or More                     |
| opt      | Optional                        |
| apd      | And predicate                   |
| npd      | Not predicate                   |
| lit      | Literal string                  |
| liti     | Case-insensitive Literal string |
| cls      | Character class                 |
| ncls     | Negated Character class         |
| chr      | Character                       |
| dot      | Any character                   |
| tok      | Token boundary                  |
| ign      | Ignore semantic value           |
| csc      | Capture scope                   |
| cap      | Capture                         |
| bkr      | Back reference                  |
| dic      | Dictionary                      |
| pre      | Infix expression                |
| usr      | User defined parser             |

Adjust definitions
------------------

It's possible to add/override definitions.

```cpp
auto syntax = R"(
    ROOT <- _ 'Hello' _ NAME '!' _
)";

Rules additional_rules = {
    {
        "NAME", usr([](const char* s, size_t n, SemanticValues& sv, any& dt) -> size_t {
            static vector<string> names = { "PEG", "BNF" };
            for (const auto& name: names) {
                if (name.size() <= n && !name.compare(0, name.size(), s, name.size())) {
                    return name.size(); // processed length
                }
            }
            return -1; // parse error
        })
    },
    {
        "~_", zom(cls(" \t\r\n"))
    }
};

auto g = parser(syntax, additional_rules);

assert(g.parse(" Hello BNF! "));
```

Unicode support
---------------

cpp-peglib accepts UTF8 text. `.` matches a Unicode codepoint. Also, it supports `\u????`.

peglint - PEG syntax lint utility
---------------------------------

### Build peglint

```
> cd lint
> mkdir build
> cd build
> cmake ..
> make
> ./peglint
usage: grammar_file_path [source_file_path]

  options:
    --ast: show AST tree
    --opt, --opt-all: optimaze all AST nodes except nodes selected with --opt-rules
    --opt-only: optimaze only AST nodes selected with --opt-rules
    --opt-rules rules: comma delimitted definition rules for optimazation
    --source: source text
    --trace: show trace messages
```

### Lint grammar

```
> cat a.peg
A <- 'hello' ^ 'world'

> peglint a.peg
a.peg:1:14: syntax error
```

```
> cat a.peg
A <- B

> peglint a.peg
a.peg:1:6: 'B' is not defined.
```

```
> cat a.peg
A <- B / C
B <- 'b'
C <- A

> peglint a.peg
a.peg:1:10: 'C' is left recursive.
a.peg:3:6: 'A' is left recursive.
```

### Lint source text

```
> cat a.peg
Additive    <- Multitive '+' Additive / Multitive
Multitive   <- Primary '*' Multitive / Primary
Primary     <- '(' Additive ')' / Number
Number      <- < [0-9]+ >
%whitespace <- [ \t\r\n]*

> peglint --source "1 + a * 3" a.peg
[commendline]:1:3: syntax error
```

```
> cat a.txt
1 + 2 * 3

> peglint --ast a.peg a.txt
+ Additive
  + Multitive
    + Primary
      - Number (1)
  + Additive
    + Multitive
      + Primary
        - Number (2)
      + Multitive
        + Primary
          - Number (3)
```

```
> peglint --ast --opt --source "1 + 2 * 3" a.peg
+ Additive
  - Multitive[Number] (1)
  + Additive[Multitive]
    - Primary[Number] (2)
    - Multitive[Number] (3)
```

```
> peglint --ast --opt --opt-rules "Primary" --source "1 + 2 * 3" a.peg
+ Additive/0
  + Multitive/1[Primary]
    - Number (1)
  + Additive/1[Multitive]
    + Primary/1
      - Number (2)
    + Multitive/1[Primary]
      - Number (3)
```

```
> peglint --ast --opt-only --opt-rules "Primary" --source "1 + 2 * 3" a.peg
+ Additive/0
  + Multitive/1
    - Primary/1[Number] (1)
  + Additive/1
    + Multitive/0
      - Primary/1[Number] (2)
      + Multitive/1
        - Primary/1[Number] (3)
```

Sample codes
------------

  * [Calculator](https://github.com/yhirose/cpp-peglib/blob/master/example/calc.cc)
  * [Calculator (with parser operators)](https://github.com/yhirose/cpp-peglib/blob/master/example/calc2.cc)
  * [Calculator (AST version)](https://github.com/yhirose/cpp-peglib/blob/master/example/calc3.cc)
  * [Calculator (parsing expressions by precedence climbing)](https://github.com/yhirose/cpp-peglib/blob/master/example/calc4.cc)
  * [Calculator (AST version and parsing expressions by precedence climbing)](https://github.com/yhirose/cpp-peglib/blob/master/example/calc5.cc)
  * [Monkey language](https://github.com/yhirose/monkey-cpp) described in [Writing An Interpreter In Go](https://interpreterbook.com/).
  * [PL/0 language example](https://github.com/yhirose/cpp-peglib/blob/master/pl0/pl0.cc)
  * [A tiny PL/0 JIT compiler in less than 700 LOC with LLVM and PEG parser](https://github.com/yhirose/pl0-jit-compiler)
  * [A Programming Language just for writing Fizz Buzz program. :)](https://github.com/yhirose/fizzbuzzlang)

PEG debug
---------

A debug viewer for Parsing Expression Grammars using cpp-peglib by [mqnc](https://github.com/mqnc). Please see [his gihub project page](https://github.com/mqnc/pegdebug) for the detail. You can see a parse result of PL/0 code [here](https://mqnc.github.io/pegdebug/example/output.html).

License
-------

MIT license (© 2020 Yuji Hirose)
