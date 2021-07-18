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

	inline uint16_t resolve_node(const char *&str, bool *stopped_out_of_children = nullptr)
	{
		uint16_t cur_i = 0;
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

	inline bool resolve_u16(const char *str, uint16_t payload_size, uint16_t *res)
	{
		auto i = resolve_node(str);
		auto cur = m_root[i];
		if (*str == 0 && cur.control & Block::Control::has_payload) {
			for (uint16_t j = 0; j < payload_size; j++)
				res[j] = *reinterpret_cast<const uint16_t*>(m_root + i + 3 + j);
			return true;
		} else
			return false;
	}

	inline const char* resolve_u8(const char *str)
	{
		auto i = resolve_node(str);
		auto cur = m_root[i];
		if (*str == 0 && cur.control & Block::Control::has_payload)
			return reinterpret_cast<const char*>(m_root + i + 3);
		else
			return nullptr;
	}

	inline bool insert_u16(const char *str, uint16_t payload_size, const uint16_t *payload)
	{
		bool is_child;
		auto ind = resolve_node(str, &is_child);
		auto cur = m_root[ind];
		if (*str == 0 && cur.control & Block::Control::has_payload)
			return false;

		auto blk_size = [this, payload_size](uint16_t ndx) -> uint16_t {
			auto cur = m_root[ndx];
			return (cur.control & Block::Control::next_child_direct ? 1 : 3) +
			(cur.control & Block::Control::has_payload ? payload_size : 0);
		};
		uint16_t entry_size;
		uint16_t needed = 3 + (*str == 0 ? payload_size : 0);
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
			add_blocks(nsize);
			for (size_t i = 0; i < nsize; i++)	// relocate children at end to make room on current node
				add_block(m_root[nind + i]);
			if (next.control & Block::Control::next_child_direct) {
				m_root[end() - 1].control &= ~Block::Control::next_child_direct;
				add_blocks(2);
				add_block(nind + nsize);
				m_count++;
			}

			reinterpret_cast<uint16_t&>(m_root[ind + 1]) = reloc;	// set relocated child index in current node
			m_root[ind].control &= ~Block::Control::next_child_direct;
		} else {
			entry_size = 3;	// we have some space but no room for any payload
		}

		// at this point we have at least 3 blocks available at ind but possibly no room for payload
		if (entry_size < needed) {
			auto new_c = end();
			add_blocks(3 + payload_size);
			for (size_t i = 0; i < 3; i++)
				add_block(m_root[ind + i]);
			m_count += payload_size;
			m_root[ind].c = 0x7F;	// insert dummy block with current block as next entry
			m_root[ind].control = Block::Control::has_next_entry;
			reinterpret_cast<uint16_t&>(m_root[ind + 2]) = new_c;
			ind = new_c;
		}
		// at this point we got all the space we need at ind
		if (*str != 0) {
			// finally add next entry
			if (is_child) {
				m_root[ind].control |= Block::Control::has_next_child;
				reinterpret_cast<uint16_t&>(m_root[ind + 1]) = end();
			} else {
				m_root[ind].control |= Block::Control::has_next_entry;
				reinterpret_cast<uint16_t&>(m_root[ind + 2]) = end();
			}

			while (*str != 0) {
				add_blocks(1);
				Block blk{*str, Block::Control::has_next_child | Block::Control::next_child_direct};
				add_block(blk);
				str++;
			}
			ind = end() - 1;
			m_root[ind].control &= ~(Block::Control::has_next_child | Block::Control::next_child_direct);
			add_blocks(2 + payload_size);
			m_count += 2 + payload_size;
		}
		m_root[ind].control |= Block::Control::has_payload;
		for (uint16_t i = 0; i < payload_size; i++)
			m_root[ind + 3 + i] = reinterpret_cast<const Block&>(payload[i]);

		return true;
	}

	template <typename T>
	static constexpr uint16_t get_payload_size(void)
	{
		if (sizeof(T) % 2 == 0)
			return sizeof(T) / 2;
		else
			return sizeof(T) / 2 + 1;
	}

public:
	inline BlockGroup(void)
	{
		add_blocks(3);
		auto n = &m_root[m_count++];
		m_count += 2;
		n->c = 0x7F;
		n->control = 0;
	}
	inline ~BlockGroup(void)
	{
		free(m_root);
	}

	template <typename T>
	inline bool resolve(const char *str, T &res)
	{
		if constexpr (sizeof(T) % 2 == 0)
			return resolve_u16(str, get_payload_size<T>(), &reinterpret_cast<uint16_t&>(res));
		else {
			auto c = resolve_u8(str);
			if (c == nullptr)
				return false;
			res = *reinterpret_cast<const T*>(c);
			return true;
		}
	}

	template <typename T>
	inline bool insert(const char *str, const T &payload)
	{
		if constexpr (sizeof(T) % 2 == 0)
			return insert_u16(str, get_payload_size<T>(), &reinterpret_cast<const uint16_t&>(payload));
		else {
			uint16_t payload16[get_payload_size<T>()];
			reinterpret_cast<T&>(*payload16) = payload;
			return insert_u16(str, get_payload_size<T>(), reinterpret_cast<const uint16_t*>(payload16));
		}
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