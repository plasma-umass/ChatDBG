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

    assert(static_cast<bool>(parser) == true);

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
