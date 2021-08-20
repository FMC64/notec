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

static inline constexpr size_t base = 8;

#define RESOLVE(cont, name) uint32_t name; test_assert(c.cont_resolve(cont, #name, name, true));
#define MUST_NOT_RESOLVE(cont, name) uint32_t name; test_assert(!c.cont_resolve(cont, #name, name, true));

test_case(cpp_0)
{
	auto c = init_file("typedef const int * const volatile * volatile ppi32;");
	c.run();
	auto b = c.get_buffer();
	RESOLVE(0, ppi32);
	test_assert(b[ppi32++] == static_cast<char>(Cpp::ContType::Using));
	test_assert(b[ppi32++] == (Type::volatile_flag | static_cast<char>(Type::Prim::Ptr)));
	test_assert(b[ppi32++] == (Type::const_flag | Type::volatile_flag | static_cast<char>(Type::Prim::Ptr)));
	test_assert(b[ppi32++] == (Type::const_flag | static_cast<char>(Type::Prim::S32)));
}

test_case(cpp_1)
{
	auto c = init_file("typedef const char *&n;");
	c.run();
	auto b = c.get_buffer();
	RESOLVE(0, n);
	test_assert(b[n++] == static_cast<char>(Cpp::ContType::Using));
	test_assert(b[n++] == static_cast<char>(Type::Prim::Lref));
	test_assert(b[n++] == static_cast<char>(Type::Prim::Ptr));
	test_assert(b[n++] == (Type::const_flag | static_cast<char>(Type::Prim::S8)));
}

test_case(cpp_2)
{
	auto c = init_file("typedef const struct { public: } S;");
	c.run();
	auto b = c.get_buffer();

	RESOLVE(0, S);
	test_assert(b[S++] == static_cast<char>(Cpp::ContType::Using));
	test_assert(b[S++] == (Type::const_flag | static_cast<char>(Type::Prim::Struct)));
	auto S_anon = load_part<3, uint32_t>(b + S);
	test_assert(S_anon == base);	// make sure struct goes before type

	test_assert(b[S_anon++] == static_cast<char>(Cpp::ContType::Struct));
	S_anon += 4;
	test_assert(load_part<3, uint32_t>(b + S_anon) == 0);	// struct belongs to main scope
}

test_case(cpp_3)
{
	auto c = init_file("typedef const struct S_t { public: } *S;");
	c.run();
	auto b = c.get_buffer();
	RESOLVE(0, S_t);
	auto S_tb = S_t;
	test_assert(b[S_t++] == static_cast<char>(Cpp::ContType::Struct));
	S_t += 4;
	test_assert(load_part<3, uint32_t>(b + S_t) == 0);

	RESOLVE(0, S);
	test_assert(b[S++] == static_cast<char>(Cpp::ContType::Using));
	test_assert(b[S++] == static_cast<char>(Type::Prim::Ptr));
	test_assert(b[S++] == (Type::const_flag | static_cast<char>(Type::Prim::Struct)));
	test_assert(load_part<3, uint32_t>(b + S) == S_tb);
}

test_case(cpp_4)
{
	auto c = init_file(
R"raw(

namespace ns {
	typedef struct{} a;
}

typedef struct{} bs;

)raw"
);
	c.run();
	auto b = c.get_buffer();
	RESOLVE(0, ns);
	auto nsb = ns;
	RESOLVE(ns, a);
	test_assert(b[ns++] == static_cast<char>(Cpp::ContType::Namespace));
	ns += 4;
	test_assert(load_part<3, uint32_t>(b + ns) == 0);

	test_assert(b[a++] == static_cast<char>(Cpp::ContType::Using));
	test_assert(b[a++] == static_cast<char>(Type::Prim::Struct));
	auto a_anon = load_part<3, uint32_t>(b + a);
	//test_assert(a_anon == base + 6);

	test_assert(b[a_anon++] == static_cast<char>(Cpp::ContType::Struct));
	a_anon += 4;
	test_assert(load_part<3, uint32_t>(b + a_anon) == nsb);	// parent must be namespace

	RESOLVE(0, bs);
	test_assert(b[bs++] == static_cast<char>(Cpp::ContType::Using));
	test_assert(b[bs++] == static_cast<char>(Type::Prim::Struct));
	auto bs_anon = load_part<3, uint32_t>(b + bs);

	test_assert(b[bs_anon++] == static_cast<char>(Cpp::ContType::Struct));
	bs_anon += 4;
	test_assert(load_part<3, uint32_t>(b + bs_anon) == 0);
}

