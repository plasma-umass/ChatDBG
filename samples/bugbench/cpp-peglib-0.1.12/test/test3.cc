#include "catch.hh"
#include <peglib.h>

using namespace peg;

inline bool exact(Grammar& g, const char* d, const char* s) {
    auto n = strlen(s);
    auto r = g[d].parse(s, n);
    return r.ret && r.len == n;
}

inline Grammar& make_peg_grammar() {
    return ParserGenerator::grammar();
}

TEST_CASE("PEG Grammar", "[peg]")
{
    auto g = ParserGenerator::grammar();
    REQUIRE(exact(g, "Grammar", " Definition <- a / ( b c ) / d \n rule2 <- [a-zA-Z][a-z0-9-]+ ") == true);
}

TEST_CASE("PEG Definition", "[peg]")
{
    auto g = ParserGenerator::grammar();
    REQUIRE(exact(g, "Definition", "Definition <- a / (b c) / d ") == true);
    REQUIRE(exact(g, "Definition", "Definition <- a / b c / d ") == true);
    REQUIRE(exact(g, "Definition", u8"Definitiond ← a ") == true);
    REQUIRE(exact(g, "Definition", "Definition ") == false);
    REQUIRE(exact(g, "Definition", " ") == false);
    REQUIRE(exact(g, "Definition", "") == false);
    REQUIRE(exact(g, "Definition", "Definition = a / (b c) / d ") == false);
	REQUIRE(exact(g, "Definition", "Macro(param) <- a ") == true);
	REQUIRE(exact(g, "Definition", "Macro (param) <- a ") == false);
}

TEST_CASE("PEG Expression", "[peg]")
{
    auto g = ParserGenerator::grammar();
    REQUIRE(exact(g, "Expression", "a / (b c) / d ") == true);
    REQUIRE(exact(g, "Expression", "a / b c / d ") == true);
    REQUIRE(exact(g, "Expression", "a b ") == true);
    REQUIRE(exact(g, "Expression", "") == true);
    REQUIRE(exact(g, "Expression", " ") == false);
    REQUIRE(exact(g, "Expression", " a b ") == false);
}

TEST_CASE("PEG Sequence", "[peg]")
{
    auto g = ParserGenerator::grammar();
    REQUIRE(exact(g, "Sequence", "a b c d ") == true);
    REQUIRE(exact(g, "Sequence", "") == true);
    REQUIRE(exact(g, "Sequence", "!") == false);
    REQUIRE(exact(g, "Sequence", "<-") == false);
    REQUIRE(exact(g, "Sequence", " a") == false);
}

TEST_CASE("PEG Prefix", "[peg]")
{
    auto g = ParserGenerator::grammar();
    REQUIRE(exact(g, "Prefix", "&[a]") == true);
    REQUIRE(exact(g, "Prefix", "![']") == true);
    REQUIRE(exact(g, "Prefix", "-[']") == false);
    REQUIRE(exact(g, "Prefix", "") == false);
    REQUIRE(exact(g, "Prefix", " a") == false);
}

TEST_CASE("PEG Suffix", "[peg]")
{
    auto g = ParserGenerator::grammar();
    REQUIRE(exact(g, "Suffix", "aaa ") == true);
    REQUIRE(exact(g, "Suffix", "aaa? ") == true);
    REQUIRE(exact(g, "Suffix", "aaa* ") == true);
    REQUIRE(exact(g, "Suffix", "aaa+ ") == true);
    REQUIRE(exact(g, "Suffix", "aaa{} ") == false);
    REQUIRE(exact(g, "Suffix", "aaa{10} ") == true);
    REQUIRE(exact(g, "Suffix", "aaa{10,} ") == true);
    REQUIRE(exact(g, "Suffix", "aaa{10,100} ") == true);
    REQUIRE(exact(g, "Suffix", "aaa{,100} ") == true);
    REQUIRE(exact(g, "Suffix", ". + ") == true);
    REQUIRE(exact(g, "Suffix", ". {10} ") == true);
    REQUIRE(exact(g, "Suffix", "?") == false);
    REQUIRE(exact(g, "Suffix", "+") == false);
    REQUIRE(exact(g, "Suffix", "{10}") == false);
    REQUIRE(exact(g, "Suffix", "") == false);
    REQUIRE(exact(g, "Suffix", " a") == false);
}

