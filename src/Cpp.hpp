#pragma once

#include "Pp.hpp"

class Cpp
{
	Pp m_pp;

	inline void error(const char *str)
	{
		m_pp.error(str);
	}

public:
	inline auto& err_src(void)
	{
		return m_pp.get_tstream();
	}

	inline void include_dir(const char *path)
	{
		m_pp.include_dir(path);
	}

	inline void open(const char *path)
	{
		m_pp.open(path);
	}

	inline void run(void)
	{
		while (m_pp.next());
	}
};