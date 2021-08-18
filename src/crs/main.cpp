#include "cint.hpp"
#include "Cpp.hpp"
#include "sref.hpp"
#include <fxlib.h>

extern "C" {

void strncmp(void)
{
	tactical_exit("dummy strncmp actually called", -1);
}

}

static sref<Cpp> cpp_ref;

static void on_terminate(void)
{
	cpp_ref.destroy();
}

void display_pressure(const Cpp::Pressure &pres)
{
	tactical_exit("TOTAL PRESSURE", pres.total(), false);
	tactical_exit("buffer", pres.buffer, false);
	tactical_exit("macros", pres.macros, false);
	Bdisp_AllClr_DDVRAM();
}

int main(void)
{
	SetQuitHandler(on_terminate);

	Cpp c;
	auto cc = cpp_ref.capture(c);
	catch (c.err_src()) {
		display_pressure(c.get_pressure());
		auto err = c.err_src().get_error();
		{
			auto tc = c.err_src().reopen();
			auto o = c.err_src().get_off();
			size_t d = 10;
			size_t car = d;
			if (o < d) {
				car = d - o;
				o = 0;
			} else
				o -= d;
			auto tr = c.err_src().get_stream().seek(o);
			tr = 21;
			char buf[tr + 1];
			buf[tr] = 0;
			c.err_src().get_stream().read(buf, tr);
			if (tc)
				c.err_src().get_stream().close();
			locate(1, 1);
			Print(reinterpret_cast<const uint8_t*>(buf));
			locate(1 + car, 2);
			Print(reinterpret_cast<const uint8_t*>("^"));
		}
		locate(1, 3);
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