TEST_CASE("PEG Primary", "[peg]")
{
    auto g = ParserGenerator::grammar();
    REQUIRE(exact(g, "Primary", "_Identifier0_ ") == true);
    REQUIRE(exact(g, "Primary", "_Identifier0_<-") == false);
    REQUIRE(exact(g, "Primary", "( _Identifier0_ _Identifier1_ )") == true);
    REQUIRE(exact(g, "Primary", "'Literal String'") == true);
    REQUIRE(exact(g, "Primary", "\"Literal String\"") == true);
    REQUIRE(exact(g, "Primary", "[a-zA-Z]") == true);
    REQUIRE(exact(g, "Primary", ".") == true);
    REQUIRE(exact(g, "Primary", "") == false);
    REQUIRE(exact(g, "Primary", " ") == false);
    REQUIRE(exact(g, "Primary", " a") == false);
    REQUIRE(exact(g, "Primary", "") == false);
}

TEST_CASE("PEG Identifier", "[peg]")
{
    auto g = ParserGenerator::grammar();
    REQUIRE(exact(g, "Identifier", "_Identifier0_ ") == true);
    REQUIRE(exact(g, "Identifier", "0Identifier_ ") == false);
    REQUIRE(exact(g, "Identifier", "Iden|t ") == false);
    REQUIRE(exact(g, "Identifier", " ") == false);
    REQUIRE(exact(g, "Identifier", " a") == false);
    REQUIRE(exact(g, "Identifier", "") == false);
}

TEST_CASE("PEG IdentStart", "[peg]")
{
    auto g = ParserGenerator::grammar();
    REQUIRE(exact(g, "IdentStart", "_") == true);
    REQUIRE(exact(g, "IdentStart", "a") == true);
    REQUIRE(exact(g, "IdentStart", "Z") == true);
    REQUIRE(exact(g, "IdentStart", "") == false);
    REQUIRE(exact(g, "IdentStart", " ") == false);
    REQUIRE(exact(g, "IdentStart", "0") == false);
}

TEST_CASE("PEG IdentRest", "[peg]")
{
    auto g = ParserGenerator::grammar();
    REQUIRE(exact(g, "IdentRest", "_") == true);
    REQUIRE(exact(g, "IdentRest", "a") == true);
    REQUIRE(exact(g, "IdentRest", "Z") == true);
    REQUIRE(exact(g, "IdentRest", "") == false);
    REQUIRE(exact(g, "IdentRest", " ") == false);
    REQUIRE(exact(g, "IdentRest", "0") == true);
}

TEST_CASE("PEG Literal", "[peg]")
{
    auto g = ParserGenerator::grammar();
    REQUIRE(exact(g, "Literal", "'abc' ") == true);
    REQUIRE(exact(g, "Literal", "'a\\nb\\tc' ") == true);
    REQUIRE(exact(g, "Literal", "'a\\277\tc' ") == true);
    REQUIRE(exact(g, "Literal", "'a\\77\tc' ") == true);
    REQUIRE(exact(g, "Literal", "'a\\80\tc' ") == false);
    REQUIRE(exact(g, "Literal", "'\n' ") == true);
    REQUIRE(exact(g, "Literal", "'a\\'b' ") == true);
    REQUIRE(exact(g, "Literal", "'a'b' ") == false);
    REQUIRE(exact(g, "Literal", "'a\"'b' ") == false);
    REQUIRE(exact(g, "Literal", "\"'\\\"abc\\\"'\" ") == true);
    REQUIRE(exact(g, "Literal", "\"'\"abc\"'\" ") == false);
    REQUIRE(exact(g, "Literal", "abc") == false);
    REQUIRE(exact(g, "Literal", "") == false);
    REQUIRE(exact(g, "Literal", "\\") == false);
    REQUIRE(exact(g, "Literal", u8"'日本語'") == true);
    REQUIRE(exact(g, "Literal", u8"\"日本語\"") == true);
    REQUIRE(exact(g, "Literal", u8"日本語") == false);
}

