#pragma once

#include "cint.hpp"
#include "Token.hpp"
#include "File.hpp"
#include <fxlib.h>

static inline void tactical_exit(const char *msg)
{
	locate(1, 1);
	Print(reinterpret_cast<const uint8_t*>("FATAL ERROR"));
	locate(1, 2);
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

	inline bool open_file(const char *base_folder, const char *search_dir, const char *filepath, char *&stack, const char *stack_top)
	{
		char buf[1 + (search_dir != nullptr ? static_cast<size_t>(Token::size(search_dir)) : 0) + static_cast<size_t>(Token::size(filepath))];
		if (search_dir != nullptr) {
			if (sizeof(buf) > 255) {
				*stack = 0x7F;
				return false;
			}
			auto b = buf;
			auto bs = b++;
			auto s = Token::size(search_dir);
			auto d = Token::data(search_dir);
			for (uint8_t i = 0; i < s; i++)
				*b++ = *d++;
			s = Token::size(filepath);
			d = Token::data(filepath);
			for (uint8_t i = 0; i < s; i++)
				*b++ = *d++;
			*bs = b - buf - 1;
			filepath = buf;
		} else
			filepath++;

		File::hashed h;
		File::hash_path(filepath, h);
		uint8_t s = File::hashed_len;
		const char *d = h;
		auto crd_s = Token::size(base_folder);
		auto crd_d = Token::data(base_folder);
		FONTCHARACTER path[crd_s + s + 3];
		uint8_t i = 0;
		for (uint8_t j = 0; j < crd_s; j++)
			path[i++] = crd_d[j];
		for (uint8_t j = 0; j < s; j++)
			path[i++] = d[j];
		path[i] = 0;
		auto base = stack;
		if (stack_top != nullptr) {
			auto s = static_cast<uint8_t>(filepath[0]);
			auto d = filepath + 1;
			if (stack + 2 + s > stack_top) {
				*stack = 0x7F;
				return false;
			}
			*stack++ = static_cast<char>(Token::Type::StringLiteral);
			*stack++ = s;
			for (uint8_t i = 0; i < s; i++)
				*stack++ = d[i];
		}
		m_handle = Bfile_OpenFile(path, _OPENMODE_READ);
		auto res = m_handle >= 0;
		if (!res)
			stack = base;
		return res;
	}

	// stack contains ptr for opened file signature or opening error
	// when stack_top is nullptr, filepath contains previously opened file signature (must not be overwritten unless error)
	inline bool open(const char *filepath, const char *ctx, char *&stack, const char *stack_top)
	{
		if (stack_top == nullptr) {
			if (!open_file(crd, nullptr, filepath, stack, nullptr))
				tactical_exit("Bfile_OpenFile failed");
			return true;
		}

		if (ctx != nullptr) {
			auto s = Token::size(ctx);
			auto d = Token::data(ctx);
			uint8_t i;
			for (i = 0; i < s; i++)
				if (d[s - 1 - i] == '/')
					break;
			s -= i;
			char c[2 + s];
			for (uint8_t i = 0; i < 2 + s; i++)
				c[i] = ctx[i];
			c[1] = s;

			if (open_file(crd, c, filepath, stack, stack_top))
				return true;
			if (*stack == 0x7F)
				return false;
		}
		if (open_file(crd, nullptr, filepath, stack, stack_top))
			return true;
		if (*stack == 0x7F)
			return false;

		auto s = Token::whole_size(filepath);
		if (stack + s > stack_top) {
			*stack = 0x7F;
			return false;
		}
		for (uint8_t i = 0; i < s; i++)
			stack[i] = filepath[i];
		return false;
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