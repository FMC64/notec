#include "Pp.hpp"
#include "test.hpp"

using namespace Token;

const char dummy_name[] = {
	static_cast<char>(Type::StringLiteral),
	1,
	'f'
};

static void next_assert(Pp &p, Type exp_type, const char *exp)
{
	auto n = p.next();
	if (n == nullptr)
		throw "Expected token, got nothing";
	test_assert(static_cast<Type>(n[0]) == exp_type);
	auto exp_size = std::strlen(exp);
	test_assert(static_cast<uint8_t>(n[1]) == exp_size);
	for (size_t i = 0; i < exp_size; i++)
		test_assert(n[2 + i] == exp[i]);
}

static void next_assert_op(Pp &p, Type exp_type, Op exp)
{
	auto n = p.next();
	if (n == nullptr)
		throw "Expected token, got nothing";
	test_assert(static_cast<Type>(n[0]) == exp_type);
	test_assert(static_cast<Op>(n[1]) == exp);
}

test_case(pp_0)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(2);
	s.add_file("f", "a #include <f2>\n b");
	s.add_file("f2", "c");
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "a");
	next_assert(p, Token::Type::Identifier, "c");
	next_assert(p, Token::Type::Identifier, "b");
	test_assert(p.next() == nullptr);
}

test_case(pp_1)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(2);
	s.add_file("f", "a #include\n<f2>\n b");
	s.add_file("f2", "c");
	catch (p.get_tstream()) {
		p.get_stream().close();
		return;
	}
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "a");
	next_assert(p, Token::Type::Identifier, "c");
	next_assert(p, Token::Type::Identifier, "b");
	test_assert(p.next() == nullptr);
	test_assert(false);
}

test_case(pp_2)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(2);
	s.add_file("f", "a #include \\\n<f2> \\\n \n b");
	s.add_file("f2", "c");
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "a");
	next_assert(p, Token::Type::Identifier, "c");
	next_assert(p, Token::Type::Identifier, "b");
	test_assert(p.next() == nullptr);
}

