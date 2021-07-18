#include "TokenStream.hpp"

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
				throw;
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
	m_error = "Unknown operator";
	throw;
}

bool Stream::adv_str(char delim, Type type)
{
	m_i++;
	m_res = m_buf_raw + 2;
	char *filler = m_res;	// Optmization: overengineered on short strings ? See whether extra code is worth less cycles
	fill_str<0>(delim, filler);
	m_res[-1] = static_cast<uint8_t>(filler - m_res);
	m_res[-2] = static_cast<char>(type);
	m_res -= 2;
	m_i++;
	return true;
}

char* Stream::next(void)
{
	if (!skip_blank())
		return nullptr;
	m_res = m_i;
	return gather_type(tok_type(*m_res));
}

char* Stream::next_include(void)
{
	if (!skip_blank())
		return nullptr;
	m_res = m_i;
	auto type = tok_type(*m_res);
	if (*m_res == '<')
		type = Type::StringSysInclude;
	return gather_type(type);
}

}