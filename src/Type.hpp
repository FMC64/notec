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
	Enum,	// followed by 3 bytes of index to enum def
	Function,	// return type then 1 byte arg count, then arg count types

	Ptr,
	Array,

	// from now on, quite high level stuff that will be implemented later™
	Lref,
	Rref,

	// edit Cpp::skip_type when finally used
	//Param,	// followed by 1 byte of index to invocation table
	//Invoke	// 3 bytes = template index, then 1 byte = arg count, then arg count types
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

enum class Visib : char {
	Private,
	Protected,
	Public
};

enum class Storage : char {
	Default,
	Static,
	Extern
};

static inline constexpr uint8_t inline_flag = 0x04;
static inline constexpr uint8_t constexpr_flag = 0x08;
static inline constexpr uint8_t consteval_flag = 0x10;
static inline constexpr uint8_t constinit_flag = 0x20;
static inline constexpr uint8_t thread_local_flag = 0x40;
static inline constexpr uint8_t mutable_flag = 0x80;
static inline constexpr uint8_t storage_mask = 0x03;

static inline constexpr Storage storage(char type)
{
	return static_cast<Storage>(static_cast<uint8_t>(type & storage_mask));
}

}