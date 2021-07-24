#include "StrMap.hpp"

namespace StrMap {

bool BlockGroup::resolve_u16(const char *str, uint16_t payload_size, uint16_t *res)
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

const char* BlockGroup::resolve_u8(const char *str)
{
	auto i = resolve_node(str);
	auto cur = m_root[i];
	if (*str == 0 && cur.control & Block::Control::has_payload)
		return reinterpret_cast<const char*>(m_root + i + 3);
	else
		return nullptr;
}

bool BlockGroup::insert_u16(const char *str, uint16_t payload_size, const uint16_t *payload)
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

}