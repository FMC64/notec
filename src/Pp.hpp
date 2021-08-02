#pragma once

#include "clib.hpp"

#include "TokenStream.hpp"
#include "StrMap.hpp"

class Pp
{
	Token::Stream m_stream;
	StrMap::BlockGroup m_dirs;
	StrMap::BlockGroup m_ponce;

public:
	inline Pp(void)
	{
		#define PP_DIRECTIVE_NEXT(id) m_dirs.insert(#id, &Pp::id)
		PP_DIRECTIVE_NEXT(include);
		PP_DIRECTIVE_NEXT(define);
		PP_DIRECTIVE_NEXT(undef);
		PP_DIRECTIVE_NEXT(error);
		PP_DIRECTIVE_NEXT(pragma);
		#undef PP_DIRECTIVE_NEXT
		m_stack = m_stack_base;
	}
	inline ~Pp(void)
	{
		free(m_buffer);
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

	inline const char* include(void)
	{
		char *n;
		if (!next_token_dir_include(n))
			m_stream.error("Expected string");
		assert_token(n);
		auto t = Token::type(n);
		if (t != Token::Type::StringLiteral && t != Token::Type::StringSysInclude)
			m_stream.error("Expected string");
		m_stream.push(n);
		token_nter(sn, m_stream.get_file_sign());
		if (m_ponce.resolve(sn))
			m_stream.pop();
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
		static inline constexpr char arg = 6;	// data: arg ndx, __VA_ARGS__ is encoded as one extra arg (ndx arg_count)
		static inline constexpr char opt = 7;	// data: arg count
		static inline constexpr char spat = 8;	// data: spat count (at least 2)
		static inline constexpr char str = 9;	// no data, following is either arg or opt
		static inline constexpr char end = 10;	// no data
	};

	static inline constexpr size_t define_arg_size = 128;
	static inline constexpr const char* va_args = "\xB__VA_ARGS__";
	static inline constexpr const char* va_opt = "\xA__VA_OPT__";

	enum class TokPoll : char {
		Do = 0,
		Dont = 1,
		End = 2
	};
	TokPoll define_add_token(char *&n, bool has_va, size_t arg_count, const char *args, const char *arg_top, size_t last);

	inline bool define_is_tok_spattable_ll(const char *token)
	{
		auto t = Token::type(token);
		return static_cast<char>(t) <= static_cast<char>(Token::Type::Identifier);
	}

	inline bool define_is_tok_spattable(const char *token)
	{
		auto t = Token::type(token);
		return define_is_tok_spattable_ll(token) ||// t == Token::Type::Operator ||
			t == static_cast<Token::Type>(TokType::arg) || t == static_cast<Token::Type>(TokType::opt);
	}

	inline const char* define(void)
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
			bool is_dir_cont = true;
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
				if (arg_count == 0 && !has_va)	// one unamed argument at beginning
					arg_count = 1;	// no first argument is indistiguishable from empty first arg
				is_dir_cont = next_token_dir(n);
			}
			m_buffer[m_size++] = arg_count | (has_va ? 0x80 : 0);
			size_t last = -1;
			if (is_dir_cont)
				while (true) {
					auto cur = m_size;
					auto p = define_add_token(n, has_va, arg_count, args, arg_top, last);
					if (p == TokPoll::End)
						break;
					if (p == TokPoll::Do)
						if (!next_token_dir(n))
							break;
					last = cur;
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

	// give opportunity to trim macro buffer if definition at end?
	// doesn't seem worth extra code for now and macros already fairly slim
	inline const char* undef(void)
	{
		char *n;
		if (!next_token_dir(n))
			m_stream.error("Expected token");
		assert_token_type(n, Token::Type::Identifier);
		token_nter(nn, n);
		m_macros.remove(nn);
		if (next_token_dir(n))
			m_stream.error("Expected no further token");
		return n;
	}

	inline const char* error(void)
	{
		m_stream.error("#error");
		__builtin_unreachable();
	}

	static inline constexpr auto once = make_cstr("once");

	inline const char* pragma(void)
	{
		char *n;
		if (next_token_dir(n)) {
			if (Token::type(n) == Token::Type::Identifier) {
				if (streq(n + 1, once)) {
					auto sign = m_stream.get_file_sign();
					token_nter(sn, sign);
					m_ponce.insert(sn);
				}
				if (next_token_dir(n))
					m_stream.error("Expected no further token");
			}
		}
		return n;
	}

	inline const char* directive(void)
	{
		char *n;
		if (!next_token_dir(n))
			return n;
		assert_token_type(n, Token::Type::Identifier);
		token_nter(nn, n);
		const char* (Pp::*dir)(void);
		if (!m_dirs.resolve(nn, dir))
			m_stream.error("Unknown directive");
		return (this->*dir)();
	}

	static inline constexpr size_t stack_size = 256;
	char m_stack_base[stack_size];
	char *m_stack;

	struct StackFrameType {
		static inline constexpr uint8_t macro = 0;
		static inline constexpr uint8_t tok = 1;
		static inline constexpr uint8_t arg = 2;
	};

	void tok_skip(const char *&n);
	void tok_str(const char *&n, char *&c, const char *c_top, const char *args);

	inline bool macro(char *entry, const char *&res)
	{
		auto off = load<uint16_t>(entry);
		const char *n = m_buffer + off;
		auto t = *n;
		if (t == TokType::end) {
			res = nullptr;
			return true;
		} else if (t == TokType::arg) {
			store(entry, static_cast<uint16_t>(off + 2));
			auto m = static_cast<uint8_t>(n[1]);
			auto n = entry + 3;
			for (uint8_t i = 0; i < m; i++) {
				while (*n != TokType::end)
					n += Token::whole_size(n);
				n++;
			}
			if (m_stack + 5 > m_stack_base + stack_size)
				m_stream.error("Macro stack overflow");
			*m_stack++ = StackFrameType::arg;
			m_stack += store(m_stack, static_cast<uint16_t>(n - m_stack_base));
			m_stack += store(m_stack, static_cast<uint16_t>(5));
			return false;
		} else if (t == TokType::opt) {
			if (entry[2])
				n += 2;
			else {
				auto m = static_cast<uint8_t>(n[1]);
				n += 2;
				for (uint8_t i = 0; i < m; i++)
					tok_skip(n);
			}
			store(entry, static_cast<uint16_t>(n - m_buffer));
			return false;
		} else if (t == TokType::str) {
			res = m_stack;
			auto c = m_stack;
			*c++ = static_cast<char>(Token::Type::StringLiteral);
			auto size = reinterpret_cast<uint8_t*>(c++);
			n++;
			tok_str(n, c, m_stack_base + stack_size, entry + 2);
			*size = c - m_stack - 2;
			store(entry, static_cast<uint16_t>(n - m_buffer));
			return true;
		} else if (t == TokType::spat) {	// hardest one, bufferize generated tokens on a separate stack then output them one by one on the lowest level
			auto m = static_cast<uint8_t>(n[1]);
			n += 2;
			store(entry, static_cast<uint16_t>(n - m_buffer));
			char stack_base[stack_size];
			auto stack = stack_base;
			char *last = nullptr;
			bool can_spat = false;
			auto base = m_stack;
			auto next = n;
			for (uint8_t i = 0; i < m; i++) {
				tok_skip(next);	// next is next spat arg in macro seq

				while (m_stack >= base) {	// can't poll tokens from earlier frames
					auto s = load<uint16_t>(m_stack - 2);
					if (stack_poll(m_stack - s, n)) {
						if (n == nullptr)
							m_stack -= s;
						else {
							if (can_spat && last != nullptr) {
								if (!define_is_tok_spattable_ll(n))
									m_stream.error("Invalid token for spatting");
								auto s = Token::size(n);
								if (stack + s > stack_base + stack_size)
									m_stream.error("Macro stack overflow");
								auto d = Token::data(n);
								for (uint8_t i = 0; i < s; i++)
									*stack++ = *d++;
								last[1] += s;
							} else {
								last = stack;
								auto s = Token::whole_size(n);
								if (stack + s > stack_base + stack_size)
									m_stream.error("Macro stack overflow");
								for (uint8_t i = 0; i < s; i++)
									*stack++ = *n++;
							}
							can_spat = false;
						}
					}
					if (m_stack == base)	// do not signal current token while upper macro stack has not been consumed
						if (m_buffer + load<uint16_t>(entry) >= next)	// next spat token reached
							break;
				}
				can_spat = true;
			}
			if (stack + 1 > stack_base + stack_size)
				m_stream.error("Macro stack overflow");
			*stack++ = TokType::end;
			uint16_t s = stack - stack_base;
			if (m_stack + 5 + s > m_stack_base + stack_size)
				m_stream.error("Macro stack overflow");
			*m_stack++ = StackFrameType::arg;
			m_stack += store(m_stack, static_cast<uint16_t>(m_stack + 2 - m_stack_base));
			stack = stack_base;
			for (uint8_t i = 0; i < s; i++)
				*m_stack++ = *stack++;
			m_stack += store(m_stack, static_cast<uint16_t>(5 + s));
			return false;
		}
		auto s = Token::whole_size(n);
		store(entry, static_cast<uint16_t>(off + s));
		res = n;
		return true;
	}

	inline bool tok(char *entry, const char *&res)
	{
		if (*entry == TokType::end) {
			res = nullptr;
			return true;
		}
		*entry = TokType::end;
		res = entry + 1;
		return true;
	}

	inline bool arg(char *entry, const char *&res)
	{
		auto off = load<uint16_t>(entry);
		auto n = m_stack_base + off;
		if (*n == TokType::end) {
			res = nullptr;
			return true;
		}
		auto s = Token::whole_size(n);
		store(entry, static_cast<uint16_t>(off + s));
		res = n;
		return true;
	}

	using stack_t = bool (Pp::*)(char *entry, const char *&res);
	static inline constexpr stack_t stacks[] = {
		&Pp::macro,	// 0
		&Pp::tok,	// 1
		&Pp::arg	// 2
	};

	inline bool stack_poll(char *entry, const char *&res)
	{
		return (this->*stacks[*entry])(entry + 1, res);
	}

public:
	const char* next(void);
};