#pragma once

#include "Token.hpp"
#include "Stream.hpp"

#include "arith.hpp"
#include "excp.hpp"

namespace Token {

namespace Char {
	static inline constexpr bool is_whitespace(char c)
	{
		return c <= 32;
	}

	static inline constexpr bool is_digit(char c)
	{
		return c >= '0' && c <= '9';
	}

	static inline constexpr bool is_alpha(char c)
	{
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
	}

	static inline constexpr bool is_num_lit(char c)
	{
		return is_digit(c) || c == '.' || is_alpha(c);
	}

	static inline constexpr bool is_num_lit_point(char c)
	{
		return c == '.';
	}

	static inline constexpr bool is_identifier_first(char c)
	{
		return is_alpha(c) || c == '_';
	}

	static inline constexpr bool is_identifier(char c)
	{
		return is_identifier_first(c) || is_digit(c);
	}

	static inline constexpr char eob = 0x7F;
	static inline constexpr char buf_overflow = 0x80;

	template <size_t Size>
	struct Table
	{
		char data[Size];
		static inline constexpr uint8_t csize = static_cast<uint8_t>(Size);

		template <typename T>
		constexpr auto& operator[](T i)
		{
			return data[i];
		}

		template <typename T>
		constexpr auto operator[](T i) const
		{
			return data[i];
		}
	};
	using STable = Table<0x80>;

	namespace Trait {
		static inline constexpr char is_num_lit_point = 1;
		static inline constexpr char is_digit = 4;
		static inline constexpr char is_alpha = 8;
		static inline constexpr char is_num_lit = 16;
		static inline constexpr char is_identifier_first = 32;
		static inline constexpr char is_identifier = 64;
	}

	static inline constexpr STable computeTraitTable(void)
	{
		STable res;
		for (uint8_t i = 0; i < res.csize; i++) {
			auto &c = res[i];
			c = 0;
			#define TOKEN_CHAR_TRAIT_TABLE_NEXT(name) if (name(static_cast<char>(i))) c |= Trait::name
			TOKEN_CHAR_TRAIT_TABLE_NEXT(is_num_lit_point);
			TOKEN_CHAR_TRAIT_TABLE_NEXT(is_digit);
			TOKEN_CHAR_TRAIT_TABLE_NEXT(is_alpha);
			TOKEN_CHAR_TRAIT_TABLE_NEXT(is_num_lit);
			TOKEN_CHAR_TRAIT_TABLE_NEXT(is_identifier_first);
			TOKEN_CHAR_TRAIT_TABLE_NEXT(is_identifier);
			#undef TOKEN_CHAR_TRAIT_TABLE_NEXT
		}
		return res;
	}
	static inline constexpr STable trait_table = computeTraitTable();

	static inline constexpr STable computeTypeTable(void)
	{
		constexpr auto type_for_char = [](char c) -> Token::Type {
			auto t = Char::trait_table[c];
			if (t & Char::Trait::is_digit)
				return Token::Type::NumberLiteral;
			else if (t & Char::Trait::is_num_lit_point)
				return static_cast<Type>(static_cast<char>(0x08) | static_cast<char>(Type::NumberLiteral));	// Special NumberLiteralPoint
			else if (t & Char::Trait::is_identifier_first)
				return Token::Type::Identifier;
			else if (c == '\"')
				return Token::Type::StringLiteral;
			else if (c == '\'')
				return Token::Type::ValueChar8;
			else
				return Token::Type::Operator;
		};
		STable res;
		for (uint8_t i = 0; i < res.csize; i++)
			res[i] = static_cast<char>(type_for_char(static_cast<char>(i)));
		return res;
	}
	static inline constexpr STable type_table = computeTypeTable();

