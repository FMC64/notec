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
	if (fail > 0)
		printf("\n\t/!\\   [WRN] FAIL: %zu / %zu   /!\\\n", fail, count);
	return 0;
}