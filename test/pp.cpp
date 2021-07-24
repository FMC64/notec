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
	char *n = p.next();
	if (n == nullptr)
		throw "Expected token, got nothing";
	test_assert(static_cast<Type>(n[0]) == exp_type);
	auto exp_size = std::strlen(exp);
	test_assert(static_cast<uint8_t>(n[1]) == exp_size);
	for (size_t i = 0; i < exp_size; i++)
		test_assert(n[2 + i] == exp[i]);
}

test_case(pp_0)
{
	Pp p;
	auto &s = p.get_stream();
	s.set_file_count(2);
	s.add_file("f", "a #include <f2> b");
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
	s.add_file("f", "a #include\n<f2> b");
	s.add_file("f2", "c");
	catch (p.get_tstream())
		return;
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
	s.add_file("f", "a #include \\\n<f2> b");
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