test_case(cpp_5)
{
	auto c = init_file(
R"raw(

struct S {
	int a;
private:
	static inline float bf;

public:
	class Ssub {
	protected:
		char cs;
	};
};

)raw"
);
	c.run();
	auto b = c.get_buffer();
	RESOLVE(0, S);
	auto Sb = S;
	test_assert(b[S++] == static_cast<char>(Cpp::ContType::Struct));
	S += 4;
	test_assert(load_part<3, uint32_t>(b + S) == 0);

	{
		RESOLVE(Sb, a);
		test_assert(b[a++] == static_cast<char>(Type::Visib::Public));
		test_assert(b[a++] == static_cast<char>(Cpp::ContType::Member));
		test_assert(b[a++] == 0);
		test_assert(b[a++] == static_cast<char>(Type::Prim::S32));
	}
	{
		RESOLVE(Sb, bf);
		test_assert(b[bf++] == static_cast<char>(Type::Visib::Private));
		test_assert(b[bf++] == static_cast<char>(Cpp::ContType::Member));
		test_assert(b[bf++] == (Type::inline_flag | static_cast<char>(Type::Storage::Static)));
		test_assert(b[bf++] == static_cast<char>(Type::Prim::FP32));
	}
	{
		RESOLVE(Sb, Ssub);
		test_assert(b[Ssub++] == static_cast<char>(Type::Visib::Public));
		auto Ssubb = Ssub;
		test_assert(b[Ssub++] == static_cast<char>(Cpp::ContType::Struct));
		Ssub += 4;
		test_assert(load_part<3, uint32_t>(b + Ssub) == Sb);

		{
			RESOLVE(Ssubb, cs);
			test_assert(b[cs++] == static_cast<char>(Type::Visib::Protected));
			test_assert(b[cs++] == static_cast<char>(Cpp::ContType::Member));
			test_assert(b[cs++] == 0);
			test_assert(b[cs++] == static_cast<char>(Type::Prim::S8));
		}
	}
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
	RESOLVE(0, var);
	test_assert(b[var++] == static_cast<char>(Cpp::ContType::Member));
	test_assert(b[var++] == static_cast<char>(Type::Storage::Extern));
	test_assert(b[var++] == static_cast<char>(Type::Prim::S32));
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

	RESOLVE(0, vec2_s);
	auto vec2b = vec2_s;
	test_assert(b[vec2_s++] == static_cast<char>(Cpp::ContType::Struct));
	vec2_s += 4;
	test_assert(load_part<3, uint32_t>(b + vec2_s) == 0);

	RESOLVE(0, vec2);
	test_assert(b[vec2++] == static_cast<char>(Cpp::ContType::Using));
	test_assert(b[vec2++] == static_cast<char>(Type::Prim::Struct));
	test_assert(load_part<3, uint32_t>(b + vec2) == vec2b);

	{
		RESOLVE(vec2b, x);
		test_assert(b[x++] == static_cast<char>(Type::Visib::Public));
		test_assert(b[x++] == static_cast<char>(Cpp::ContType::Member));
		test_assert(b[x++] == 0);
		test_assert(b[x++] == static_cast<char>(Type::Prim::S32));
	}
	{
		RESOLVE(vec2b, y);
		test_assert(b[y++] == static_cast<char>(Type::Visib::Public));
		test_assert(b[y++] == static_cast<char>(Cpp::ContType::Member));
		test_assert(b[y++] == 0);
		test_assert(b[y++] == static_cast<char>(Type::Prim::S32));
	}

	RESOLVE(0, aabb2_s);
	auto aabb2 = aabb2_s;
	test_assert(b[aabb2_s++] == static_cast<char>(Cpp::ContType::Struct));
	aabb2_s += 4;
	test_assert(load_part<3, uint32_t>(b + aabb2_s) == 0);

	{
		RESOLVE(aabb2, pos);
		test_assert(b[pos++] == static_cast<char>(Type::Visib::Public));
		test_assert(b[pos++] == static_cast<char>(Cpp::ContType::Member));
		test_assert(b[pos++] == 0);
		test_assert(b[pos++] == static_cast<char>(Type::Prim::Struct));
		test_assert(load_part<3, uint32_t>(b + pos) == vec2b);
	}
	{
		RESOLVE(aabb2, size);
		test_assert(b[size++] == static_cast<char>(Type::Visib::Public));
		test_assert(b[size++] == static_cast<char>(Cpp::ContType::Member));
		test_assert(b[size++] == 0);
		test_assert(b[size++] == static_cast<char>(Type::Prim::Struct));
		test_assert(load_part<3, uint32_t>(b + size) == vec2b);
	}
}

