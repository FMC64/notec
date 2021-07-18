#pragma once

#include "TokenStream.hpp"

class Pp
{
	Token::Stream m_stream;

public:
	inline Pp(void)
	{
	}

	inline Stream& get_stream(void)	// call on error and for custom stream modifications before polling (testing only)
	{
		return m_stream.get_stream();
	}

	inline void open(const char *filename)	// to call once before polling
	{
		m_stream.push(filename);
	}

	inline char* next(void)
	{
		return m_stream.next();
	}
};