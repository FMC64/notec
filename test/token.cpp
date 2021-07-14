#include "StrStream.hpp"
#include "Token.hpp"
#include "test.hpp"
#include <cstdio>

using namespace Token;

static void next_assert(Token::Stream &toks, Type exp_type, const char *exp)
{
	auto n = toks.next();
	if (n == nullptr)
		throw "Expected token, got nothing";
	test_assert(static_cast<Type>(n[0]) == exp_type);
	auto exp_size = std::strlen(exp);
	test_assert(static_cast<uint8_t>(n[1]) == exp_size);
	for (size_t i = 0; i < exp_size; i++)
		test_assert(n[2 + i] == exp[i]);
}

static void next_assert_op(Token::Stream &toks, Type exp_type, Op exp)
{
	auto n = toks.next();
	if (n == nullptr)
		throw "Expected token, got nothing";
	test_assert(static_cast<Type>(n[0]) == exp_type);
	test_assert(static_cast<Op>(n[1]) == exp);
}

static void next_assert_char(Token::Stream &toks, Type exp_type, char exp)
{
	auto n = toks.next();
	if (n == nullptr)
		throw "Expected token, got nothing";
	test_assert(static_cast<Type>(n[0]) == exp_type);
	test_assert(n[1] == exp);
}

static StrStream::Buffer read_file(const char *path)
{
	auto file = std::fopen(path, "rb");
	if (file == nullptr)
		throw path;
	std::fseek(file, 0, SEEK_END);
	auto size = static_cast<size_t>(std::ftell(file));
	std::rewind(file);
	auto buf = new char[size];
	test_assert(std::fread(buf, 1, size, file) == size);
	return StrStream::Buffer(size, buf, StrStream::Buffer::buf_rvalue_v);
}

test_case(token_0)
{
	StrStream s("a b\tc\n");
	Token::Stream toks(s);
	next_assert(toks, Type::Identifier, "a");
	next_assert(toks, Type::Identifier, "b");
	next_assert(toks, Type::Identifier, "c");
	test_assert(toks.next() == nullptr);
}

test_case(token_1)
{
	StrStream s("\t\t12   a b\tc _90 0.2e10 .2e10\n");
	Token::Stream toks(s);
	next_assert(toks, Type::NumberLiteral, "12");
	next_assert(toks, Type::Identifier, "a");
	next_assert(toks, Type::Identifier, "b");
	next_assert(toks, Type::Identifier, "c");
	next_assert(toks, Type::Identifier, "_90");
	next_assert(toks, Type::NumberLiteral, "0.2e10");
	next_assert(toks, Type::NumberLiteral, ".2e10");
	test_assert(toks.next() == nullptr);
}

test_case(token_2)
{
	StrStream s("+");
	Token::Stream toks(s);
	next_assert_op(toks, Type::Operator, Op::Plus);
	test_assert(toks.next() == nullptr);
}

test_case(token_3)
{
	StrStream s("++");
	Token::Stream toks(s);
	next_assert_op(toks, Type::Operator, Op::PlusPlus);
	test_assert(toks.next() == nullptr);
}

test_case(token_4)
{
	StrStream s("...");
	Token::Stream toks(s);
	next_assert_op(toks, Type::Operator, Op::Expand);
	test_assert(toks.next() == nullptr);
}

test_case(token_5)
{
	StrStream s("<<=");
	Token::Stream toks(s);
	next_assert_op(toks, Type::Operator, Op::BitLeftEqual);
	test_assert(toks.next() == nullptr);
}

test_case(token_6)
{
	StrStream s(". . .");
	Token::Stream toks(s);
	next_assert_op(toks, Type::Operator, Op::Point);
	next_assert_op(toks, Type::Operator, Op::Point);
	next_assert_op(toks, Type::Operator, Op::Point);
	test_assert(toks.next() == nullptr);
}

test_case(token_7)
{
	StrStream s(". . .\n\na");
	Token::Stream toks(s);
	test_assert(toks.get_off() == 0);
	while (toks.next());
	test_assert(toks.get_row() == 3);
}

test_case(token_8)
{
	StrStream s("(0 ? a::b : c::d) <=a<=>");
	Token::Stream toks(s);
	next_assert_op(toks, Type::Operator, Op::LPar);
	next_assert(toks, Type::NumberLiteral, "0");
	next_assert_op(toks, Type::Operator, Op::Huh);
	next_assert(toks, Type::Identifier, "a");
	next_assert_op(toks, Type::Operator, Op::Scope);
	next_assert(toks, Type::Identifier, "b");
	next_assert_op(toks, Type::Operator, Op::Colon);
	next_assert(toks, Type::Identifier, "c");
	next_assert_op(toks, Type::Operator, Op::Scope);
	next_assert(toks, Type::Identifier, "d");
	next_assert_op(toks, Type::Operator, Op::RPar);
	next_assert_op(toks, Type::Operator, Op::LessEqual);
	next_assert(toks, Type::Identifier, "a");
	next_assert_op(toks, Type::Operator, Op::TWComp);
	test_assert(toks.next() == nullptr);
}

test_case(token_9)
{
	StrStream s("\"abc\"  ");
	Token::Stream toks(s);
	next_assert(toks, Type::StringLiteral, "abc");
	test_assert(toks.next() == nullptr);
}

test_case(token_10)
{
	StrStream s("\'g\'  ");
	Token::Stream toks(s);
	next_assert_char(toks, Type::ValueChar8, 'g');
	test_assert(toks.next() == nullptr);
}

test_case(token_11)
{
	StrStream s("a	// bcd   \nefg  ");
	Token::Stream toks(s);
	next_assert(toks, Type::Identifier, "a");
	next_assert(toks, Type::Identifier, "efg");
	test_assert(toks.next() == nullptr);
}

test_case(token_12)
{
	StrStream s("a	/* //bcd  */ \nefg  ");
	Token::Stream toks(s);
	next_assert(toks, Type::Identifier, "a");
	next_assert(toks, Type::Identifier, "efg");
	test_assert(toks.next() == nullptr);
}

test_case(token_13)	// make sure PP is disabled for now
{
	StrStream s("#define test\nefg  ");
	Token::Stream toks(s);
	next_assert(toks, Type::Identifier, "efg");
	test_assert(toks.next() == nullptr);
}

test_case(token_14)
{
	StrStream s(read_file("./src/Token.hpp"));
	Token::Stream toks(s);
	while (toks.next());
	test_assert(toks.get_error() == nullptr);
}

test_case(token_15)
{
	StrStream s("'aa'");
	Token::Stream toks(s);
	catch(toks) {
		return;
	}
	toks.next();
	test_assert(false);
}
