#pragma once

#include "cint.hpp"
#include "clib.hpp"

namespace StrMap {

struct Block
{
	char c;

	/*
	By default, next child is right after current block
	control flag 0x01 - next child at all ?
	control flag 0x02 - next block is next child
	control flag 0x04 - next entry at all ?
	control flag 0x08 - have payload, contained in N next blocks
	All flags are compatible with each other, ordering of next blocks will be
	0 - current block
	1 - next child
	2 - next entry
	3 - payload
	*/
	char control;

	struct Control {
		static inline constexpr char has_next_child = 0x01;
		static inline constexpr char next_child_direct = 0x02;
		static inline constexpr char has_next_entry = 0x04;
		static inline constexpr char has_payload = 0x08;
		static inline constexpr char has_payload_storage = 0x10;
	};
};

static_assert(sizeof(Block) == sizeof(uint16_t), "Block must be 2 bytes long");

class BlockGroup
{
	Block *m_root = nullptr;
	size_t m_count = 0;
	size_t m_allocated = 0;

	inline uint16_t end(void) const
	{
		return m_count;
	}

	inline uint16_t resolve_node(uint16_t root, const char *&str, bool *stopped_out_of_children = nullptr)
	{
		uint16_t cur_i = root;
		while (true) {
			auto cur = m_root[cur_i];
			if (cur.c == *str) {	// visit next child
				str++;
				if (*str == 0)
					return cur_i;
				if (cur.control & Block::Control::next_child_direct)
					cur_i++;
				else if (cur.control & Block::Control::has_next_child)
					cur_i = *reinterpret_cast<const uint16_t*>(m_root + cur_i + 1);
				else {
					if (stopped_out_of_children)
						*stopped_out_of_children = true;
					return cur_i;
				}
			} else {	// visit next entry
				if (cur.control & Block::Control::has_next_entry)
					cur_i = *reinterpret_cast<const uint16_t*>(m_root + cur_i + 2);
				else {
					if (stopped_out_of_children)
						*stopped_out_of_children = false;
					return cur_i;
				}
			}
		}
	}

	bool resolve_u16(uint16_t root, const char *str, uint16_t payload_size, uint16_t *res);

	const char* resolve_u8(uint16_t root, const char *str);

	bool insert_u16(uint16_t root, const char *str, uint16_t payload_size, const uint16_t *payload);

	template <typename T>
	static constexpr uint16_t get_payload_size(void)
	{
		if (sizeof(T) % 2 == 0)
			return sizeof(T) / 2;
		else
			return sizeof(T) / 2 + 1;
	}

public:
	inline uint16_t* resolve_mut_u16(uint16_t root, const char *str)
	{
		return reinterpret_cast<uint16_t*>(const_cast<char*>(resolve_u8(root, str)));
	}

	inline BlockGroup(void)
	{
	}
	inline ~BlockGroup(void)
	{
		free(m_root);
	}

	inline bool resolve(uint16_t root, const char *str)
	{
		return resolve_u16(root, str, 0, nullptr);
	}

	template <typename T>
	inline bool resolve(uint16_t root, const char *str, T &res)
	{
		if constexpr (sizeof(T) % 2 == 0)
			return resolve_u16(root, str, get_payload_size<T>(), &reinterpret_cast<uint16_t&>(res));
		else {
			auto c = resolve_u8(root, str);
			if (c == nullptr)
				return false;
			for (size_t i = 0; i < sizeof(T); i++)
				reinterpret_cast<char*>(&res)[i] = c[i];
			return true;
		}
	}

	inline bool insert(uint16_t root, const char *str)
	{
		return insert_u16(root, str, 0, nullptr);
	}

	template <typename T>
	inline bool insert(uint16_t root, const char *str, const T &payload)
	{
		if constexpr (sizeof(T) % 2 == 0)
			return insert_u16(root, str, get_payload_size<T>(), &reinterpret_cast<const uint16_t&>(payload));
		else {
			uint16_t payload16[get_payload_size<T>()];
			for (size_t i = 0; i < sizeof(T); i++)
				reinterpret_cast<char*>(payload16)[i] = reinterpret_cast<const char*>(&payload)[i];
			return insert_u16(root, str, get_payload_size<T>(), payload16);
		}
	}

	inline bool remove(uint16_t root, const char *str)
	{
		auto n = resolve_node(root, str);
		if (*str == 0) {
			m_root[n].control &= ~Block::Control::has_payload;
			return true;
		}
		return false;
	}

	inline uint16_t alloc(void)
	{
		add_blocks(3);
		auto r = m_count;
		auto n = &m_root[m_count++];
		m_count += 2;
		n->c = 0x7F;
		n->control = 0;
		return r;
	}

	size_t get_count(void) const
	{
		return m_count;
	}

	size_t get_allocated(void) const
	{
		return m_allocated;
	}

private:
	inline void add_blocks(size_t count)
	{
		auto needed = m_count + count;
		if (needed > m_allocated) {
			m_allocated *= 2;
			if (m_allocated < needed)
				m_allocated = needed;
			m_root = reinterpret_cast<Block*>(realloc(m_root, m_allocated * sizeof(Block)));
		}
	}

	inline void add_block(const Block &value)
	{
		m_root[m_count++] = value;
	}

	inline void add_block(uint16_t value)
	{
		*reinterpret_cast<uint16_t*>(&m_root[m_count++]) = value;
	}

	inline uint16_t end(void)
	{
		return m_count;
	}
};

}