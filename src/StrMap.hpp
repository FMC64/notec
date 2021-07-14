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
	};
};

static_assert(sizeof(Block) == sizeof(uint16_t), "Block must be 2 bytes long");

class BlockReserve;

class BlockGroup
{
	Block *m_root;
	BlockReserve &m_reserve;

	inline uint16_t end(void) const;

	inline uint16_t resolve_node(const char *&str)
	{
		uint16_t cur_i = 0;
		while (true) {
			auto cur = m_root[cur_i];
			if (cur.c == *str) {	// visit next child
				str++;
				if (cur.control & Block::Control::next_child_direct)
					cur_i++;
				else if (cur.control & Block::Control::has_next_child)
					cur_i = *reinterpret_cast<const uint16_t*>(m_root + cur_i + 1);
				else
					return cur_i;
				if (*str == 0)
					return cur_i;
			} else {	// visit next entry
				if (cur.control & Block::Control::has_next_entry)
					cur_i = *reinterpret_cast<const uint16_t*>(m_root + cur_i + 2);
				else
					return cur_i;
			}
		}
	}

public:
	inline BlockGroup(Block *root, BlockReserve &reserve) :
		m_root(root),
		m_reserve(reserve)
	{
	}

	template <typename T>
	inline bool resolve(const char *str, T &res)
	{
		static_assert(sizeof(T) == sizeof(uint16_t), "T size must be 2");
		auto i = resolve_node(str);
		auto cur = m_root[i];
		if (*str == 0 && cur.control & Block::Control::has_payload) {
			res = *reinterpret_cast<const T*>(m_root + i + 3);
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

	inline void add_blocks(size_t count)
	{
		auto needed = m_count + count;
		if (needed > m_allocated) {
			m_allocated *= 2;
			if (m_allocated < needed)
				m_allocated = needed;
			m_buffer = reinterpret_cast<Block*>(std::realloc(m_buffer, m_allocated * sizeof(Block)));
		}
	}

	inline void add_block(const Block &value)
	{
		m_buffer[m_count++] = value;
	}

	inline void add_block(uint16_t value)
	{
		*reinterpret_cast<uint16_t*>(&m_buffer[m_count++]) = value;
	}

	inline uint16_t end(void)
	{
		return m_count;
	}

public:
	inline BlockReserve(void)
	{
	}
	inline ~BlockReserve(void)
	{
		std::free(m_buffer);
	}

	inline BlockGroup allocate(void)
	{
		add_blocks(3);
		auto n = &m_buffer[m_count++];
		m_count += 2;
		n->c = 0x7F;
		n->control = 0;
		return BlockGroup(n, *this);
	}
};

inline uint16_t BlockGroup::end(void) const
{
	return &m_reserve.m_buffer[m_reserve.end()] - m_root;
}

template <typename T>
inline bool BlockGroup::insert(const char *str, const T &payload)
{
	auto ind = resolve_node(str);
	auto cur = m_root[ind];
	if (*str == 0 && cur.control & Block::Control::has_payload)
		return false;

	constexpr uint16_t payload_size = 1;//sizeof(T) / sizeof(uint16_t);
	static_assert(sizeof(T) == sizeof(uint16_t), "T size must be 2");
	auto blk_size = [this](uint16_t ndx) -> uint16_t {
		auto cur = m_root[ndx];
		return (cur.control & Block::Control::next_child_direct ? 1 : 3) +
		(cur.control & Block::Control::has_payload ? payload_size : 0);
	};
	uint16_t entry_size;
	uint16_t needed = *str == 0 ? 4 : 3;
	if (cur.control & Block::Control::next_child_direct) {	// inline child
		auto nind = ind + 1;
		auto nsize = blk_size(nind);
		auto next = m_root[nind];
		while ((1 + nsize) < needed && next.control & Block::Control::next_child_direct) {	// gather as much inline children as we can and need
			next = m_root[nind + nsize];
			nsize += blk_size(nind + nsize);
		}
		entry_size = 1 + nsize;	// this is how much space we can work with from current node

		auto reloc = end();
		m_reserve.add_blocks(nsize);
		for (size_t i = 0; i < nsize; i++)	// relocate children at end to make room on current node
			m_reserve.add_block(m_root[nind + i]);
		if (next.control & Block::Control::next_child_direct) {
			m_root[end() - 1].control &= ~Block::Control::next_child_direct;
			m_reserve.add_blocks(2);
			m_reserve.add_block(nind + nsize);
			m_reserve.add_block(0);
		}

		reinterpret_cast<uint16_t&>(m_root[ind + 1]) = reloc;	// set relocated child index in current node
		m_root[ind].control &= ~Block::Control::next_child_direct;
	} else {
		entry_size = 3;	// we have some space but no room for any payload
	}

	// at this point we have at least 3 blocks available at ind but possibly no room for payload
	if (entry_size < needed) {
		auto new_c = end();
		m_reserve.add_blocks(4);
		for (size_t i = 0; i < 3; i++)
			m_reserve.add_block(m_root[ind + i]);
		m_root[ind].c = 0x7F;	// insert dummy block with current block as next entry
		m_root[ind].control = Block::Control::has_next_entry;
		reinterpret_cast<uint16_t&>(m_root[ind + 2]) = new_c;
		ind = new_c;
	}
	// at this point we got all the space we need at ind
	if (*str != 0) {
		m_root[ind].control |= Block::Control::has_next_entry;	// finally add next entry
		reinterpret_cast<uint16_t&>(m_root[ind + 2]) = end();

		while (*str != 0) {
			m_reserve.add_blocks(1);
			Block blk{*str, Block::Control::has_next_child | Block::Control::next_child_direct};
			m_reserve.add_block(blk);
			str++;
		}
		ind = end() - 1;
		m_root[ind].control &= ~(Block::Control::has_next_child | Block::Control::next_child_direct);
		m_reserve.add_blocks(3);
		for (size_t i = 0; i < 3; i++)
			m_reserve.add_block(0);
	}
	m_root[ind].control |= Block::Control::has_payload;
	m_root[ind + 3] = reinterpret_cast<const Block&>(payload);

	return true;
}

}