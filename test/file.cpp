#include "crs/File.hpp"
#include "Token.hpp"
#include "test.hpp"

test_case(file_0)
{
	char buf[256];
	test_assert(File::path_absolute(make_cstr("/root"), make_cstr("file"), buf, buf + sizeof buf));
	test_assert(streq(buf, make_cstr("/root/file")));
}

test_case(file_1)
{
	char buf[256];
	test_assert(File::path_absolute(make_cstr("/root"), make_cstr("/file"), buf, buf + sizeof buf));
	test_assert(streq(buf, make_cstr("/file")));
}

test_case(file_2)
{
	char buf[256];
	test_assert(File::path_absolute(make_cstr("/root//"), make_cstr("file"), buf, buf + sizeof buf));
	test_assert(streq(buf, make_cstr("/root/file")));
}

test_case(file_3)
{
	char buf[256];
	test_assert(File::path_absolute(make_cstr("/root//"), make_cstr("./folder/."), buf, buf + sizeof buf));
	test_assert(streq(buf, make_cstr("/root/folder")));
}

test_case(file_4)
{
	char buf[256];
	test_assert(File::path_absolute(make_cstr("/root/"), make_cstr("././././folder/."), buf, buf + sizeof buf));
	test_assert(streq(buf, make_cstr("/root/folder")));
}

test_case(file_5)
{
	char buf[256];
	test_assert(File::path_absolute(nullptr, make_cstr("/root/notec:folder/."), buf, buf + sizeof buf));
	test_assert(streq(buf, make_cstr("/root/notec:folder")));
}


test_case(file_6)
{
	char buf[256];
	test_assert(File::path_absolute(nullptr, make_cstr("/root/notec:folder/.."), buf, buf + sizeof buf));
	test_assert(streq(buf, make_cstr("/root/notec")));
}

test_case(file_7)
{
	char buf[256];
	test_assert(File::path_absolute(nullptr, make_cstr("/root/notec:folder/../..////"), buf, buf + sizeof buf));
	test_assert(streq(buf, make_cstr("/root")));
}

test_case(file_8)
{
	char buf[256];
	test_assert(File::path_absolute(nullptr, make_cstr("/root/notec:folder/../../pj:src.cpp"), buf, buf + sizeof buf));
	test_assert(streq(buf, make_cstr("/root/pj:src.cpp")));
}

test_case(file_9)
{
	char buf[256];
	test_assert(File::path_absolute(nullptr, make_cstr("/root/notec:folder/..:include/stdio.h"), buf, buf + sizeof buf));
	test_assert(streq(buf, make_cstr("/root/notec:include/stdio.h")));
}

test_case(file_10)
{
	char buf[256];
	test_assert(!File::path_absolute(nullptr, make_cstr("/root/notec:folder/..:include:stdio.h"), buf, buf + sizeof buf));
}