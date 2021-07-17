#pragma once

#include <cstring>

#include "arith.hpp"

class StrStream
{
	size_t m_ndx = 0;

public:
	struct Buffer
	{
		size_t size = 0;
		char *data = nullptr;

		Buffer(void)
		{
		}
		~Buffer(void)
		{
			if (data != nullptr) {
				delete[] data;
				size = 0;
				data = nullptr;
			}
		}
	};

private:
	Buffer m_buf;
public:

	StrStream(void)
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

	bool open(const char *filepath)
	{
		static_cast<void>(filepath);
		return true;
	}

	void seek(size_t ndx)
	{
		m_ndx = ndx;
	}

	void close(void)
	{
	}

	// custom test methods
	void set_file_data(const char *str)
	{
		m_buf.size = std::strlen(str);
		m_buf.data = new char[m_buf.size];
		std::memcpy(m_buf.data, str, m_buf.size);
	}

	void set_file_data(Buffer &&buf)
	{
		m_buf.size = buf.size;
		m_buf.data = buf.data;
		buf.size = 0;
		buf.data = nullptr;
	}
};

using Stream = StrStream;