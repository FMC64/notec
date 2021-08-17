#include "clib.hpp"
#include "arith.hpp"

class Map
{
	size_t m_size = 0;
	size_t m_allocated = 0;
	char *m_buffer = nullptr;

public:
	inline void alloc(size_t size)
	{
		auto needed = m_size + size;
		if (m_allocated < needed) {
			m_allocated *= 2;
			if (m_allocated < needed)
				m_allocated = needed;
			m_buffer = reinterpret_cast<char*>(realloc(m_buffer, m_allocated));
		}
	}

	template <typename T>
	inline void store(const T &v)
	{
		m_size += ::store(m_buffer + m_size, v);
	}

	template <size_t Size, typename T>
	inline void store_part(const T &v)
	{
		m_size += ::store_part<Size>(m_buffer + m_size, v);
	}

	inline uint32_t create_root(void)
	{
		auto res = m_size;
		alloc(4);
		store_part<3>(0);
		store(static_cast<uint8_t>(0xFF));
		return res;
	}

private:


public:
	inline void insert(uint32_t root, const char *str)
	{
	}

	template <typename T>
	inline void insert(uint32_t root, const char *str, const T &v)
	{
		insert(root, str);
		alloc(sizeof(T));
		store(v);
	}

	inline char* resolve(uint32_t root, const char *str)
	{
		return nullptr;
	}

	template <typename T>
	inline bool resolve(uint32_t root, const char *str, T &v)
	{
		auto p = resolve(root, str);
		if (p == nullptr)
			return false;
		v = ::load<T>(reinterpret_cast<char*>(&v));
		return true;
	}
};