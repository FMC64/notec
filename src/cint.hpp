#pragma once

using int8_t = char;
using uint8_t = unsigned char;

static_assert(sizeof(int8_t) == 1 && sizeof(uint8_t) == 1, "int8_t != 1");

using int16_t = short;
using uint16_t = unsigned short;

static_assert(sizeof(int16_t) == 2 && sizeof(uint16_t) == 2, "uint16_t != 2");

using int32_t = long;
using uint32_t = unsigned long;

static_assert(sizeof(int32_t) == 4 && sizeof(uint32_t) == 4, "uint32_t != 4");