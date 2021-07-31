#pragma once

#include "cint.hpp"

template <typename T>
T min(const T &a, const T &b)
{
	return a < b ? a : b;
}

template <typename T, size_t Size>
constexpr size_t array_size(T (&)[Size])
{
	return Size;
}

template <typename T>
size_t store(char *dst, const T &src)
{
	auto csrc = reinterpret_cast<const char*>(&src);
	for (size_t i = 0; i < sizeof(T); i++)
		*dst++ = *csrc++;
	return sizeof(T);
}

template <typename T>
T load(const char *src)
{
	T res;
	auto cres = reinterpret_cast<char*>(&res);
	for (size_t i = 0; i < sizeof(T); i++)
		*cres++ = *src++;
	return res;
}

template <size_t Size, typename T>
size_t store_part(char *dst, const T &src)
{
	static_assert(Size < sizeof(T), "Size must be less than sizeof(T)");
	auto csrc = reinterpret_cast<const char*>(&src);
#ifndef CINT_HOST	// big-endian
	csrc += sizeof(T) - Size;
#endif
	for (size_t i = 0; i < Size; i++)
		*dst++ = *csrc++;
	return Size;
}

template <size_t Size, typename T>
T load_part(const char *src)
{
	static_assert(Size < sizeof(T), "Size must be less than sizeof(T)");
	T res = 0;
	auto cres = reinterpret_cast<char*>(&res);
#ifndef CINT_HOST	// big-endian
	cres += sizeof(T) - Size;
#endif
	for (size_t i = 0; i < Size; i++)
		*cres++ = *src++;
	return res;
}

template <typename T, size_t Size>
struct carray
{
	static constexpr auto size(void) { return Size; }
	T data[Size];
};