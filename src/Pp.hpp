#pragma once

#include "TokenStream.hpp"
#include "StrMap.hpp"

class Pp
{
	Token::Stream m_stream;
	StrMap::BlockGroup m_dirs;

public:
	inline Pp(void)
	{
		#define PP_DIRECTIVE_NEXT(id) m_dirs.insert(#id, &Pp::id)
		PP_DIRECTIVE_NEXT(include);
		PP_DIRECTIVE_NEXT(define);
		#undef PP_DIRECTIVE_NEXT
	}

	inline Token::Stream& get_tstream(void)
	{
		return m_stream;
	}

	inline Stream& get_stream(void)	// call on error and for custom stream modifications before polling (testing only)
	{
		return m_stream.get_stream();
	}

	inline void open(const char *filename)	// to call once before polling
	{
		m_stream.push(filename);
	}

private:
	inline void assert_token(const char *token)
	{
		if (token == nullptr)
			m_stream.error("Expected token");
	}

	inline void assert_token_type(const char *token, Token::Type type)
	{
		assert_token(token);
		if (Token::type(token) != type)
			m_stream.error("Bad token type");
	}

	inline char* next_token(bool is_include = false)
	{
		while (true) {
			char *res;
			if (is_include)
				res = m_stream.next_include();
			else
				res = m_stream.next();
			if (res != nullptr)
				return res;
			if (!m_stream.pop())
				return nullptr;
		}
	}

	inline bool next_token_dir(char* &res, bool is_include = false)
	{
		auto lrow = m_stream.get_row();
		auto lstack = m_stream.get_stack();
		res = next_token(is_include);
		if (lrow != m_stream.get_row())
			if (!m_stream.get_line_escaped() || lstack != m_stream.get_stack())
				return false;
		return true;
	}

	inline char* include(void)
	{
		char *n;
		if (!next_token_dir(n, true))
			m_stream.error("Expected string");
		assert_token(n);
		auto t = Token::type(n);
		if (t != Token::Type::StringLiteral && t != Token::Type::StringSysInclude)
			m_stream.error("Expected string");
		m_stream.push(n);
		return next();
	}

	inline char* define(void)
	{
		return next();
	}

	inline char* directive(void)
	{
		auto n = next_token();
		assert_token_type(n, Token::Type::Identifier);
		token_nter(nn, n);
		char* (Pp::*dir)(void);
		if (!m_dirs.resolve(nn, dir))
			m_stream.error("Unknown directive");
		return (this->*dir)();
	}

public:
	char* next(void);
};