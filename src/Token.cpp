#include "Token.hpp"

namespace Token {

bool Stream::adv_operator(void)
{
	auto ret_base = [&](char res) -> bool {
		m_i[-1] = res;
		m_res = m_i - 2;
		*m_res = static_cast<char>(Type::Operator);
		return true;
	};
	auto ret = [&](char res) -> bool {
		if (static_cast<Op>(res) == Op::Comment)
			return adv_comment();
		if (static_cast<Op>(res) == Op::SLComment)
			return adv_sl_comment();
		return ret_base(res);
	};
	auto t = Char::op_table[*m_i];
	if (t & Char::is_op_direct) {
		m_i++;
		return ret_base(t & Char::op_direct_mask);
	}
	if (t & Char::is_op_node) {
		auto n = t & Char::op_node_mask;
		char cur = n;
		auto *tb = &OpCplx::table.nodes[cur];
		auto ret_node = [&]() -> bool {
			if (cur == static_cast<char>(0x80)) {
				m_error = "Unknown operator";
				lthrow;
			}
			return ret(cur);
		};
		while (true) {
			m_i++;
			if (*m_i == Char::eob) {
				feed_buf();
				m_i = m_buf;
			}

			auto si = tb->ind[Char::op_node_table[*m_i]];
			if (si == 0)
				return ret_node();
			if (si & OpCplx::has_valid_code)
				cur = tb->res[si & OpCplx::res_ndx_mask];
			else
				cur = 0x80;
			tb = reinterpret_cast<const OpCplx::Table*>(reinterpret_cast<const char*>(OpCplx::table.nodes) + (si & OpCplx::next_node_mask));
		}
		return true;
	}
	if (*m_i == '#') {
		m_i++;
		return adv_sl_comment();
	}
	m_error = "Unknown operator";
	lthrow;
}

bool Stream::adv_str(char delim, Type type)
{
	m_i++;
	m_res = m_buf_raw + 2;
	char *filler = m_res;	// Optmization: overengineered on short strings ? See whether extra code is worth less cycles
	fill_str<0>(delim, filler);
	m_res[-1] = filler - m_res;
	m_res[-2] = static_cast<char>(type);
	m_res -= 2;
	m_i++;
	return true;
}

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
		return m_res;
	} else {
		if (*m_i == Char::eob) {
			carriage_buf();
			feed_buf();
			if (adv_i(type))
			if (*m_i == Char::eob) {
				m_error = "Max token size is 255";
				lthrow;
			}
		}
		m_res[-1] = m_i - m_res;
		m_res[-2] = static_cast<char>(type) & 0x07;
		return m_res - 2;
	}
}

}