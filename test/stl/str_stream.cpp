#include "test.hpp"
#include "StrStream.hpp"

#include <string>
#include <vector>

template <size_t BufSize>
static std::vector<std::string> set(const char *in)
{
	char buf[BufSize + 1];
	std::vector<std::string> res;

	StrStream s;
	s.set_file_data(in);
	while (auto size = s.read(buf, BufSize)) {
		buf[size] = 0;
		res.emplace_back(buf);
	}
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