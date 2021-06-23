#include "cint.hpp"
#include "Tokenizer.hpp"
#include <fxlib.h>

int main(void)
{
	Tokenizer toks(*(Stream*)(nullptr));
	while (auto t = toks.next());
	locate(1, 1);
	Print(reinterpret_cast<const uint8_t*>("NOTE-C"));
	while (1) {
		unsigned int key;
		GetKey(&key);
	}
	return 1;
}