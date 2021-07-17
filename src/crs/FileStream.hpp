#pragma once

#include "cint.hpp"

class Stream
{
public:
	// returns 0 on EOF
	virtual size_t read(char *buf, size_t size) = 0;
	virtual bool eof(void) const = 0;
	virtual bool open(const char *filepath) = 0;
	virtual void seek(size_t ndx) = 0;
};