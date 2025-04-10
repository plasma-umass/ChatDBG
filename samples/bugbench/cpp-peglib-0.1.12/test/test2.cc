#include "catch.hh"
#include <peglib.h>

using namespace peg;

TEST_CASE("Infinite loop 1", "[infinite loop]")
{
    parser pg(R"(
        ROOT  <- WH TOKEN* WH
        TOKEN <- [a-z0-9]*
        WH    <- [ \t]*
    )");

    REQUIRE(!pg);
}

TEST_CASE("Infinite loop 2", "[infinite loop]")
{
    parser pg(R"(
        ROOT  <- WH TOKEN+ WH
        TOKEN <- [a-z0-9]*
        WH    <- [ \t]*
    )");

    REQUIRE(!pg);
}

TEST_CASE("Infinite loop 3", "[infinite loop]")
{
    parser pg(R"(
        ROOT  <- WH TOKEN* WH
        TOKEN <- !'word1'
        WH    <- [ \t]*
    )");

    REQUIRE(!pg);
}

TEST_CASE("Infinite loop 4", "[infinite loop]")
{
    parser pg(R"(
        ROOT  <- WH TOKEN* WH
        TOKEN <- &'word1'
        WH    <- [ \t]*
    )");

    REQUIRE(!pg);
}

TEST_CASE("Infinite loop 5", "[infinite loop]")
{
    parser pg(R"(
        Numbers <- Number*
        Number <- [0-9]+ / Spacing
        Spacing <- ' ' / '\t' / '\n' / EOF # EOF is empty
        EOF <- !.
    )");

    REQUIRE(!pg);
}

TEST_CASE("Infinite loop 6", "[infinite loop]")
{
    parser pg(R"(
        S <- ''*
    )");

    REQUIRE(!pg);
}

TEST_CASE("Infinite loop 7", "[infinite loop]")
{
    parser pg(R"(
        S <- A*
        A <- ''
    )");

    REQUIRE(!pg);
}

TEST_CASE("Not infinite 1", "[infinite loop]")
{
    parser pg(R"(
        Numbers <- Number* EOF
        Number <- [0-9]+ / Spacing
        Spacing <- ' ' / '\t' / '\n'
        EOF <- !.
    )");

    REQUIRE(!!pg); // OK
}

TEST_CASE("Not infinite 2", "[infinite loop]")
{
    parser pg(R"(
        ROOT      <-  _ ('[' TAG_NAME ']' _)*
        # In a sequence operator, if there is at least one non-empty element, we can treat it as non-empty
        TAG_NAME  <-  (!']' .)+
        _         <-  [ \t]*
    )");

    REQUIRE(!!pg); // OK
}

