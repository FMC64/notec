#include "Pp.hpp"

Pp::TokPoll Pp::define_add_token(const char *&n, bool has_va, size_t arg_count, const char *args, const char *arg_top, size_t last)
{
	auto size = Token::whole_size(n);
	if (Token::type(n) == Token::Type::Identifier) {
		if (streq(n + 1, va_args)) {
			if (!has_va)
				m_stream.error("Non-variadic macro");
			alloc(2);
			m_buffer[m_size++] = TokType::arg;
			m_buffer[m_size++] = arg_count;
			return TokPoll::Do;
		}
		if (streq(n + 1, va_opt)) {
			if (!has_va)
				m_stream.error("Non-variadic macro");
			alloc(2);
			m_buffer[m_size++] = TokType::opt;
			auto count = m_size++;
			if (!next_token_dir(n))
				m_stream.error("Expected token");
			if (!Token::is_op(n, Token::Op::LPar))
				m_stream.error("Expected '('");
			size_t deep = 1;
			uint8_t c = 0;
			size_t last = -1;
			if (!next_token_dir(n))
				m_stream.error("Expected token");
			while (true) {
				if (Token::type(n) == Token::Type::Operator) {
					auto o = Token::op(n);
					if (o == Token::Op::LPar)
						deep++;
					else if (o == Token::Op::RPar) {
						deep--;
						if (deep == 0)
							break;
					}
				}
				c++;
				auto cur = m_size;
				auto p = define_add_token(n, has_va, arg_count, args, arg_top, last);
				if (p == TokPoll::End)
					m_stream.error("Expected token");
				if (p == TokPoll::Do)
					if (!next_token_dir(n))
						m_stream.error("Expected token");
				last = cur;
			}
			reinterpret_cast<uint8_t&>(m_buffer[count]) = c;
			return TokPoll::Do;
		}
		auto s = Token::size(n);
		auto data = Token::data(n);
		auto cur = args;
		uint8_t ndx = 0;
		while (cur < arg_top) {
			bool match = true;
			for (uint8_t i = 0; i < s; i++) {
				if (data[i] != *cur) {
					match = false;
					break;
				}
				cur++;
			}
			if (match && *cur == 0) {
				alloc(2);
				m_buffer[m_size++] = TokType::arg;
				m_buffer[m_size++] = ndx;
				return TokPoll::Do;
			}
			while (*cur != 0)
				cur++;
			cur++;
			ndx++;
		}
	} else if (Token::type(n) == Token::Type::Operator) {
		auto o = Token::op(n);
		if (o == Token::Op::Sharp) {
			alloc(1);
			m_buffer[m_size++] = TokType::str;
			auto strd = m_size;
			if (!next_token_dir(n))
				m_stream.error("Expected token");
			define_add_token(n, has_va, arg_count, args, arg_top, -1);
			if (m_buffer[strd] != TokType::arg && m_buffer[strd] != TokType::opt)
				m_stream.error("Can only convert arguments or opt");
			return TokPoll::Do;
		} else if (o == Token::Op::DoubleSharp) {
			if (last == static_cast<size_t>(-1))
				m_stream.error("Expected token on left");
			alloc(2);
			n = m_buffer + last;
			if (!define_is_tok_spattable(n))
				m_stream.error("Left token non-spattable");
			auto size = Token::whole_size(n);
			m_size += 2;
			auto bo = m_size - 1;
			for (size_t i = 0; i < size; i++)
				m_buffer[bo - i] = m_buffer[bo - i - 2];
			m_buffer[last] = TokType::spat;
			uint8_t count = 1;
			TokPoll res;
			while (true) {
				if (!next_token_dir(n))
					m_stream.error("Expected token");
				auto cur = m_size;
				define_add_token(n, has_va, arg_count, args, arg_top, -1);
				if (!define_is_tok_spattable(m_buffer + cur))
					m_stream.error("Token non-spattable");
				count++;
				if (!next_token_dir(n)) {
					res = TokPoll::End;
					break;
				}
				if (!Token::is_op(n, Token::Op::DoubleSharp)) {
					res = TokPoll::Dont;
					break;
				}
			}
			m_buffer[last + 1] = count;
			return res;
		}
	}
	alloc(size);
	for (size_t i = 0; i < size; i++)
		m_buffer[m_size++] = n[i];
	return TokPoll::Do;
}