TEST_CASE("PEG Class", "[peg]")
{
    auto g = ParserGenerator::grammar();
    REQUIRE(exact(g, "Class", "[]") == false); // NOTE: This is different from the Brian Ford's paper, but same as RegExp
    REQUIRE(exact(g, "Class", "[a]") == true);
    REQUIRE(exact(g, "Class", "[a-z]") == true);
    REQUIRE(exact(g, "Class", "[az]") == true);
    REQUIRE(exact(g, "Class", "[a-zA-Z-]") == true);
    REQUIRE(exact(g, "Class", "[a-zA-Z-0-9]") == true);
    REQUIRE(exact(g, "Class", "[a-]") == false);
    REQUIRE(exact(g, "Class", "[-a]") == true);
    REQUIRE(exact(g, "Class", "[") == false);
    REQUIRE(exact(g, "Class", "[a") == false);
    REQUIRE(exact(g, "Class", "]") == false);
    REQUIRE(exact(g, "Class", "a]") == false);
    REQUIRE(exact(g, "Class", u8"[あ-ん]") == true);
    REQUIRE(exact(g, "Class", u8"あ-ん") == false);
    REQUIRE(exact(g, "Class", "[-+]") == true);
    REQUIRE(exact(g, "Class", "[+-]") == false);
    REQUIRE(exact(g, "Class", "[\\^]") == true);
}

TEST_CASE("PEG Negated Class", "[peg]")
{
    auto g = ParserGenerator::grammar();
    REQUIRE(exact(g, "NegatedClass", "[^]") == false);
    REQUIRE(exact(g, "NegatedClass", "[^a]") == true);
    REQUIRE(exact(g, "NegatedClass", "[^a-z]") == true);
    REQUIRE(exact(g, "NegatedClass", "[^az]") == true);
    REQUIRE(exact(g, "NegatedClass", "[^a-zA-Z-]") == true);
    REQUIRE(exact(g, "NegatedClass", "[^a-zA-Z-0-9]") == true);
    REQUIRE(exact(g, "NegatedClass", "[^a-]") == false);
    REQUIRE(exact(g, "NegatedClass", "[^-a]") == true);
    REQUIRE(exact(g, "NegatedClass", "[^") == false);
    REQUIRE(exact(g, "NegatedClass", "[^a") == false);
    REQUIRE(exact(g, "NegatedClass", "^]") == false);
    REQUIRE(exact(g, "NegatedClass", "^a]") == false);
    REQUIRE(exact(g, "NegatedClass", u8"[^あ-ん]") == true);
    REQUIRE(exact(g, "NegatedClass", u8"^あ-ん") == false);
    REQUIRE(exact(g, "NegatedClass", "[^-+]") == true);
    REQUIRE(exact(g, "NegatedClass", "[^+-]") == false);
    REQUIRE(exact(g, "NegatedClass", "[^^]") == true);
}

TEST_CASE("PEG Range", "[peg]")
{
    auto g = ParserGenerator::grammar();
    REQUIRE(exact(g, "Range", "a") == true);
    REQUIRE(exact(g, "Range", "a-z") == true);
    REQUIRE(exact(g, "Range", "az") == false);
    REQUIRE(exact(g, "Range", "") == false);
    REQUIRE(exact(g, "Range", "a-") == false);
    REQUIRE(exact(g, "Range", "-a") == false);
}

