#pragma once

#include "cint.hpp"
#include <cstdlib>

namespace StrMap {

struct Block
{
	char c;

	/*
	By default, next child is right after current block
	control flag 0x01 - next child at all ?
	control flag 0x02 - next block contains index to next child
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
		static inline constexpr char next_child_indirect = 0x02;
		static inline constexpr char has_next_entry = 0x04;
		static inline constexpr char has_payload = 0x08;
	};
};

static_assert(sizeof(Block) == sizeof(uint16_t), "Block must be 2 bytes long");

class BlockReserve;

class BlockGroup
{
	Block *m_root;
	BlockReserve &m_reserve;

	inline uint16_t resolve_node(const char *&str)
	{
		uint16_t cur_i = 0;
		while (true) {
			auto cur = m_root[cur_i];
			if (cur.c == *str) {	// visit next child
				str++;
				if (cur.control & Block::Control::has_next_child) {
					if (cur.control & Block::Control::next_child_indirect) {
						cur_i = *reinterpret_cast<const uint16_t*>(m_root + cur_i + 1);
					} else
						cur_i++;
				} else
					return cur_i;
			} else {	// visit next entry
				if (*str == 0)
					return cur_i;
				if (cur.control & Block::Control::has_next_entry) {
					cur_i = *reinterpret_cast<const uint16_t*>(m_root + cur_i +
						(cur.control & Block::Control::next_child_indirect ? 1 : 0) +
						1);
				} else {
					return cur_i;
				}
			}
		}
	}

public:
	BlockGroup(Block *root, BlockReserve &reserve) :
		m_root(root),
		m_reserve(reserve)
	{
	}

	template <typename T>
	inline bool resolve(const char *str, T &res)
	{
		auto i = resolve_node(str);
		auto cur = m_root[i];
		if (*str == 0 && cur.control & Block::Control::has_payload) {
			res = *reinterpret_cast<const T*>(m_root + i +
						(cur.control & Block::Control::next_child_indirect ? 1 : 0) +
						(cur.control & Block::Control::has_next_entry ? 1 : 0));
			return true;
		} else
			return false;
	}

	template <typename T>
	inline bool insert(const char *str, const T &payload);
};

class BlockReserve
{
	Block *m_buffer = nullptr;
	size_t m_count = 0;
	size_t m_allocated = 0;

	friend class BlockGroup;

	void add_blocks(size_t count)
	{
		auto needed = m_count + count;
		if (needed < m_allocated) {
			m_allocated *= 2;
			if (m_allocated < needed)
				m_allocated = needed;
			m_buffer = reinterpret_cast<Block*>(std::realloc(m_buffer, m_allocated * sizeof(Block)));
		}
	}

public:
	BlockReserve(void)
	{
	}
	~BlockReserve(void)
	{
		std::free(m_buffer);
	}

	BlockGroup allocate(void)
	{
		add_blocks(1);
		auto n = &m_buffer[m_count++];
		n->c = 0x7F;
		n->control = 0;
		return BlockGroup(n, *this);
	}
};

template <typename T>
inline bool BlockGroup::insert(const char *str, const T &payload)
{
	auto i = resolve_node(str);
	auto cur = m_root[i];
	if (*str == 0 && cur.control & Block::Control::has_payload)
		return false;
	// WIP
}

}