void Pp::tok_skip(const char *&n)
{
	auto t = *n;
	n++;
	// make table? might improve perfs
	if (t < Token::type_first_constant) {
		auto s = static_cast<uint8_t>(*n++);
		n += s;
	} else if (t >= Token::type_first_constant && t <= TokType::arg) {
		n++;
	} else if (t == TokType::str) {
		tok_skip(n);
	} else if (t == TokType::spat || t == TokType::opt) {
		auto m = static_cast<uint8_t>(*n++);
		for (uint8_t i = 0; i < m; i++)
			tok_skip(n);
	}
}

template <Token::Op op, auto str, auto ...rest>
static inline constexpr auto computeOpStrTableSize(void)
{
	size_t s = str.size() + 1;
	if constexpr (sizeof...(rest) > 0)
		s += computeOpStrTableSize<rest...>();
	return s;
}

template <Token::Op op, auto str, auto ...rest>
static inline constexpr auto fillOpStrTable(auto &res, size_t &ndx)
{
	res.data[static_cast<uint8_t>(static_cast<char>(op))] = ndx;
	size_t s = str.size();
	res.data[ndx++] = static_cast<uint8_t>(s);
	for (size_t i = 0; i < s; i++)
		res.data[ndx++] = str.data[i];
	if constexpr (sizeof...(rest) > 0)
		s += fillOpStrTable<rest...>(res, ndx);
	return s;
}

template <Token::Op op, auto str, auto ...rest>
static inline constexpr auto computeOpStrTable(void)
{
	constexpr size_t s = static_cast<char>(Token::last_op) + 1 + computeOpStrTableSize<op, str, rest...>();
	carray<char, s> res;
	for (size_t i = 0; i < s; i++)
		res.data[i] = 0;	// 5 & 7 are undef because of gap in op enum
	size_t i = static_cast<char>(Token::last_op) + 1;
	fillOpStrTable<op, str, rest...>(res, i);
	return res;
}

template <size_t Size>
using CStr = carray<char, Size>;

template <size_t Size>
static constexpr auto genCStr(const char (&str)[Size])
{
	CStr<Size - 1> res;
	for (size_t i = 0; i < Size - 1; i++)
		res.data[i] = str[i];
	return res;
}

static constexpr auto op_str_table = computeOpStrTable<
#define OP_STR_ENTRY(n, s) Token::Op::n, genCStr(s)
	OP_STR_ENTRY(Not, "!"),
	OP_STR_ENTRY(Plus, "+"),
	OP_STR_ENTRY(Minus, "-"),
	OP_STR_ENTRY(BitAnd, "&"),
	OP_STR_ENTRY(BitOr, "|"),
	OP_STR_ENTRY(Mul, "*"),
	OP_STR_ENTRY(Colon, ":"),
	OP_STR_ENTRY(Less, "<"),
	OP_STR_ENTRY(Greater, ">"),
	OP_STR_ENTRY(Equal, "="),
	OP_STR_ENTRY(Point, "."),
	OP_STR_ENTRY(BitXor, "^"),
	OP_STR_ENTRY(Mod, "%"),
	OP_STR_ENTRY(Div, "/"),
	OP_STR_ENTRY(Sharp, "#"),

	OP_STR_ENTRY(NotEqual, "!="),

	OP_STR_ENTRY(PlusPlus, "++"),
	OP_STR_ENTRY(PlusEqual, "+="),

	OP_STR_ENTRY(BitAndEqual, "&="),

	OP_STR_ENTRY(Or, "|"),
	OP_STR_ENTRY(BitOrEqual, "|="),

	OP_STR_ENTRY(BitXorEqual, "^="),

	OP_STR_ENTRY(MulEqual, "*="),

	OP_STR_ENTRY(DivEqual, "/="),
	OP_STR_ENTRY(Comment, "/*"),
	OP_STR_ENTRY(SLComment, "//"),

	OP_STR_ENTRY(ModEqual, "%="),

	OP_STR_ENTRY(LessEqual, "<="),
	OP_STR_ENTRY(BitLeft, "<<"),
	OP_STR_ENTRY(BitLeftEqual, "<<="),

	OP_STR_ENTRY(GreaterEqual, ">="),
	OP_STR_ENTRY(BitRight, ">>"),
	OP_STR_ENTRY(BitRightEqual, ">>="),

	OP_STR_ENTRY(EqualEqual, "=="),
	OP_STR_ENTRY(Expand, "..."),

	OP_STR_ENTRY(LPar, "("),
	OP_STR_ENTRY(RPar, ")"),
	OP_STR_ENTRY(LArr, "["),
	OP_STR_ENTRY(RArr, "]"),
	OP_STR_ENTRY(Tilde, "~"),
	OP_STR_ENTRY(Comma, ","),
	OP_STR_ENTRY(Huh, "?"),
	OP_STR_ENTRY(Semicolon, ";"),
	OP_STR_ENTRY(LBra, "{"),
	OP_STR_ENTRY(RBra, "}"),

	OP_STR_ENTRY(TWComp, "<=>"),
	OP_STR_ENTRY(Scope, "::"),

	OP_STR_ENTRY(MinusMinus, "--"),
	OP_STR_ENTRY(MinusEqual, "-="),

	OP_STR_ENTRY(PointMember, ".*"),
	OP_STR_ENTRY(ArrowMember, "->*"),

	OP_STR_ENTRY(Arrow, "->"),
	OP_STR_ENTRY(And, "&"),
	OP_STR_ENTRY(DoubleSharp, "##")
