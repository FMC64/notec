#include <fxlib.h>
#include "cint.hpp"

int main(void)
{
	locate(1, 1);
	Print(reinterpret_cast<const uint8_t*>("NOTE-C"));
	while (1) {
		unsigned int key;
		GetKey(&key);
	}
	return 1;
}