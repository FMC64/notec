#include "cint.hpp"
#include "Pp.hpp"
#include <fxlib.h>

int main(void)
{
	Pp p;
	catch (p.get_tstream()) {
		auto err = p.get_tstream().get_error();
		locate(1, 1);
		if (err[0] <= Token::type_range) {
			token_nter(errnt, err);
			Print(reinterpret_cast<const uint8_t*>(errnt));
		} else
			Print(reinterpret_cast<const uint8_t*>(err));
		while (1) {
			unsigned int key;
			GetKey(&key);
		}
		return 1;
	}
	while (auto t = p.next());
	locate(1, 1);
	Print(reinterpret_cast<const uint8_t*>("NOTE-C - no error"));
	while (1) {
		unsigned int key;
		GetKey(&key);
	}
	return 1;
}