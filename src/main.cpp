#include <fxlib.h>

int main(void)
{
	locate(1, 1);
	Print((unsigned char*)"notec, NOTE-C but better!! :D");
	while (1) {
		unsigned int key;
		GetKey(&key);
	}
	return 1;
}