	static inline constexpr uint8_t is_op_direct = 0x80;
	static inline constexpr uint8_t is_op_node = 0x40;
	static inline constexpr uint8_t op_direct_mask = 0x3F;
	static inline constexpr uint8_t op_node_mask = 0x3F;
	static inline constexpr STable computeOpTable(void)
	{
		STable res;
		for (uint8_t i = 0; i < res.csize; i++)
			res[i] = 0;

		// That's a nice cube right
		res['('] = static_cast<char>(is_op_direct | static_cast<uint8_t>(Op::LPar));
		res[')'] = static_cast<char>(is_op_direct | static_cast<uint8_t>(Op::RPar));
		res['['] = static_cast<char>(is_op_direct | static_cast<uint8_t>(Op::LArr));
		res[']'] = static_cast<char>(is_op_direct | static_cast<uint8_t>(Op::RArr));
		res['~'] = static_cast<char>(is_op_direct | static_cast<uint8_t>(Op::Tilde));
		res['?'] = static_cast<char>(is_op_direct | static_cast<uint8_t>(Op::Huh));
		res[';'] = static_cast<char>(is_op_direct | static_cast<uint8_t>(Op::Semicolon));
		res[','] = static_cast<char>(is_op_direct | static_cast<uint8_t>(Op::Comma));
		res['{'] = static_cast<char>(is_op_direct | static_cast<uint8_t>(Op::LBra));
		res['}'] = static_cast<char>(is_op_direct | static_cast<uint8_t>(Op::RBra));

		res['!'] = static_cast<char>(is_op_node | static_cast<uint8_t>(Op::Not));
		res['+'] = static_cast<char>(is_op_node | static_cast<uint8_t>(Op::Plus));
		res['-'] = static_cast<char>(is_op_node | static_cast<uint8_t>(Op::Minus));
		res['&'] = static_cast<char>(is_op_node | static_cast<uint8_t>(Op::BitAnd));
		res['|'] = static_cast<char>(is_op_node | static_cast<uint8_t>(Op::BitOr));
		res['^'] = static_cast<char>(is_op_node | static_cast<uint8_t>(Op::BitXor));
		res['*'] = static_cast<char>(is_op_node | static_cast<uint8_t>(Op::Mul));
		res['/'] = static_cast<char>(is_op_node | static_cast<uint8_t>(Op::Div));
		res['%'] = static_cast<char>(is_op_node | static_cast<uint8_t>(Op::Mod));
		res['<'] = static_cast<char>(is_op_node | static_cast<uint8_t>(Op::Less));
		res['>'] = static_cast<char>(is_op_node | static_cast<uint8_t>(Op::Greater));
		res['='] = static_cast<char>(is_op_node | static_cast<uint8_t>(Op::Equal));
		res['.'] = static_cast<char>(is_op_node | static_cast<uint8_t>(Op::Point));
		res[':'] = static_cast<char>(is_op_node | static_cast<uint8_t>(Op::Colon));
		res['#'] = static_cast<char>(is_op_node | static_cast<uint8_t>(Op::Sharp));

		return res;
	}
	static inline constexpr STable op_table = computeOpTable();

	static inline constexpr STable computeOpNodeTable(void)
	{
		STable res;
		for (uint8_t i = 0; i < res.csize; i++)
			res[i] = 12;

		res['='] = 0;
		res['+'] = 1;
		res['-'] = 2;
		res['>'] = 3;
		res['<'] = 4;
		res['&'] = 5;
		res['|'] = 6;
		res['*'] = 7;
		res['/'] = 8;
		res['.'] = 9;
		res[':'] = 10;
		res['#'] = 11;

		return res;
	}
	static inline constexpr STable op_node_table = computeOpNodeTable();

	static inline constexpr STable computeEscapeTable(void)
	{
		STable res;
		for (uint8_t i = 0; i < res.csize; i++)
			res[i] = 0;

		res['a'] = 0x07;
		res['b'] = 0x08;
		res['e'] = 0x1B;
		res['f'] = 0x0C;
		res['n'] = 0x0A;
		res['r'] = 0x0D;
		res['t'] = 0x09;
		res['v'] = 0x0B;
		res['\\'] = 0x5C;
		res['\''] = 0x27;
		res['\"'] = 0x22;
		res['\?'] = 0x3F;

		return res;
	}
	static inline constexpr STable escape_table = computeEscapeTable();
}

namespace OpCplx {
	struct Table
	{
		// 0xF0 mask next node addr
		// 0x08 flag has next node
		// 0x04 flag valid code
		// 0x01 mask res ndx for output code
		char ind[13];	// =+-><&|*/.:#
		char res[3];
	};

