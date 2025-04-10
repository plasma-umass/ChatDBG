#include <peglib.h>
#include <assert.h>
#include <iostream>

using namespace peg;
using namespace std;

int main(void) {
    parser parser(R"(
        EXPRESSION  <- ATOM (OPERATOR ATOM)* {
                         precedence
                           L - +
                           L / *
                       }
        ATOM        <- NUMBER / '(' EXPRESSION ')'
        OPERATOR    <- < [-+/*] >
        NUMBER      <- < '-'? [0-9]+ >
        %whitespace <- [ \t\r\n]*
    )");

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

    long val;
    parser.parse(" -1 + (1 + 2) * 3 - -1", val);

    assert(val == 9);
}
