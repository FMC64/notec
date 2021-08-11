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

	// nested_id should be at least 256 bytes long (maximum token size)
	// if nested_id is nullptr, means nested identified will be discarded
	// nested_id will be filled with a null-terminated string, of size 0 if not found
	inline const char* parse_type(const char *n, char *nested_id)
	{
		return n;
	}

	// id should be at least 256 bytes long (maximum token size)
	inline const char* parse_type_id(const char *n, char *id)
	{
		n = parse_type(n, id);
		if (*id == 0) {
			if (Token::type(n) != Token::Type::Identifier)
				error("Expected identifier");
			Token::fill_nter(id, n);
			n = next_exp();
		}
		return n;
	}

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
		if (Token::type(n) == Token::Type::Operator) {
			auto o = Token::op(n);
			if (o == Token::Op::Extern) {
				n = next_exp();
				if (Token::type(n) == Token::Type::StringLiteral) {
					if (streq(n + 1, make_cstr("C++")))
						return extern_linkage(false);
					if (streq(n + 1, make_cstr("C")))
						return extern_linkage(true);
					error("Unknown language linkage");
				}
			} else if (o == Token::Op::Typedef) {
				n = next_exp();
				char id[256];
				n = parse_type_id(n, id);
				if (!Token::is_op(n, Token::Op::Semicolon))
					error("Expected ';'");
			}
		}
	}

public:
	~Cpp(void)
	{
		free(m_buffer);
	}

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