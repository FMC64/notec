#pragma once

#include "clib.hpp"

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
		return next_token();
	}

	StrMap::BlockGroup m_macros;
	char *m_buffer = nullptr;
	size_t m_size = 0;
	size_t m_allocated = 0;

	inline void alloc(size_t size)
	{
		auto needed = m_size + size;
		if (m_allocated < needed) {
			m_allocated *= 2;
			if (m_allocated < needed)
				m_allocated = needed;
			m_buffer = reinterpret_cast<char*>(realloc(m_buffer, m_allocated));
		}
	}

	struct TokType {	// exts
		static inline constexpr char end = 6;
	};

	static inline constexpr size_t define_arg_size = 256;

	inline char* define(void)
	{
		char *n;
		if (!next_token_dir(n))
			m_stream.error("Expected token");
		assert_token_type(n, Token::Type::Identifier);
		token_nter(nn, n);
		size_t base_size = m_size;
		bool suc_ins = m_macros.insert(nn, static_cast<uint16_t>(m_size));
		auto name_off = m_stream.get_off();
		alloc(1);
		if (next_token_dir(n) && n != nullptr) {	// arguments or tokens
			char args[define_arg_size];
			char *arg_top = args;
			size_t arg_count = 0;
			bool has_va = false;
			if (m_stream.get_off() == name_off + 1 && Token::is_op(n, Token::Op::LPar)) {	// has args first
			}
			m_buffer[m_size++] = arg_count | (has_va ? 0x80 : 0);
			while (next_token_dir(n) && n != nullptr) {
				auto size = Token::whole_size(n);
				alloc(size);
				for (size_t i = 0; i < size; i++)
					m_buffer[m_size++] = n[i];
			}
		} else {	// zero token macro
			m_buffer[m_size++] = 0;	// zero args
			n = next_token();
		}
		alloc(1);
		m_buffer[m_size++] = TokType::end;
		if (!suc_ins) {	// assert redefinition is same
			auto size = m_size - base_size;
			auto n = &m_buffer[m_size - size];
			uint16_t e_ndx;
			m_macros.resolve(nn, e_ndx);
			auto e = &m_buffer[e_ndx];
			for (size_t i = 0; i < size; i++)
				if (e[i] != n[i])
					m_stream.error("Redefinition do not match");
			m_size -= size;	// don't keep same buffer
		}
		return n;
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