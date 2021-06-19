#pragma once

#define test_case(name) void name(void)

using test_t = void (void);

#define test_prototypes(first, ...) test_case(first); __VA_OPT__(test_prototypes(__VA_ARGS__))
#define test_list_impl(first, ...) first, __VA_OPT__(test_list_impl(__VA_ARGS__))

#define test_list(...) test_prototypes(__VA_ARGS__) static test_t *tests[] = {test_list_impl(__VA_ARGS__)}