TEST_CASE("Not infinite 3", "[infinite loop]")
{
    parser pg(R"(
        EXPRESSION       <-  _ TERM (TERM_OPERATOR TERM)*
        TERM             <-  FACTOR (FACTOR_OPERATOR FACTOR)*
        FACTOR           <-  NUMBER / '(' _ EXPRESSION ')' _ # Recursive...
        TERM_OPERATOR    <-  < [-+] > _
        FACTOR_OPERATOR  <-  < [/*] > _
        NUMBER           <-  < [0-9]+ > _
        _                <-  [ \t\r\n]*
    )");

    REQUIRE(!!pg); // OK
}

TEST_CASE("Precedence climbing", "[precedence]")
{
    parser parser(R"(
        START            <-  _ EXPRESSION
        EXPRESSION       <-  ATOM (OPERATOR ATOM)* {
                               precedence
                                 L + -
                                 L * /
                             }
        ATOM             <-  NUMBER / T('(') EXPRESSION T(')')
        OPERATOR         <-  T([-+/*])
        NUMBER           <-  T('-'? [0-9]+)
		~_               <-  [ \t]*
		T(S)             <-  < S > _
	)");

    parser.enable_packrat_parsing();

    // Setup actions
    parser["EXPRESSION"] = [](const SemanticValues& sv) -> long {
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

    bool ret = parser;
    REQUIRE(ret == true);

    {
        auto expr = " 1 + 2 * 3 * (4 - 5 + 6) / 7 - 8 ";
        long val = 0;
        ret = parser.parse(expr, val);

        REQUIRE(ret == true);
        REQUIRE(val == -3);
    }

    {
      auto expr = "-1+-2--3"; // -1 + -2 - -3 = 0
      long val = 0;
      ret = parser.parse(expr, val);

      REQUIRE(ret == true);
      REQUIRE(val == 0);
    }
}

TEST_CASE("Precedence climbing with macro", "[precedence]")
{
    // Create a PEG parser
    parser parser(R"(
        EXPRESSION             <-  INFIX_EXPRESSION(ATOM, OPERATOR)
        INFIX_EXPRESSION(A, O) <-  A (O A)* {
                                     precedence
                                       L + -
                                       L * /
                                   }
        ATOM                   <-  NUMBER / '(' EXPRESSION ')'
        OPERATOR               <-  < [-+/*] >
        NUMBER                 <-  < '-'? [0-9]+ >
        %whitespace            <-  [ \t]*
	)");

    parser.enable_packrat_parsing();

    bool ret = parser;
    REQUIRE(ret == true);

    // Setup actions
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

    {
        auto expr = " 1 + 2 * 3 * (4 - 5 + 6) / 7 - 8 ";
        long val = 0;
        ret = parser.parse(expr, val);

        REQUIRE(ret == true);
        REQUIRE(val == -3);
    }

    {
      auto expr = "-1+-2--3"; // -1 + -2 - -3 = 0
      long val = 0;
      ret = parser.parse(expr, val);

      REQUIRE(ret == true);
      REQUIRE(val == 0);
    }
}

TEST_CASE("Precedence climbing error1", "[precedence]")
{
    parser parser(R"(
        START            <-  _ EXPRESSION
        EXPRESSION       <-  ATOM (OPERATOR ATOM1)* {
                               precedence
                                 L + -
                                 L * /
                             }
        ATOM             <-  NUMBER / T('(') EXPRESSION T(')')
        ATOM1            <-  NUMBER / T('(') EXPRESSION T(')')
        OPERATOR         <-  T([-+/*])
        NUMBER           <-  T('-'? [0-9]+)
		~_               <-  [ \t]*
		T(S)             <-  < S > _
	)");

    bool ret = parser;
    REQUIRE(ret == false);
}

TEST_CASE("Precedence climbing error2", "[precedence]")
{
    parser parser(R"(
        START            <-  _ EXPRESSION
        EXPRESSION       <-  ATOM OPERATOR ATOM {
                               precedence
                                 L + -
                                 L * /
                             }
        ATOM             <-  NUMBER / T('(') EXPRESSION T(')')
        OPERATOR         <-  T([-+/*])
        NUMBER           <-  T('-'? [0-9]+)
		~_               <-  [ \t]*
		T(S)             <-  < S > _
	)");

    bool ret = parser;
    REQUIRE(ret == false);
}

TEST_CASE("Precedence climbing error3", "[precedence]") {
    parser parser(R"(
        EXPRESSION               <-  PRECEDENCE_PARSING(ATOM, OPERATOR)
        PRECEDENCE_PARSING(A, O) <-  A (O A)+ {
                                       precedence
                                         L + -
                                         L * /
                                     }
        ATOM                     <-  NUMBER / '(' EXPRESSION ')'
        OPERATOR                 <-  < [-+/*] >
        NUMBER                   <-  < '-'? [0-9]+ >
        %whitespace              <-  [ \t]*
	)");

    bool ret = parser;
    REQUIRE(ret == false);
}

TEST_CASE("Packrat parser test with %whitespace%", "[packrat]") {
    peg::parser parser(R"(
        ROOT         <-  'a'
        %whitespace  <-  SPACE*
        SPACE        <-  ' '
    )");

    parser.enable_packrat_parsing();

    auto ret = parser.parse("a");
    REQUIRE(ret == true);
}

TEST_CASE("Packrat parser test with macro", "[packrat]")
{
    parser parser(R"(
        EXPRESSION       <-  _ LIST(TERM, TERM_OPERATOR)
        TERM             <-  LIST(FACTOR, FACTOR_OPERATOR)
        FACTOR           <-  NUMBER / T('(') EXPRESSION T(')')
        TERM_OPERATOR    <-  T([-+])
        FACTOR_OPERATOR  <-  T([/*])
        NUMBER           <-  T([0-9]+)
		~_               <-  [ \t]*
		LIST(I, D)       <-  I (D I)*
		T(S)             <-  < S > _
	)");

    parser.enable_packrat_parsing();

    auto ret = parser.parse(" 1 + 2 * 3 * (4 - 5 + 6) / 7 - 8 ");
    REQUIRE(ret == true);
}

TEST_CASE("Packrat parser test with precedence expression parser", "[packrat]") {
  peg::parser parser(R"(
    Expression  <- Atom (Operator Atom)* { precedence L + - L * / }
    Atom        <- _? Number _?
    Number      <- [0-9]+
    Operator    <- '+' / '-' / '*' / '/'
    _           <- ' '+
  )");

  bool ret = parser;
  REQUIRE(ret == true);

  parser.enable_packrat_parsing();

  ret = parser.parse(" 1 + 2 * 3 ");
  REQUIRE(ret == true);
}

TEST_CASE("Backreference test", "[backreference]")
{
    parser parser(R"(
        START  <- _ LQUOTE < (!RQUOTE .)* > RQUOTE _
        LQUOTE <- 'R"' $delm< [a-zA-Z]* > '('
        RQUOTE <- ')' $delm '"'
        ~_     <- [ \t\r\n]*
    )");

    std::string token;
    parser["START"] = [&](const SemanticValues& sv) {
        token = sv.token();
    };

    {
        token.clear();
        auto ret = parser.parse(R"delm(
            R"("hello world")"
        )delm");

        REQUIRE(ret == true);
        REQUIRE(token == "\"hello world\"");
    }

    {
        token.clear();
        auto ret = parser.parse(R"delm(
            R"foo("(hello world)")foo"
        )delm");

        REQUIRE(ret == true);
        REQUIRE(token == "\"(hello world)\"");
    }

    {
        token.clear();
        auto ret = parser.parse(R"delm(
            R"foo("(hello world)foo")foo"
        )delm");

        REQUIRE(ret == false);
        REQUIRE(token == "\"(hello world");
    }

    {
        token.clear();
        auto ret = parser.parse(R"delm(
            R"foo("(hello world)")bar"
        )delm");

        REQUIRE(ret == false);
        REQUIRE(token.empty());
    }
}

TEST_CASE("Invalid backreference test", "[backreference]")
{
    parser parser(R"(
        START  <- _ LQUOTE (!RQUOTE .)* RQUOTE _
        LQUOTE <- 'R"' $delm< [a-zA-Z]* > '('
        RQUOTE <- ')' $delm2 '"'
        ~_     <- [ \t\r\n]*
    )");

    REQUIRE_THROWS_AS(
        parser.parse(R"delm(
            R"foo("(hello world)")foo"
        )delm"),
        std::runtime_error);
}


TEST_CASE("Nested capture test", "[backreference]")
{
    parser parser(R"(
        ROOT      <- CONTENT
        CONTENT   <- (ELEMENT / TEXT)*
        ELEMENT   <- $(STAG CONTENT ETAG)
        STAG      <- '<' $tag< TAG_NAME > '>'
        ETAG      <- '</' $tag '>'
        TAG_NAME  <- 'b' / 'u'
        TEXT      <- TEXT_DATA
        TEXT_DATA <- ![<] .
    )");

    REQUIRE(parser.parse("This is <b>a <u>test</u> text</b>."));
    REQUIRE(!parser.parse("This is <b>a <u>test</b> text</u>."));
    REQUIRE(!parser.parse("This is <b>a <u>test text</b>."));
    REQUIRE(!parser.parse("This is a <u>test</u> text</b>."));
}

TEST_CASE("Backreference with Prioritized Choice test", "[backreference]")
{
    parser parser(R"(
        TREE           <- WRONG_BRANCH / CORRECT_BRANCH
        WRONG_BRANCH   <- BRANCH THAT IS_capture WRONG
        CORRECT_BRANCH <- BRANCH THAT IS_backref CORRECT
        BRANCH         <- 'branch'
        THAT           <- 'that'
        IS_capture     <- $ref<..>
        IS_backref     <- $ref
        WRONG          <- 'wrong'
        CORRECT        <- 'correct'
    )");

    REQUIRE_THROWS_AS(parser.parse("branchthatiscorrect"), std::runtime_error);
}

TEST_CASE("Backreference with Zero or More test", "[backreference]")
{
    parser parser(R"(
        TREE           <- WRONG_BRANCH* CORRECT_BRANCH
        WRONG_BRANCH   <- BRANCH THAT IS_capture WRONG
        CORRECT_BRANCH <- BRANCH THAT IS_backref CORRECT
        BRANCH         <- 'branch'
        THAT           <- 'that'
        IS_capture     <- $ref<..>
        IS_backref     <- $ref
        WRONG          <- 'wrong'
        CORRECT        <- 'correct'
    )");

    REQUIRE(parser.parse("branchthatiswrongbranchthatiscorrect"));
    REQUIRE(!parser.parse("branchthatiswrongbranchthatIscorrect"));
    REQUIRE(!parser.parse("branchthatiswrongbranchthatIswrongbranchthatiscorrect"));
    REQUIRE(parser.parse("branchthatiswrongbranchthatIswrongbranchthatIscorrect"));
    REQUIRE_THROWS_AS(parser.parse("branchthatiscorrect"), std::runtime_error);
    REQUIRE_THROWS_AS(parser.parse("branchthatiswron_branchthatiscorrect"), std::runtime_error);
}

TEST_CASE("Backreference with One or More test", "[backreference]")
{
    parser parser(R"(
        TREE           <- WRONG_BRANCH+ CORRECT_BRANCH
        WRONG_BRANCH   <- BRANCH THAT IS_capture WRONG
        CORRECT_BRANCH <- BRANCH THAT IS_backref CORRECT
        BRANCH         <- 'branch'
        THAT           <- 'that'
        IS_capture     <- $ref<..>
        IS_backref     <- $ref
        WRONG          <- 'wrong'
        CORRECT        <- 'correct'
    )");

    REQUIRE(parser.parse("branchthatiswrongbranchthatiscorrect"));
    REQUIRE(!parser.parse("branchthatiswrongbranchthatIscorrect"));
    REQUIRE(!parser.parse("branchthatiswrongbranchthatIswrongbranchthatiscorrect"));
    REQUIRE(parser.parse("branchthatiswrongbranchthatIswrongbranchthatIscorrect"));
    REQUIRE(!parser.parse("branchthatiscorrect"));
    REQUIRE(!parser.parse("branchthatiswron_branchthatiscorrect"));
}

TEST_CASE("Backreference with Option test", "[backreference]")
{
    parser parser(R"(
        TREE           <- WRONG_BRANCH? CORRECT_BRANCH
        WRONG_BRANCH   <- BRANCH THAT IS_capture WRONG
        CORRECT_BRANCH <- BRANCH THAT IS_backref CORRECT
        BRANCH         <- 'branch'
        THAT           <- 'that'
        IS_capture     <- $ref<..>
        IS_backref     <- $ref
        WRONG          <- 'wrong'
        CORRECT        <- 'correct'
    )");

    REQUIRE(parser.parse("branchthatiswrongbranchthatiscorrect"));
    REQUIRE(!parser.parse("branchthatiswrongbranchthatIscorrect"));
    REQUIRE(!parser.parse("branchthatiswrongbranchthatIswrongbranchthatiscorrect"));
    REQUIRE(!parser.parse("branchthatiswrongbranchthatIswrongbranchthatIscorrect"));
    REQUIRE_THROWS_AS(parser.parse("branchthatiscorrect"), std::runtime_error);
    REQUIRE_THROWS_AS(parser.parse("branchthatiswron_branchthatiscorrect"), std::runtime_error);
}

TEST_CASE("Repetition {0}", "[repetition]")
{
    parser parser(R"(
        START <- '(' DIGIT{3} ') ' DIGIT{3} '-' DIGIT{4}
        DIGIT <- [0-9]
    )");
    REQUIRE(parser.parse("(123) 456-7890"));
    REQUIRE(!parser.parse("(12a) 456-7890"));
    REQUIRE(!parser.parse("(123) 45-7890"));
    REQUIRE(!parser.parse("(123) 45-7a90"));
}

TEST_CASE("Repetition {2,4}", "[repetition]")
{
    parser parser(R"(
        START <- DIGIT{2,4}
        DIGIT <- [0-9]
    )");
    REQUIRE(!parser.parse("1"));
    REQUIRE(parser.parse("12"));
    REQUIRE(parser.parse("123"));
    REQUIRE(parser.parse("1234"));
    REQUIRE(!parser.parse("12345"));
}

TEST_CASE("Repetition {2,1}", "[repetition]")
{
    parser parser(R"(
        START <- DIGIT{2,1} # invalid range
        DIGIT <- [0-9]
    )");
    REQUIRE(!parser.parse("1"));
    REQUIRE(parser.parse("12"));
    REQUIRE(!parser.parse("123"));
}

TEST_CASE("Repetition {2,}", "[repetition]")
{
    parser parser(R"(
        START <- DIGIT{2,}
        DIGIT <- [0-9]
    )");
    REQUIRE(!parser.parse("1"));
    REQUIRE(parser.parse("12"));
    REQUIRE(parser.parse("123"));
    REQUIRE(parser.parse("1234"));
}

TEST_CASE("Repetition {,2}", "[repetition]")
{
    parser parser(R"(
        START <- DIGIT{,2}
        DIGIT <- [0-9]
    )");
    REQUIRE(parser.parse("1"));
    REQUIRE(parser.parse("12"));
    REQUIRE(!parser.parse("123"));
    REQUIRE(!parser.parse("1234"));
}

TEST_CASE("Left recursive test", "[left recursive]")
{
    parser parser(R"(
        A <- A 'a'
        B <- A 'a'
    )");

    REQUIRE(!parser);
}

TEST_CASE("Left recursive with option test", "[left recursive]")
{
    parser parser(R"(
        A  <- 'a' / 'b'? B 'c'
        B  <- A
    )");

    REQUIRE(!parser);
}

TEST_CASE("Left recursive with zom test", "[left recursive]")
{
    parser parser(R"(
        A <- 'a'* A*
    )");

    REQUIRE(!parser);
}

TEST_CASE("Left recursive with a ZOM content rule", "[left recursive]")
{
    parser parser(R"(
        A <- B
        B <- _ A
        _ <- ' '* # Zero or more
    )");

    REQUIRE(!parser);
}

TEST_CASE("Left recursive with empty string test", "[left recursive]")
{
    parser parser(
        " A <- '' A"
    );

    REQUIRE(!parser);
}

TEST_CASE("User defined rule test", "[user rule]")
{
    auto g = parser(R"(
        ROOT <- _ 'Hello' _ NAME '!' _
    )",
    {
        {
            "NAME", usr([](const char* s, size_t n, SemanticValues& /*sv*/, any& /*dt*/) -> size_t {
                static std::vector<std::string> names = { "PEG", "BNF" };
                for (const auto& name: names) {
                    if (name.size() <= n && !name.compare(0, name.size(), s, name.size())) {
                        return name.size();
                    }
                }
                return static_cast<size_t>(-1);
            })
        },
        {
            "~_", zom(cls(" \t\r\n"))
        }
    });

    REQUIRE(g.parse(" Hello BNF! ") == true);
}

