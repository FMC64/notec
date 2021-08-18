#include "clib.hpp"
#include "arith.hpp"

class Map
{
protected:
	size_t m_size = 0;
	size_t m_allocated = 0;
	char *m_buffer = nullptr;

public:
	~Map(void)
	{
		free(m_buffer);
	}

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
		store(static_cast<uint8_t>(0x7F));
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

	inline bool search_node_ins(char *cur, char c, char *&res, char *&last)
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
			last = cur;
			cur = m_buffer + n;
		}
	}

	inline bool walk_through_node(char *cur, char *&s, const char *&str)
	{
		s = cur + 4;
		while (*str == *s && *str) {
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

	inline size_t copy_node_size(uint32_t node, uint32_t node_nter, char nter, bool force_next_at_end)
	{
		bool has_next = nter & Attr::has_next;
		bool hasp = nter & Attr::has_payload;
		size_t res = node_nter - node + 1 + (has_next || force_next_at_end ? 3 : 0) + (hasp ? 3 : 0);
		return res;
	}

	inline char* copy_node_alloc(size_t size)
	{
		auto res = m_size;
		alloc(size);
		m_size += size;
		return m_buffer + res;
	}

	static inline constexpr size_t base_recycl = 5;
	static inline constexpr size_t top_recycl = 20;
	static inline constexpr size_t recycl_size = top_recycl - base_recycl;
	uint32_t m_recycl[recycl_size] = {};

	static inline uint32_t node_size(const char *node, const char *nter)
	{
		auto has_next = *nter & Attr::has_next;
		return nter - node + 1 + (has_next ? 3 : 0);
	}

	inline void add_recycl(uint32_t node, uint32_t node_nter)
	{
		auto size = node_size(m_buffer + node, m_buffer + node_nter);
		uint32_t *c;
		if (size < top_recycl)
			c = m_recycl + (size - base_recycl);
		else
			c = m_recycl + (recycl_size - 1);
		for (size_t i = 0; i < 6; i++) {
			if (*c == 0) {
				*c = node;
				break;
			}
			if (c == m_recycl)
				break;
			c--;
		}
	}

	inline char* copy_node_alloc_recycl(size_t size)
	{
		auto c = m_recycl + (size - base_recycl);
		for (size_t i = 0; (c < m_recycl + recycl_size) && i < 6; i++) {
			if (*c) {
				auto r = m_buffer + *c;
				*c = 0;
				return r;
			}
			c++;
		}
		return copy_node_alloc(size);
	}

	// implicit copy to end, referencing node payload if any
	// ow_ptr contains index to node, will be replaced with index to copy
	// returns ptr to copy nter
	// if force_next_at_end, nter must not have has_next set
	inline char* copy_node(char *dst, uint32_t node, uint32_t node_nter, char nter, bool force_next_at_end)
	{
		//{
			uint32_t pndx;
			bool hasp = nter & Attr::has_payload;
			if (hasp)
				pndx = static_cast<uint32_t>(get_payload(m_buffer + node_nter, nter & Attr::has_next) - m_buffer);
		//}

		auto has_next = nter & Attr::has_next;
		while (node != node_nter)
			dst += ::store(dst, m_buffer[node++]);
		auto res = dst;
		dst += ::store(dst, static_cast<char>(has_next | (force_next_at_end ? Attr::has_next : 0)));
		if (has_next) {
			node_nter++;
			for (size_t i = 0; i < 3; i++)
				dst += ::store(dst, m_buffer[node_nter++]);
		}

		{
			if (hasp)
				*res |= Attr::has_payload | Attr::payload_ind;
			if (force_next_at_end)
				dst += ::store_part<3>(dst, m_size);
			if (hasp)
				dst += ::store_part<3>(dst, pndx);
		}

		return res;
	}

	inline void insert_str_payload_node(const char *str)
	{
		alloc(3);
		store_part<3>(0);
		while (*str) {
			alloc(2);
			store(*str++);
		}
		store(static_cast<char>(Attr::has_payload));
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
			if (!search_node_ins(cur, *str, cur, last)) {	// no node match, insert at cur (cleanest, no cost)
				::store_part<3>(cur, m_size);
				insert_str_payload_node(str);
				return true;
			}
			str++;
			char *s;
			if (!walk_through_node(cur, s, str)) {
				// no match in inline
				bool dirp = *str == 0;
				uint32_t node = cur - m_buffer;
				uint32_t mid_nter = s - m_buffer;
				while (*s >= 32)
					s++;
				uint32_t nter = s - m_buffer;
				if (dirp) {
					uint32_t ilast = last - m_buffer;
					uint32_t base;
					{
						auto size = copy_node_size(mid_nter, nter, m_buffer[nter], false);
						auto blk = copy_node_alloc_recycl(size + 3);
						base = blk - m_buffer;
						blk += ::store_part<3>(blk, static_cast<uint32_t>(0));
						copy_node(blk, mid_nter, nter, m_buffer[nter], false);
					}
					{
						::store_part<3>(m_buffer + ilast, static_cast<uint32_t>(m_size));
						auto size = copy_node_size(node, mid_nter, 0, false);
						auto blk = copy_node_alloc(size + 3);
						auto n = copy_node(blk, node, mid_nter, 0, false);
						*n |= Attr::has_next | Attr::has_payload;
						::store_part<3>(n + 1, static_cast<uint32_t>(base));
					}
				} else {
					{
						uint32_t ilast = last - m_buffer;
						auto size = copy_node_size(node, mid_nter, 0, false);
						auto blk = copy_node_alloc_recycl(size + 3);
						::store_part<3>(m_buffer + ilast, static_cast<uint32_t>(blk - m_buffer));
						auto n = copy_node(blk, node, mid_nter, 0, false);
						*n |= Attr::has_next;
						::store_part<3>(n + 1, static_cast<uint32_t>(m_size));
					}
					auto fptr = m_size;
					{
						auto size = copy_node_size(mid_nter, nter, m_buffer[nter], false);
						auto blk = copy_node_alloc(size + 3);
						blk += 3;
						copy_node(blk, mid_nter, nter, m_buffer[nter], false);
					}
					::store_part<3>(m_buffer + fptr, static_cast<uint32_t>(m_size));
					insert_str_payload_node(str);
				}
				add_recycl(node, nter);
				return true;
			}
			bool has_next = *s & Attr::has_next;
			if (*str == 0) {
				// bad case, node must be fully copied to add payload after it
				if (*s & Attr::has_payload)
					return false;	// already a payload on such node
				::store_part<3>(last, static_cast<uint32_t>(m_size));
				uint32_t node = cur - m_buffer;
				uint32_t nter = s - m_buffer;
				add_recycl(node, nter);
				auto size = copy_node_size(node, nter, m_buffer[nter], false);
				auto blk = copy_node_alloc(size);
				*copy_node(blk, node, nter, m_buffer[nter], false) |= Attr::has_payload;
				return true;
			}
			if (has_next) {
				last = s + 1;
				cur = nter_next(s);
			} else {	// bad case, discard cur and copy current node at end with reference to current node payload if any
				::store_part<3>(last, static_cast<uint32_t>(m_size));
				uint32_t node = cur - m_buffer;
				uint32_t nter = s - m_buffer;
				add_recycl(node, nter);
				auto size = copy_node_size(node, nter, m_buffer[nter], true);
				auto blk = copy_node_alloc(size);
				copy_node(blk, node, nter, m_buffer[nter], true);
				insert_str_payload_node(str);
				return true;
			}
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
		v = ::load<T>(p);
		return true;
	}

	inline bool remove(uint32_t root, const char *str)
	{
		char *cur = m_buffer + root;
		while (true) {
			cur = search_node(cur, *str++);
			if (cur == nullptr)
				return false;	// no node match
			char *s;
			if (!walk_through_node(cur, s, str))
				return false;	// no match in inline
			bool has_next = *s & Attr::has_next;
			if (*str == 0) {
				*s &= ~(Attr::has_payload | Attr::payload_ind);
				return true;
			}
			if (has_next)
				cur = nter_next(s);
			else	// no sub child
				return false;
		}
	}
};