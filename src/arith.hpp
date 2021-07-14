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