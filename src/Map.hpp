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
	inline char* search_node(char *cur, char c)
	{
		while (true) {	// search node
			if (cur[3] == c)
				return cur;
			auto n = ::load_part<3, uint32_t>(cur);
			if (n == 0)
				return nullptr;
			cur = m_buffer + n;
		}
	}

	inline bool search_node_ins(char *cur, char c, char *&res)
	{
		while (true) {	// search node
			if (cur[3] == c) {
				res = cur;
				return true;
			}
			auto n = ::load_part<3, uint32_t>(cur);
			if (n == 0) {
				res = cur;
				return false;
			}
			cur = m_buffer + n;
		}
	}

	inline bool walk_through_node(char *cur, char *&s, const char *&str)
	{
		s = cur + 4;
		while (*str == *s) {
			str++;
			s++;
		}
		return *s < 32;
	}

	inline char* nter_next(const char *nter)
	{
		return m_buffer + ::load_part<3, uint32_t>(nter + 1);
	}

	inline char* get_payload(char *nter, bool has_next)
	{
		if (*nter & Attr::has_payload) {
			auto res = nter + (has_next ? 4 : 1);
			if (*nter & Attr::payload_ind)
				res = m_buffer + ::load_part<3, uint32_t>(res);
			return res;
		} else	// node has no payload, just a splitting point
			return nullptr;
	}

	struct Attr
	{
		static inline constexpr char has_payload = 0x01;
		static inline constexpr char has_next = 0x02;
		static inline constexpr char payload_ind = 0x04;
	};

public:
	inline bool insert(uint32_t root, const char *str)
	{
		char *cur = m_buffer + root;
		char *last = cur;
		while (true) {
			if (!search_node_ins(cur, *str, cur)) {	// no node match, insert at cur (cleanest, no cost)
				::store_part<3>(cur, m_size);
				alloc(3);
				store_part<3>(0);
				while (*str) {
					alloc(2);
					store(*str++);
				}
				store(static_cast<char>(Attr::has_payload));
				return true;
			}
			str++;
			char *s;
			if (!walk_through_node(cur, s, str))
				return false;	// no match in inline
			bool has_next = *s & Attr::has_next;
			if (*str == 0) {
				auto p = get_payload(s, has_next);
				return false;
			}
			if (has_next)
				cur = nter_next(s);
			else
				return false;
		}
	}

	template <typename T>
	inline bool insert(uint32_t root, const char *str, const T &v)
	{
		if (!insert(root, str))
			return false;
		alloc(sizeof(T));
		store(v);
		return true;
	}

	inline char* resolve(uint32_t root, const char *str)
	{
		char *cur = m_buffer + root;
		while (true) {
			cur = search_node(cur, *str++);
			if (cur == nullptr)
				return nullptr;	// no node match
			char *s;
			if (!walk_through_node(cur, s, str))
				return nullptr;	// no match in inline
			bool has_next = *s & Attr::has_next;
			if (*str == 0)
				return get_payload(s, has_next);
			if (has_next)
				cur = nter_next(s);
			else	// no sub child
				return nullptr;
		}
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