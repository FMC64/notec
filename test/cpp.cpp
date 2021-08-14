#include "Cpp.hpp"
#include "test.hpp"

const char dummy_name[] = {
	static_cast<char>(Token::Type::StringLiteral),
	1,
	'f'
};

static Cpp init_file(const char *src)
{
	auto res = Cpp();
	auto &s = res.err_src().get_stream();
	s.set_file_count(1);
	s.add_file("f", src);
	res.open(dummy_name);
	return res;
}

static inline constexpr size_t base = 6;

test_case(cpp_0)
{
	auto c = init_file("typedef const int * const volatile * volatile ppi32;");
	c.run();
	auto b = c.get_buffer();
	test_assert(b[base + 0] == static_cast<char>(Cpp::ContType::Using));
	test_assert(b[base + 1] == (Type::volatile_flag | static_cast<char>(Type::Prim::Ptr)));
	test_assert(b[base + 2] == (Type::const_flag | Type::volatile_flag | static_cast<char>(Type::Prim::Ptr)));
	test_assert(b[base + 3] == (Type::const_flag | static_cast<char>(Type::Prim::S32)));
}

test_case(cpp_1)
{
	auto c = init_file("typedef const char *&n;");
	c.run();
	auto b = c.get_buffer();
	test_assert(b[base + 0] == static_cast<char>(Cpp::ContType::Using));
	test_assert(b[base + 1] == static_cast<char>(Type::Prim::Lref));
	test_assert(b[base + 2] == static_cast<char>(Type::Prim::Ptr));
	test_assert(b[base + 3] == (Type::const_flag | static_cast<char>(Type::Prim::S8)));
}

test_case(cpp_2)
{
	auto c = init_file("typedef const struct { public: } S;");
	c.run();
	auto b = c.get_buffer();
	test_assert(b[base + 0] == static_cast<char>(Cpp::ContType::Struct));
	test_assert(load<uint16_t>(b + base + 1));
	test_assert(load_part<3, uint32_t>(b + base + 3) == 0);

	test_assert(b[base + 6] == static_cast<char>(Cpp::ContType::Using));
	test_assert(b[base + 7] == (Type::const_flag | static_cast<char>(Type::Prim::Struct)));
	test_assert(load_part<3, uint32_t>(b + base + 8) == base);
}

test_case(cpp_3)
{
	auto c = init_file("typedef const struct S_t { public: } *S;");
	c.run();
	auto b = c.get_buffer();
	test_assert(b[base + 0] == static_cast<char>(Cpp::ContType::Struct));
	test_assert(load<uint16_t>(b + base + 1));
	test_assert(load_part<3, uint32_t>(b + base + 3) == 0);

	test_assert(b[base + 6] == static_cast<char>(Cpp::ContType::Using));
	test_assert(b[base + 7] == static_cast<char>(Type::Prim::Ptr));
	test_assert(b[base + 8] == (Type::const_flag | static_cast<char>(Type::Prim::Struct)));
	test_assert(load_part<3, uint32_t>(b + base + 9) == base);
}

test_case(cpp_4)
{
	auto c = init_file(
R"raw(

namespace ns {
	typedef struct{} a;
}

typedef struct{} b;

)raw"
);
	c.run();
	auto b = c.get_buffer();
	test_assert(b[base + 0] == static_cast<char>(Cpp::ContType::Namespace));
	test_assert(load<uint16_t>(b + base + 1));
	test_assert(load_part<3, uint32_t>(b + base + 3) == 0);

	test_assert(b[base + 6] == static_cast<char>(Cpp::ContType::Struct));
	test_assert(load<uint16_t>(b + base + 7));
	test_assert(load_part<3, uint32_t>(b + base + 9) == base);

	test_assert(b[base + 12] == static_cast<char>(Cpp::ContType::Using));
	test_assert(b[base + 13] == static_cast<char>(Type::Prim::Struct));
	test_assert(load_part<3, uint32_t>(b + base + 14) == base + 6);

	test_assert(b[base + 17] == static_cast<char>(Cpp::ContType::Struct));
	test_assert(load<uint16_t>(b + base + 18));
	test_assert(load_part<3, uint32_t>(b + base + 20) == 0);

	test_assert(b[base + 23] == static_cast<char>(Cpp::ContType::Using));
	test_assert(b[base + 24] == static_cast<char>(Type::Prim::Struct));
	test_assert(load_part<3, uint32_t>(b + base + 25) == base + 17);
}

