#include "test.hpp"
#include "StrMap.hpp"

test_case(strmap_0)
{
	StrMap::BlockReserve r;
	auto m = r.allocate();
	m.insert("abc", static_cast<uint16_t>(1));
	m.insert("abe", static_cast<uint16_t>(2));
}