#pragma once

#include "cint.hpp"

namespace Token {

enum class Type : char {
	NumberLiteral = 0,
	Identifier = 1,
	Operator = 2,
	StringLiteral = 3,
	ValueChar8 = 4,
	StringSysInclude = 5
};
static inline constexpr char type_range = 0x07;

static inline Type type(const char *token)
{
	return static_cast<Type>(token[0]);
}

static inline uint8_t size(const char *token)
{
	return static_cast<uint8_t>(token[1]);
}

static inline const char* data(const char *token)
{
	return &token[2];
}

enum class Op : char {
	Not = 0,	// !
	Plus = 1,	// +
	Minus = 2,	// -
	BitAnd = 3,	// &
	BitOr = 4,	// |
	Mul = 6,	// *
	Colon = 8,	// :
	Less = 9,	// <
	Greater = 10,	// >
	Equal = 11,	// =
	Point = 12,	// .
	BitXor = 16,	// ^
	Mod = 17,	// %
	Div = 18,	// /
	Sharp = 19,	// #

	NotEqual = 13,	// !=

	PlusPlus = 14,	// ++
	PlusEqual = 15,	// +=

	BitAndEqual = 20,	// &=

	Or = 21,	// |
	BitOrEqual = 22,	// |=

	BitXorEqual = 23,	// ^=

	MulEqual = 24,	// *=

	DivEqual = 25,	// /=
	Comment = 26,	// /*
	SLComment = 27,	// //

	ModEqual = 28,	// %=

	LessEqual = 29,	// <=
	BitLeft = 30,	// <<
	BitLeftEqual = 31,	// <<=

	GreaterEqual = 32,	// >=
	BitRight = 33,	// >>
	BitRightEqual = 34,	// >>=

	EqualEqual = 35,	// ==
	Expand = 36,	// ...

	LPar = 37,	// (
	RPar = 38,	// )
	LArr = 39,	// [
	RArr = 40,	// ]
	Tilde = 41,	// ~
	Comma = 42,	// ,
	Huh = 43,	// ?
	Semicolon = 45,	// ;
	LBra = 46,	// {
	RBra = 47,	// }

	TWComp = 48,	// <=>
	Scope = 49,	// ::

	MinusMinus = 50,	// --
	MinusEqual = 51,	// -=

	PointMember = 52,	// .*
	ArrowMember = 53,	// ->*

	Arrow = 54,	// ->
	And = 55,	// &
	DoubleSharp = 56	// ##
};

}