test_case(cpp_8)
{
	auto c = init_file(
R"raw(

namespace ns {
	bool ia;
}

namespace ns {
	short ib;
}

)raw"
);
	c.run();
	RESOLVE(0, ns);
	RESOLVE(ns, ia);
	RESOLVE(ns, ib);
}

test_case(cpp_9)
{
	auto c = init_file(
R"raw(

namespace ns {
	namespace a::bn::cn {
		int ig;
	}
}

namespace ns {
	namespace a{
		namespace bn {
			namespace cn {
				unsigned int ih;
			}
		}
	}
}

)raw"
);
	c.run();
	RESOLVE(0, ns);
	RESOLVE(ns, a);
	RESOLVE(a, bn);
	RESOLVE(bn, cn);
	RESOLVE(cn, ig);
	RESOLVE(cn, ih);
}

test_case(cpp_10)
{
	auto c = init_file(
R"raw(

namespace ns {
	struct A;
}

namespace nsb {
	struct nsc {
		struct ns::A {
			short bi;
		};
	};
}

)raw"
);
	c.run();
	auto b = c.get_buffer();
	RESOLVE(0, ns);
	RESOLVE(ns, A);
	test_assert(b[A] == static_cast<char>(Cpp::ContType::Struct));
	RESOLVE(A, bi);
	test_assert(b[bi++] == static_cast<char>(Type::Visib::Public));
	test_assert(b[bi++] == static_cast<char>(Cpp::ContType::Member));
	test_assert(b[bi++] == 0);
	test_assert(b[bi++] == static_cast<char>(Type::Prim::S16));
}

test_case(cpp_11)
{
	auto c = init_file(
R"raw(

enum en : const char {
	A,
	B,
	C
};

)raw"
);
	c.run();
	auto b = c.get_buffer();
	RESOLVE(0, en);
	auto enb = en;
	test_assert(b[en++] == static_cast<char>(Cpp::ContType::Enum));
	en += 4;
	test_assert(load_part<3, uint32_t>(b + en) == 0);
	en += 3;
	test_assert(b[en++] == false);
	test_assert(b[en++] == (Type::const_flag | static_cast<char>(Type::Prim::S8)));

	RESOLVE(0, A);
	{
		RESOLVE(enb, A);
	}
	RESOLVE(0, B);
	{
		RESOLVE(enb, B);
	}
	RESOLVE(0, C);
	{
		RESOLVE(enb, C);
		test_assert(b[C++] == static_cast<char>(Cpp::ContType::Value));
		test_assert(b[C++] == (Type::const_flag | static_cast<char>(Type::Prim::S8)));
		test_assert(b[C++] == 2);
	}
}

test_case(cpp_12)
{
	auto c = init_file(
R"raw(

enum class en : const unsigned short {
	A,
	B,
	C
};

)raw"
);
	c.run();
	auto b = c.get_buffer();
	RESOLVE(0, en);
	auto enb = en;
	test_assert(b[en++] == static_cast<char>(Cpp::ContType::Enum));
	en += 4;
	test_assert(load_part<3, uint32_t>(b + en) == 0);
	en += 3;
	test_assert(b[en++] == true);
	test_assert(b[en++] == (Type::const_flag | static_cast<char>(Type::Prim::U16)));

	MUST_NOT_RESOLVE(0, A);
	{
		RESOLVE(enb, A);
	}
	MUST_NOT_RESOLVE(0, B);
	{
		RESOLVE(enb, B);
	}
	MUST_NOT_RESOLVE(0, C);
	{
		RESOLVE(enb, C);
		test_assert(b[C++] == static_cast<char>(Cpp::ContType::Value));
		test_assert(b[C++] == static_cast<char>(Type::Prim::Enum));
		test_assert(load_part<3, uint32_t>(b + C) == enb);
		C += 3;
		test_assert(load<uint16_t>(b + C) == 2);
	}
}

