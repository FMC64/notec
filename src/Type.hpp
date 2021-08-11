#pragma once

namespace Type {

enum class Prim : char {
	Void,
	Auto,
	FP32,
	FP64,
	S8,
	U8,
	S16,
	U16,
	S32,
	U32,
	S64,
	U64,

	Struct,	// followed by 3 bytes of index to struct def
	Function,	// return type then 1 byte arg count, then arg count types

	Ptr,
	Array,

	// from now on, quite high level stuff that will be implemented later™
	Lref,
	Rref,

	Param,	// followed by 1 byte of index to invocation table
	Invoke	// 3 bytes = template index, then 1 byte = arg count, then arg count types
	
};

static inline constexpr char const_flag = 0x40;
static inline constexpr char volatile_flag = 0x80;
static inline constexpr char prim_mask = 0xFF & ~(const_flag | volatile_flag);

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