	static_assert(sizeof(Table) == 16, "Table size must be 16");

	struct GTable
	{
		Table nodes[20];
	};

	static inline constexpr uint8_t has_next_node = 0x08;
	static inline constexpr uint8_t has_valid_code = 0x04;
	static inline constexpr uint8_t next_node_mask = 0xF0;
	static inline constexpr uint8_t res_ndx_mask = 0x03;

	static inline constexpr uint8_t empty = 0xFF;
	static inline constexpr char gtb(uint8_t res_ndx, uint8_t node_ndx)
	{
		char res = 0;
		if (res_ndx != empty) {
			res |= has_valid_code;
			res |= res_ndx & res_ndx_mask;
		}
		if (node_ndx != empty) {
			res |= has_next_node;
			res |= (node_ndx << 4) & next_node_mask;
		}
		return res;
	}
	static inline constexpr GTable getTable(void)
	{
		return {{
			{	// [0] !
				{
					gtb(0, empty),	// [0] =
					0,	// [1] +
					0,	// [2] -
					0,	// [3] >
					0,	// [4] <
					0,	// [5] &
					0,	// [6] |
					0,	// [7] *
					0,	// [8] /
					0	// [9] .
				},
				{
					static_cast<char>(Op::NotEqual)	// !=
				}
			},
			{	// [1] +
				{
					gtb(0, empty),	// [0] =
					gtb(1, empty),	// [1] +
					0,	// [2] -
					0,	// [3] >
					0,	// [4] <
					0,	// [5] &
					0,	// [6] |
					0,	// [7] *
					0,	// [8] /
					0	// [9] .
				},
				{
					static_cast<char>(Op::PlusEqual),	// +=
					static_cast<char>(Op::PlusPlus)	// ++
				}
			},
			{	// [2] -
				{
					gtb(0, empty),	// [0] =
					0,	// [1] +
					gtb(1, empty),	// [2] -
					gtb(2, 7),	// [3] >
					0,	// [4] <
					0,	// [5] &
					0,	// [6] |
					0,	// [7] *
					0,	// [8] /
					0	// [9] .
				},
				{
					static_cast<char>(Op::MinusEqual),	// -=
					static_cast<char>(Op::MinusMinus),	// --
					static_cast<char>(Op::Arrow)	// ->
				}
			},
			{	// [3] &
				{
					gtb(0, empty),	// [0] =
					0,	// [1] +
					0,	// [2] -
					0,	// [3] >
					0,	// [4] <
					gtb(1, empty),	// [5] &
					0,	// [6] |
					0,	// [7] *
					0,	// [8] /
					0	// [9] .
				},
				{
					static_cast<char>(Op::BitAndEqual),	// &=
					static_cast<char>(Op::And)	// &&
				}
			},
			{	// [4] |
				{
					gtb(0, empty),	// [0] =
					0,	// [1] +
					0,	// [2] -
					0,	// [3] >
					0,	// [4] <
					0,	// [5] &
					gtb(1, empty),	// [6] |
					0,	// [7] *
					0,	// [8] /
					0	// [9] .
				},
				{
					static_cast<char>(Op::BitOrEqual),	// |=
					static_cast<char>(Op::Or)	// ||
				}
			},
			{	// [5] <=
				{
					0,	// [0] =
					0,	// [1] +
					0,	// [2] -
					gtb(0, empty),	// [3] >
					0,	// [4] <
					0,	// [5] &
					0,	// [6] |
					0,	// [7] *
					0,	// [8] /
					0	// [9] .
				},
				{
					static_cast<char>(Op::TWComp)	// <=>
				}
			},
			{	// [6] *
				{
					gtb(0, empty),	// [0] =
					0,	// [1] +
					0,	// [2] -
					0,	// [3] >
					0,	// [4] <
					0,	// [5] &
					0,	// [6] |
					0,	// [7] *
					0,	// [8] /
					0	// [9] .
				},
				{
					static_cast<char>(Op::MulEqual)	// *=
				}
			},
			{	// [7] ->
				{
					0,	// [0] =
					0,	// [1] +
					0,	// [2] -
					0,	// [3] >
					0,	// [4] <
					0,	// [5] &
					0,	// [6] |
					gtb(0, empty),	// [7] *
					0,	// [8] /
					0	// [9] .
				},
				{
					static_cast<char>(Op::ArrowMember)	// ->*
				}
			},
			{	// [8] :
				{
					0,	// [0] =
					0,	// [1] +
					0,	// [2] -
					0,	// [3] >
					0,	// [4] <
					0,	// [5] &
					0,	// [6] |
					0,	// [7] *
					0,	// [8] /
					0,	// [9] .
					gtb(0, empty)	// [A] :
				},
				{
					static_cast<char>(Op::Scope)	// ::
				}
			},
			{	// [9] <
				{
					gtb(0, 5),	// [0] =
					0,	// [1] +
					0,	// [2] -
					0,	// [3] >
					gtb(1, 14),	// [4] <
					0,	// [5] &
					0,	// [6] |
					0,	// [7] *
					0,	// [8] /
					0	// [9] .
				},
				{
					static_cast<char>(Op::LessEqual),	// <=
					static_cast<char>(Op::BitLeft)	// <<
				}
			},
			{	// [10] >
				{
					gtb(0, empty),	// [0] =
					0,	// [1] +
					0,	// [2] -
					0,	// [3] >
					gtb(1, 15),	// [4] >
					0,	// [5] &
					0,	// [6] |
					0,	// [7] *
					0,	// [8] /
					0	// [9] .
				},
				{
					static_cast<char>(Op::GreaterEqual),	// >=
					static_cast<char>(Op::Greater)	// >>
				}
			},
			{	// [11] =
				{
					gtb(0, empty),	// [0] =
					0,	// [1] +
					0,	// [2] -
					0,	// [3] >
					0,	// [4] >
					0,	// [5] &
					0,	// [6] |
					0,	// [7] *
					0,	// [8] /
					0	// [9] .
				},
				{
					static_cast<char>(Op::EqualEqual)	// ==
				}
			},
			{	// [12] .
				{
					0,	// [0] =
					0,	// [1] +
					0,	// [2] -
					0,	// [3] >
					0,	// [4] >
					0,	// [5] &
					0,	// [6] |
					gtb(0, empty),	// [7] *
					0,	// [8] /
					gtb(empty, 13)	// [9] .
				},
				{
					static_cast<char>(Op::PointMember)	// .*
				}
			},
			{	// [13] ..
				{
					0,	// [0] =
					0,	// [1] +
					0,	// [2] -
					0,	// [3] >
					0,	// [4] >
					0,	// [5] &
					0,	// [6] |
					0,	// [7] *
					0,	// [8] /
					gtb(0, empty)	// [9] .
				},
				{
					static_cast<char>(Op::Expand)	// ...
				}
			},
			{	// [14] <<
				{
					gtb(0, empty),	// [0] =
					0,	// [1] +
					0,	// [2] -
					0,	// [3] >
					0,	// [4] >
					0,	// [5] &
					0,	// [6] |
					0,	// [7] *
					0,	// [8] /
					0	// [9] .
				},
				{
					static_cast<char>(Op::BitLeftEqual)	// <<=
				}
			},
			{	// [15] >>
				{
					gtb(0, empty),	// [0] =
					0,	// [1] +
					0,	// [2] -
					0,	// [3] >
					0,	// [4] >
					0,	// [5] &
					0,	// [6] |
					0,	// [7] *
					0,	// [8] /
					0	// [9] .
				},
				{
					static_cast<char>(Op::BitRightEqual)	// >>=
				}
			},
			{	// [16] ^
				{
					gtb(0, empty),	// [0] =
					0,	// [1] +
					0,	// [2] -
					0,	// [3] >
					0,	// [4] <
					0,	// [5] &
					0,	// [6] |
					0,	// [7] *
					0,	// [8] /
					0	// [9] .
				},
				{
					static_cast<char>(Op::BitXorEqual)	// ^=
				}
			},
			{	// [17] %
				{
					gtb(0, empty),	// [0] =
					0,	// [1] +
					0,	// [2] -
					0,	// [3] >
					0,	// [4] <
					0,	// [5] &
					0,	// [6] |
					0,	// [7] *
					0,	// [8] /
					0	// [9] .
				},
				{
					static_cast<char>(Op::ModEqual)	// %=
				}
			},
			{	// [18] /
				{
					gtb(0, empty),	// [0] =
					0,	// [1] +
					0,	// [2] -
					0,	// [3] >
					0,	// [4] <
					0,	// [5] &
					0,	// [6] |
					gtb(1, empty),	// [7] *
					gtb(2, empty),	// [8] /
					0	// [9] .
				},
				{
					static_cast<char>(Op::DivEqual),	// /=
					static_cast<char>(Op::Comment),	// /*
					static_cast<char>(Op::SLComment)	// //
				}
			},
			{	// [19] #
				{
					0,	// [0] =
					0,	// [1] +
					0,	// [2] -
					0,	// [3] >
					0,	// [4] <
					0,	// [5] &
					0,	// [6] |
					0,	// [7] *
					0,	// [8] /
					0,	// [9] .
					0,	// [A] :
					gtb(0, empty)	// [B] #
				},
				{
					static_cast<char>(Op::DoubleSharp)	// ##
				}
			},
		}};
	}
	static inline constexpr GTable table = getTable();
}

class Stream
{
	inline Type tok_type(char c)
	{
		return static_cast<Type>(Char::type_table[c]);
	}

