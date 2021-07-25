#include "Pp.hpp"

char* Pp::next_token(void)
{
	return _next_token();
}

char* Pp::next_token_include(void)
{
	return _next_token<true>();
}

bool Pp::next_token_dir(char* &res)
{
	return _next_token_dir(res);
}

bool Pp::next_token_dir_include(char* &res)
{
	return _next_token_dir<true>(res);
}

Pp::TokPoll Pp::define_add_token(char *&n, bool has_va, const char *args, const char *arg_top, size_t last)
{
	auto size = Token::whole_size(n);
	if (Token::type(n) == Token::Type::Identifier) {
		if (streq(n + 1, va_args)) {
			if (!has_va)
				m_stream.error("Non-variadic macro");
			alloc(2);
			m_buffer[m_size++] = TokType::arg;
			m_buffer[m_size++] = 0x80;
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
				auto p = define_add_token(n, has_va, args, arg_top, last);
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
			define_add_token(n, has_va, args, arg_top, -1);
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
			n[0] = TokType::spat;
			uint8_t count = 1;
			TokPoll res;
			while (true) {
				if (!next_token_dir(n))
					m_stream.error("Expected token");
				auto cur = m_size;
				define_add_token(n, has_va, args, arg_top, -1);
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

char* Pp::next(void)
{
	while (true) {
		auto n = next_token();
		while (true) {
			if (n == nullptr)
				return nullptr;
			if (Token::is_op(n, Token::Op::Sharp))
				n = directive();
			else
				return n;
		}
	}
}