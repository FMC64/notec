#pragma once

#include "cint.hpp"
#include <fxlib.h>

static inline void tactical_exit(const char *msg)
{
	locate(1, 1);
	Print(reinterpret_cast<const uint8_t*>(msg));
	while (1) {
		unsigned int key;
		GetKey(&key);
	}
}

class Stream
{
	int m_handle;

public:
	// returns 0 on EOF
	inline size_t read(char *buf, size_t size)
	{
		auto s = Bfile_ReadFile(m_handle, buf, size, -1);
		if (s < 0)
			tactical_exit("Bfile_ReadFile failed");
		return s;
	}

	// stack contains ptr for opened file signature or opening error
	// when stack_top is nullptr, filepath contains previously opened file signature (must not be overwritten unless error)
	inline bool open(const char *filepath, char *&stack, const char *stack_top)
	{
		return true;
	}

	inline void seek(size_t ndx)
	{
		auto r = Bfile_SeekFile(m_handle, ndx);
		if (r < 0)
			tactical_exit("Bfile_SeekFile failed");
	}

	inline void close(void)
	{
		auto r = Bfile_CloseFile(m_handle);
		if (r < 0)
			tactical_exit("Bfile_CloseFile failed");
	}
};