#pragma once

#include "Pp.hpp"
#include "Type.hpp"

class Cpp
{
	Pp m_pp;
	StrMap::BlockGroup m_blk;
	uint32_t m_cur;

	inline void error(const char *str)
	{
		m_pp.error(str);
	}

	inline const char* next(void)
	{
		return m_pp.next();
	}

	inline const char* next_exp(void)
	{
		auto res = m_pp.next();
		if (res == nullptr)
			error("Expected token");
		return res;
	}

	size_t m_size = 0;
	size_t m_allocated = 0;
	char *m_buffer = nullptr;

	inline void alloc(size_t size)
	{
		auto needed = m_size + size;
		if (m_allocated < needed) {
			m_allocated *= 2;
			if (m_allocated < needed)
				m_allocated = needed;
			m_buffer = reinterpret_cast<char*>(realloc(m_buffer, m_allocated));
		}
	}

	bool m_is_c_linkage = false;

	enum class ContType : char {
		// 1 - type
		// 2 - map root
		// 3 - parent
		Namespace,

		// 1 - type
		// 2 - map root
		// 3 - parent
		// 1 - arg count
		// arg count args
		// 1 - non static member count
		// non static members indexes, three bytes each
		Struct
	};

	inline ContType cont_type(uint32_t cont) const
	{
		return static_cast<ContType>(m_buffer[cont]);
	}

	inline uint16_t cont_map(uint32_t cont) const
	{
		return load<uint16_t>(m_buffer + cont + 1);
	}

	inline uint32_t cont_parent(uint32_t cont) const
	{
		return load_part<3, uint32_t>(m_buffer + cont + 3);
	}

	// name is null terminated
	inline bool cont_resolve(uint32_t cont, const char *name, uint32_t &res)
	{
		auto m = cont_map(cont);
		return m_blk.resolve(m, name, res);
	}

	// name is null terminated
	inline bool cont_resolve_mut(uint32_t cont, const char *name, uint16_t *&res)
	{
		auto m = cont_map(cont);
		res = m_blk.resolve_mut_u16(m, name);
		return res != nullptr;
	}

	// name is null terminated
	inline bool cont_resolve_mut_rev(uint32_t cont, const char *name, uint16_t *&res)
	{
		while (true) {
			if (cont_resolve_mut(cont, name, res))
				return true;
			cont = cont_parent(cont);
			if (cont == 0)
				return false;
		}
	}

	// name is null terminated
	inline bool cont_resolve_mut_rev_cur_fallback(uint32_t cont, const char *name, uint16_t *&res)
	{
		if (cont_resolve_mut_rev(cont, name, res))
			return true;
		else if (cont_resolve_mut_rev(m_cur, name, res))
			return true;
		else
			return false;
	}

	// name is null terminated
	inline void cont_insert(uint32_t cont, const char *name, uint32_t payload)
	{
		if (!m_blk.insert(cont_map(cont), name, payload))
			error("Primitive redeclaration");
	}

	// id should be at least 256 bytes long
	// id size is 0 if res
	// res is nullptr if nothing
	inline const char* id_or_res(const char *n, uint32_t climb_first, char *id, uint16_t *&res)
	{
		uint32_t resc;
		if (Token::type(n) == Token::Type::Identifier) {
			Token::fill_nter(id, n);
			if (cont_resolve_mut_rev_cur_fallback(climb_first, id, res))
				resc = load_u16<uint32_t>(res);
			else
				*id = 0;
			n = next_exp();
			if (!Token::is_op(n, Token::Op::Scope))
				return n;
			if (*id == 0) {
				res = nullptr;
				return n;
			}
			n = next_exp();
		} else if (Token::is_op(n, Token::Op::Scope)) {
			res = nullptr;	// can't mutate main namespace address
			resc = 0;
			n = next_exp();
		} else {
			*id = 0;
			res = nullptr;
			return n;
		}

		while (true) {
			// guarrantee next must be identifier
			if (Token::type(n) != Token::Type::Identifier)
				error("Must have identifier");
			Token::fill_nter(id, n);
			if (!cont_resolve_mut(resc, id, res))
				error("No such member");
			resc = load_u16<uint32_t>(res);
			n = next_exp();
			if (!Token::is_op(n, Token::Op::Scope))
				break;
			n = next_exp();
		}
		*id = 0;
		return n;
	}