test_case(cpp_13)
{
	auto c = init_file("using n = const char *&;;;");
	c.run();
	auto b = c.get_buffer();
	RESOLVE(0, n);
	test_assert(b[n++] == static_cast<char>(Cpp::ContType::Using));
	test_assert(b[n++] == static_cast<char>(Type::Prim::Lref));
	test_assert(b[n++] == static_cast<char>(Type::Prim::Ptr));
	test_assert(b[n++] == (Type::const_flag | static_cast<char>(Type::Prim::S8)));
}

test_case(cpp_14)
{
	auto c = init_file(
R"raw(

void* proc(void);

)raw"
);
	c.run();
	auto b = c.get_buffer();
	RESOLVE(0, proc);
	test_assert(b[proc++] == static_cast<char>(Cpp::ContType::Member));
	test_assert(b[proc++] == 0);
	test_assert(b[proc++] == static_cast<char>(Type::Prim::Function));
	test_assert(b[proc++] == static_cast<char>(Type::Prim::Ptr));
	test_assert(b[proc++] == static_cast<char>(Type::Prim::Void));
	test_assert(b[proc++] == static_cast<uint8_t>(0));
}

test_case(cpp_15)
{
	auto c = init_file(
R"raw(

void* proc(int a, float);

)raw"
);
	c.run();
	auto b = c.get_buffer();
	RESOLVE(0, proc);
	test_assert(b[proc++] == static_cast<char>(Cpp::ContType::Member));
	test_assert(b[proc++] == 0);
	test_assert(b[proc++] == static_cast<char>(Type::Prim::Function));
	test_assert(b[proc++] == static_cast<char>(Type::Prim::Ptr));
	test_assert(b[proc++] == static_cast<char>(Type::Prim::Void));
	test_assert(b[proc++] == static_cast<uint8_t>(2));
	test_assert(b[proc++] == static_cast<char>(Type::Prim::S32));
	test_assert(b[proc++] == static_cast<char>(Type::Prim::FP32));
}

test_case(cpp_16)
{
	auto c = init_file(
R"raw(

void* proc(int a, float, const struct Str { short memb; } &c);

)raw"
);
	c.run();
	auto b = c.get_buffer();
	RESOLVE(0, proc);
	RESOLVE(0, Str);
	auto Strb = Str;
	{
		RESOLVE(Strb, memb);
		test_assert(b[memb++] == static_cast<char>(Type::Visib::Public));
		test_assert(b[memb++] == static_cast<char>(Cpp::ContType::Member));
		test_assert(b[memb++] == 0);
		test_assert(b[memb++] == static_cast<char>(Type::Prim::S16));
	}
	test_assert(b[proc++] == static_cast<char>(Cpp::ContType::Member));
	test_assert(b[proc++] == 0);
	test_assert(b[proc++] == static_cast<char>(Type::Prim::Function));
	test_assert(b[proc++] == static_cast<char>(Type::Prim::Ptr));
	test_assert(b[proc++] == static_cast<char>(Type::Prim::Void));
	test_assert(b[proc++] == static_cast<uint8_t>(3));
	test_assert(b[proc++] == static_cast<char>(Type::Prim::S32));
	test_assert(b[proc++] == static_cast<char>(Type::Prim::FP32));
	test_assert(b[proc++] == static_cast<char>(Type::Prim::Lref));
	test_assert(b[proc++] == (Type::const_flag | static_cast<char>(Type::Prim::Struct)));
	test_assert(load_part<3, uint32_t>(b + proc) == Strb);
}

test_case(cpp_17)
{
	auto c = init_file(
R"raw(

void* (*(&proc));

)raw"
);
	c.run();
	auto b = c.get_buffer();
	RESOLVE(0, proc);
	test_assert(b[proc++] == static_cast<char>(Cpp::ContType::Member));
	test_assert(b[proc++] == 0);
	test_assert(b[proc++] == static_cast<char>(Type::Prim::Lref));
	test_assert(b[proc++] == static_cast<char>(Type::Prim::Ptr));
	test_assert(b[proc++] == static_cast<char>(Type::Prim::Ptr));
	test_assert(b[proc++] == static_cast<char>(Type::Prim::Void));
}