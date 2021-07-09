#pragma once

using test_t = void (void);

void test_run(const char *name, test_t *test);

#define test_case(name) void name(void); static struct _caller_##name { _caller_##name() { test_run(#name, name); } } _caller_inst_##name; void name(void)
#define test_assert(...) if (!(__VA_ARGS__)) throw #__VA_ARGS__