#pragma once

#include "cint.hpp"

namespace File {

// 8 base chars + '.' + 3 ext
static inline constexpr size_t hashed_len = 11;
using hashed = char[hashed_len];

// path is cstr
static inline void hash_path(const char *path, hashed dst)
{
	char base[5] = {};	// we actually hash encode on 5 bytes
	uint8_t size = path[0];
	path++;
	uint8_t n = 0;
	for (uint8_t i = 0; i < size; i++) {
		base[n++] += *path++;
		if (n == sizeof(base))
			n = 0;
	}
	for (size_t i = 0; i < 4; i++) {
		auto c = base[i];
		dst[i * 2] = 'A' + (c & 0x0F);
		dst[i * 2 + 1] = 'A' + ((c & 0xF0) >> 4);
	}
	dst[8] = '.';
	auto c = base[4];
	dst[9] = 'a' + (c & 0x0F);
	dst[10] = 'a' + ((c & 0xF0) >> 4);
}

}