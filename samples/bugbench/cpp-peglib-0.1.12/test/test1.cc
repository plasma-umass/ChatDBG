#include "catch.hh"
#include <peglib.h>

using namespace peg;

#if !defined(PEGLIB_NO_UNICODE_CHARS)
TEST_CASE("Simple syntax test (with unicode)", "[general]")
{
    parser parser(
        u8" ROOT ← _ "
        " _ <- ' ' "
    );

    bool ret = parser;
    REQUIRE(ret == true);
}
#endif

TEST_CASE("Simple syntax test", "[general]")
{
    parser parser(R"(
        ROOT <- _
        _ <- ' '
    )");

    bool ret = parser;
    REQUIRE(ret == true);
}

TEST_CASE("Empty syntax test", "[general]")
{
    parser parser("");
    bool ret = parser;
    REQUIRE(ret == false);
}

TEST_CASE("Backslash escape sequence test", "[general]")
{
    parser parser(R"(
        ROOT <- _
        _ <- '\\'
    )");

    bool ret = parser;
    REQUIRE(ret == true);
}

TEST_CASE("Invalid escape sequence test", "[general]")
{
    parser parser(R"(
        ROOT <- _
        _ <- '\'
    )");

    bool ret = parser;
    REQUIRE(ret == false);
}

TEST_CASE("Action taking non const Semantic Values parameter", "[general]")
{
    parser parser(R"(
        ROOT <- TEXT
        TEXT <- [a-zA-Z]+
    )");

    parser["ROOT"] = [&](SemanticValues& sv) {
        auto s = any_cast<std::string>(sv[0]);
        s[0] = 'H'; // mutate
        return std::string(std::move(s)); // move
    };

    parser["TEXT"] = [&](SemanticValues& sv) {
        return sv.token();
    };

    std::string val;
    auto ret = parser.parse("hello", val);
    REQUIRE(ret == true);
    REQUIRE(val == "Hello");
}

TEST_CASE("String capture test", "[general]")
{
    parser parser(R"(
        ROOT      <-  _ ('[' TAG_NAME ']' _)*
        TAG_NAME  <-  (!']' .)+
        _         <-  [ \t]*
    )");

    std::vector<std::string> tags;

    parser["TAG_NAME"] = [&](const SemanticValues& sv) {
        tags.push_back(sv.str());
    };

    auto ret = parser.parse(" [tag1] [tag:2] [tag-3] ");

    REQUIRE(ret == true);
    REQUIRE(tags.size() == 3);
    REQUIRE(tags[0] == "tag1");
    REQUIRE(tags[1] == "tag:2");
    REQUIRE(tags[2] == "tag-3");
}

using namespace peg;

TEST_CASE("String capture test2", "[general]")
{
    std::vector<std::string> tags;

    Definition ROOT, TAG, TAG_NAME, WS;
    ROOT     <= seq(WS, zom(TAG));
    TAG      <= seq(chr('['), TAG_NAME, chr(']'), WS);
    TAG_NAME <= oom(seq(npd(chr(']')), dot())), [&](const SemanticValues& sv) { tags.push_back(sv.str()); };
    WS       <= zom(cls(" \t"));

    auto r = ROOT.parse(" [tag1] [tag:2] [tag-3] ");

    REQUIRE(r.ret == true);
    REQUIRE(tags.size() == 3);
    REQUIRE(tags[0] == "tag1");
    REQUIRE(tags[1] == "tag:2");
    REQUIRE(tags[2] == "tag-3");
}

TEST_CASE("String capture test3", "[general]")
{
    parser pg(R"(
        ROOT  <- _ TOKEN*
        TOKEN <- '[' < (!']' .)+ > ']' _
        _     <- [ \t\r\n]*
    )");


    std::vector<std::string> tags;

    pg["TOKEN"] = [&](const SemanticValues& sv) {
        tags.push_back(sv.token());
    };

    auto ret = pg.parse(" [tag1] [tag:2] [tag-3] ");

    REQUIRE(ret == true);
    REQUIRE(tags.size() == 3);
    REQUIRE(tags[0] == "tag1");
    REQUIRE(tags[1] == "tag:2");
    REQUIRE(tags[2] == "tag-3");
}

