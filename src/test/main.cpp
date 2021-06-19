#include <cstdio>
#include "test.hpp"
#include "arith.hpp"

test_list(
	dummy_test
);

int main(void)
{
	size_t count = array_size(tests);
	size_t pass = 0;
	size_t fail = 0;
	for (size_t i = 0; i < count; i++) {
		try {
			tests[i]();
			count++;
		} catch (const char *e) {
			fail++;
			printf("NOPASS #%zu: %s\n", i, e);
		}
	}
	printf("[SUM] PASS: %zu / %zu\n", pass, count);
	if (fail > 0)
		printf("/!\\ [WRN] FAIL: %zu / %zu /!\\\n", fail, count);
	return 0;
}