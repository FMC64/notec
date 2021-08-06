#pragma once

#include "cint.hpp"

namespace File {

// 8 base chars + '.' + 3 ext
static inline constexpr size_t hashed_len = 12;
using hashed = char[hashed_len];

// path is cstr
static inline void hash_path(const char *path, hashed dst)
{
	char base[6] = {};	// we actually hash encode on 6 bytes
	uint8_t size = path[0];
	path++;
	uint8_t n = 0;
	uint8_t ent = 0;
	for (uint8_t i = 0; i < size; i++) {
		auto c = *path++;
		base[n++] += c ^ ent;
		ent ^= c;
		auto car = (ent & 0x01) << 7;
		ent >>= 1;
		ent ^= car;
		if (n == sizeof(base))
			n = 0;
	}
	{
		auto s = (base[5] & 0xF0) >> 4;
		for (size_t i = 0; i < 4; i++)
			base[i] += (s & (1 << i)) >> i;
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
	c = base[5];
	dst[11] = 'a' + (c & 0x0F);
}

}