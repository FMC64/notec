#include <string>
#include <cstring>

#include "Stream.hpp"
#include "arith.hpp"

class StrStream : public Stream
{
	std::string m_buf;
	size_t m_ndx = 0;

public:
	StrStream(const std::string &buf) :
		m_buf(buf)
	{
	}

	size_t read(char *buf, size_t size) override
	{
		if (m_ndx >= m_buf.size())
			return 0;
		size_t to_read = min(m_buf.size() - m_ndx, size);
		std::memcpy(buf, &m_buf[m_ndx], to_read);
		m_ndx += to_read;
		return to_read;
	}
};