test_case(pp_3)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#define a b c d\n#define a b c d\ne");
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_4)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#define g() h\n#define a(b, c, d, ...) b + c * d - (__VA_ARGS__) \ne");
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_5)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#define a(...) __VA_OPT__(b c d)\ne");
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_6)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#define a(first, ...) #__VA_ARGS__#first\ne");
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_7)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#define cat(a, b) a ## b ## sup\ne");
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_8)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#define mac a b c\nmac e");
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "a");
	next_assert(p, Token::Type::Identifier, "b");
	next_assert(p, Token::Type::Identifier, "c");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_9)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#define mac() a b c\nmac e");
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "mac");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_10)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#define mac () a b c\nmac e");
	p.open(dummy_name);
	next_assert_op(p, Token::Type::Operator, Token::Op::LPar);
	next_assert_op(p, Token::Type::Operator, Token::Op::RPar);
	next_assert(p, Token::Type::Identifier, "a");
	next_assert(p, Token::Type::Identifier, "b");
	next_assert(p, Token::Type::Identifier, "c");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_11)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#define mac() a b c\nmac() e");
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "a");
	next_assert(p, Token::Type::Identifier, "b");
	next_assert(p, Token::Type::Identifier, "c");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_12)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#define add(a, b) a + b\nadd(1, 2) e");
	p.open(dummy_name);
	next_assert(p, Token::Type::NumberLiteral, "1");
	next_assert_op(p, Token::Type::Operator, Token::Op::Plus);
	next_assert(p, Token::Type::NumberLiteral, "2");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_13)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#define add(a, b, ...) a + b __VA_ARGS__ c\nadd(1, 2, 3, 4) e");
	p.open(dummy_name);
	next_assert(p, Token::Type::NumberLiteral, "1");
	next_assert_op(p, Token::Type::Operator, Token::Op::Plus);
	next_assert(p, Token::Type::NumberLiteral, "2");
	next_assert(p, Token::Type::NumberLiteral, "3");
	next_assert_op(p, Token::Type::Operator, Token::Op::Comma);
	next_assert(p, Token::Type::NumberLiteral, "4");
	next_assert(p, Token::Type::Identifier, "c");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_14)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#define add(a, b, ...) a + b __VA_ARGS__ c\nadd(1, 2) e");
	p.open(dummy_name);
	next_assert(p, Token::Type::NumberLiteral, "1");
	next_assert_op(p, Token::Type::Operator, Token::Op::Plus);
	next_assert(p, Token::Type::NumberLiteral, "2");
	next_assert(p, Token::Type::Identifier, "c");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_15)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#define cat_str(a, b, ...) #__VA_OPT__(a + b - __VA_ARGS__)\ncat_str(1, 2, 3) e");
	p.open(dummy_name);
	next_assert(p, Token::Type::StringLiteral, "1+2-3");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_16)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#define cat_str(a, b, ...) #__VA_OPT__(a + b - __VA_ARGS__)\ncat_str(1, 2) e");
	p.open(dummy_name);
	next_assert(p, Token::Type::StringLiteral, "");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_17)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#define cat_str(a, b, ...) __VA_OPT__(a + b - __VA_ARGS__)\ncat_str(1, 2, 3) e");
	p.open(dummy_name);
	next_assert(p, Token::Type::NumberLiteral, "1");
	next_assert_op(p, Token::Type::Operator, Token::Op::Plus);
	next_assert(p, Token::Type::NumberLiteral, "2");
	next_assert_op(p, Token::Type::Operator, Token::Op::Minus);
	next_assert(p, Token::Type::NumberLiteral, "3");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_18)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#define cat_str(a, b, ...) __VA_OPT__(a + b - __VA_ARGS__)\ncat_str(1, 2) e");
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_19)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#define cat_id(a, b) a ## b c\ncat_id(a, b) e");
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "ab");
	next_assert(p, Token::Type::Identifier, "c");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_20)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#define cat_id(a, b, ...) a ## __VA_ARGS__ ## b\ncat_id(a, b, d \" some str \" f) e");
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "ad");
	next_assert(p, Token::Type::StringLiteral, " some str ");
	next_assert(p, Token::Type::Identifier, "fb");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_21)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#define mac a b c\n#undef mac\nmac e");
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "mac");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_22)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#\n#define mac a\n#\nmac e");
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "a");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_23)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(2);
	s.add_file("f", "#include <f2>\ne");
	s.add_file("f2", "#pragma once\na\n#include <f2>\nb");
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "a");
	next_assert(p, Token::Type::Identifier, "b");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_24)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#line 96 \"abc\"\n__FILE__ __LINE__ e");
	p.open(dummy_name);
	next_assert(p, Token::Type::StringLiteral, "abc");
	next_assert(p, Token::Type::NumberLiteral, "96");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_25)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#line 65935\n__FILE__ __LINE__ e");
	p.open(dummy_name);
	next_assert(p, Token::Type::StringLiteral, "f");
	next_assert(p, Token::Type::NumberLiteral, "65935");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_26)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "\"abc \" \" e f g\" h");
	p.open(dummy_name);
	next_assert(p, Token::Type::StringLiteral, "abc  e f g");
	next_assert(p, Token::Type::Identifier, "h");
	test_assert(p.next() == nullptr);
}

test_case(pp_26_nh)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "\"abc \" \" e f g\"");
	p.open(dummy_name);
	next_assert(p, Token::Type::StringLiteral, "abc  e f g");
	test_assert(p.next() == nullptr);
}

test_case(pp_27)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f", "#define mac\na mac e");
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "a");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_28)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f",
R"raw(
	#define mac
	#ifdef mac
		g
		#ifdef wut
			err
		#else
			o
		#endif
	#else
		err
	#endif
	e
)raw"
);
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "g");
	next_assert(p, Token::Type::Identifier, "o");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_29)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(2);
	s.add_file("f", "#define to_inc <f2>\n a #include to_inc\n b");
	s.add_file("f2", "c");
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "a");
	next_assert(p, Token::Type::Identifier, "c");
	next_assert(p, Token::Type::Identifier, "b");
	test_assert(p.next() == nullptr);
}

test_case(pp_30)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f",
R"raw(
	#if 1 + 2 * 3 + 6 * 2 == 19 && 19 == 1 + 2 * 3 + 6 * 2 + sample_undef
		g
	#else
		err
	#endif
	e
)raw"
);
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "g");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_31)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f",
R"raw(
	#define mac
	#if defined(mac)
		g
	#else
		err
	#endif
	e
)raw"
);
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "g");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}

test_case(pp_32)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(1);
	s.add_file("f",
R"raw(
	#define mac
	#if !defined mac
		err
	#else
		g
	#endif
	e
)raw"
);
	p.open(dummy_name);
	next_assert(p, Token::Type::Identifier, "g");
	next_assert(p, Token::Type::Identifier, "e");
	test_assert(p.next() == nullptr);
}
