#ifdef CINT_HOST

#include <cstdint>

#else

using int8_t = char;
using uint8_t = unsigned char;

static_assert(sizeof(int8_t) == 1 && sizeof(uint8_t) == 1, "(u)int8_t != 1");

using int16_t = short;
using uint16_t = unsigned short;

static_assert(sizeof(int16_t) == 2 && sizeof(uint16_t) == 2, "(u)int16_t != 2");

using int32_t = long;
using uint32_t = unsigned long;

static_assert(sizeof(int32_t) == 4 && sizeof(uint32_t) == 4, "(u)int32_t != 4");

#define _SIZE_T
using size_t = uint32_t;

#endif