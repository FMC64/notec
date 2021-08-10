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
	StrMap::BlockGroup m_blk;
	uint16_t m_ponce;
	static inline constexpr size_t include_dirs_size = 128;
	char m_include_dirs_base[include_dirs_size];
	char *m_include_dirs;

public:
	Stream(void)
	{
		m_ponce = m_blk.alloc();
		m_include_dirs = m_include_dirs_base;
	}

	bool include_dir(const char *path)
	{
		size_t s = 1 + static_cast<size_t>(static_cast<uint8_t>(*path));
		if (m_include_dirs + s > m_include_dirs_base + include_dirs_size)
			return false;
		for (size_t i = 0; i < s; i++)
			*m_include_dirs++ = *path++;
		return true;
	}

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
		m_blk.insert(m_ponce, sn);
	}

	inline bool open_file(const char *filepath, const char *search_dir, const char *ctx,
		char *&stack, const char *stack_top)
	{
		char buf[256];
		if (!File::path_absolute(search_dir, filepath, buf, buf + sizeof buf))
			return false;
		filepath = buf;

		if (ctx != nullptr)
			if (streq(filepath, ctx)) {
				seek(0);
				goto push_file;
			}
		if (stack_top != nullptr) {	// don't check pragma once on resume
			token_nter(fn, filepath - 1);
			if (m_blk.resolve(m_ponce, fn)) {
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
			auto s = static_cast<uint8_t>(*filepath);
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
		if (ctx != nullptr)
			ctx++;
		if (stack_top == nullptr) {
			if (!open_file(filepath + 1, nullptr, ctx, stack, nullptr))
				tactical_exit("Bfile_OpenFile failed", 0);
			return true;
		}

		if (static_cast<uint8_t>(filepath[1]) >= 1 && filepath[2] == '/') {
			if (open_file(filepath + 1, nullptr, ctx, stack, stack_top))
				return true;
			if (*stack == 0x7F)
				return false;
		} else {
			auto check_user = [&](){
				auto i = m_include_dirs_base;
				size_t s = 1 + static_cast<size_t>(static_cast<uint8_t>(*i));
				i += s;
				while (i < m_include_dirs) {
					size_t s = 1 + static_cast<size_t>(static_cast<uint8_t>(*i));
					if (open_file(filepath + 1, i, ctx, stack, stack_top))
						return true;
					if (*stack == 0x7F)
						return false;
					i += s;
				}
				return false;
			};
			auto check_sys = [&](){
				if (open_file(filepath + 1, m_include_dirs_base, ctx, stack, stack_top))
					return true;
				if (*stack == 0x7F)
					return false;
				return false;
			};
			auto t = Token::type(filepath);
			if (t == Token::Type::StringSysInclude) {
				if (check_sys())
					return true;
				if (*stack == 0x7F)
					return false;
				if (check_user())
					return true;
				if (*stack == 0x7F)
					return false;
			} else if (t == Token::Type::StringLiteral) {
				if (check_user())
					return true;
				if (*stack == 0x7F)
					return false;
				if (check_sys())
					return true;
				if (*stack == 0x7F)
					return false;
			} else
				tactical_exit("Bad include token type", filepath[0]);
		}

		if (ctx != nullptr) {
			size_t s = static_cast<uint8_t>(*ctx);
			auto d = ctx + 1;
			uint8_t i;
			for (i = 0; i < s; i++) {
				auto c = d[s - 1 - i];
				if (c == '/' || c == ':')
					break;
			}
			s -= i;
			char c[1 + s];
			for (uint8_t i = 0; i < 1 + s; i++)
				c[i] = ctx[i];
			c[0] = s;

			if (open_file(filepath + 1, c, ctx, stack, stack_top))
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