	inline void adv_wspace(void)
	{
		char last = 0;
		while (Char::is_whitespace(*m_i) || *m_i == '\\') {
			if (*m_i == '\n') {
				m_line_escaped = last == '\\';
				m_row++;
			}
			last = *m_i;
			m_i++;
		}
	}

	inline bool adv_number_literal(void)
	{
		while (Char::trait_table[*m_i] & Char::Trait::is_num_lit)
			m_i++;
		return false;
	}

	inline void carriage_buf(void)
	{
		size_t size = m_i - m_res;
		char *nres = m_buf - size;
		for (size_t i = 0; i < size; i++)
			nres[i] = m_res[i];
		m_res = nres;
		m_i = m_buf;
	}

	inline bool adv_number_literal_point(void)
	{
		char had = *m_i;
		m_i++;
		if (*m_i == Char::eob) {
			carriage_buf();
			feed_buf();
		}
		if (Char::trait_table[*m_i] & Char::Trait::is_digit)
			return adv_number_literal();
		else {
			m_i--;
			*m_i = had;
			return adv_operator();
		}
	}

	inline bool adv_identifier(void)
	{
		while (Char::trait_table[*m_i] & Char::Trait::is_identifier)
			m_i++;
		return false;
	}

	inline bool adv_comment(void)
	{
		char last_char = 0;
		while (true) {
			if (*m_i == Char::eob) {
				if (!feed_buf()) {
					m_res = nullptr;
					return true;
				}
				m_i = m_buf;
			}
			if (*m_i == '\n')
				m_row++;
			if (last_char == '*' && *m_i == '/') {
				m_i++;
				m_res = next();
				return true;
			}
			last_char = *m_i;
			m_i++;
		}
	}

