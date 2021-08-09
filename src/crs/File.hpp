#pragma once

#include <cstdio>
#include "cint.hpp"

namespace File {

static inline bool is_sep(char c)
{
	return c == '/' || c == ':';
}

// remove high-level . and .. from path, along with redundent /
// writes a path guarranted to start with / and ending with the last character of designated file/folder
// returns path relative to search, if path first character is not /
static inline bool path_absolute(const char *search, const char *path, char *out, const char *out_top)
{
	auto s = out++;
	auto base = out;
	auto o = out;
	{
		auto ps = static_cast<uint8_t>(*path++);
		if (ps == 0 || *path != '/') {
			if (search == nullptr)
				return false;
			auto ss = static_cast<uint8_t>(*search++);
			if (o + ss + 1 > out_top)
				return false;
			for (uint8_t i  = 0; i < ss; i++)
				*o++ = *search++;
			*o++ = '/';
		}
		if (o + ps > out_top)
			return false;
		for (uint8_t i  = 0; i < ps; i++)
			*o++ = *path++;
	}
	{
		auto i = base;
		auto w = base;
		char last = '/';
		auto seek_back = [&](){
			if (w > base)
				w--;
			while (w > base && !is_sep(w[-1]))
				w--;
			if (w > base)
				w--;
			last = w > base ? w[-1] : '/';
		};
		while (i < o) {
			auto c = *i++;
			if (is_sep(c)) {
				while (i < o && *i == c)
					i++;
				if (i < o)
					if (is_sep(*i))	// can't mix separators
						return false;
			} else if (c == '.' && is_sep(last)) {
				if (i == o) {
					if (w > base)
						w--;
					continue;
				} else {
					if (is_sep(*i)) {
						auto a = *i++;
						while (i < o && *i == a)
							i++;
						continue;
					} else if (*i == '.') {
						i++;
						if (i == o) {
							seek_back();
							continue;
						} else if (is_sep(*i)) {
							seek_back();
							continue;
						}
					}
				}
			}
			last = c;
			*w++ = c;
		}
		if (w > base) {
			auto c = w[-1];
			if (c == ':')
				return false;
			if (c == '/')
				w--;
		}
		{
			size_t arc_count = 0;
			for (auto i = base; i < w; i++)
				if (*i == ':')
					arc_count++;
			if (arc_count > 1)
				return false;
		}
		*s = static_cast<uint8_t>(w - base);
	}
	return true;
}

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
		auto tx = c ^ ent;
		if (tx == 0)
			tx = c;
		base[n++] ^= tx;
		ent ^= c;
		auto car = (ent & 0x01) << 7;
		ent >>= 1;
		ent ^= car;
		ent ^= base[n - 1];
		if (n == sizeof(base))
			n = 0;
	}
	{
		auto s = (base[5] & 0xF0) >> 4;
		for (size_t i = 0; i < 5; i++)
			base[i] ^= s;
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