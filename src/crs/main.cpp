#include "cint.hpp"
#include "Cpp.hpp"
#include <fxlib.h>

extern "C" {

void strncmp(void)
{
	tactical_exit("dummy strncmp actually called", -1);
}

}

int main(void)
{
	Cpp c;
	catch (c.err_src()) {
		auto err = c.err_src().get_error();
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
	c.include_dir(make_cstr("/crd0:"));	// first dir is system include
	c.include_dir(make_cstr("/crd0:src"));
	c.open(make_tstr(Token::Type::StringLiteral, "fxlib.h"));
	c.run();
	locate(1, 1);
	Print(reinterpret_cast<const uint8_t*>("NOTE-C - no error"));
	while (1) {
		unsigned int key;
		GetKey(&key);
	}
	return 1;
}