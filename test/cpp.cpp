#include "Cpp.hpp"
#include "test.hpp"

const char dummy_name[] = {
	static_cast<char>(Token::Type::StringLiteral),
	1,
	'f'
};

static Cpp init_file(const char *src)
{
	auto res = Cpp();
	auto &s = res.err_src().get_stream();
	s.set_file_count(1);
	s.add_file("f", src);
	res.open(dummy_name);
	return res;
}

static inline constexpr size_t base = 6;

test_case(cpp_0)
{
	auto c = init_file("typedef const int * const volatile * volatile ppi32;");
	c.run();
	auto b = c.get_buffer();
	test_assert(b[base + 0] == (Type::volatile_flag | static_cast<char>(Type::Prim::Ptr)));
	test_assert(b[base + 1] == (Type::const_flag | Type::volatile_flag | static_cast<char>(Type::Prim::Ptr)));
	test_assert(b[base + 2] == (Type::const_flag | static_cast<char>(Type::Prim::S32)));
}

test_case(cpp_1)
{
	auto c = init_file("typedef const char *&n;");
	c.run();
	auto b = c.get_buffer();
	test_assert(b[base + 0] == static_cast<char>(Type::Prim::Lref));
	test_assert(b[base + 1] == static_cast<char>(Type::Prim::Ptr));
	test_assert(b[base + 2] == (Type::const_flag | static_cast<char>(Type::Prim::S8)));
}

test_case(cpp_2)
{
	auto c = init_file("typedef const struct { public: } S;");
	c.run();
	auto b = c.get_buffer();
	test_assert(b[base + 0] == static_cast<char>(Cpp::ContType::Struct));
	test_assert(load<uint16_t>(b + base + 1));
	test_assert(load_part<3, uint32_t>(b + base + 3) == base);
}

test_case(cpp_3)
{
	auto c = init_file("typedef const struct S_t { public: } S;");
	c.run();
	auto b = c.get_buffer();
	test_assert(b[base + 0] == static_cast<char>(Cpp::ContType::Struct));
	test_assert(load<uint16_t>(b + base + 1));
	test_assert(load_part<3, uint32_t>(b + base + 3) == base);
}