#undef OP_STR_ENTRY
>();

void Pp::tok_str(const char *&n, char *&c, const char *c_top, const char *args)
{
	auto t = *n;
	n++;
	// make table? might shrink bin & improve perfs
	if (t == static_cast<char>(Token::Type::NumberLiteral) || t == static_cast<char>(Token::Type::Identifier)) {
		auto s = static_cast<uint8_t>(*n++);
		if (c + s > c_top)
			m_stream.error("Macro stack overflow");
		for (uint8_t i = 0; i < s; i++)
			*c++ = *n++;
	} else if (t == static_cast<char>(Token::Type::StringLiteral)) {	// sys string can't be parsed outside of #include
		auto s = static_cast<uint8_t>(*n++);
		if (c + s + 2 > c_top)
			m_stream.error("Macro stack overflow");
		*c++ = '\"';
		for (uint8_t i = 0; i < s; i++)
			*c++ = *n++;
		*c++ = '\"';
	} else if (t == static_cast<char>(Token::Type::ValueChar8)) {
		if (c + 3 > c_top)
			m_stream.error("Macro stack overflow");
		*c++ = '\'';
		*c++ = *n++;
		*c++ = '\'';
	} else if (t == static_cast<char>(Token::Type::Operator)) {
		auto m = &op_str_table.data[op_str_table.data[static_cast<uint8_t>(*n++)]];
		auto s = static_cast<uint8_t>(*m++);
		if (c + s > c_top)
			m_stream.error("Macro stack overflow");
		for (uint8_t i = 0; i < s; i++)
			*c++ = *m++;
	} else if (t == TokType::arg) {
		auto m = static_cast<uint8_t>(*n++);
		auto a = args + 1;
		for (uint8_t i = 0; i < m; i++) {
			while (*a != TokType::end)
				a += Token::whole_size(a);
			a++;
		}
		while (*a != TokType::end)
			tok_str(a, c, c_top, args);
	} else if (t == TokType::str) {
		if (c + 1 > c_top)
			m_stream.error("Macro stack overflow");
		*c++ = '\"';
		tok_str(n, c, c_top, args);
		if (c + 1 > c_top)
			m_stream.error("Macro stack overflow");
		*c++ = '\"';
	} else if (t == TokType::spat) {	// we ignore whitespace so spatting is trivial
		auto m = static_cast<uint8_t>(*n++);
		for (uint8_t i = 0; i < m; i++)
			tok_str(n, c, c_top, args);
	} else if (t == TokType::opt) {
		if (*args) {
			auto m = static_cast<uint8_t>(*n++);
			for (uint8_t i = 0; i < m; i++)
				tok_str(n, c, c_top, args);
		} else {
			n--;
			tok_skip(n);
		}
	}
}

