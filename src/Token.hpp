#pragma once

#include "cint.hpp"
#include "arith.hpp"

namespace Token {

// order-dependent constants:
//	- Token::Stream::adv_types
//	- Pp::TokType
enum class Type : char {
	NumberLiteral = 0,
	Identifier = 1,
	StringLiteral = 2,
	StringSysInclude = 3,
	Operator = 4,
	ValueChar8 = 5
};
static inline constexpr char type_range = 0x07;
static inline constexpr char type_first_constant = static_cast<char>(Type::Operator);

static inline Type type(const char *token)
{
	return static_cast<Type>(token[0]);
}

static inline uint8_t size(const char *token)
{
	return static_cast<uint8_t>(token[1]);
}

static inline uint8_t whole_size(const char *token)
{
	if (token[0] < type_first_constant)
		return 2 + size(token);
	else
		return 2;
}

static inline const char* data(const char *token)
{
	return &token[2];
}

enum class Op : uint8_t {
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

	Or,	// |
	BitOrEqual,	// |=

	BitXorEqual,	// ^=

	MulEqual,	// *=

	DivEqual,	// /=
	Comment,	// /*
	SLComment,	// //

	ModEqual,	// %=

	LessEqual,	// <=
	BitLeft,	// <<
	BitLeftEqual,	// <<=

	GreaterEqual,	// >=
	BitRight,	// >>
	BitRightEqual,	// >>=

	EqualEqual,	// ==
	Expand,	// ...

	LPar,	// (
	RPar,	// )
	LArr,	// [
	RArr,	// ]
	Tilde,	// ~
	Comma,	// ,
	Huh,	// ?
	DoubleSharp,	// ##
	Semicolon,	// ;
	LBra,	// {
	RBra,	// }

	TWComp,	// <=>
	Scope,	// ::

	MinusMinus,	// --
	MinusEqual,	// -=

	PointMember,	// .*
	ArrowMember,	// ->*

	Arrow,	// ->
	And,	// &


	// keywords
	Alignas,
	Alignof,
	Asm,
	Auto,
	Bool,
	Break,
	Case,
	Catch,
	Char,
	Char8_t,
	Char16_t,
	Char32_t,
	Class,
	Compl,
	Concept,
	Const,
	Consteval,
	Constexpr,
	Constinit,
	ConstCast,
	Continue,
	CoAwait,
	CoReturn,
	CoYield,

	Decltype,
	Default,
	Delete,
	Do,
	Double,
	DynamicCast,
	Else,
	Enum,
	Explicit,
	Export,
	Extern,
	False,
	Float,
	For,
	Friend,
	Goto,
	If,
	Inline,
	Int,
	Long,
	Mutable,
	Namespace,
	New,
	Noexcept,
	Nullptr,
	Operator,
	Private,
	Protected,
	Public,

	Register,
	ReinterpretCast,
	Requires,
	Return,
	Short,
	Signed,
	Sizeof,
	Static,
	StaticAssert,
	StaticCast,
	Struct,
	Switch,
	Template,
	This,
	ThreadLocal,
	Throw,
	True,
	Try,
	Typedef,
	Typeid,
	Typename,
	Union,
	Unsigned,
	Using,
	Virtual,
	Void,
	Volatile,
	Wchar_t,
	While
};
static inline constexpr auto last_op = Op::And;

static inline Op op(const char *token)
{
	return static_cast<Op>(token[1]);
}

static inline bool is_op(const char *token, Op op)
{
	return type(token) == Type::Operator && Token::op(token) == op;
}

}

#define token_nter(id, src) char id[Token::size(src) + 1];	\
	{							\
		auto size = Token::size(src);			\
		id[size] = 0;					\
		auto s = Token::data(src);			\
		auto d = id;					\
		for (uint8_t i = 0; i < size; i++)		\
			*d++ = *s++;				\
	}

#define token_copy(id, src) char id[Token::whole_size(src)];	\
	{							\
		for (uint8_t i = 0; i < sizeof(id); i++)	\
			id[i] = src[i];				\
	}

// [0] size, [1...size] data
static inline bool streq(const char *a, const char *b)
{
	auto size = static_cast<uint8_t>(a[0]);
	if (size != static_cast<uint8_t>(b[0]))
		return false;
	a++;
	b++;
	for (uint8_t i = 0; i < size; i++)
		if (a[i] != b[i])
			return false;
	return true;
}

template <size_t Size>
static inline constexpr auto make_cstr(const char (&str)[Size])
{
	auto res = carray<char, Size>();
	res.data[0] = static_cast<uint8_t>(Size - 1);
	for (size_t i = 0; i < (Size - 1); i++)
		res.data[1 + i] = str[i];
	return res;
}

template <size_t Size>
static inline constexpr auto make_tstr(Token::Type type, const char (&str)[Size])
{
	auto res = carray<char, Size + 1>();
	res.data[0] = static_cast<char>(type);
	res.data[1] = static_cast<uint8_t>(Size - 1);
	for (size_t i = 0; i < (Size - 1); i++)
		res.data[2 + i] = str[i];
	return res;
}