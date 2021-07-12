#pragma once

#include "cint.hpp"

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

static inline uint16_t resolve_node(const Block *root, const char *&str)
{
	uint16_t cur_i = 0;
	while (true) {
		auto cur = root[cur_i];
		if (cur.c == *str) {	// visit next child
			str++;
			if (cur.control & Block::Control::has_next_child) {
				if (cur.control & Block::Control::next_child_indirect) {
					cur_i = *reinterpret_cast<const uint16_t*>(root + cur_i + 1);
				} else
					cur_i++;
			} else
				return cur_i;
		} else {	// visit next entry
			if (*str == 0)
				return cur_i;
			if (cur.control & Block::Control::has_next_entry) {
				cur_i = *reinterpret_cast<const uint16_t*>(root + cur_i +
					(cur.control & Block::Control::next_child_indirect ? 1 : 0) +
					1);
			} else {
				return cur_i;
			}
		}
	}
}

template <typename T>
static inline bool resolve(const Block *root, const char *str, T &res)
{
	static_assert(sizeof(T) % sizeof(uint16_t) == 0, "sizeof(T) must be a multiple of two");
	auto i = resolve_node(root, str);
	auto cur = root[i];
	if (*str == 0 && cur.control & Block::Control::has_payload) {
		auto src = reinterpret_cast<const uint16_t*>(root + i +
					(cur.control & Block::Control::next_child_indirect ? 1 : 0) +
					(cur.control & Block::Control::has_next_entry ? 1 : 0));
		auto dst = reinterpret_cast<uint16_t*>(&res);
		for (size_t i = 0; i < sizeof(T) / sizeof(uint16_t); i++)
			dst[i] = src[i];
		return true;
	} else
		return false;
}

}