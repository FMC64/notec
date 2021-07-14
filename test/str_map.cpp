#include "test.hpp"
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