TEST_CASE("Cyclic grammer test", "[general]")
{
    Definition PARENT;
    Definition CHILD;

    PARENT <= seq(CHILD);
    CHILD  <= seq(PARENT);
}

TEST_CASE("Visit test", "[general]")
{
    Definition ROOT, TAG, TAG_NAME, WS;

    ROOT     <= seq(WS, zom(TAG));
    TAG      <= seq(chr('['), TAG_NAME, chr(']'), WS);
    TAG_NAME <= oom(seq(npd(chr(']')), dot()));
    WS       <= zom(cls(" \t"));

    AssignIDToDefinition defIds;
    ROOT.accept(defIds);

    REQUIRE(defIds.ids.size() == 4);
}

TEST_CASE("Token check test", "[general]")
{
    parser parser(R"(
        EXPRESSION       <-  _ TERM (TERM_OPERATOR TERM)*
        TERM             <-  FACTOR (FACTOR_OPERATOR FACTOR)*
        FACTOR           <-  NUMBER / '(' _ EXPRESSION ')' _
        TERM_OPERATOR    <-  < [-+] > _
        FACTOR_OPERATOR  <-  < [/*] > _
        NUMBER           <-  < [0-9]+ > _
        _                <-  [ \t\r\n]*
    )");

    REQUIRE(parser["EXPRESSION"].is_token() == false);
    REQUIRE(parser["FACTOR"].is_token() == false);
    REQUIRE(parser["FACTOR_OPERATOR"].is_token() == true);
    REQUIRE(parser["NUMBER"].is_token() == true);
    REQUIRE(parser["_"].is_token() == true);
}

TEST_CASE("Lambda action test", "[general]")
{
    parser parser(R"(
       START <- (CHAR)*
       CHAR  <- .
    )");

    std::string ss;
    parser["CHAR"] = [&](const SemanticValues& sv) {
        ss += *sv.c_str();
    };

    bool ret = parser.parse("hello");
    REQUIRE(ret == true);
    REQUIRE(ss == "hello");
}

TEST_CASE("enter/leave handlers test", "[general]")
{
    parser parser(R"(
        START  <- LTOKEN '=' RTOKEN
        LTOKEN <- TOKEN
        RTOKEN <- TOKEN
        TOKEN  <- [A-Za-z]+
    )");

    parser["LTOKEN"].enter = [&](const char*, size_t, any& dt) {
        auto& require_upper_case = *any_cast<bool*>(dt);
        require_upper_case = false;
    };
    parser["LTOKEN"].leave = [&](const char*, size_t, size_t, any&, any& dt) {
        auto& require_upper_case = *any_cast<bool*>(dt);
        require_upper_case = true;
    };

    auto message = "should be upper case string...";

    parser["TOKEN"] = [&](const SemanticValues& sv, any& dt) {
        auto& require_upper_case = *any_cast<bool*>(dt);
        if (require_upper_case) {
            const auto& s = sv.str();
            if (!std::all_of(s.begin(), s.end(), ::isupper)) {
                throw parse_error(message);
            }
        }
    };

    bool require_upper_case = false;
    any dt = &require_upper_case;
    REQUIRE(parser.parse("hello=world", dt) == false);
    REQUIRE(parser.parse("HELLO=world", dt) == false);
    REQUIRE(parser.parse("hello=WORLD", dt) == true);
    REQUIRE(parser.parse("HELLO=WORLD", dt) == true);

    parser.log = [&](size_t ln, size_t col, const std::string& msg) {
        REQUIRE(ln == 1);
        REQUIRE(col == 7);
        REQUIRE(msg == message);
    };
    parser.parse("hello=world", dt);
}

TEST_CASE("WHITESPACE test", "[general]")
{
    parser parser(R"(
        # Rules
        ROOT         <-  ITEM (',' ITEM)*
        ITEM         <-  WORD / PHRASE

        # Tokens
        WORD         <-  < [a-zA-Z0-9_]+ >
        PHRASE       <-  < '"' (!'"' .)* '"' >

        %whitespace  <-  [ \t\r\n]*
    )");

    auto ret = parser.parse(R"(  one, 	 "two, three",   four  )");

    REQUIRE(ret == true);
}

TEST_CASE("WHITESPACE test2", "[general]")
{
    parser parser(R"(
        # Rules
        ROOT         <-  ITEM (',' ITEM)*
        ITEM         <-  '[' < [a-zA-Z0-9_]+ > ']'

        %whitespace  <-  (SPACE / TAB)*
        SPACE        <-  ' '
        TAB          <-  '\t'
    )");

    std::vector<std::string> items;
    parser["ITEM"] = [&](const SemanticValues& sv) {
        items.push_back(sv.token());
    };

    auto ret = parser.parse(R"([one], 	[two] ,[three] )");

    REQUIRE(ret == true);
    REQUIRE(items.size() == 3);
    REQUIRE(items[0] == "one");
    REQUIRE(items[1] == "two");
    REQUIRE(items[2] == "three");
}

TEST_CASE("WHITESPACE test3", "[general]") {
    parser parser(R"(
        StrQuot      <- < '"' < (StrEscape / StrChars)* > '"' >
        StrEscape    <- '\\' any
        StrChars     <- (!'"' !'\\' any)+
        any          <- .
        %whitespace  <- [ \t]*
    )");

    parser["StrQuot"] = [](const SemanticValues& sv) {
        REQUIRE(sv.token() == R"(  aaa \" bbb  )");
    };

    auto ret = parser.parse(R"( "  aaa \" bbb  " )");
    REQUIRE(ret == true);
}

TEST_CASE("WHITESPACE test4", "[general]") {
    parser parser(R"(
        ROOT         <-  HELLO OPE WORLD
        HELLO        <-  'hello'
        OPE          <-  < [-+] >
        WORLD        <-  'world' / 'WORLD'
        %whitespace  <-  [ \t\r\n]*
    )");

    parser["HELLO"] = [](const SemanticValues& sv) {
        REQUIRE(sv.token() == "hello");
    };

    parser["OPE"] = [](const SemanticValues& sv) {
        REQUIRE(sv.token() == "+");
    };

    parser["WORLD"] = [](const SemanticValues& sv) {
        REQUIRE(sv.token() == "world");
    };

    auto ret = parser.parse("  hello + world  ");
    REQUIRE(ret == true);
}

TEST_CASE("Word expression test", "[general]") {
    parser parser(R"(
        ROOT         <-  'hello' ','? 'world'
        %whitespace  <-  [ \t\r\n]*
        %word        <-  [a-z]+
    )");

	REQUIRE(parser.parse("helloworld") == false);
	REQUIRE(parser.parse("hello world") == true);
	REQUIRE(parser.parse("hello,world") == true);
	REQUIRE(parser.parse("hello, world") == true);
	REQUIRE(parser.parse("hello , world") == true);
}

TEST_CASE("Skip token test", "[general]")
{
    parser parser(
        "  ROOT  <-  _ ITEM (',' _ ITEM _)* "
        "  ITEM  <-  ([a-z0-9])+  "
        "  ~_    <-  [ \t]*    "
    );

    parser["ROOT"] = [&](const SemanticValues& sv) {
        REQUIRE(sv.size() == 2);
    };

    auto ret = parser.parse(" item1, item2 ");

    REQUIRE(ret == true);
}

TEST_CASE("Skip token test2", "[general]")
{
    parser parser(R"(
        ROOT        <-  ITEM (',' ITEM)*
        ITEM        <-  < ([a-z0-9])+ >
        %whitespace <-  [ \t]*
    )");

    parser["ROOT"] = [&](const SemanticValues& sv) {
        REQUIRE(sv.size() == 2);
    };

    auto ret = parser.parse(" item1, item2 ");

    REQUIRE(ret == true);
}

TEST_CASE("Custom AST test", "[general]")
{
	struct CustomType {};
	using CustomAst = AstBase<CustomType>;
	
    parser parser(R"(
        ROOT <- _ TEXT*
        TEXT <- [a-zA-Z]+ _
        _ <- [ \t\r\n]*
    )");

    parser.enable_ast<CustomAst>();
    std::shared_ptr<CustomAst> ast;
    bool ret = parser.parse("a b c", ast);
    REQUIRE(ret == true);
    REQUIRE(ast->nodes.size() == 4);
}

TEST_CASE("Backtracking test", "[general]")
{
    parser parser(R"(
       START <- PAT1 / PAT2
       PAT1  <- HELLO ' One'
       PAT2  <- HELLO ' Two'
       HELLO <- 'Hello'
    )");

    size_t count = 0;
    parser["HELLO"] = [&](const SemanticValues& /*sv*/) {
        count++;
    };

    parser.enable_packrat_parsing();

    bool ret = parser.parse("Hello Two");
    REQUIRE(ret == true);
    REQUIRE(count == 1); // Skip second time
}

TEST_CASE("Backtracking with AST", "[general]")
{
    parser parser(R"(
        S <- A? B (A B)* A
        A <- 'a'
        B <- 'b'
    )");

    parser.enable_ast();
    std::shared_ptr<Ast> ast;
    bool ret = parser.parse("ba", ast);
    REQUIRE(ret == true);
    REQUIRE(ast->nodes.size() == 2);
}

TEST_CASE("Octal/Hex/Unicode value test", "[general]")
{
    parser parser(
        R"( ROOT <- '\132\x7a\u30f3' )"
    );

    auto ret = parser.parse("Zzン");

    REQUIRE(ret == true);
}

TEST_CASE("Ignore case test", "[general]") {
    parser parser(R"(
        ROOT         <-  HELLO WORLD
        HELLO        <-  'hello'i
        WORLD        <-  'world'i
        %whitespace  <-  [ \t\r\n]*
    )");

    parser["HELLO"] = [](const SemanticValues& sv) {
        REQUIRE(sv.token() == "Hello");
    };

    parser["WORLD"] = [](const SemanticValues& sv) {
        REQUIRE(sv.token() == "World");
    };

    auto ret = parser.parse("  Hello World  ");
    REQUIRE(ret == true);
}

TEST_CASE("mutable lambda test", "[general]")
{
    std::vector<std::string> vec;

    parser pg("ROOT <- 'mutable lambda test'");

    // This test makes sure if the following code can be compiled.
    pg["TOKEN"] = [=](const SemanticValues& sv) mutable {
        vec.push_back(sv.str());
    };
}

TEST_CASE("Simple calculator test", "[general]")
{
    parser parser(R"(
        Additive  <- Multitive '+' Additive / Multitive
        Multitive <- Primary '*' Multitive / Primary
        Primary   <- '(' Additive ')' / Number
        Number    <- [0-9]+
    )");

    parser["Additive"] = [](const SemanticValues& sv) {
        switch (sv.choice()) {
        case 0:
            return any_cast<int>(sv[0]) + any_cast<int>(sv[1]);
        default:
            return any_cast<int>(sv[0]);
        }
    };

    parser["Multitive"] = [](const SemanticValues& sv) {
        switch (sv.choice()) {
        case 0:
            return any_cast<int>(sv[0]) * any_cast<int>(sv[1]);
        default:
            return any_cast<int>(sv[0]);
        }
    };

    parser["Number"] = [](const SemanticValues& sv) {
        return atoi(sv.c_str());
    };

    int val;
    parser.parse("(1+2)*3", val);

    REQUIRE(val == 9);
}

TEST_CASE("Calculator test", "[general]")
{
    // Construct grammer
    Definition EXPRESSION, TERM, FACTOR, TERM_OPERATOR, FACTOR_OPERATOR, NUMBER;

    EXPRESSION      <= seq(TERM, zom(seq(TERM_OPERATOR, TERM)));
    TERM            <= seq(FACTOR, zom(seq(FACTOR_OPERATOR, FACTOR)));
    FACTOR          <= cho(NUMBER, seq(chr('('), EXPRESSION, chr(')')));
    TERM_OPERATOR   <= cls("+-");
    FACTOR_OPERATOR <= cls("*/");
    NUMBER          <= oom(cls("0-9"));

    // Setup actions
    auto reduce = [](const SemanticValues& sv) -> long {
        long ret = any_cast<long>(sv[0]);
        for (auto i = 1u; i < sv.size(); i += 2) {
            auto num = any_cast<long>(sv[i + 1]);
            switch (any_cast<char>(sv[i])) {
                case '+': ret += num; break;
                case '-': ret -= num; break;
                case '*': ret *= num; break;
                case '/': ret /= num; break;
            }
        }
        return ret;
    };

    EXPRESSION      = reduce;
    TERM            = reduce;
    TERM_OPERATOR   = [](const SemanticValues& sv) { return *sv.c_str(); };
    FACTOR_OPERATOR = [](const SemanticValues& sv) { return *sv.c_str(); };
    NUMBER          = [](const SemanticValues& sv) { return stol(sv.str(), nullptr, 10); };

    // Parse
    long val;
    auto r = EXPRESSION.parse_and_get_value("1+2*3*(4-5+6)/7-8", val);

    REQUIRE(r.ret == true);
    REQUIRE(val == -3);
}

TEST_CASE("Calculator test2", "[general]")
{
    // Parse syntax
    auto syntax = R"(
        # Grammar for Calculator...
        EXPRESSION       <-  TERM (TERM_OPERATOR TERM)*
        TERM             <-  FACTOR (FACTOR_OPERATOR FACTOR)*
        FACTOR           <-  NUMBER / '(' EXPRESSION ')'
        TERM_OPERATOR    <-  [-+]
        FACTOR_OPERATOR  <-  [/*]
        NUMBER           <-  [0-9]+
    )";

    std::string start;
    auto grammar = ParserGenerator::parse(syntax, strlen(syntax), start, nullptr);
    auto& g = *grammar;

    // Setup actions
    auto reduce = [](const SemanticValues& sv) -> long {
        long ret = any_cast<long>(sv[0]);
        for (auto i = 1u; i < sv.size(); i += 2) {
            auto num = any_cast<long>(sv[i + 1]);
            switch (any_cast<char>(sv[i])) {
                case '+': ret += num; break;
                case '-': ret -= num; break;
                case '*': ret *= num; break;
                case '/': ret /= num; break;
            }
        }
        return ret;
    };

    g["EXPRESSION"]      = reduce;
    g["TERM"]            = reduce;
    g["TERM_OPERATOR"]   = [](const SemanticValues& sv) { return *sv.c_str(); };
    g["FACTOR_OPERATOR"] = [](const SemanticValues& sv) { return *sv.c_str(); };
    g["NUMBER"]          = [](const SemanticValues& sv) { return stol(sv.str(), nullptr, 10); };

    // Parse
    long val;
    auto r = g[start].parse_and_get_value("1+2*3*(4-5+6)/7-8", val);

    REQUIRE(r.ret == true);
    REQUIRE(val == -3);
}

TEST_CASE("Calculator test3", "[general]")
{
    // Parse syntax
    parser parser(R"(
        # Grammar for Calculator...
        EXPRESSION       <-  TERM (TERM_OPERATOR TERM)*
        TERM             <-  FACTOR (FACTOR_OPERATOR FACTOR)*
        FACTOR           <-  NUMBER / '(' EXPRESSION ')'
        TERM_OPERATOR    <-  [-+]
        FACTOR_OPERATOR  <-  [/*]
        NUMBER           <-  [0-9]+
    )");

    auto reduce = [](const SemanticValues& sv) -> long {
        long ret = any_cast<long>(sv[0]);
        for (auto i = 1u; i < sv.size(); i += 2) {
            auto num = any_cast<long>(sv[i + 1]);
            switch (any_cast<char>(sv[i])) {
                case '+': ret += num; break;
                case '-': ret -= num; break;
                case '*': ret *= num; break;
                case '/': ret /= num; break;
            }
        }
        return ret;
    };

    // Setup actions
    parser["EXPRESSION"]      = reduce;
    parser["TERM"]            = reduce;
    parser["TERM_OPERATOR"]   = [](const SemanticValues& sv) { return static_cast<char>(*sv.c_str()); };
    parser["FACTOR_OPERATOR"] = [](const SemanticValues& sv) { return static_cast<char>(*sv.c_str()); };
    parser["NUMBER"]          = [](const SemanticValues& sv) { return stol(sv.str(), nullptr, 10); };

    // Parse
    long val;
    auto ret = parser.parse("1+2*3*(4-5+6)/7-8", val);

    REQUIRE(ret == true);
    REQUIRE(val == -3);
}

TEST_CASE("Calculator test with AST", "[general]")
{
    parser parser(R"(
        EXPRESSION       <-  _ TERM (TERM_OPERATOR TERM)*
        TERM             <-  FACTOR (FACTOR_OPERATOR FACTOR)*
        FACTOR           <-  NUMBER / '(' _ EXPRESSION ')' _
        TERM_OPERATOR    <-  < [-+] > _
        FACTOR_OPERATOR  <-  < [/*] > _
        NUMBER           <-  < [0-9]+ > _
        ~_               <-  [ \t\r\n]*
    )");

    parser.enable_ast();

    std::function<long (const Ast&)> eval = [&](const Ast& ast) {
        if (ast.name == "NUMBER") {
            return stol(ast.token);
        } else {
            const auto& nodes = ast.nodes;
            auto result = eval(*nodes[0]);
            for (auto i = 1u; i < nodes.size(); i += 2) {
                auto num = eval(*nodes[i + 1]);
                auto ope = nodes[i]->token[0];
                switch (ope) {
                    case '+': result += num; break;
                    case '-': result -= num; break;
                    case '*': result *= num; break;
                    case '/': result /= num; break;
                }
            }
            return result;
        }
    };

    std::shared_ptr<Ast> ast;
    auto ret = parser.parse("1+2*3*(4-5+6)/7-8", ast);
    ast = AstOptimizer(true).optimize(ast);
    auto val = eval(*ast);

    REQUIRE(ret == true);
    REQUIRE(val == -3);
}

TEST_CASE("Calculator test with combinators and AST", "[general]") {
  // Construct grammer
  AST_DEFINITIONS(EXPRESSION, TERM, FACTOR, TERM_OPERATOR, FACTOR_OPERATOR, NUMBER);

  EXPRESSION <= seq(TERM, zom(seq(TERM_OPERATOR, TERM)));
  TERM <= seq(FACTOR, zom(seq(FACTOR_OPERATOR, FACTOR)));
  FACTOR <= cho(NUMBER, seq(chr('('), EXPRESSION, chr(')')));
  TERM_OPERATOR <= cls("+-");
  FACTOR_OPERATOR <= cls("*/");
  NUMBER <= oom(cls("0-9"));

  std::function<long(const Ast &)> eval = [&](const Ast &ast) {
    if (ast.name == "NUMBER") {
      return stol(ast.token);
    } else {
      const auto &nodes = ast.nodes;
      auto result = eval(*nodes[0]);
      for (auto i = 1u; i < nodes.size(); i += 2) {
        auto num = eval(*nodes[i + 1]);
        auto ope = nodes[i]->token[0];
        switch (ope) {
        case '+': result += num; break;
        case '-': result -= num; break;
        case '*': result *= num; break;
        case '/': result /= num; break;
        }
      }
      return result;
    }
  };

  std::shared_ptr<Ast> ast;
  auto r = EXPRESSION.parse_and_get_value("1+2*3*(4-5+6)/7-8", ast);
  ast = AstOptimizer(true).optimize(ast);
  auto val = eval(*ast);

  REQUIRE(r.ret == true);
  REQUIRE(val == -3);
}

TEST_CASE("Ignore semantic value test", "[general]")
{
    parser parser(R"(
       START <-  ~HELLO WORLD
       HELLO <- 'Hello' _
       WORLD <- 'World' _
       _     <- [ \t\r\n]*
    )");

    parser.enable_ast();

    std::shared_ptr<Ast> ast;
    auto ret = parser.parse("Hello World", ast);

    REQUIRE(ret == true);
    REQUIRE(ast->nodes.size() == 1);
    REQUIRE(ast->nodes[0]->name == "WORLD");
}

TEST_CASE("Ignore semantic value of 'or' predicate test", "[general]")
{
    parser parser(R"(
       START       <- _ !DUMMY HELLO_WORLD '.'
       HELLO_WORLD <- HELLO 'World' _
       HELLO       <- 'Hello' _
       DUMMY       <- 'dummy' _
       ~_          <- [ \t\r\n]*
   )");

    parser.enable_ast();

    std::shared_ptr<Ast> ast;
    auto ret = parser.parse("Hello World.", ast);

    REQUIRE(ret == true);
    REQUIRE(ast->nodes.size() == 1);
    REQUIRE(ast->nodes[0]->name == "HELLO_WORLD");
}

TEST_CASE("Ignore semantic value of 'and' predicate test", "[general]")
{
    parser parser(R"(
       START       <- _ &HELLO HELLO_WORLD '.'
       HELLO_WORLD <- HELLO 'World' _
       HELLO       <- 'Hello' _
       ~_          <- [ \t\r\n]*
    )");

    parser.enable_ast();

    std::shared_ptr<Ast> ast;
    auto ret = parser.parse("Hello World.", ast);

    REQUIRE(ret == true);
    REQUIRE(ast->nodes.size() == 1);
    REQUIRE(ast->nodes[0]->name == "HELLO_WORLD");
}

TEST_CASE("Literal token on AST test1", "[general]")
{
    parser parser(R"(
        STRING_LITERAL  <- '"' (('\\"' / '\\t' / '\\n') / (!["] .))* '"'
    )");
    parser.enable_ast();

    std::shared_ptr<Ast> ast;
    auto ret = parser.parse(R"("a\tb")", ast);

    REQUIRE(ret == true);
    REQUIRE(ast->is_token == true);
    REQUIRE(ast->token == R"("a\tb")");
    REQUIRE(ast->nodes.empty());
}

TEST_CASE("Literal token on AST test2", "[general]")
{
    parser parser(R"(
        STRING_LITERAL  <-  '"' (ESC / CHAR)* '"'
        ESC             <-  ('\\"' / '\\t' / '\\n')
        CHAR            <-  (!["] .)
    )");
    parser.enable_ast();

    std::shared_ptr<Ast> ast;
    auto ret = parser.parse(R"("a\tb")", ast);

    REQUIRE(ret == true);
    REQUIRE(ast->is_token == false);
    REQUIRE(ast->token.empty());
    REQUIRE(ast->nodes.size() == 3);
}

TEST_CASE("Literal token on AST test3", "[general]")
{
    parser parser(R"(
        STRING_LITERAL  <-  < '"' (ESC / CHAR)* '"' >
        ESC             <-  ('\\"' / '\\t' / '\\n')
        CHAR            <-  (!["] .)
    )");
    parser.enable_ast();

    std::shared_ptr<Ast> ast;
    auto ret = parser.parse(R"("a\tb")", ast);

    REQUIRE(ret == true);
    REQUIRE(ast->is_token == true);
    REQUIRE(ast->token == R"("a\tb")");
    REQUIRE(ast->nodes.empty());
}

TEST_CASE("Missing missing definitions test", "[general]")
{
    parser parser(R"(
        A <- B C
    )");

    REQUIRE(!parser);
}

TEST_CASE("Definition duplicates test", "[general]")
{
    parser parser(R"(
        A <- ''
        A <- ''
    )");

    REQUIRE(!parser);
}

TEST_CASE("Semantic values test", "[general]")
{
    parser parser(R"(
        term <- ( a b c x )? a b c
        a <- 'a'
        b <- 'b'
        c <- 'c'
        x <- 'x'
    )");

	for (const auto& rule: parser.get_rule_names()){
		parser[rule.c_str()] = [rule](const SemanticValues& sv, any&) {
            if (rule == "term") {
                REQUIRE(any_cast<std::string>(sv[0]) == "a at 0");
                REQUIRE(any_cast<std::string>(sv[1]) == "b at 1");
                REQUIRE(any_cast<std::string>(sv[2]) == "c at 2");
                return std::string();
            } else {
                return rule + " at " + std::to_string(sv.c_str() - sv.ss);
            }
		};
	}

	REQUIRE(parser.parse("abc"));
}

TEST_CASE("Ordered choice count", "[general]")
{
    parser parser(R"(
        S <- 'a' / 'b'
    )");

    parser["S"] = [](const SemanticValues& sv) {
        REQUIRE(sv.choice() == 1);
        REQUIRE(sv.choice_count() == 2);
    };

    parser.parse("b");
}

TEST_CASE("Ordered choice count 2", "[general]")
{
    parser parser(R"(
        S <- ('a' / 'b')*
    )");

    parser["S"] = [](const SemanticValues& sv) {
        REQUIRE(sv.choice() == 0);
        REQUIRE(sv.choice_count() == 0);
    };

    parser.parse("b");
}

TEST_CASE("Semantic value tag", "[general]")
{
    parser parser(R"(
        S <- A? B* C?
        A <- 'a'
        B <- 'b'
        C <- 'c'
    )");

    {
        using namespace udl;
        parser["S"] = [](const SemanticValues& sv) {
            REQUIRE(sv.size() == 1);
            REQUIRE(sv.tags.size() == 1);
            REQUIRE(sv.tags[0] == "C"_);
        };
        auto ret = parser.parse("c");
        REQUIRE(ret == true);
    }

    {
        using namespace udl;
        parser["S"] = [](const SemanticValues& sv) {
            REQUIRE(sv.size() == 2);
            REQUIRE(sv.tags.size() == 2);
            REQUIRE(sv.tags[0] == "B"_);
            REQUIRE(sv.tags[1] == "B"_);
        };
        auto ret = parser.parse("bb");
        REQUIRE(ret == true);
    }

    {
        using namespace udl;
        parser["S"] = [](const SemanticValues& sv) {
            REQUIRE(sv.size() == 2);
            REQUIRE(sv.tags.size() == 2);
            REQUIRE(sv.tags[0] == "A"_);
            REQUIRE(sv.tags[1] == "C"_);
        };
        auto ret = parser.parse("ac");
        REQUIRE(ret == true);
    }
}

TEST_CASE("Negated Class test", "[general]")
{
    parser parser(R"(
        ROOT <- [^a-z_]+
    )");

    bool ret = parser;
    REQUIRE(ret == true);

    REQUIRE(parser.parse("ABC123"));
    REQUIRE_FALSE(parser.parse("ABcZ"));
    REQUIRE_FALSE(parser.parse("ABCZ_"));
    REQUIRE_FALSE(parser.parse(""));
}

// vim: et ts=4 sw=4 cin cino={1s ff=unix
