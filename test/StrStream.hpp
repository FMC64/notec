#pragma once

#include <cstring>
#include <cstdlib>

#include "arith.hpp"
#include "Token.hpp"

class StrStream
{
	size_t m_ndx;

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
	struct Entry
	{
		char *name;
		size_t size;
		char *data;
	};
	size_t m_entry_count = 0;
	Entry *m_entries = nullptr;

public:

	StrStream(void)
	{
	}
	~StrStream(void)
	{
		for (size_t i = 0; i < m_entry_count; i++) {
			std::free(m_entries[i].name);
			delete[] m_entries[i].data;
		}
		delete[] m_entries;
		m_entry_count = 0;
		m_entries = nullptr;
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
		const char *str = Token::data(filepath);
		uint8_t size = Token::size(filepath);
		for (size_t i = 0; i < m_entry_count; i++) {
			auto &e = m_entries[i];
			auto n = e.name;
			bool match = true;
			for (uint8_t j = 0; j < size; j++)
				if (str[j] != *n++) {
					match = false;
					break;
				}
			if (!match || *n != 0)
				continue;
			m_ndx = 0;
			m_buf.size = e.size;
			m_buf.data = new char[e.size];
			std::memcpy(m_buf.data, e.data, e.size);
			return true;
		}
		return false;
	}

	void seek(size_t ndx)
	{
		m_ndx = ndx;
	}

	void close(void)
	{
		if (m_buf.data != nullptr) {
			delete[] m_buf.data;
			m_buf.data = nullptr;
			m_buf.size = 0;
		}
	}

	// custom test methods
	void set_file_data(const char *str)
	{
		m_buf.size = std::strlen(str);
		m_buf.data = new char[m_buf.size];
		std::memcpy(m_buf.data, str, m_buf.size);
	}

	void set_file_count(size_t count)
	{
		m_entries = new Entry[count];
	}

	void add_file(const char *path, const char *data)
	{
		auto &e = m_entries[m_entry_count++];
		e.name = strdup(path);
		e.size = std::strlen(data);
		e.data = new char[e.size];
		std::memcpy(e.data, data, e.size);
	}

	void add_file(const char *path, Buffer &&buf)
	{
		auto &e = m_entries[m_entry_count++];
		e.name = strdup(path);
		e.size = buf.size;
		e.data = buf.data;
		buf.size = 0;
		buf.data = nullptr;
	}
};

using Stream = StrStream;