	inline bool adv_sl_comment(void)
	{
		char last = 0;
		while (true) {
			if (*m_i == Char::eob) {
				if (!feed_buf()) {
					m_res = nullptr;
					return true;
				}
				m_i = m_buf;
			}
			if (*m_i == '\n') {
				m_row++;
				if (last != '\\') {
					m_i++;
					m_res = next();
					return true;
				}
			}
			last = *m_i;
			m_i++;
		}
	}

	bool adv_operator(void);

	inline bool require_neob(char * const filler)
	{
		if (*m_i == Char::eob) {
			if (filler >= m_buf) {
				m_error = "Max string size is 96";
				throw;
			}
			feed_buf();
			m_i = m_buf;
		}
		return true;
	}

	inline char escape(char * const filler)
	{
		if (!require_neob(filler))
			return 0;
		auto g = Char::escape_table[*m_i];
		if (g != 0) {
			m_i++;
			return g;
		}
		m_error = "Unknown escape sequence";
		throw;
	}

	template <size_t Deep>
	inline void fill_str(char delim, char *&filler)
	{
		while (*m_i != delim) {
			if (*m_i == Char::eob) {
				if constexpr (Deep > 0) {
					m_error = "Max string size is 96";
					throw;
				} else {
					if (!require_neob(filler))
						return;
					return fill_str<Deep + 1>(delim, filler);
				}
			}
			*filler = *m_i;
			m_i++;
			if (*filler == '\\')
				*filler = escape(filler);
			filler++;
		}
	}