	struct TypeAttr
	{
		static inline constexpr uint8_t Char = 1 << 0;
		static inline constexpr uint8_t Short = 1 << 1;
		static inline constexpr uint8_t Long = 1 << 2;
		static inline constexpr uint8_t Int = 1 << 3;
		static inline constexpr uint8_t Signed = 1 << 4;
		static inline constexpr uint8_t Unsigned = 1 << 5;
		static inline constexpr uint8_t Const = 1 << 6;
		static inline constexpr uint8_t Volatile = 1 << 7;

		static inline constexpr uint8_t int_mask = 0x0F;
		static inline constexpr uint8_t sign_mask = 0x30;
		static inline constexpr uint8_t cv_mask = 0xC0;
	};

	inline const char* acc_type_attrs(const char *n, uint8_t &dst)
	{
		while (Token::type(n) == Token::Type::Operator) {
			uint8_t o = static_cast<uint8_t>(Token::op(n)) - static_cast<uint8_t>(Token::Op::Char);
			if (o >= 8)
				break;
			uint8_t b = 1 << o;
			if (o < TypeAttr::Signed)	// signed - first stackable qualifier
				if (dst & b)
					error("Non stackable qualifier");
			dst |= b;
			n = next_exp();
		}
		return n;
	}

	inline Type::Prim type_attr_intprim(uint8_t attrs)
	{
		if ((attrs & TypeAttr::sign_mask) == TypeAttr::sign_mask)
			error("Can't have signed and unsigned");
		if (attrs & TypeAttr::Char) {
			if ((attrs & TypeAttr::int_mask) != TypeAttr::Char)
				error("Invalid int comb");
			return attrs & TypeAttr::Unsigned ? Type::Prim::U8 : Type::Prim::S8;
		}
		if (attrs & TypeAttr::Short) {
			if ((attrs & (TypeAttr::int_mask & ~(TypeAttr::Short | TypeAttr::Int))) != 0)
				error("Invalid int comb");
			return attrs & TypeAttr::Unsigned ? Type::Prim::U16 : Type::Prim::S16;
		}
		if (attrs & TypeAttr::Long) {
			if ((attrs & (TypeAttr::int_mask & ~(TypeAttr::Long | TypeAttr::Int))) != 0)
				error("Invalid int comb");
			return attrs & TypeAttr::Unsigned ? Type::Prim::U32 : Type::Prim::S32;
		}
		return attrs & TypeAttr::Unsigned ? Type::Prim::U32 : Type::Prim::S32;	// must be int
	}

	inline const char* parse_type_nprim(const char *n, char *nested_id)
	{
		auto single_op = [&](Type::Prim p, bool must_be_last){
				n = next_exp();
				uint8_t attrs = 0;
				n = acc_type_attrs(n, attrs);
				if (must_be_last) {
					if (attrs)
						error("Can't have more qualifiers");
				} else if (attrs & ~TypeAttr::cv_mask)
					error("Illegal qualifier: not an integer type");
				auto had = n;
				n = parse_type_nprim(n, nested_id);
				if (must_be_last)
					if (had != n)
						error("Can't have more qualifiers");
				alloc(1);
				m_buffer[m_size++] = attrs | static_cast<char>(p);
		};
		if (Token::type(n) == Token::Type::Operator) {
			auto o = Token::op(n);
			if (o == Token::Op::Mul)
				single_op(Type::Prim::Ptr, false);
			else if (o == Token::Op::BitAnd)
				single_op(Type::Prim::Lref, true);
			else if (o == Token::Op::And)
				single_op(Type::Prim::Rref, true);
		}
		return n;
	}

	uint32_t m_building_type_base;