test_case(cpp_5)
{
	auto c = init_file(
R"raw(

struct S {
	int a;
private:
	static inline float b;

protected:
	class Ssub {
	protected:
		char c;
	};
};

)raw"
);
	c.run();
	auto b = c.get_buffer();
	test_assert(b[base + 0] == static_cast<char>(Cpp::ContType::Struct));
	test_assert(load<uint16_t>(b + base + 1));
	test_assert(load_part<3, uint32_t>(b + base + 3) == 0);

	test_assert(b[base + 6] == static_cast<char>(Type::Visib::Public));
	test_assert(b[base + 7] == 0);
	test_assert(b[base + 8] == static_cast<char>(Type::Prim::S32));

	test_assert(b[base + 9] == static_cast<char>(Type::Visib::Private));
	test_assert(b[base + 10] == (Type::inline_flag | static_cast<char>(Type::Storage::Static)));
	test_assert(b[base + 11] == static_cast<char>(Type::Prim::FP32));

	test_assert(b[base + 12] == static_cast<char>(Type::Visib::Protected));
	test_assert(b[base + 13] == static_cast<char>(Cpp::ContType::Struct));
	test_assert(load<uint16_t>(b + base + 14));
	test_assert(load_part<3, uint32_t>(b + base + 16) == base);

	test_assert(b[base + 19] == static_cast<char>(Type::Visib::Protected));
	test_assert(b[base + 20] == 0);
	test_assert(b[base + 21] == static_cast<char>(Type::Prim::S8));
}

test_case(cpp_6)
{
	auto c = init_file(
R"raw(

extern "C" {

extern int var;

}

)raw"
);
	c.run();
	auto b = c.get_buffer();
	test_assert(b[base] == static_cast<char>(Type::Storage::Extern));
	test_assert(b[base + 1] == static_cast<char>(Type::Prim::S32));
}

test_case(cpp_7)
{
	auto c = init_file(
R"raw(

typedef struct vec2_s {
	int x;
	int y;
} vec2;

typedef struct aabb2_s{
	vec2 pos;
	vec2 size;
} aabb2;

)raw"
);
	c.run();
	auto b = c.get_buffer();

	test_assert(b[base + 0] == static_cast<char>(Cpp::ContType::Struct));
	test_assert(load<uint16_t>(b + base + 1));
	test_assert(load_part<3, uint32_t>(b + base + 3) == 0);

	test_assert(b[base + 6] == static_cast<char>(Type::Visib::Public));
	test_assert(b[base + 7] == 0);
	test_assert(b[base + 8] == static_cast<char>(Type::Prim::S32));

	test_assert(b[base + 9] == static_cast<char>(Type::Visib::Public));
	test_assert(b[base + 10] == 0);
	test_assert(b[base + 11] == static_cast<char>(Type::Prim::S32));

	test_assert(b[base + 12] == static_cast<char>(Cpp::ContType::Using));
	test_assert(b[base + 13] == static_cast<char>(Type::Prim::Struct));
	test_assert(load_part<3, uint32_t>(b + base + 14) == base);


	test_assert(b[base + 17] == static_cast<char>(Cpp::ContType::Struct));
	test_assert(load<uint16_t>(b + base + 18));
	test_assert(load_part<3, uint32_t>(b + base + 20) == 0);

	test_assert(b[base + 23] == static_cast<char>(Type::Visib::Public));
	test_assert(b[base + 24] == 0);
	test_assert(b[base + 25] == static_cast<char>(Type::Prim::Struct));
	test_assert(load_part<3, uint32_t>(b + base + 26) == base);
	test_assert(b[base + 29] == static_cast<char>(Type::Visib::Public));
	test_assert(b[base + 30] == 0);
	test_assert(b[base + 31] == static_cast<char>(Type::Prim::Struct));
	test_assert(load_part<3, uint32_t>(b + base + 32) == base);

	test_assert(b[base + 35] == static_cast<char>(Cpp::ContType::Using));
	test_assert(b[base + 36] == static_cast<char>(Type::Prim::Struct));
	test_assert(load_part<3, uint32_t>(b + base + 37) == base + 17);
}