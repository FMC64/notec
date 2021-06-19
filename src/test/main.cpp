#include <cstdio>
#include "test.hpp"
#include "arith.hpp"

static size_t count = 0;
static size_t pass = 0;
static size_t fail = 0;

void test_run(const char *name, test_t *test)
{
	count++;
	try {
		test();
		pass++;
	} catch (const char *e) {
		fail++;
		printf("NOPASS %s: %s\n", name, e);
	}
}

int main(void)
{
	printf("[SUM] PASS: %zu / %zu\n", pass, count);
	auto any_fail = fail > 0;
	if (any_fail)
		printf("\n\t/!\\   [WRN] FAIL: %zu / %zu   /!\\\n", fail, count);
	return any_fail ? 1 : 0;
}