TEST_CASE("Semantic predicate test", "[predicate]")
{
    parser parser("NUMBER  <-  [0-9]+");

    parser["NUMBER"] = [](const SemanticValues& sv) {
        auto val = stol(sv.token(), nullptr, 10);
        if (val != 100) {
            throw parse_error("value error!!");
        }
        return val;
    };

    long val;
    REQUIRE(parser.parse("100", val));
    REQUIRE(val == 100);

    parser.log = [](size_t line, size_t col, const std::string& msg) {
        REQUIRE(line == 1);
        REQUIRE(col == 1);
        REQUIRE(msg == "value error!!");
    };
    REQUIRE(!parser.parse("200", val));
}

TEST_CASE("Japanese character", "[unicode]")
{
    peg::parser parser(u8R"(
        文 <- 修飾語? 主語 述語 '。'
        主語 <- 名詞 助詞
        述語 <- 動詞 助詞
        修飾語 <- 形容詞
        名詞 <- 'サーバー' / 'クライアント'
        形容詞 <- '古い' / '新しい'
        動詞 <- '落ち' / '復旧し'
        助詞 <- 'が' / 'を' / 'た' / 'ます' / 'に'
    )");

    bool ret = parser;
    REQUIRE(ret == true);

    REQUIRE(parser.parse(u8R"(サーバーを復旧します。)"));
}

