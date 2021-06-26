#include "Stream.hpp"

namespace Token {

enum class Type : char {
	NumberLiteral = 0,
	Identifier = 1,
	Operator = 2,
	StringLiteral = 3,
	ValueChar8 = 4
};

namespace Char {
	static inline constexpr bool is_whitespace(char c)
	{
		return c <= 32;
	}

	static inline constexpr bool is_digit(char c)
	{
		return c >= '0' && c <= '9';
	}

	static inline constexpr bool is_digit_first(char c)
	{
		return is_digit(c) || c == '.';
	}

	static inline constexpr bool is_alpha(char c)
	{
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
	}

	static inline constexpr bool is_num_lit(char c)
	{
		return is_digit_first(c) || is_alpha(c);
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
		static inline constexpr char is_whitespace = 1;
		static inline constexpr char is_digit = 2;
		static inline constexpr char is_digit_first = 4;
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
			TOKEN_CHAR_TRAIT_TABLE_NEXT(is_whitespace);
			TOKEN_CHAR_TRAIT_TABLE_NEXT(is_digit);
			TOKEN_CHAR_TRAIT_TABLE_NEXT(is_digit_first);
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
			if (t & Char::Trait::is_digit_first)
				return Token::Type::NumberLiteral;
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
}

class Stream
{
	inline Type tok_type(char c)
	{
		return static_cast<Type>(Char::type_table[c]);
	}

	inline void adv_wspace(void)
	{
		while (Char::is_whitespace(*m_i))
			m_i++;
	}

	inline void adv_number_literal(void)
	{
		while (Char::trait_table[*m_i] & Char::Trait::is_num_lit)
			m_i++;
	}

	inline void adv_identifier(void)
	{
		while (Char::trait_table[*m_i] & Char::Trait::is_identifier)
			m_i++;
	}

	using adv_t = void (Stream::*)(void);
	static inline constexpr adv_t adv_types[] = {
		&Stream::adv_number_literal,
		&Stream::adv_identifier
	};

	inline void adv_i(Type type)
	{
		(this->*adv_types[static_cast<char>(type)])();
	}

	inline size_t feed_buf(void)
	{
		auto r = m_stream.read(m_buf, buf_size);
		m_buf[r] = Char::eob;
		return r;
	}

public:
	char *m_i;
	::Stream &m_stream;
	static inline constexpr size_t max_token_size = 255;	// uint8_t bytes
	static inline constexpr size_t buf_size = max_token_size;
	char m_buf_raw[2 +	// token struct type + size
		max_token_size + buf_size +
		1];	// eob
	char *m_buf;

	inline Stream(::Stream &stream) :
		m_stream(stream),
		m_buf(m_buf_raw + 2 + max_token_size)
	{
		m_i = m_buf;
		*m_i = Char::eob;
	}

	inline char* next(void)
	{
		adv_wspace();
		while (*m_i == Char::eob) {
			if (!feed_buf())
				return nullptr;
			adv_wspace();
		}
		char *res = m_i;
		auto type = tok_type(*res);
		adv_i(type);
		if (*m_i == Char::eob) {
			if (feed_buf()) {
				size_t size = m_i - res;
				char *nres = m_buf - size;
				for (size_t i = 0; i < size; i++)
					nres[i] = res[i];
				res = nres;
				m_i = m_buf;
				adv_i(type);
				if (*m_i == Char::eob) {
					while (true);
					//throw "Max token size is 255";
				}
			}
		}
		res[-1] = m_i - res;
		res[-2] = static_cast<char>(type);
		return res - 2;
	}
};

}