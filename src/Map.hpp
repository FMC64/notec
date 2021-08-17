#include "clib.hpp"
#include "arith.hpp"

class Map
{
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

	// implicit copy to end, referencing node payload if any
	// ow_ptr contains index to node, will be replaced with index to copy
	// returns ptr to copy nter
	// if force_next_at_end, nter must not have has_next set
	inline char* copy_node(uint32_t node, uint32_t node_nter, char nter, bool force_next_at_end, char *ow_ptr)
	{
		::store_part<3>(ow_ptr, static_cast<uint32_t>(m_size));

		//{
			uint32_t pndx;
			bool hasp = nter & Attr::has_payload;
			if (hasp)
				pndx = static_cast<uint32_t>(get_payload(m_buffer + node_nter, nter & Attr::has_next) - m_buffer);
		//}

		auto has_next = nter & Attr::has_next;
		alloc(node_nter - node + 1 + (has_next ? 3 : 0));
		while (node != node_nter)
			store(m_buffer[node++]);
		auto res = m_size;
		store(static_cast<char>(has_next | (force_next_at_end ? Attr::has_next : 0)));
		if (has_next) {
			node_nter++;
			for (size_t i = 0; i < 3; i++)
				store(m_buffer[node_nter++]);
		}

		{
			if (hasp)
				m_buffer[res] |= Attr::has_payload | Attr::payload_ind;
			uint32_t esize = (force_next_at_end ? 3 : 0) + (hasp ? 3 : 0);
			alloc(esize);
			if (force_next_at_end)
				store_part<3>(m_size + esize);
			if (hasp)
				store_part<3>(pndx);
		}

		return m_buffer + res;
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
					auto base = m_size;
					alloc(3);
					store_part<3>(static_cast<uint32_t>(0));
					char ow_ptr[3];
					copy_node(mid_nter, nter, m_buffer[nter], false, ow_ptr);
					auto n = copy_node(node, mid_nter, 0, false, m_buffer + ilast);
					*n |= Attr::has_next | Attr::has_payload;
					alloc(3);
					store_part<3>(static_cast<uint32_t>(base));
				} else {
					auto n = copy_node(node, mid_nter, 0, false, last);
					*n |= Attr::has_next;
					alloc(3);
					store_part<3>(static_cast<uint32_t>(m_size + 3));
					auto fptr = m_size;
					m_size += 3;
					char ow_ptr[3];
					copy_node(mid_nter, nter, m_buffer[nter], false, ow_ptr);
					::store_part<3>(m_buffer + fptr, static_cast<uint32_t>(m_size));
					insert_str_payload_node(str);
				}
				return true;
			}
			bool has_next = *s & Attr::has_next;
			if (*str == 0) {
				// bad case, node must be fully copied to add payload after it
				if (*s & Attr::has_payload)
					return false;	// already a payload on such node
				*copy_node(cur - m_buffer, s - m_buffer, *s, false, last) |= Attr::has_payload;
				return true;
			}
			if (has_next) {
				last = cur;
				cur = nter_next(s);
			} else {	// bad case, discard cur and copy current node at end with reference to current node payload if any
				copy_node(cur - m_buffer, s - m_buffer, *s, true, last);
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
};