TEST_CASE("PEG Char", "[peg]")
{
    auto g = ParserGenerator::grammar();
    REQUIRE(exact(g, "Char", "\\n") == true);
    REQUIRE(exact(g, "Char", "\\r") == true);
    REQUIRE(exact(g, "Char", "\\t") == true);
    REQUIRE(exact(g, "Char", "\\'") == true);
    REQUIRE(exact(g, "Char", "\\\"") == true);
    REQUIRE(exact(g, "Char", "\\[") == true);
    REQUIRE(exact(g, "Char", "\\]") == true);
    REQUIRE(exact(g, "Char", "\\\\") == true);
    REQUIRE(exact(g, "Char", "\\000") == true);
    REQUIRE(exact(g, "Char", "\\377") == true);
    REQUIRE(exact(g, "Char", "\\477") == false);
    REQUIRE(exact(g, "Char", "\\087") == false);
    REQUIRE(exact(g, "Char", "\\079") == false);
    REQUIRE(exact(g, "Char", "\\00") == true);
    REQUIRE(exact(g, "Char", "\\77") == true);
    REQUIRE(exact(g, "Char", "\\80") == false);
    REQUIRE(exact(g, "Char", "\\08") == false);
    REQUIRE(exact(g, "Char", "\\0") == true);
    REQUIRE(exact(g, "Char", "\\7") == true);
    REQUIRE(exact(g, "Char", "\\8") == false);
    REQUIRE(exact(g, "Char", "a") == true);
    REQUIRE(exact(g, "Char", ".") == true);
    REQUIRE(exact(g, "Char", "0") == true);
    REQUIRE(exact(g, "Char", "\\") == false);
    REQUIRE(exact(g, "Char", " ") == true);
    REQUIRE(exact(g, "Char", "  ") == false);
    REQUIRE(exact(g, "Char", "") == false);
    REQUIRE(exact(g, "Char", u8"あ") == true);
}

TEST_CASE("PEG Operators", "[peg]")
{
    auto g = ParserGenerator::grammar();
    REQUIRE(exact(g, "LEFTARROW", "<-") == true);
    REQUIRE(exact(g, "SLASH", "/ ") == true);
    REQUIRE(exact(g, "AND", "& ") == true);
    REQUIRE(exact(g, "NOT", "! ") == true);
    REQUIRE(exact(g, "QUESTION", "? ") == true);
    REQUIRE(exact(g, "STAR", "* ") == true);
    REQUIRE(exact(g, "PLUS", "+ ") == true);
    REQUIRE(exact(g, "OPEN", "( ") == true);
    REQUIRE(exact(g, "CLOSE", ") ") == true);
    REQUIRE(exact(g, "DOT", ". ") == true);
}

TEST_CASE("PEG Comment", "[peg]")
{
    auto g = ParserGenerator::grammar();
    REQUIRE(exact(g, "Comment", "# Comment.\n") == true);
    REQUIRE(exact(g, "Comment", "# Comment.") == false);
    REQUIRE(exact(g, "Comment", " ") == false);
    REQUIRE(exact(g, "Comment", "a") == false);
}

TEST_CASE("PEG Space", "[peg]")
{
    auto g = ParserGenerator::grammar();
    REQUIRE(exact(g, "Space", " ") == true);
    REQUIRE(exact(g, "Space", "\t") == true);
    REQUIRE(exact(g, "Space", "\n") == true);
    REQUIRE(exact(g, "Space", "") == false);
    REQUIRE(exact(g, "Space", "a") == false);
}

TEST_CASE("PEG EndOfLine", "[peg]")
{
    auto g = ParserGenerator::grammar();
    REQUIRE(exact(g, "EndOfLine", "\r\n") == true);
    REQUIRE(exact(g, "EndOfLine", "\n") == true);
    REQUIRE(exact(g, "EndOfLine", "\r") == true);
    REQUIRE(exact(g, "EndOfLine", " ") == false);
    REQUIRE(exact(g, "EndOfLine", "") == false);
    REQUIRE(exact(g, "EndOfLine", "a") == false);
}

TEST_CASE("PEG EndOfFile", "[peg]")
{
    Grammar g = make_peg_grammar();
    REQUIRE(exact(g, "EndOfFile", "") == true);
    REQUIRE(exact(g, "EndOfFile", " ") == false);
}

// vim: et ts=4 sw=4 cin cino={1s ff=unix