	// nested_id should be at least 256 bytes long (maximum token size)
	// if nested_id is nullptr, means nested identified will be discarded
	// nested_id will be filled with a null-terminated string, of size 0 if not found
	inline const char* parse_type_base(const char *n, char *nested_id)
	{
		uint8_t attrs = 0;
		n = acc_type_attrs(n, attrs);
		char type;
		if (attrs & TypeAttr::int_mask)	// attrs grabbed int type
			type = (attrs & TypeAttr::cv_mask) | static_cast<char>(type_attr_intprim(attrs));
		else {
			if (attrs & TypeAttr::sign_mask)
				error("Can't have signed or unsigned without integer type");
			if (Token::type(n) == Token::Type::Operator) {
				auto ob = Token::op(n);
				auto o = static_cast<uint8_t>(ob) - static_cast<uint8_t>(Token::Op::Auto);
				if (o <= 4) {	// from op auto up to bool
					n = next_exp();
					n = acc_type_attrs(n, attrs);
					if (attrs & ~TypeAttr::cv_mask)
						error("Illegal qualifier: not an integer type");
					type = (attrs & TypeAttr::cv_mask) | o;
					goto valid_type;
				}
				if (ob == Token::Op::Struct || ob == Token::Op::Class) {
					n = next_exp();
					char id[256];
					uint16_t *res;
					n = id_or_res(n, 0, id, res);
					if (Token::is_op(n, Token::Op::LBra)) {
					} else {
					}
				}
				o = static_cast<uint8_t>(ob) - static_cast<uint8_t>(Token::Op::Char8_t);
				if (o <= 3) {
					n = next_exp();
					n = acc_type_attrs(n, attrs);
					if (attrs & ~TypeAttr::cv_mask)
						error("Illegal qualifier: not an integer type");
					if (o == 3)
						o = 2;
					type = (attrs & TypeAttr::cv_mask) | (static_cast<uint8_t>(Type::Prim::U8) + 2 * o);
					goto valid_type;
				}
			}
			error("Bad type");
			__builtin_unreachable();
		}

		valid_type:;
		*nested_id = 0;
		n = parse_type_nprim(n, nested_id);
		alloc(1);
		m_buffer[m_size++] = type;
		return n;
	}

	inline const char* parse_type(const char *n, char *nested_id, uint32_t &ndx)
	{
		m_building_type_base = m_size;
		auto res = parse_type_base(n, nested_id);
		ndx = m_building_type_base;
		return res;
	}

	// id should be at least 256 bytes long (maximum token size)
	inline const char* parse_type_id(const char *n, char *id, uint32_t &ndx)
	{
		n = parse_type(n, id, ndx);
		if (*id == 0) {
			if (Token::type(n) != Token::Type::Identifier)
				error("Expected identifier");
			Token::fill_nter(id, n);
			n = next_exp();
		}
		return n;
	}

	// if is_c is false, c++
	inline void extern_linkage(bool is_c)
	{
		auto last = m_is_c_linkage;
		m_is_c_linkage = is_c;
		auto n = next_exp();
		if (Token::is_op(n, Token::Op::LBra)) {
			n = next_exp();
			if (!Token::is_op(n, Token::Op::RBra)) {
				parse_obj(n);
				n = next_exp();
				if (!Token::is_op(n, Token::Op::RBra))
					error("Expected '}'");
			}
		} else
			parse_obj(n);
		m_is_c_linkage = last;
	}

	inline void parse_obj(const char *n)
	{
		if (Token::type(n) == Token::Type::Operator) {
			auto o = Token::op(n);
			if (o == Token::Op::Extern) {
				n = next_exp();
				if (Token::type(n) == Token::Type::StringLiteral) {
					if (streq(n + 1, make_cstr("C++")))
						return extern_linkage(false);
					if (streq(n + 1, make_cstr("C")))
						return extern_linkage(true);
					error("Unknown language linkage");
				}
			} else if (o == Token::Op::Typedef) {
				n = next_exp();
				char id[256];
				uint32_t t;
				n = parse_type_id(n, id, t);
				cont_insert(m_cur, id, t);
				if (!Token::is_op(n, Token::Op::Semicolon))
					error("Expected ';'");
			}
		}
	}

public:
	Cpp(void)
	{
		m_cur = m_size;
		auto map = m_blk.alloc();
		alloc(6);
		m_buffer[m_size++] = static_cast<char>(ContType::Namespace);
		m_size += store(m_buffer + m_size, map);
		m_size += store_part<3>(m_buffer + m_size, 0);
	}
	~Cpp(void)
	{
		free(m_buffer);
	}

	inline auto& err_src(void)
	{
		return m_pp.get_tstream();
	}

	inline void include_dir(const char *path)
	{
		m_pp.include_dir(path);
	}

	inline void open(const char *path)
	{
		m_pp.open(path);
	}

	inline void run(void)
	{
		const char *n;
		while ((n = next()))
			parse_obj(n);
	}

	inline const char* get_buffer(void) const
	{
		return m_buffer;
	}
};