#include "cint.hpp"
#include "Token.hpp"
#include <fxlib.h>

int main(void)
{
	Token::Stream toks(*(::Stream*)(nullptr));
	catch (toks) {
		return 1;
	}
	while (auto t = toks.next());
	locate(1, 1);
	Print(reinterpret_cast<const uint8_t*>("NOTE-C"));
	while (1) {
		unsigned int key;
		GetKey(&key);
	}
	return 1;
}