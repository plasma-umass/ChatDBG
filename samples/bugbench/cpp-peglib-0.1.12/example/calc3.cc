//
//  calc3.cc
//
//  Copyright (c) 2015 Yuji Hirose. All rights reserved.
//  MIT License
//

#include <peglib.h>
#include <iostream>
#include <cstdlib>

using namespace peg;

int main(int argc, const char** argv)
{
    if (argc < 2 || std::string("--help") == argv[1]) {
        std::cout << "usage: calc3 [formula]" << std::endl;
        return 1;
    }

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

    parser parser(R"(
        EXPRESSION       <-  TERM (TERM_OPERATOR TERM)*
        TERM             <-  FACTOR (FACTOR_OPERATOR FACTOR)*
        FACTOR           <-  NUMBER / '(' EXPRESSION ')'

        TERM_OPERATOR    <-  < [-+] >
        FACTOR_OPERATOR  <-  < [/*] >
        NUMBER           <-  < [0-9]+ >

        %whitespace      <-  [ \t\r\n]*
    )");

    parser.enable_ast();

    auto expr = argv[1];
    std::shared_ptr<Ast> ast;
    if (parser.parse(expr, ast)) {
        ast = AstOptimizer(true).optimize(ast);
        std::cout << ast_to_s(ast);
        std::cout << expr << " = " << eval(*ast) << std::endl;
        return 0;
    }

    std::cout << "syntax error..." << std::endl;

    return -1;
}

// vim: et ts=4 sw=4 cin cino={1s ff=unix
