#pragma once

#include "Pp.hpp"

class Cpp
{
	Pp m_pp;

	inline void error(const char *str)
	{
		m_pp.error(str);
	}

	inline const char* next(void)
	{
		return m_pp.next();
	}

	inline const char* next_exp(void)
	{
		auto res = m_pp.next();
		if (res == nullptr)
			error("Expected token");
		return res;
	}

	size_t m_size = 0;
	size_t m_allocated = 0;
	char *m_buffer = nullptr;

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

	bool m_is_c_linkage = false;

	// if is_c is false, c++
	inline void extern_linkage(bool is_c)
	{
		auto last = m_is_c_linkage;
		m_is_c_linkage = is_c;
		auto n = next_exp();
		if (Token::is_op(n, Token::Op::LBra)) {
			n = next_exp();
			if (!Token::is_op(n, Token::Op::RBra)) {
				parse_obj(n);
				n = next_exp();
				if (!Token::is_op(n, Token::Op::RBra))
					error("Expected '}'");
			}
		} else
			parse_obj(n);
		m_is_c_linkage = last;
	}

	inline void parse_obj(const char *n)
	{
		if (Token::is_op(n, Token::Op::Extern)) {
			n = next_exp();
			if (Token::type(n) == Token::Type::StringLiteral) {
				if (streq(n + 1, make_cstr("C++")))
					return extern_linkage(false);
				if (streq(n + 1, make_cstr("C")))
					return extern_linkage(true);
				error("Unknown language linkage");
			}
		}
	}

public:
	inline auto& err_src(void)
	{
		return m_pp.get_tstream();
	}

	inline void include_dir(const char *path)
	{
		m_pp.include_dir(path);
	}

	inline void open(const char *path)
	{
		m_pp.open(path);
	}

	inline void run(void)
	{
		const char *n;
		while ((n = next()))
			parse_obj(n);
	}
};