TEST_CASE("dot with a code", "[unicode]")
{
    peg::parser parser(" S <- 'a' . 'b' ");
    REQUIRE(parser.parse(u8R"(aあb)"));
}

TEST_CASE("dot with a char", "[unicode]")
{
    peg::parser parser(" S <- 'a' . 'b' ");
    REQUIRE(parser.parse(u8R"(aåb)"));
}

TEST_CASE("character class", "[unicode]")
{
    peg::parser parser(R"(
        S <- 'a' [い-おAさC-Eた-とは] 'b'
    )");

    bool ret = parser;
    REQUIRE(ret == true);

    REQUIRE(!parser.parse(u8R"(aあb)"));
    REQUIRE(parser.parse(u8R"(aいb)"));
    REQUIRE(parser.parse(u8R"(aうb)"));
    REQUIRE(parser.parse(u8R"(aおb)"));
    REQUIRE(!parser.parse(u8R"(aかb)"));
    REQUIRE(parser.parse(u8R"(aAb)"));
    REQUIRE(!parser.parse(u8R"(aBb)"));
    REQUIRE(parser.parse(u8R"(aEb)"));
    REQUIRE(!parser.parse(u8R"(aFb)"));
    REQUIRE(!parser.parse(u8R"(aそb)"));
    REQUIRE(parser.parse(u8R"(aたb)"));
    REQUIRE(parser.parse(u8R"(aちb)"));
    REQUIRE(parser.parse(u8R"(aとb)"));
    REQUIRE(!parser.parse(u8R"(aなb)"));
    REQUIRE(parser.parse(u8R"(aはb)"));
    REQUIRE(!parser.parse(u8R"(a?b)"));
}

