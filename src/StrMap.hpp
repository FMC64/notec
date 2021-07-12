#pragma once

#include "cint.hpp"

namespace StrMap {

struct Block
{
	char c;

	/*
	By default, next child is right after current block
	control flag 0x01 - next child at all ?
	control flag 0x02 - next block conttains index to next child
	control flag 0x04 - next entry at all ?
	control flag 0x08 - have payload, contained in two next blocks
	All flags are compatible with each other, ordering of next blocks will be
	0 - current block
	1 - next child
	2 - next entry
	3 - payload
	*/
	char control;
};

}