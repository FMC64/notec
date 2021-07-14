#include "test.hpp"
#include "arith.hpp"
#include "StrMap.hpp"

test_case(strmap_0)
{
	StrMap::BlockReserve r;
	auto m = r.allocate();
	m.insert("abc", static_cast<uint16_t>(1));
	uint16_t res;
	test_assert(m.resolve("abc", res));
	test_assert(res == 1);
	m.insert("abe", static_cast<uint16_t>(2));
	test_assert(m.resolve("abe", res));
	test_assert(res == 2);
}

test_case(strmap_1)
{
	StrMap::BlockReserve r;
	auto m = r.allocate();
	const char *strs[] = {
		"auto",
		"break"
		"case",
		"char",
		"const"
		"continue",
		"default",
		"do",
		"double",
		"else",
		"enum",
		"extern",
		"float",
		"for",
		"goto",
		"if",
		"inline",
		"int",
		"long",
		"register",
		"return",
		"short",
		"signed",
		"sizeof",
		"static",
		"struct",
		"switch",
		"typedef",
		"union",
		"unsigned",
		"void",
		"volatile",
		"while"
	};

	for (size_t i = 0; i < array_size(strs); i++)
		m.insert(strs[i], static_cast<uint16_t>(i));
}