#if 0 // TODO: Unicode Grapheme support
TEST_CASE("dot with a grapheme", "[unicode]")
{
    peg::parser parser(" S <- 'a' . 'b' ");
    REQUIRE(parser.parse(u8R"(aसिb)"));
}
#endif

TEST_CASE("Macro simple test", "[macro]")
{
    parser parser(R"(
		S     <- HELLO WORLD
		HELLO <- T('hello')
		WORLD <- T('world')
		T(a)  <- a [ \t]*
	)");

    REQUIRE(parser.parse("hello \tworld "));
}

TEST_CASE("Macro two parameters", "[macro]")
{
    parser parser(R"(
		S           <- HELLO_WORLD
		HELLO_WORLD <- T('hello', 'world')
		T(a, b)     <- a [ \t]* b [ \t]*
	)");

    REQUIRE(parser.parse("hello \tworld "));
}

TEST_CASE("Macro syntax error", "[macro]")
{
    parser parser(R"(
		S     <- T('hello')
		T (a) <- a [ \t]*
	)");

    bool ret = parser;
    REQUIRE(ret == false);
}

TEST_CASE("Macro missing argument", "[macro]")
{
    parser parser(R"(
		S       <- T ('hello')
		T(a, b) <- a [ \t]* b
	)");

    bool ret = parser;
    REQUIRE(ret == false);
}

