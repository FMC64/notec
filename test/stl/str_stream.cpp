#include <string>
#include <vector>

#include "StrStream.hpp"
#include "test.hpp"

const char dummy_name[] = {
	static_cast<char>(Token::Type::StringLiteral),
	1,
	'f'
};

template <size_t BufSize>
static std::vector<std::string> set(const char *in)
{
	char buf[BufSize + 1];
	std::vector<std::string> res;

	StrStream s;
	s.set_file_count(1);
	s.add_file("f", in);
	char stack_base[256];
	auto stack = stack_base;
	test_assert(s.open(dummy_name, stack_base, stack, stack_base + 256));
	while (auto size = s.read(buf, BufSize)) {
		buf[size] = 0;
		res.emplace_back(buf);
	}
	s.close();
	return res;
}

test_case(str_stream_0)
{
	test_assert(set<4>("123456789") == std::vector<std::string>{
		"1234", "5678", "9"
	});
}

test_case(str_stream_1)
{
	test_assert(set<100000>("123456789") == std::vector<std::string>{
		"123456789"
	});
}