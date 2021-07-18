#include "Pp.hpp"

char* Pp::next(void)
{
	while (true) {
		auto n = next_token();
		if (n == nullptr)
			return nullptr;
		if (Token::is_op(n, Token::Op::Sharp))
			return directive();
		else
			return n;
	}
}