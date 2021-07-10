#include "Token.hpp"

namespace Token {

char* Stream::next(void)
{
	adv_wspace();
	while (*m_i == Char::eob) {
		if (!feed_buf())
			return nullptr;
		m_i = m_buf;
		adv_wspace();
	}
	m_res = m_i;
	auto type = tok_type(*m_res);
	if (adv_i(type)) {
		if (m_error)
			return nullptr;
		return m_res;
	} else {
		if (*m_i == Char::eob) {
			carriage_buf();
			feed_buf();
			if (adv_i(type))
				if (m_error)
					return nullptr;
			if (*m_i == Char::eob) {
				m_error = "Max token size is 255";
				return nullptr;
			}
		}
		m_res[-1] = m_i - m_res;
		m_res[-2] = static_cast<char>(type) & 0x07;
		return m_res - 2;
	}
}

}