	bool adv_str(char delim, Type type);

	inline bool adv_string_literal(void)
	{
		return adv_str('\"', Type::StringLiteral);
	}

	inline bool adv_string_sys_include(void)
	{
		return adv_str('>', Type::StringSysInclude);
	}

	inline bool adv_value_char_8(void)
	{
		adv_str('\'', Type::ValueChar8);
		if (m_res[1] != 1) {
			m_error = "Expected one character";
			throw;
		}
		m_res[1] = m_res[2];
		return true;
	}

	// g++, can you please support designated initializers??
	// clang++ does!
	using adv_t = bool (Stream::*)(void);
	static inline constexpr adv_t adv_types[] = {
		&Stream::adv_number_literal,	// 0
		&Stream::adv_identifier,	// 1
		&Stream::adv_string_literal,	// 2
		&Stream::adv_string_sys_include,	// 3
		&Stream::adv_operator,		// 4
		&Stream::adv_value_char_8,	// 5
		nullptr,	// 6
		nullptr,	// 7
		&Stream::adv_number_literal_point	// 8
	};

	inline bool adv_i(Type type)
	{
		return (this->*adv_types[static_cast<char>(type)])();
	}

	inline size_t feed_buf(void)
	{
		auto r = m_stream.read(m_buf, buf_size);
		m_buf[buf_size] = Char::buf_overflow;
		m_buf[r] = Char::eob;
		m_off += r;
		return r;
	}

	char *m_i;
	char *m_res;
	::Stream m_stream;
	static inline constexpr size_t max_token_size = 96;	// keep whole buffer below 256 bytes
	static inline constexpr size_t buf_size = max_token_size;
	char m_buf_raw[2 +	// token struct type + size
		max_token_size + buf_size +
		1];	// eob
	char *m_buf;

	const char *m_error = nullptr;
public:
	unwind_capable;
private:
	size_t m_off;	// bytes read on the file so far, any error will be happening before that offset
	size_t m_row;
	bool m_line_escaped;
	static inline constexpr size_t stack_size = 256;
	char m_stack_base[stack_size];
	char *m_stack;

	inline bool skip_blank(void)
	{
		adv_wspace();
		while (*m_i == Char::eob) {
			if (!feed_buf())
				return false;
			m_i = m_buf;
			adv_wspace();
		}
		return true;
	}

