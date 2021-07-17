#pragma once

#include "cint.hpp"

class Stream
{
public:
	// returns 0 on EOF
	size_t read(char *buf, size_t size)
	{
		return 0;
	}

	bool eof(void) const
	{
		return true;
	}

	bool open(const char *filepath)
	{
		return true;
	}

	void seek(size_t ndx)
	{
	}

	void close(void)
	{
	}
};