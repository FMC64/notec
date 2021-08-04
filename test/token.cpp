#include "StrStream.hpp"
#include "TokenStream.hpp"
#include "test.hpp"
#include <cstdio>

using namespace Token;

const char dummy_name[] = {
	static_cast<char>(Type::StringLiteral),
	1,
	'f'
};

static Token::Stream init_file(const char *src)
{
	auto res = Token::Stream();
	auto &s = res.get_stream();
	s.set_file_count(1);
	s.add_file("f", src);
	res.push(dummy_name);
	return res;
}

static Token::Stream init_file(StrStream::Buffer &&buf)
{
	auto res = Token::Stream();
	auto &s = res.get_stream();
	s.set_file_count(1);
	s.add_file("f", static_cast<StrStream::Buffer&&>(buf));
	res.push(dummy_name);
	return res;
}

static void next_assert(Token::Stream &toks, Type exp_type, const char *exp)
{
	char *n;
	n = toks.next();
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
	StrStream::Buffer res;
	res.size = size;
	res.data = buf;
	return res;
}

test_case(token_0)
{
	auto toks = init_file("a b\tc\n");
	next_assert(toks, Type::Identifier, "a");
	next_assert(toks, Type::Identifier, "b");
	next_assert(toks, Type::Identifier, "c");
	test_assert(toks.next() == nullptr);
	toks.get_stream().close();
}

test_case(token_1)
{
	auto toks = init_file("\t\t12   a b\tc _90 0.2e10 .2e10\n");
	next_assert(toks, Type::NumberLiteral, "12");
	next_assert(toks, Type::Identifier, "a");
	next_assert(toks, Type::Identifier, "b");
	next_assert(toks, Type::Identifier, "c");
	next_assert(toks, Type::Identifier, "_90");
	next_assert(toks, Type::NumberLiteral, "0.2e10");
	next_assert(toks, Type::NumberLiteral, ".2e10");
	test_assert(toks.next() == nullptr);
	toks.get_stream().close();
}

test_case(token_2)
{
	auto toks = init_file("+");
	next_assert_op(toks, Type::Operator, Op::Plus);
	test_assert(toks.next() == nullptr);
	toks.get_stream().close();
}

test_case(token_3)
{
	auto toks = init_file("++");
	next_assert_op(toks, Type::Operator, Op::PlusPlus);
	test_assert(toks.next() == nullptr);
	toks.get_stream().close();
}

test_case(token_4)
{
	auto toks = init_file("...");
	next_assert_op(toks, Type::Operator, Op::Expand);
	test_assert(toks.next() == nullptr);
	toks.get_stream().close();
}

test_case(token_5)
{
	auto toks = init_file("<<=");
	next_assert_op(toks, Type::Operator, Op::BitLeftEqual);
	test_assert(toks.next() == nullptr);
	toks.get_stream().close();
}

test_case(token_6)
{
	auto toks = init_file(". . .");
	next_assert_op(toks, Type::Operator, Op::Point);
	next_assert_op(toks, Type::Operator, Op::Point);
	next_assert_op(toks, Type::Operator, Op::Point);
	test_assert(toks.next() == nullptr);
	toks.get_stream().close();
}

test_case(token_7)
{
	auto toks = init_file(". . .\n\na");
	test_assert(toks.get_off() == 0);
	while (toks.next());
	test_assert(toks.get_row() == 3);
	toks.get_stream().close();
}

test_case(token_8)
{
	auto toks = init_file("(0 ? a::b : c::d) <=a<=>");
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
	toks.get_stream().close();
}

test_case(token_9)
{
	auto toks = init_file("\"abc\"  ");
	next_assert(toks, Type::StringLiteral, "abc");
	test_assert(toks.next() == nullptr);
	toks.get_stream().close();
}

test_case(token_10)
{
	auto toks = init_file("\'g\'  ");
	next_assert_char(toks, Type::ValueChar8, 'g');
	test_assert(toks.next() == nullptr);
	toks.get_stream().close();
}

test_case(token_11)
{
	auto toks = init_file("a	// bcd   \nefg  ");
	next_assert(toks, Type::Identifier, "a");
	next_assert(toks, Type::Identifier, "efg");
	test_assert(toks.next() == nullptr);
	toks.get_stream().close();
}

test_case(token_12)
{
	auto toks = init_file("a	/* //bcd  */ \nefg  ");
	next_assert(toks, Type::Identifier, "a");
	next_assert(toks, Type::Identifier, "efg");
	test_assert(toks.next() == nullptr);
	toks.get_stream().close();
}

test_case(token_13)
{
	auto toks = init_file("#define test ##\nefg  ");
	next_assert_op(toks, Type::Operator, Op::Sharp);
	next_assert(toks, Type::Identifier, "define");
	next_assert(toks, Type::Identifier, "test");
	next_assert_op(toks, Type::Operator, Op::DoubleSharp);
	next_assert(toks, Type::Identifier, "efg");
	test_assert(toks.next() == nullptr);
	toks.get_stream().close();
}

test_case(token_14)
{
	{
		auto toks = init_file(read_file("./src/TokenStream.hpp"));
		while (toks.next());
		test_assert(toks.get_error() == nullptr);
		toks.get_stream().close();
	}
	{
		auto toks = init_file(read_file("./src/Pp.hpp"));
		while (toks.next());
		test_assert(toks.get_error() == nullptr);
		toks.get_stream().close();
	}
}

test_case(token_15)
{
	auto toks = init_file("'aa'");
	catch(toks) {
		toks.get_stream().close();
		return;
	}
	toks.next();
	test_assert(false);
}


test_case(token_16)
{
	auto toks = init_file("a // abc \\\ndef\ng");
	next_assert(toks, Type::Identifier, "a");
	next_assert(toks, Type::Identifier, "g");
	toks.get_stream().close();
}


test_case(token_17)
{
	auto toks = init_file("a\\\ndef");
	next_assert(toks, Type::Identifier, "a");
	next_assert(toks, Type::Identifier, "def");
	test_assert(toks.get_line_escaped());
	toks.get_stream().close();
}

test_case(token_18)
{
	auto toks = init_file("<cstdio>");
	next_assert_op(toks, Type::Operator, Op::Less);
	next_assert(toks, Type::Identifier, "cstdio");
	next_assert_op(toks, Type::Operator, Op::Greater);
	test_assert(toks.next() == nullptr);
	toks.get_stream().close();
}

test_case(token_19)
{
	auto res = Token::Stream();
	auto &s = res.get_stream();
	s.set_file_count(1);
	s.add_file("f", "sample tok");
	res.push(dummy_name);
	test_assert(!res.pop());
}

test_case(token_20)
{
	auto res = Token::Stream();
	auto &s = res.get_stream();
	s.set_file_count(1);
	s.add_file("f", "sample tok");
	res.push(dummy_name);
	res.push(dummy_name);
	res.push(dummy_name);
	test_assert(res.pop());
	test_assert(res.pop());
	test_assert(!res.pop());
}

test_case(token_21)
{
	auto toks = init_file("->*");
	next_assert_op(toks, Type::Operator, Op::ArrowMember);
	test_assert(toks.next() == nullptr);
	toks.get_stream().close();
}

test_case(token_22)
{
	auto toks = init_file("\"abc\\xFF\\77\"  ");
	next_assert(toks, Type::StringLiteral, "abc\xFF\77");
	test_assert(toks.next() == nullptr);
	toks.get_stream().close();
}