	inline char* gather_type(Type type)
	{
		if (adv_i(type)) {
			return m_res;
		} else {
			if (*m_i == Char::eob) {
				carriage_buf();
				feed_buf();
				type = static_cast<Type>(static_cast<char>(type) & type_range);
				adv_i(type);	// type & range is continuous
				if (*m_i == Char::eob && !m_stream.eof()) {
					m_error = "Max token size is 96";
					throw;
				}
			}
			m_res[-1] = static_cast<uint8_t>(m_i - m_res);
			m_res[-2] = static_cast<char>(type) & 0x07;
			return m_res - 2;
		}
	}

public:
	inline Stream(void)
	{
		m_buf = m_buf_raw + 2 + max_token_size;
		m_stack = m_stack_base;
	}

	char* next(void);
	char* next_include(void);

	inline ::Stream& get_stream(void)
	{
		return m_stream;
	}

	inline const char* get_error(void) const
	{
		return m_error;
	}

	inline size_t get_off(void) const	// current byte offset in file
	{
		if (m_buf[buf_size] == Char::buf_overflow) {	// buffer at end of file, expensive
			auto end = m_i;
			while (*end != Char::eob)
				end++;
			return m_off - (end - m_i);
		} else {
			return m_off - buf_size + (m_i - m_buf);	// file fills whole buffer, trivial
		}
	}

	inline size_t get_row(void) const
	{
		return m_row;
	}

	inline bool get_line_escaped(void) const
	{
		return m_line_escaped;
	}

	inline void reset_buf(void)
	{
		m_i = m_buf;
		*m_i = Char::eob;
		m_buf[buf_size] = Char::buf_overflow;
		m_line_escaped = false;
	}

public:
	inline void error(const char *str)
	{
		m_error = str;
		throw;
	}

	inline void push(const char *filepath)
	{
		m_stream.close();
		if (m_stack > m_stack_base) {	// if stack isn't empty
			if (m_stack + 5 > m_stack_base + stack_size) {
				m_error = "File stack overflow";
				throw;
			}
			m_stack += store_part<3>(m_stack, get_off());
			m_stack += store_part<2>(m_stack, m_row);
		}
		auto base = m_stack;
		if (!m_stream.open(filepath, m_stack, m_stack_base + stack_size)) {
			m_error = base;
			throw;
		}
		{
			if (m_stack + 2 > m_stack_base + stack_size) {
				m_error = "File stack overflow";
				throw;
			}
			*m_stack++ = static_cast<uint8_t>(m_stack - base + 2);
			*m_stack++ = 0;
		}

		reset_buf();
		m_off = 0;
		m_row = 1;
	}

	inline const char* get_file_path(void) const
	{
		return m_stack - static_cast<uint8_t>(m_stack[-2]);
	}

	inline void set_file_alias(const char *token)
	{
		m_stack = const_cast<char*>(get_file_sign());
		auto fs = Token::whole_size(m_stack);
		m_stack += fs;
		*m_stack++ = static_cast<uint8_t>(fs + 1);
		auto s = Token::whole_size(token);
		for (uint8_t i = 0; i < s; i++)
			*m_stack++ = token[i];
		*m_stack++ = static_cast<uint8_t>(s + 2);
		*m_stack++ = 1;
	}

	inline const char* get_file_sign(void) const
	{
		if (m_stack[-1]) {
			auto add_off = static_cast<uint8_t>(m_stack[-2]);
			return m_stack - add_off - static_cast<uint8_t>(m_stack[-add_off - 1]);
		} else
			return m_stack - static_cast<uint8_t>(m_stack[-2]);
	}

	inline void set_line(size_t ll_l)
	{
		m_row = ll_l;
	}

	inline bool pop(void)
	{
		m_stream.close();
		m_stack = const_cast<char*>(get_file_sign());
		if (m_stack == m_stack_base)
			return false;

		reset_buf();
		m_stack -= 2;
		m_row = load_part<2, size_t>(m_stack);
		m_stack -= 3;
		m_off = load_part<3, size_t>(m_stack);
		auto filepath = get_file_sign();
		if (!m_stream.open(filepath, m_stack, nullptr)) {
			m_error = filepath;
			throw;
		}
		m_stream.seek(m_off);
		return true;
	}

	inline const char* get_stack(void)	// only to check if stack has been modified or not
	{
		return m_stack;
	}
};

}