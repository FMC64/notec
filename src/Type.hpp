#pragma once

namespace Type {

enum class Prim : char {
	Void = 0,
	U8 = 1,
	S8 = 2,
	U16 = 3,
	S16 = 4,
	U32 = 5,
	S32 = 6,
	U64 = 7,
	S64 = 8,
	FP32 = 9,
	FP64 = 10,

	Struct = 11,	// followed by 3 bytes of index to struct def
	Function = 12,

	Ptr = 13,
	Array = 14,

	// from now on, quite high level stuff that will be implemented later™
	Lref = 15,
	Rref = 16,

	Auto = 17,
	Param = 18,	// followed by 1 byte of index to invocation table
	Invoke = 19,	// 3 bytes = template index, then 1 byte = arg count, then arg count types
	
};

static inline constexpr char prim_mask = 0x3F;
static inline constexpr char volatile_flag = 0xC0;
static inline constexpr char const_flag = 0x80;

static inline constexpr Prim prim(char type)
{
	return static_cast<Prim>(static_cast<char>(type & prim_mask));
}

static inline constexpr bool is_volatile(char type)
{
	return type & volatile_flag;
}

static inline constexpr bool is_const(char type)
{
	return type & const_flag;
}

}