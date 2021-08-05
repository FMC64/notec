#pragma once

#include "cint.hpp"
#include "Token.hpp"
#include "File.hpp"
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

	static inline constexpr auto crd = make_tstr(Token::Type::StringLiteral, "\\\\crd0\\");

	// stack contains ptr for opened file signature or opening error
	// when stack_top is nullptr, filepath contains previously opened file signature (must not be overwritten unless error)
	inline bool open(const char *filepath, const char *ctx, char *&stack, const char *stack_top)
	{
		static_cast<void>(ctx);	// context ignored for now

		if (stack_top == nullptr) {
			auto s = Token::size(filepath);
			auto d = Token::data(filepath);
			FONTCHARACTER path[s + 1];
			for (uint8_t i = 0; i < s; i++)
				path[i] = d[i];
			path[s] = 0;
			m_handle = Bfile_OpenFile(path, _OPENMODE_READ);
			if (m_handle < 0)
				tactical_exit("Bfile_OpenFile failed");
			return true;
		}
		File::hashed h;
		File::hash_path(filepath + 1, h);
		uint8_t s = File::hashed_len;
		const char *d = h;
		auto crd_s = Token::size(crd);
		auto crd_d = Token::data(crd);
		FONTCHARACTER path[crd_s + s + 1];
		uint8_t i = 0;
		for (uint8_t j = 0; j < crd_s; j++)
			path[i++] = crd_d[j];
		for (uint8_t j = 0; j < s; j++)
			path[i++] = d[j];
		path[i] = 0;
		if (stack + 2 + i > stack_top) {
			*stack = 0x7F;
			return false;
		}
		auto base = stack;
		*stack++ = static_cast<char>(Token::Type::StringLiteral);
		*stack++ = i;
		for (uint8_t j = 0; j < i; j++)
			*stack++ = path[j];
		m_handle = Bfile_OpenFile(path, _OPENMODE_READ);
		if (m_handle < 0) {
			s = Token::whole_size(filepath);
			if (base + s > stack_top) {
				*base = 0x7F;
				return false;
			}
			for (uint8_t i = 0; i < s; i++)
				base[i] = filepath[i];
			return false;
		}
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