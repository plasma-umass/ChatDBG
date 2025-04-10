//
//  calc2.cc
//
//  Copyright (c) 2015 Yuji Hirose. All rights reserved.
//  MIT License
//

#include <peglib.h>
#include <iostream>
#include <cstdlib>

using namespace peg;

//
//  PEG syntax:
//
//      EXPRESSION       <-  TERM (TERM_OPERATOR TERM)*
//      TERM             <-  FACTOR (FACTOR_OPERATOR FACTOR)*
//      FACTOR           <-  NUMBER / '(' EXPRESSION ')'
//      TERM_OPERATOR    <-  [-+]
//      FACTOR_OPERATOR  <-  [/*]
//      NUMBER           <-  [0-9]+
//
int main(int argc, const char** argv)
{
    if (argc < 2 || std::string("--help") == argv[1]) {
        std::cout << "usage: calc [formula]" << std::endl;
        return 1;
    }

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

    Definition EXPRESSION, TERM, FACTOR, TERM_OPERATOR, FACTOR_OPERATOR, NUMBER;

    EXPRESSION      <= seq(TERM, zom(seq(TERM_OPERATOR, TERM))),         reduce;
    TERM            <= seq(FACTOR, zom(seq(FACTOR_OPERATOR, FACTOR))),   reduce;
    FACTOR          <= cho(NUMBER, seq(chr('('), EXPRESSION, chr(')')));
    TERM_OPERATOR   <= cls("+-"),                                        [](const SemanticValues& sv) { return static_cast<char>(*sv.c_str()); };
    FACTOR_OPERATOR <= cls("*/"),                                        [](const SemanticValues& sv) { return static_cast<char>(*sv.c_str()); };
    NUMBER          <= oom(cls("0-9")),                                  [](const SemanticValues& sv) { return atol(sv.c_str()); };

    auto expr = argv[1];
    long val = 0;
    if (EXPRESSION.parse_and_get_value(expr, val).ret) {
        std::cout << expr << " = " << val << std::endl;
        return 0;
    }

    return -1;
}

// vim: et ts=4 sw=4 cin cino={1s ff=unix
