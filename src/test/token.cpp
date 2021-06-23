#include "Tokenizer.hpp"
#include "StrStream.hpp"
#include "test.hpp"

static void next_assert(Tokenizer &toks, TokenType exp_type, const std::string &exp)
{
	auto n = toks.next();
	if (n == nullptr)
		throw "Expected token, got nothing";
	test_assert(static_cast<TokenType>(n[0]) == exp_type);
	test_assert(static_cast<uint8_t>(n[1]) == exp.size());
	for (size_t i = 0; i < exp.size(); i++)
		test_assert(n[2 + i] == exp[i]);
}

test_case(token_0)
{
	StrStream s("a b\tc\n");
	Tokenizer toks(s);
	next_assert(toks, TokenType::Identifier, "a");
	next_assert(toks, TokenType::Identifier, "b");
	next_assert(toks, TokenType::Identifier, "c");
	test_assert(toks.next() == nullptr);
}

test_case(token_1)
{
	StrStream s("\t\t12   a b\tc _90 0.2e10\n");
	Tokenizer toks(s);
	next_assert(toks, TokenType::NumberLiteral, "12");
	next_assert(toks, TokenType::Identifier, "a");
	next_assert(toks, TokenType::Identifier, "b");
	next_assert(toks, TokenType::Identifier, "c");
	next_assert(toks, TokenType::Identifier, "_90");
	next_assert(toks, TokenType::NumberLiteral, "0.2e10");
	test_assert(toks.next() == nullptr);
}