TEST_CASE("Macro reference syntax error", "[macro]")
{
    parser parser(R"(
		S    <- T ('hello')
		T(a) <- a [ \t]*
	)");

    bool ret = parser;
    REQUIRE(ret == false);
}

TEST_CASE("Macro invalid macro reference error", "[macro]")
{
    parser parser(R"(
		S <- T('hello')
		T <- 'world'
	)");

    bool ret = parser;
    REQUIRE(ret == false);
}

TEST_CASE("Macro calculator", "[macro]")
{
	// Create a PEG parser
    parser parser(R"(
        # Grammar for simple calculator...
        EXPRESSION       <-  _ LIST(TERM, TERM_OPERATOR)
        TERM             <-  LIST(FACTOR, FACTOR_OPERATOR)
        FACTOR           <-  NUMBER / T('(') EXPRESSION T(')')
        TERM_OPERATOR    <-  T([-+])
        FACTOR_OPERATOR  <-  T([/*])
        NUMBER           <-  T([0-9]+)
		~_               <-  [ \t]*
		LIST(I, D)       <-  I (D I)*
		T(S)             <-  < S > _
	)");

	// Setup actions
    auto reduce = [](const SemanticValues& sv) -> long {
        auto result = any_cast<long>(sv[0]);
        for (auto i = 1u; i < sv.size(); i += 2) {
            auto num = any_cast<long>(sv[i + 1]);
            auto ope = any_cast<char>(sv[i]);
            switch (ope) {
                case '+': result += num; break;
                case '-': result -= num; break;
                case '*': result *= num; break;
                case '/': result /= num; break;
            }
        }
        return result;
    };

    parser["EXPRESSION"]      = reduce;
    parser["TERM"]            = reduce;
    parser["TERM_OPERATOR"]   = [](const SemanticValues& sv) { return static_cast<char>(*sv.c_str()); };
    parser["FACTOR_OPERATOR"] = [](const SemanticValues& sv) { return static_cast<char>(*sv.c_str()); };
    parser["NUMBER"]          = [](const SemanticValues& sv) { return atol(sv.c_str()); };

    bool ret = parser;
    REQUIRE(ret == true);

	auto expr = " 1 + 2 * 3 * (4 - 5 + 6) / 7 - 8 ";
    long val = 0;
    ret = parser.parse(expr, val);

    REQUIRE(ret == true);
    REQUIRE(val == -3);
}

TEST_CASE("Macro expression arguments", "[macro]")
{
    parser parser(R"(
		S             <- M('hello' / 'Hello', 'world' / 'World')
		M(arg0, arg1) <- arg0 [ \t]+ arg1
	)");

    REQUIRE(parser.parse("Hello world"));
}

TEST_CASE("Macro recursive", "[macro]")
{
    parser parser(R"(
		S    <- M('abc')
		M(s) <- !s / s ' ' M(s / '123') / s
	)");

    REQUIRE(parser.parse(""));
    REQUIRE(parser.parse("abc"));
    REQUIRE(parser.parse("abc abc"));
    REQUIRE(parser.parse("abc 123 abc"));
}

TEST_CASE("Macro recursive2", "[macro]")
{
	auto syntaxes = std::vector<const char*>{
		"S <- M('abc') M(s) <- !s / s ' ' M(s* '-' '123') / s",
		"S <- M('abc') M(s) <- !s / s ' ' M(s+ '-' '123') / s",
		"S <- M('abc') M(s) <- !s / s ' ' M(s? '-' '123') / s",
		"S <- M('abc') M(s) <- !s / s ' ' M(&s s+ '-' '123') / s",
		"S <- M('abc') M(s) <- !s / s ' ' M(s '-' !s '123') / s",
		"S <- M('abc') M(s) <- !s / s ' ' M(< s > '-' '123') / s",
		"S <- M('abc') M(s) <- !s / s ' ' M(~s '-' '123') / s",
	};

	for (const auto& syntax: syntaxes) {
        parser parser(syntax);
        REQUIRE(parser.parse("abc abc-123"));
	}
}

TEST_CASE("Macro exclusive modifiers", "[macro]")
{
    parser parser(R"(
		S                   <- Modifiers(!"") _
		Modifiers(Appeared) <- (!Appeared) (
								   Token('public') Modifiers(Appeared / 'public') /
								   Token('static') Modifiers(Appeared / 'static') /
								   Token('final') Modifiers(Appeared / 'final') /
								   "")
		Token(t)            <- t _
		_                   <- [ \t\r\n]*
	)");

	REQUIRE(parser.parse("public"));
	REQUIRE(parser.parse("static"));
	REQUIRE(parser.parse("final"));
	REQUIRE(parser.parse("public static final"));
	REQUIRE(!parser.parse("public public"));
	REQUIRE(!parser.parse("public static public"));
}

TEST_CASE("Macro token check test", "[macro]")
{
    parser parser(R"(
        # Grammar for simple calculator...
        EXPRESSION       <-  _ LIST(TERM, TERM_OPERATOR)
        TERM             <-  LIST(FACTOR, FACTOR_OPERATOR)
        FACTOR           <-  NUMBER / T('(') EXPRESSION T(')')
        TERM_OPERATOR    <-  T([-+])
        FACTOR_OPERATOR  <-  T([/*])
        NUMBER           <-  T([0-9]+)
		~_               <-  [ \t]*
		LIST(I, D)       <-  I (D I)*
		T(S)             <-  < S > _
	)");

    REQUIRE(parser["EXPRESSION"].is_token() == false);
    REQUIRE(parser["TERM"].is_token() == false);
    REQUIRE(parser["FACTOR"].is_token() == false);
    REQUIRE(parser["FACTOR_OPERATOR"].is_token() == true);
    REQUIRE(parser["NUMBER"].is_token() == true);
    REQUIRE(parser["_"].is_token() == true);
    REQUIRE(parser["LIST"].is_token() == false);
    REQUIRE(parser["T"].is_token() == true);
}

TEST_CASE("Macro passes an arg to another macro", "[macro]") {
  parser parser(R"(
        A    <- B(C)
        B(D) <- D
        C    <- 'c'
        D    <- 'd'
	)");

  REQUIRE(parser.parse("c"));
}

TEST_CASE("Nested macro call", "[macro]") {
  parser parser(R"(
        A    <- B(T)
        B(X) <- C(X)
        C(Y) <- Y
        T    <- 'val'
	)");

  REQUIRE(parser.parse("val"));
}

TEST_CASE("Nested macro call2", "[macro]")
{
    parser parser(R"(
        START           <- A('TestVal1', 'TestVal2')+
        A(Aarg1, Aarg2) <- B(Aarg1) '#End'
        B(Barg1)        <- '#' Barg1
	)");

    REQUIRE(parser.parse("#TestVal1#End"));
}

TEST_CASE("Line information test", "[line information]") {
    parser parser(R"(
        S    <- _ (WORD _)+
        WORD <- [A-Za-z]+
        ~_   <- [ \t\r\n]+
    )");

    std::vector<std::pair<size_t, size_t>> locations;
    parser["WORD"] = [&](const peg::SemanticValues& sv) {
        locations.push_back(sv.line_info());
    };

    bool ret = parser;
    REQUIRE(ret == true);

    ret = parser.parse(" Mon Tue Wed \nThu  Fri  Sat\nSun\n");
    REQUIRE(ret == true);

    REQUIRE(locations[0] == std::make_pair<size_t, size_t>(1, 2));
    REQUIRE(locations[1] == std::make_pair<size_t, size_t>(1, 6));
    REQUIRE(locations[2] == std::make_pair<size_t, size_t>(1, 10));
    REQUIRE(locations[3] == std::make_pair<size_t, size_t>(2, 1));
    REQUIRE(locations[4] == std::make_pair<size_t, size_t>(2, 6));
    REQUIRE(locations[5] == std::make_pair<size_t, size_t>(2, 11));
    REQUIRE(locations[6] == std::make_pair<size_t, size_t>(3, 1));
}

TEST_CASE("Dictionary", "[dic]")
{
    parser parser(R"(
        START <- 'This month is ' MONTH '.'
        MONTH <- 'Jan' | 'January' | 'Feb' | 'February'
	)");

    REQUIRE(parser.parse("This month is Jan."));
    REQUIRE(parser.parse("This month is January."));
    REQUIRE_FALSE(parser.parse("This month is Jannuary."));
    REQUIRE_FALSE(parser.parse("This month is ."));
}

TEST_CASE("Dictionary invalid", "[dic]")
{
    parser parser(R"(
        START <- 'This month is ' MONTH '.'
        MONTH <- 'Jan' | 'January' | [a-z]+ | 'Feb' | 'February'
	)");

    bool ret = parser;
    REQUIRE_FALSE(ret);
}

// vim: et ts=4 sw=4 cin cino={1s ff=unix
