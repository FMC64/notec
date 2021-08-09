#pragma once

#include "cint.hpp"
#include "Token.hpp"
#include "File.hpp"
#include "StrMap.hpp"
#include <fxlib.h>

static inline void tactical_exit(const char *msg, int code)
{
	locate(1, 1);
	Print(reinterpret_cast<const uint8_t*>("FATAL ERROR"));
	locate(1, 2);
	Print(reinterpret_cast<const uint8_t*>(msg));
	locate(1, 3);
	char cbuf[64];
	{
		auto c = cbuf;
		if (code < 0) {
			*c++ = '-';
			code *= -1;
		}
		auto base = c;
		uint8_t s = 0;
		while (code > 0) {
			s++;
			*c++ = static_cast<char>(code % 10) + '0';
			code /= 10;
		}
		auto h = s / 2;
		for (uint8_t i = 0; i < h; i++) {
			auto ir = s - 1 - i;
			auto tmp = base[i];
			base[i] = base[ir];
			base[ir] = tmp;
		}
		*c = 0;
	}
	Print(reinterpret_cast<const uint8_t*>(cbuf));
	while (1) {
		unsigned int key;
		GetKey(&key);
	}
}

class Stream
{
	int m_handle;
	StrMap::BlockGroup m_ponce;

public:
	// returns 0 on EOF
	inline size_t read(char *buf, size_t size)
	{
		auto s = Bfile_ReadFile(m_handle, buf, size, -1);
		if (s < 0)
			tactical_exit("Bfile_ReadFile failed", s);
		return s;
	}

	void insert_ponce(const char *sign)
	{
		token_nter(sn, sign);
		m_ponce.insert(sn);
	}

	inline bool open_file(const char *filepath, const char *search_dir, const char *ctx,
		char *&stack, const char *stack_top)
	{
		char buf[256];
		if (!File::path_absolute(search_dir ? search_dir + 1 : nullptr, filepath + 1, buf, buf + sizeof buf))
			return false;
		filepath = buf;

		if (ctx != nullptr)
			if (streq(filepath, ctx + 1)) {
				seek(0);
				goto push_file;
			}
		if (stack_top != nullptr) {	// don't check pragma once on resume
			token_nter(fn, filepath - 1);
			if (m_ponce.resolve(fn)) {
				*stack = 0x7E;
				return true;
			}
		}

		{
			FONTCHARACTER path[File::path_fc_size(filepath)];
			size_t s = static_cast<size_t>(static_cast<uint8_t>(buf[0])) + 1;
			char buf_cpy[s];
			for (size_t i = 0; i < s; i++)
				buf_cpy[i] = buf[i];
			File::path_fc(buf_cpy, path);
			int hdl = Bfile_OpenFile(path, _OPENMODE_READ);
			if (hdl < 0) {
				if (hdl != IML_FILEERR_ENTRYNOTFOUND)
					tactical_exit("Bfile_OpenFile failed", hdl);
				return false;
			}
			if (ctx != nullptr)
				close();
			m_handle = hdl;
		}
		push_file:;
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
		return true;
	}

	// stack contains ptr for opened file signature or opening error
	// when stack_top is nullptr, filepath contains previously opened file signature (must not be overwritten unless error)
	inline bool open(const char *filepath, const char *ctx, char *&stack, const char *stack_top)
	{
		if (stack_top == nullptr) {
			if (!open_file(filepath, nullptr, ctx, stack, nullptr))
				tactical_exit("Bfile_OpenFile failed", 0);
			return true;
		}

		if (open_file(filepath, nullptr, ctx, stack, stack_top))
			return true;
		if (*stack == 0x7F)
			return false;

		if (ctx != nullptr) {
			auto s = Token::size(ctx);
			auto d = Token::data(ctx);
			uint8_t i;
			for (i = 0; i < s; i++) {
				auto c = d[s - 1 - i];
				if (c == '/' || c == ':')
					break;
			}
			s -= i;
			char c[2 + s];
			for (uint8_t i = 0; i < 2 + s; i++)
				c[i] = ctx[i];
			c[1] = s;

			if (open_file(filepath, c, ctx, stack, stack_top))
				return true;
			if (*stack == 0x7F)
				return false;
		}

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
			tactical_exit("Bfile_SeekFile failed", r);
	}

	inline void close(void)
	{
		auto r = Bfile_CloseFile(m_handle);
		if (r < 0)
			tactical_exit("Bfile_CloseFile failed", r);
	}
};