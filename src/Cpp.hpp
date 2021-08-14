#pragma once

#include "Pp.hpp"
#include "Type.hpp"

class Cpp
{
	Pp m_pp;
	StrMap::BlockGroup m_blk;
	uint32_t m_cur;
	uint32_t m_cur_alt = 0;

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

public:
	enum class ContType : char {
		// 1 - type
		// 2 - map root
		// 3 - parent
		Namespace,

		// 1 - type
		// 2 - map root
		// 3 - parent
		// BELOW NOT USED FOR NOW
		// 1 - arg count
		// arg count args
		// 1 - non static member count
		// non static members indexes, three bytes each
		Struct,

		Using
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

	inline uint32_t cont_skip_overhead(uint32_t cont, uint32_t resolved) const
	{
		if (cont_type(cont) == ContType::Struct)
			return resolved + 1;
		else
			return resolved;
	}

	// name is null terminated
	inline bool cont_resolve(uint32_t cont, const char *name, uint32_t &res, bool skip_overhead = false)
	{
		if (cont_type(cont) > ContType::Struct)
			error("Not a namespace or struct");
		auto m = cont_map(cont);
		if (!m_blk.resolve(m, name, res))
			return false;
		if (skip_overhead)
			res = cont_skip_overhead(cont, res);
		return true;
	}

private:
	// name is null terminated
	/*inline bool cont_resolve_mut(uint32_t cont, const char *name, uint16_t *&res)
	{
		auto m = cont_map(cont);
		res = m_blk.resolve_mut_u16(m, name);
		return res != nullptr;
	}*/

	// name is null terminated
	inline bool cont_resolve_rev(uint32_t cont, const char *name, uint32_t &res, bool skip_overhead = false)
	{
		while (true) {
			if (cont_resolve(cont, name, res, skip_overhead))
				return true;
			if (cont == 0)
				return false;
			cont = cont_parent(cont);
		}
	}

