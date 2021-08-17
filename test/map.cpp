#include "test.hpp"
#include "arith.hpp"
#include "Map.hpp"

test_case(map_0)
{
	Map b;
	auto m = b.create_root();
	b.insert(m, "abc", static_cast<uint16_t>(1));
	uint16_t res;
	test_assert(b.resolve(m, "abc", res));
	test_assert(res == 1);
	b.insert(m, "abe", static_cast<uint16_t>(2));
	test_assert(b.resolve(m, "abe", res));
	test_assert(res == 2);
}

template <typename T, size_t Size>
static void test_strs(const char* (&strs)[Size])
{
	Map b;
	auto m = b.create_root();
	for (size_t i = 0; i < Size; i++)
		b.insert(m, strs[i], static_cast<T>(i));
	for (size_t i = 0; i < Size; i++) {
		T val;
		test_assert(b.resolve(m, strs[i], val));
		test_assert(val == i);
	}
}

template <size_t Size>
static void test_strs_types(const char* (&strs)[Size])
{
	test_strs<uint8_t>(strs);
	test_strs<uint16_t>(strs);
	test_strs<uint32_t>(strs);
	test_strs<uint64_t>(strs);
	test_strs<float>(strs);
	test_strs<double>(strs);
	test_strs<long double>(strs);
}

test_case(map_1)
{
	const char *strs[] = {
		"auto",
		"break",
		"case",
		"char",
		"const",
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

	test_strs_types(strs);
}

test_case(map_2)
{
	const char *strs[] = {
		"define",
		"undef",
		"ifdef",
		"ifndef",
		"elifdef",
		"elifndef",
		"else",
		"endif",
		"include",
	};

	test_strs_types(strs);
}