#include "Pp.hpp"

char* Pp::next(void)
{
	while (true) {
		auto n = next_token();
		while (true) {
			if (n == nullptr)
				return nullptr;
			if (Token::is_op(n, Token::Op::Sharp))
				n = directive();
			else
				return n;
		}
	}
}