const char* Pp::next(void)
{
	while (true) {
		const char *n;
		if (m_stack > m_stack_base) {
			n = nullptr;
			do {
				auto s = load<uint16_t>(m_stack - 2);
				if (stack_poll(m_stack - s, n)) {
					if (n == nullptr)
						m_stack -= s;
					else
						break;
				}
			} while (m_stack > m_stack_base);
			if (n == nullptr)
				n = next_token();
		} else
			n = next_token();
		while (true) {
			if (n == nullptr)
				return nullptr;
			if (Token::is_op(n, Token::Op::Sharp))
				n = directive();
			else {	// substitution
				if (Token::type(n) == Token::Type::Identifier) {
					token_nter(nn, n);
					uint16_t ndx;
					const char* (Pp::*im)(void);
					if (m_idirs.resolve(nn, im))
						n = (this->*im)();
					else if (m_macros.resolve(nn, ndx)) {
						if (m_buffer[ndx] == 0) {	// simple macro substitution, no args needed
							if (m_stack + 6 > m_stack_base + stack_size)
								m_stream.error("Macro stack overflow");
							*m_stack++ = StackFrameType::macro;
							m_stack += store(m_stack, static_cast<uint16_t>(ndx + 1));
							m_stack++;	// undef, will not have any opt
							m_stack += store(m_stack, static_cast<uint16_t>(6));
							goto pushed;
						} else {
							auto size = Token::whole_size(n);
							char nc[size];
							for (uint8_t i = 0; i < size; i++)	// save macro name
								nc[i] = n[i];
							n = next();
							if (n != nullptr && Token::is_op(n, Token::Op::LPar)) {	// call initiated, arguments must be in order
								char stack_base[stack_size];	// dedicated stack for macro invocation, not to mess up actual stack with in-building call
								char *stack = stack_base;
								*stack++ = StackFrameType::macro;
								stack += store(stack, static_cast<uint16_t>(ndx + 1));	// first token is right after arg count
								auto va_any = stack++;
								auto has_va = static_cast<bool>(m_buffer[ndx] & 0x80);
								auto acount = static_cast<uint8_t>(m_buffer[ndx] & ~0x80);
								size_t depth = 1;
								size_t count = 1;
								while (true) {
									n = next();
									if (n == nullptr)
										m_stream.error("Expected token");
									if (Token::type(n) == Token::Type::Operator) {
										auto o = Token::op(n);
										if (o == Token::Op::LPar) {
											depth++;
											continue;
										} else if (o == Token::Op::RPar) {
											depth--;
											if (depth == 0)
												break;
											continue;
										} else if (o == Token::Op::Comma) {
											if (depth == 1) {
												if (stack + 2 > stack_base + stack_size)	// overshoot 1 byte on arg end for less code
													m_stream.error("Macro stack overflow");
												if (count > acount) {	// push new arg as comma for __VA_ARGS__
													*stack++ = static_cast<char>(Token::Type::Operator);
													*stack++ = static_cast<char>(Token::Op::Comma);
												} else {	// mark arg end
													*stack++ = TokType::end;
												}
												count++;
												continue;
											}
										}
									}
									auto size = Token::whole_size(n);
									if (stack + size > stack_base + stack_size)
										m_stream.error("Macro stack overflow");
									for (uint8_t i = 0; i < size; i++)
										*stack++ = *n++;
								}
								if (stack + 1 > stack_base + stack_size)
									m_stream.error("Macro stack overflow");
								*stack++ = TokType::end;
								if (has_va) {
									if (count < acount)
										m_stream.error("Not enough args for variadic invocation");
									*va_any = count > acount;
									if (!*va_any) {
										if (stack + 1 > stack_base + stack_size)
											m_stream.error("Macro stack overflow");
										*stack++ = TokType::end;
									}
								} else
									if (count != acount)
										m_stream.error("Wrong macro argument count");
								if (stack + 2 > stack_base + stack_size)
									m_stream.error("Macro stack overflow");
								stack += store(stack, static_cast<uint16_t>(stack - stack_base + 2));
								auto ssize = static_cast<size_t>(stack - stack_base);
								if (m_stack + ssize > m_stack_base + stack_size)
									m_stream.error("Macro stack overflow");
								for (size_t i = 0; i < ssize; i++)
									*m_stack++ = stack_base[i];
								goto pushed;
							} else {	// invocation failed return macro name
								if (n != nullptr) {	// push non lpar next token
									auto size = Token::whole_size(n);
									char nc[size];
									for (uint8_t i = 0; i < size; i++)	// copy token before pushing it to stack (might be located on top)
										nc[i] = n[i];
									n = nc;
									if (m_stack + 3 + size > m_stack_base + stack_size)
										m_stream.error("Macro stack overflow");
									*m_stack++ = StackFrameType::tok;
									*m_stack++ = 0;	// is TokType::end when already substitued
									for (uint8_t i = 0; i < size; i++)
										*m_stack++ = *n++;
									m_stack += store(m_stack, static_cast<uint16_t>(4 + size));
								}
								if (m_stack + size > m_stack_base + stack_size)
									m_stream.error("Macro stack overflow");
								for (uint8_t i = 0; i < size; i++)	// store macro name in stack top for return value
									m_stack[i] = nc[i];
								return m_stack;
							}
						}
					}
				}
				return n;
			}
		}
		pushed:;	// path for new stack entry but no immediate token generation to return
	}
}