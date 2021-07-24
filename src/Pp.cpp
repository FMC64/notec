#include "Pp.hpp"

char* Pp::next_token(void)
{
	return _next_token();
}

char* Pp::next_token_include(void)
{
	return _next_token<true>();
}

bool Pp::next_token_dir(char* &res)
{
	return _next_token_dir(res);
}

bool Pp::next_token_dir_include(char* &res)
{
	return _next_token_dir<true>(res);
}

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