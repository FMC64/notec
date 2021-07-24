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
		if (Token::type(token) != type)
			m_stream.error("Bad token type");
	}

	template <bool IsInclude = false>
	inline char* _next_token(void)
	{
		while (true) {
			char *res;
			if constexpr (IsInclude)
				res = m_stream.next_include();
			else
				res = m_stream.next();
			if (res != nullptr)
				return res;
			if (!m_stream.pop())
				return nullptr;
		}
	}

	char* next_token(void);
	char* next_token_include(void);

	template <bool IsInclude = false>
	inline bool _next_token_dir(char* &res)
	{
		auto lrow = m_stream.get_row();
		auto lstack = m_stream.get_stack();
		res = _next_token<IsInclude>();
		if (lrow != m_stream.get_row())
			if (!m_stream.get_line_escaped() || lstack != m_stream.get_stack())
				return false;
		return res != nullptr;
	}

	bool next_token_dir(char* &res);
	bool next_token_dir_include(char* &res);

	inline char* include(void)
	{
		char *n;
		if (!next_token_dir_include(n))
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
		static inline constexpr char arg = 6;	// data: arg ndx, __VA_ARGS__ if 0x80
		static inline constexpr char opt = 7;	// data: arg count
		static inline constexpr char spat = 8;	// data: spat count (at least 2)
		static inline constexpr char str = 9;	// no data, following is either arg or opt
		static inline constexpr char end = 10;	// no data
	};

	static inline constexpr size_t define_arg_size = 128;
	static inline constexpr const char* va_args = "\xB__VA_ARGS__";
	static inline constexpr const char* va_opt = "\xA__VA_OPT__";

	void define_add_token(char *n, bool has_va, const char *args, const char *arg_top);

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
		if (next_token_dir(n)) {	// arguments or tokens
			char args[define_arg_size];
			char *arg_top = args;
			size_t arg_count = 0;
			bool has_va = false;
			if (m_stream.get_off() == name_off + 1 && Token::is_op(n, Token::Op::LPar)) {	// has args first
				bool expect_id = false;
				bool expect_end = false;
				while (true) {
					if (!next_token_dir(n))
						m_stream.error("Expected token");
					if (Token::type(n) == Token::Type::Identifier) {
						auto size = Token::size(n);
						if (arg_top + size + 1 >= args + define_arg_size)
							m_stream.error("Argument overflow");
						auto data = Token::data(n);
						for (uint8_t i = 0; i < size; i++)
							*arg_top++ = *data++;
						*arg_top++ = 0;
						arg_count++;
						if (!next_token_dir(n))
							m_stream.error("Expected token");
					} else if (Token::is_op(n, Token::Op::Expand)) {
						has_va = true;
						expect_end = true;
						if (!next_token_dir(n))
							m_stream.error("Expected token");
					} else if (expect_id)
						m_stream.error("Expected token");
					if (Token::type(n) == Token::Type::Operator) {
						auto o = Token::op(n);
						if (o == Token::Op::Comma) {
							if (expect_end)
								m_stream.error("Expected ')'");
							if (arg_count == 0)
								m_stream.error("Expected arg before");
							expect_id = true;
							continue;
						} else if (o == Token::Op::RPar)
							break;
					}
					m_stream.error("Expected ',' or ')'");
				}
				if (arg_count == 0)	// one unamed argument at beginning
					arg_count = 1;
			}
			m_buffer[m_size++] = arg_count | (has_va ? 0x80 : 0);
			while (next_token_dir(n))
				define_add_token(n, has_va, args, arg_top);
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
		assert_token(n);
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