	// name is null terminated
	inline bool cont_resolve_rev(const char *name, uint32_t &res, bool skip_overhead = false)
	{
		if (cont_resolve_rev(m_cur, name, res, skip_overhead))
			return true;
		else if (m_cur_alt != 0 && cont_resolve_rev(m_cur_alt, name, res, skip_overhead))
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
	// res is nullptr if id
	// id size is 0 if nothing
	inline const char* res_or_id(const char *n, uint32_t &res, char *id, bool skip_overhead = false)
	{
		if (Token::type(n) == Token::Type::Identifier) {
			Token::fill_nter(id, n);
			if (cont_resolve_rev(id, res, skip_overhead))
				*id = 0;
			else
				res = 0;
			n = next_exp();
			if (!Token::is_op(n, Token::Op::Scope))
				return n;
			n = next_exp();
		} else if (Token::is_op(n, Token::Op::Scope)) {
			res = 0;	// start from main namespace
			n = next_exp();
		} else {	// no clue what's going on
			*id = 0;
			res = 0;
			return n;
		}

		while (true) {
			// guarrantee next must be identifier
			if (Token::type(n) != Token::Type::Identifier)
				error("Must have identifier");
			Token::fill_nter(id, n);
			if (!cont_resolve(res, id, res, skip_overhead))
				error("No such member");
			n = next_exp();
			if (!Token::is_op(n, Token::Op::Scope))
				break;
			n = next_exp();
		}
		*id = 0;
		return n;
	}

	const char* acc_storage(const char *n, uint8_t &dst)
	{
		while (Token::type(n) == Token::Type::Operator) {
			auto o = Token::op(n);
			uint8_t ob = static_cast<uint8_t>(o) - static_cast<uint8_t>(Token::Op::Static);
			if (ob <= 1) {
				if (Type::storage(dst) != Type::Storage::Default)
					error("More than one storage specifier");
				dst |= ob + 1;
				goto polled;
			}
			ob = static_cast<uint8_t>(o) - static_cast<uint8_t>(Token::Op::Inline);
			if (ob <= 5) {
				dst |= 1 << (2 + ob);
				goto polled;
			}
			break;
			polled:;
			n = next_exp();
		}
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
	bool m_has_visib = false;
	Type::Visib m_cur_visib;

	inline const char* parse_struct(Token::Op kind, uint32_t &struct_ndx)
	{
		auto n = next_exp();
		char id[256];
		uint32_t res;
		size_t cur_type_size = m_size - m_building_type_base;
		char cur_type[cur_type_size];
		for (size_t i = 0; i < cur_type_size; i++)
			cur_type[i] = m_buffer[m_building_type_base + i];
		n = res_or_id(n, res, id, true);
		if (Token::type(n) == Token::Type::Operator) {
			auto o = Token::op(n);
			if (o == Token::Op::LBra) {	// define
				m_size = m_building_type_base;	// start from building type base
				n = next_exp();

				auto last_cur = m_cur;
				auto last_alt = m_cur_alt;
				if (res) {	// define struct in existing
					if (cont_type(res) != ContType::Struct)
						error("Not a struct-like");
					if (cont_map(res))
						error("Multiple definition");
					m_cur = res;	// original definition is now current context
					struct_ndx = m_cur;
					m_cur_alt = m_cur;	// set current context as fallback for resolution
					store(m_buffer + res + 1, m_blk.alloc());
				} else {	// create struct entry
					if (*id)	// otherwise anonymous struct
						cont_insert(m_cur, id, m_size);
					if (m_has_visib) {
						alloc(1);
						m_buffer[m_size++] = static_cast<char>(m_cur_visib);
					}
					m_cur = m_size;
					struct_ndx = m_cur;
					m_cur_alt = 0;
					alloc(6);
					m_buffer[m_size++] = static_cast<char>(ContType::Struct);
					m_size += store(m_buffer + m_size, m_blk.alloc());
					m_size += store_part<3>(m_buffer + m_size, last_cur);	// reference parent
				}

				auto last_has_visib = m_has_visib;
				auto last_visib = m_cur_visib;
				m_has_visib = true;
				m_cur_visib = kind == Token::Op::Class ? Type::Visib::Private : Type::Visib::Public;

				while (true) {
					if (Token::type(n) == Token::Type::Operator) {
						auto o = Token::op(n);
						if (o == Token::Op::RBra) {
							n = next_exp();
							break;
						}
						uint8_t on = static_cast<uint8_t>(o) - static_cast<uint8_t>(Token::Op::Private);
						if (on < 3) {
							m_cur_visib = static_cast<Type::Visib>(on);
							n = next_exp();
							if (!Token::is_op(n, Token::Op::Colon))
								error("Expected ':'");
							n = next_exp();
							continue;
						}
					}
					alloc(1);
					m_buffer[m_size++] = static_cast<char>(m_cur_visib);
					parse_obj(n, 1);
					n = next_exp();
				};
				m_has_visib = last_has_visib;
				m_cur_visib = last_visib;

				m_cur_alt = last_alt;
				m_cur = last_cur;
				goto wrote_def;
			} else if (o == Token::Op::Semicolon) {	// fwd
				if (res != 0) {	// already declared or fwd
					if (cont_type(res) != ContType::Struct)
						error("Invalid fwd");
				} else if (*id) {
					cont_insert(m_cur, id, m_size);
					alloc(6);
					m_buffer[m_size++] = static_cast<char>(ContType::Struct);
					m_size += store(m_buffer + m_size, static_cast<uint16_t>(0));	// fwd: 0 map
					m_size += store_part<3>(m_buffer + m_size, m_cur);	// reference parent
					goto wrote_def;
				} else
					error("Can't fwd declare unnamed struct");
				return n;
			}
		}
		// reference
		if (res)
			struct_ndx = res;
		else
			error("No such struct");
		return n;

		// write def_base to res or id
		wrote_def:;
		m_building_type_base = m_size;
		alloc(cur_type_size);
		for (size_t i = 0; i < cur_type_size; i++)
			m_buffer[m_size++] = cur_type[i];
		return n;
	}

	uint32_t skip_type(uint32_t t)
	{
		while (true) {
			auto c = Type::prim(m_buffer[t]);
			if (c < Type::Prim::Struct) {
				t++;
				return t;
			}
			if (c == Type::Prim::Struct) {
				t += 4;
				return t;
			}
			if (c == Type::Prim::Function) {
				t = skip_type(t);
				uint8_t s = m_buffer[t++];
				for (uint8_t i = 0; i < s; i++)
					t = skip_type(t);
				return t;
			}
			t++;
		}
	}

	uint32_t type_size(uint32_t t)
	{
		return skip_type(t) - t;
	}

	// nested_id should be at least 256 bytes long (maximum token size)
	// if nested_id is nullptr, means nested identified will be discarded
	// nested_id will be filled with a null-terminated string, of size 0 if not found
	inline const char* parse_type_base(const char *n, char *nested_id)
	{
		uint8_t attrs = 0;
		n = acc_type_attrs(n, attrs);
		ContType t_type = static_cast<ContType>(0);
		uint32_t t_ndx;
		char type;
		if (attrs & TypeAttr::int_mask)	// attrs grabbed int type
			type = (attrs & TypeAttr::cv_mask) | static_cast<char>(type_attr_intprim(attrs));
		else {
			if (attrs & TypeAttr::sign_mask)
				error("Can't have signed or unsigned without integer type");
			auto t = Token::type(n);
			if (t == Token::Type::Operator) {
				auto ob = Token::op(n);
				auto o = static_cast<uint8_t>(ob) - static_cast<uint8_t>(Token::Op::Void);
				if (o <= 4) {	// from op auto up to bool
					n = next_exp();
					n = acc_type_attrs(n, attrs);
					if (attrs & ~TypeAttr::cv_mask)
						error("Illegal qualifier: not an integer type");
					type = (attrs & TypeAttr::cv_mask) | o;
					goto valid_type;
				}
				if (ob >= Token::Op::Class && ob <= Token::Op::Union) {
					t_type = ContType::Struct;
					n = parse_struct(ob, t_ndx);
					n = acc_type_attrs(n, attrs);
					if (Token::is_op(n, Token::Op::Semicolon))
						if (attrs)
							error("Can't have qualifiers on front of such declaration");
					type = (attrs & TypeAttr::cv_mask) | static_cast<char>(Type::Prim::Struct);
					goto valid_type;
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
				error("Bad op for type decl");
			} else {
				uint32_t res;
				char id[256];
				n = res_or_id(n, res, id, true);
				n = acc_type_attrs(n, attrs);
				if (res) {
					auto t = cont_type(res);
					if (t == ContType::Struct) {
						t_type = ContType::Struct;
						t_ndx = res;
						type = (attrs & TypeAttr::cv_mask) | static_cast<char>(Type::Prim::Struct);
						goto valid_type;
					} else if (t == ContType::Using) {
						t_type = ContType::Using;
						t_ndx = res + 1;
						goto valid_type;
					} else
						error("Bad type reference");
				} else {
					if (*id)
						error("Unknown object");
					else
						error("Expected a type");
				}
			}
			__builtin_unreachable();
		}

		valid_type:;
		*nested_id = 0;
		n = parse_type_nprim(n, nested_id);
		if (t_type == ContType::Using) {
			auto s = type_size(t_ndx);
			alloc(s);
			auto base = m_size;
			for (size_t i = 0; i < s; i++)
				m_buffer[m_size++] = m_buffer[t_ndx++];
			m_buffer[base] |= attrs & TypeAttr::cv_mask;
		} else {
			alloc(1);
			m_buffer[m_size++] = type;
			if (t_type == ContType::Struct) {
				alloc(3);
				m_size += store_part<3>(m_buffer + m_size, t_ndx);
			}
		}
		return n;
	}

	inline const char* parse_type(const char *n, char *nested_id, uint32_t &ndx, uint32_t base_off = 0)
	{
		auto old_base = m_building_type_base;
		m_building_type_base = m_size - base_off;
		auto res = parse_type_base(n, nested_id);
		ndx = m_building_type_base;
		m_building_type_base = old_base;
		return res;
	}

	// id should be at least 256 bytes long (maximum token size)
	inline const char* parse_type_id(const char *n, char *id, uint32_t &ndx, uint32_t base_off = 0)
	{
		n = parse_type(n, id, ndx, base_off);
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
			while (true) {
				if (Token::is_op(n, Token::Op::RBra))
					break;
				parse_obj(n);
				n = next_exp();
			}
		} else
			parse_obj(n);
		m_is_c_linkage = last;
	}

	inline void parse_memb(const char *n, uint32_t base_off, uint8_t sto = 0)
	{
		n = acc_storage(n, sto);
		alloc(1);
		m_buffer[m_size++] = sto;
		uint32_t t;
		char id[256];
		n = parse_type(n, id, t, base_off + 1);
		if (*id == 0) {
			if (Token::is_op(n, Token::Op::Semicolon) && Type::prim(m_buffer[t + base_off + 1]) == Type::Prim::Struct) {
				if ((m_buffer[t + base_off + 1] & (Type::const_flag | Type::volatile_flag)) || sto)
					error("Can't qualify struct declaration");
				m_size = t;	// remove in-building type
				return;
			}
			if (Token::type(n) != Token::Type::Identifier)
				error("Expected identifier");
			Token::fill_nter(id, n);
			n = next_exp();
		}
		if (!Token::is_op(n, Token::Op::Semicolon))
			error("Expected ';'");
		cont_insert(m_cur, id, t);
	}

	inline void parse_obj(const char *n, uint32_t base_off = 0)
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
				} else
					return parse_memb(n, base_off, static_cast<char>(Type::Storage::Extern));
			} else if (o == Token::Op::Typedef) {
				n = next_exp();
				alloc(1);
				m_buffer[m_size++] = static_cast<char>(ContType::Using);
				char id[256];
				uint32_t t;
				n = parse_type_id(n, id, t, base_off + 1);
				cont_insert(m_cur, id, t);
				if (!Token::is_op(n, Token::Op::Semicolon))
					error("Expected ';'");
				return;
			} else if (o == Token::Op::Namespace) {
				n = next_exp();
				auto last_cur = m_cur;
				auto last_alt = m_cur_alt;
				m_cur_alt = 0;
				while (true) {
					if (Token::type(n) != Token::Type::Identifier)
						error("Expected identifier");
					token_nter(nn, n);
					if (cont_resolve(m_cur, nn, m_cur, true)) {
						if (cont_type(m_cur) != ContType::Namespace)
							error("Not a namespace");
					} else {
						cont_insert(m_cur, nn, m_size);
						m_cur = m_size;
						auto map = m_blk.alloc();
						alloc(6);
						m_buffer[m_size++] = static_cast<char>(ContType::Namespace);
						m_size += store(m_buffer + m_size, map);
						m_size += store_part<3>(m_buffer + m_size, 0);
					}
					n = next_exp();
					if (Token::type(n) == Token::Type::Operator) {
						auto o = Token::op(n);
						if (o == Token::Op::LBra) {
							n = next_exp();
							break;
						} else if (o == Token::Op::Scope) {
							n = next_exp();
							continue;
						}
					}
					error("Illegal token");
				}
				while (true) {
					if (Token::is_op(n, Token::Op::RBra))
						break;
					parse_obj(n, base_off);
					n = next_exp();
				}
				m_cur_alt = last_alt;
				m_cur = last_cur;
				return;
			}
		}
		parse_memb(n, base_off);
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