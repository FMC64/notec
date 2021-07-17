#pragma once

#include <cstring>

#include "arith.hpp"

class StrStream
{
	size_t m_ndx = 0;

public:
	struct Buffer
	{
		size_t size;
		char *data;

		Buffer(size_t size, const char *buf)
		{
			this->size = size;
			data = new char[size];
			std::memcpy(data, buf, size);
		}
		struct buf_rvalue {};
		static inline constexpr buf_rvalue buf_rvalue_v{};
		Buffer(size_t size, char *buf, const buf_rvalue&) :
			size(size),
			data(buf)
		{
		}
		Buffer(Buffer &&other) :
			size(other.size),
			data(other.data)
		{
			other.data = nullptr;
		}
		~Buffer(void)
		{
			if (data != nullptr)
				delete[] data;
		}
	};

private:
	Buffer m_buf;
public:

	StrStream(const char *str) :
		StrStream(std::strlen(str), str)
	{
	}
	StrStream(size_t size, const char *buf) :
		m_buf(size, buf)
	{
	}
	StrStream(Buffer &&buf) :
		m_buf(static_cast<Buffer&&>(buf))
	{
	}

	size_t read(char *buf, size_t size)
	{
		if (m_ndx >= m_buf.size)
			return 0;
		size_t to_read = min(m_buf.size - m_ndx, size);
		std::memcpy(buf, &m_buf.data[m_ndx], to_read);
		m_ndx += to_read;
		return to_read;
	}

	bool eof(void) const
	{
		return m_ndx >= m_buf.size;
	}
};

using Stream = StrStream;