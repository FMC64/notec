#include "Stream.hpp"

enum class TokenType : char {
	NumberLiteral = 0,
	Identifier = 1,
	Operator = 2,
	StringLiteral = 3,
	ValueChar8 = 4
};

class Tokenizer
{
	static inline bool is_whitespace(char c)
	{
		return c <= 32;
	}

	static inline bool is_digit(char c)
	{
		return c >= '0' && c <= '9';
	}

	static inline bool is_alpha(char c)
	{
		return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
	}

	static inline bool is_num_lit(char c)
	{
		return is_digit(c) || c == '.' || is_alpha(c);
	}

	static inline bool is_identifier(char c)
	{
		return is_alpha(c) || (c == '_') || is_digit(c);
	}

	static inline constexpr char eob = 0x7F;

	inline TokenType tok_type(char c)
	{
		if (is_digit(c))
			return TokenType::NumberLiteral;
		else if (is_alpha(c) || c == '_')
			return TokenType::Identifier;
		else if (c == '\"')
			return TokenType::StringLiteral;
		else if (c == '\'')
			return TokenType::ValueChar8;
		else
			return TokenType::Operator;
	}

	inline void adv_wspace(void)
	{
		while (is_whitespace(*m_i))
			m_i++;
	}

	inline void adv_i(TokenType type)
	{
		if (type == TokenType::NumberLiteral) {
			while (is_num_lit(*m_i))
				m_i++;
		} else if (type == TokenType::Identifier) {
			while (is_identifier(*m_i))
				m_i++;
		}
	}

	inline size_t feed_buf(void)
	{
		auto r = m_stream.read(m_buf, buf_size);
		m_buf[r] = eob;
		return r;
	}

public:
	char *m_i;
	Stream &m_stream;
	static inline constexpr size_t max_token_size = 255;	// uint8_t bytes
	static inline constexpr size_t buf_size = max_token_size;
	char m_buf_raw[2 +	// token struct type + size
		max_token_size + buf_size +
		1];	// eob
	char *m_buf;

	inline Tokenizer(Stream &stream) :
		m_stream(stream),
		m_buf(m_buf_raw + 2 + max_token_size)
	{
		m_i = m_buf;
		*m_i = eob;
	}

	inline char* next(void)
	{
		adv_wspace();
		while (*m_i == eob) {
			if (!feed_buf())
				return nullptr;
			adv_wspace();
		}
		char *res = m_i;
		TokenType type = tok_type(*res);
		adv_i(type);
		if (*m_i == eob) {
			if (feed_buf()) {
				size_t size = m_i - res;
				char *nres = m_buf - size;
				for (size_t i = 0; i < size; i++)
					nres[i] = res[i];
				res = nres;
				m_i = m_buf;
				adv_i(type);
				if (*m_i == eob) {
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