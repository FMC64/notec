#pragma once

#include "Pp.hpp"
#include "Type.hpp"
#include "Map.hpp"

class Cpp : private Map
{
	Pp m_pp;
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

	bool m_is_c_linkage = false;

public:
	enum class ContType : char {
		// 1 - type
		// 3 - parent
		// 4 - map root
		// TOTAL: 8
		Namespace,

		// 1 - type
		// 3 - parent
		// 4 - map root
		// 1 - is class
		// 1 - underlying type
		// TOTAL: 10
		Enum,

		// 1 - type
		// 3 - parent
		// 4 - map root
		// 1 - is_union
		// STATIC TOTAL: 8
		// 1 - arg count
		// arg count args

		// BELOW NOT USED FOR NOW
		// 1 - non static member count
		// non static members indices, three bytes each
		Struct,

		// 1 - type
		// 3 - parent
		// 1 - arg count
		// arg count args
		// type
		Using,

		Value,	// then type, then value
		Member	// then storage, type and value (opt)
	};

	struct Cont {
		static inline constexpr size_t type_off = 0;
		static inline constexpr size_t parent_off = 1;
		static inline constexpr size_t map_off = 4;
		static inline constexpr size_t cont_ovhead = 8;

		struct Enum {
			static inline constexpr size_t is_class_off = cont_ovhead + 0;
			static inline constexpr size_t underlying_type_off = cont_ovhead + 1;

			static inline constexpr size_t static_size = underlying_type_off + 1;
		};

		struct Struct {
			static inline constexpr size_t is_union_off = cont_ovhead + 0;
			static inline constexpr size_t temp_args_off = cont_ovhead + 1;

			static inline constexpr size_t static_size = temp_args_off + 1;
		};
	};

	inline ContType cont_type(uint32_t cont) const
	{
		return static_cast<ContType>(m_buffer[cont]);
	}

	inline uint32_t assert_cont_map(uint32_t cont)
	{
		auto t = cont_type(cont);
		if (t > ContType::Struct)
			error("Not a namespace or struct");
		auto r = cont_map(cont);
		if (t == ContType::Struct)
			if (!is_map(r))
				error("Not yet defined");
		return r;
	}

	inline uint32_t cont_map(uint32_t cont) const
	{
		return cont + Cont::map_off;
	}

	inline bool is_map(uint32_t map) const
	{
		return m_buffer[map + 3] == 0x7F;
	}

	inline uint32_t cont_parent(uint32_t cont) const
	{
		return load_part<3, uint32_t>(m_buffer + cont + Cont::parent_off);
	}

	inline uint32_t cont_skip_overhead(uint32_t cont, uint32_t resolved) const
	{
		if (cont_type(cont) == ContType::Struct)
			return resolved + 1;
		else
			return resolved;
	}

	// name is null terminated
	inline bool cont_resolve(uint32_t cont, const char *name, uint32_t &res, bool dont_skip_overhead = false)
	{
		auto m = assert_cont_map(cont);
		auto d = resolve(m, name);
		if (d == nullptr)
			return false;
		res = d - m_buffer;
		if (!dont_skip_overhead)
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
	inline bool cont_resolve_rev(uint32_t cont, const char *name, uint32_t &res)
	{
		while (true) {
			if (cont_resolve(cont, name, res))
				return true;
			if (cont == 0)
				return false;
			cont = cont_parent(cont);
		}
	}

	// name is null terminated
	inline bool cont_resolve_rev(const char *name, uint32_t &res)
	{
		if (cont_resolve_rev(m_cur, name, res))
			return true;
		else
			return false;
	}

	// name is null terminated
	inline void cont_insert(uint32_t cont, const char *name)
	{
		if (!insert(cont_map(cont), name))
			error("Primitive redeclaration");
	}

	// id should be at least 256 bytes long
	// res is nullptr if id
	// id size is 0 if nothing
	inline const char* res_or_id(const char *n, uint32_t &res, char *id)
	{
		if (Token::type(n) == Token::Type::Identifier) {
			Token::fill_nter(id, n);
			if (cont_resolve_rev(id, res))
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
			if (!cont_resolve(res, id, res))
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

	inline const char* parse_type_extr_qual(const char *n, char *nested_id, bool exp_rpar)
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
				n = parse_type_extr_qual(n, nested_id, false);
				alloc(1);
				store(static_cast<char>(attrs | static_cast<char>(p)));
		};
		auto t = Token::type(n);
		if (t == Token::Type::Operator) {
			auto o = Token::op(n);
			if (o == Token::Op::Mul)
				single_op(Type::Prim::Ptr, false);
			else if (o == Token::Op::BitAnd)
				single_op(Type::Prim::Lref, true);
			else if (o == Token::Op::And)
				single_op(Type::Prim::Rref, true);
			else if (o == Token::Op::LPar) {
				n = next_exp();
				n = parse_type_extr_qual(n, nested_id, true);
			}
		} else if (t == Token::Type::Identifier) {
			Token::fill_nter(nested_id, n);
			n = next_exp();
		} else
			error("Expected id or op");
		if (exp_rpar) {
			if (!Token::is_op(n, Token::Op::RPar))
				error("Expected ')'");
			n = next_exp();
		}
		return n;
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
				store(static_cast<char>(attrs | static_cast<char>(p)));
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
		n = res_or_id(n, res, id);
		auto is_union = static_cast<char>(kind == Token::Op::Union);
		if (Token::type(n) == Token::Type::Operator) {
			auto o = Token::op(n);
			if (o == Token::Op::LBra) {	// define
				m_size = m_building_type_base;	// start from building type base
				n = next_exp();

				auto last_cur = m_cur;
				if (res) {	// define struct in existing
					if (cont_type(res) != ContType::Struct)
						error("Not a struct-like");
					if (is_map(cont_map(res)))
						error("Multiple definition");
					m_cur = res;	// original definition is now current context
					struct_ndx = m_cur;
					::store(m_buffer + res + Cont::map_off + 3, static_cast<char>(0x7F));
					if (m_buffer[res + Cont::Struct::is_union_off] != is_union)
						error("Struct type mismatch");
					if (m_buffer[res + Cont::Struct::temp_args_off] != 0)
						error("Does not match prev. declaration");
				} else {	// create struct entry
					if (*id)	// otherwise anonymous struct
						cont_insert(m_cur, id);
					if (m_has_visib) {
						alloc(1);
						store(static_cast<char>(m_cur_visib));
					}
					m_cur = m_size;
					struct_ndx = m_cur;
					alloc(Cont::Struct::static_size);
					store(static_cast<char>(ContType::Struct));
					store_part<3>(last_cur);	// reference parent
					create_root();
					store(is_union);
					store(static_cast<char>(0));
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
					store(static_cast<char>(m_cur_visib));
					parse_obj(n, 1);
					n = next_exp();
				};
				m_has_visib = last_has_visib;
				m_cur_visib = last_visib;

				m_cur = last_cur;
				goto wrote_def;
			} else if (o == Token::Op::Semicolon) {	// fwd
				if (res != 0) {	// already declared or fwd
					if (cont_type(res) != ContType::Struct)
						error("Invalid fwd");
					if (m_buffer[res + Cont::Struct::is_union_off] != is_union)
						error("Struct type mismatch");
					if (m_buffer[res + Cont::Struct::temp_args_off] != 0)
						error("Does not match prev. declaration");
				} else if (*id) {
					cont_insert(m_cur, id);
					alloc(Cont::Struct::static_size);
					store(static_cast<char>(ContType::Struct));
					store_part<3>(m_cur);	// reference parent
					store(static_cast<uint32_t>(0));	// fwd: 0 map
					store(is_union);
					store(static_cast<char>(0));
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
			store(cur_type[i]);
		return n;
	}

	inline size_t integer_size(char i)
	{
		auto p = (static_cast<char>(Type::prim(i)) - static_cast<char>(Type::Prim::S8)) >> 1;
		return 1 << p;
	}

	inline const char* parse_enum(uint32_t &enum_ndx)
	{
		auto n = next_exp();
		bool is_class = Token::is_op(n, Token::Op::Class);
		if (is_class)
			n = next_exp();
		char id[256];
		uint32_t res;
		size_t cur_type_size = m_size - m_building_type_base;
		char cur_type[cur_type_size];
		for (size_t i = 0; i < cur_type_size; i++)
			cur_type[i] = m_buffer[m_building_type_base + i];
		n = res_or_id(n, res, id);
		char t = static_cast<char>(Type::Prim::S32);
		if (Token::is_op(n, Token::Op::Colon)) {
			n = next_exp();
			n = parse_integral(n, t);
			if (!Token::is_op(n, Token::Op::LBra))
				error("Expected enum definition");
		}
		auto t_size = integer_size(t);
		char tv = is_class ? static_cast<char>(Type::Prim::Enum) : t;
		if (Token::type(n) == Token::Type::Operator) {
			auto o = Token::op(n);
			if (o == Token::Op::LBra) {	// define
				m_size = m_building_type_base;	// start from building type base
				n = next_exp();

				auto last_cur = m_cur;
				if (res)
					error("Multiple definition");
				else {	// create enum entry
					if (*id)	// otherwise anonymous enum
						cont_insert(m_cur, id);
					if (m_has_visib) {
						alloc(1);
						m_buffer[m_size++] = static_cast<char>(m_cur_visib);
					}
					m_cur = m_size;
					enum_ndx = m_cur;
					alloc(Cont::Enum::static_size);
					store(static_cast<char>(ContType::Enum));
					store_part<3>(last_cur);	// reference parent
					create_root();
					store(static_cast<char>(is_class));
					store(t);
				}

				uint32_t c = 0;
				while (true) {
					if (Token::is_op(n, Token::Op::RBra)) {
						n = next_exp();
						break;
					}
					if (Token::type(n) != Token::Type::Identifier)
						error("Expected id");
					token_nter(nn, n);
					n = next_exp();

					auto ins_val = [&]() {
						alloc(1 + (is_class ? 4 : 1) + t_size);
						store(static_cast<char>(ContType::Value));
						store(tv);
						if (is_class)
							store_part<3>(m_cur);
						m_size += ::store_part(m_buffer + m_size, t_size, c);
					};

					cont_insert(m_cur, nn);
					ins_val();
					if (!is_class) {
						cont_insert(last_cur, nn);
						ins_val();
					}

					if (Token::is_op(n, Token::Op::Comma))
						n = next_exp();
					c++;
				};

				m_cur = last_cur;
				goto wrote_def;
			} else if (o == Token::Op::Semicolon)
				error("Fwd enum declaration is illegal");
		}
		// reference
		if (res)
			enum_ndx = res;
		else
			error("No such enum");
		return n;

		// write def_base to res or id
		wrote_def:;
		m_building_type_base = m_size;
		alloc(cur_type_size);
		for (size_t i = 0; i < cur_type_size; i++)
			store(cur_type[i]);
		return n;
	}

	inline uint32_t skip_type(uint32_t t)
	{
		while (true) {
			auto c = Type::prim(m_buffer[t++]);
			if (c <= Type::Prim::U64) {
				t++;
				return t;
			}
			if (c == Type::Prim::Function) {
				t = skip_type(t);
				uint8_t s = m_buffer[t++];
				for (uint8_t i = 0; i < s; i++)
					t = skip_type(t);
				return t;
			}
			if (c == Type::Prim::Enum) {
				t += 6;
				return t;
			}
			if (c == Type::Prim::Struct) {
				auto s_ndx = load_part<3, uint32_t>(m_buffer + t) + Cont::Struct::temp_args_off;
				t += 6;
				t = skip_template_args(s_ndx, t);
				return t;
			}

			/*if (c == Type::Prim::Scope) {
			if (c == Type::Prim::TemplateArg) {
				t += 4;
				return t;
			}*/

			t++;
		}
	}

	inline uint32_t type_size(uint32_t t)
	{
		return skip_type(t) - t;
	}

	// nested_id should be at least 256 bytes long (maximum token size)
	// if nested_id is nullptr, means nested identified will be discarded
	// nested_id will be filled with a null-terminated string, of size 0 if not found
	inline const char* parse_type_base(const char *n, char *nested_id)
	{
		uint32_t off = m_size - m_building_type_base;
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
				uint8_t o = static_cast<uint8_t>(ob) - static_cast<uint8_t>(Token::Op::Void);
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
				if (ob == Token::Op::Enum) {
					t_type = ContType::Enum;
					n = parse_enum(t_ndx);
					n = acc_type_attrs(n, attrs);
					type = (attrs & TypeAttr::cv_mask) | static_cast<char>(Type::Prim::Enum);
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
				n = res_or_id(n, res, id);
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
					} else if (t == ContType::Enum) {
						t_type = ContType::Enum;
						t_ndx = res;
						type = (attrs & TypeAttr::cv_mask) | static_cast<char>(Type::Prim::Enum);
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
				store(m_buffer[t_ndx++]);
			m_buffer[base] |= attrs & TypeAttr::cv_mask;
		} else {
			alloc(1);
			store(type);
			if (t_type == ContType::Struct || t_type == ContType::Enum) {
				alloc(6);
				store_part<3>(t_ndx);
				store_part<3>(0);
			}
		}
		if (Token::is_op(n, Token::Op::LPar)) {
			n = next_exp();
			uint32_t extr_qual_base = m_size;
			n = parse_type_extr_qual(n, nested_id, true);
			if (Token::is_op(n, Token::Op::LPar)) {
				n = next_exp();
				load_chunk(ret, m_building_type_base + off, extr_qual_base);
				load_chunk(extr_qual, extr_qual_base, m_size);
				m_size = m_building_type_base + off;
				alloc(sizeof(extr_qual) + 2 + sizeof(ret));
				store_chunk(extr_qual);
				store(Type::Prim::Function);
				store_chunk(ret);

				uint32_t c_off = m_size++ - m_building_type_base;
				uint8_t c = 0;
				bool must_have_next = false;
				bool must_have_end = false;
				while (true) {
					if (Token::type(n) == Token::Type::Operator) {
						auto o = Token::op(n);
						if (o == Token::Op::RPar) {
							if  (must_have_next)
								error("Expected next arg");
							n = next_exp();
							break;
						} else if (o == Token::Op::Expand) {
							c |= static_cast<uint8_t>(0x80);
							n = next_exp();
							if (!Token::is_op(n, Token::Op::RPar))
								error("Expected ')'");
							break;
						}
					}
					if (must_have_end)
							error("Expected ')'");
					must_have_next = false;
					char id[256];
					n = parse_type(n, id, m_building_type_base, m_size - m_building_type_base);
					if (Token::type(n) == Token::Type::Identifier)
						n = next_exp();
					if (Token::is_op(n, Token::Op::Comma)) {
						n = next_exp();
						must_have_next = true;
					} else
						must_have_end = true;
					c++;
				}
				if (c == 1 && m_buffer[m_building_type_base + c_off + 1] == static_cast<char>(Type::Prim::Void)) {
					m_buffer[m_building_type_base + c_off] = 0;
					m_size = m_building_type_base + c_off + 1;
				} else
					m_buffer[m_building_type_base + c_off] = c;
			} else {
				load_chunk(ret, m_building_type_base + off, extr_qual_base);
				load_chunk(extr_qual, extr_qual_base, m_size);
				m_size = m_building_type_base + off;
				alloc(sizeof(extr_qual) + sizeof(ret));
				store_chunk(extr_qual);
				store_chunk(ret);
			}
		}
		return n;
	}

	inline const char* parse_type(const char *n, char *nested_id, uint32_t &ndx, uint32_t base_off = 0)
	{
		m_building_type_base = m_size - base_off;
		auto res = parse_type_base(n, nested_id);
		ndx = m_building_type_base;
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

	inline const char* parse_integral(const char *n, char &res)
	{
		auto exp_ndx = m_size;
		char id[256];
		uint32_t ndx;
		n = parse_type(n, id, ndx);
		if (ndx != exp_ndx || (m_size - ndx) != 1)
			error("Expected integral type");
		res = m_buffer[ndx];
		auto p = Type::prim(res);
		if (!(p >= Type::Prim::S8 && p <= Type::Prim::U32))
			error("Expected integer type");
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
		alloc(2);
		store(ContType::Member);
		store(sto);
		uint32_t t;
		char id[256];
		n = parse_type(n, id, t, base_off + 2);

		load_chunk(t_data, t, m_size);
		m_size = t;

		if (*id == 0) {
			auto b = m_buffer[t + base_off + 2];
			auto p = Type::prim(b);
			if (Token::is_op(n, Token::Op::Semicolon)) {
				if (p != Type::Prim::Struct && p != Type::Prim::Enum)
					error("Illegal end of statement");
				if (static_cast<char>(p) != b || sto)
					error("Can't qualify struct/enum declaration");
				m_size = t;	// remove in-building type
				return;
			}
			uint32_t res;
			n = res_or_id(n, res, id);
			if (res) {
				error("Not implemented");
			} else if (*id) {
				if (Token::is_op(n, Token::Op::LPar)) {
					n = next_exp();

					uint32_t base = m_size;
					alloc(sizeof(t_data) + 2);
					for (size_t i = 0; i < base_off + 2; i++)
						store(m_buffer[t + i]);
					store(Type::Prim::Function);
					for (size_t i = base_off + 2; i < sizeof(t_data); i++)
						store(t_data[i]);
					auto s = m_size++ - base;

					uint8_t c = 0;
					bool must_have_next = false;
					bool must_have_end = false;
					while (true) {
						if (Token::type(n) == Token::Type::Operator) {
							auto o = Token::op(n);
							if (o == Token::Op::RPar) {
								if  (must_have_next)
									error("Expected next arg");
								n = next_exp();
								break;
							} else if (o == Token::Op::Expand) {
								c |= static_cast<uint8_t>(0x80);
								n = next_exp();
								if (!Token::is_op(n, Token::Op::RPar))
									error("Expected ')'");
								n = next_exp();
								break;
							}
						}
						if (must_have_end)
							error("Expected ')'");
						must_have_next = false;
						char id[256];
						n = parse_type(n, id, base, m_size - base);
						if (Token::type(n) == Token::Type::Identifier)
							n = next_exp();
						if (Token::is_op(n, Token::Op::Comma)) {
							n = next_exp();
							must_have_next = true;
						} else
							must_have_end = true;
						c++;
					}
					if (c == 1 && m_buffer[base + s + 1] == static_cast<char>(Type::Prim::Void)) {
						m_buffer[base + s] = 0;
						m_size = base + s + 1;
					} else
						m_buffer[base + s] = c;

					{
						load_chunk(t_data, base, m_size);
						m_size = base;
						cont_insert(m_cur, id);
						alloc(sizeof(t_data));
						store_chunk(t_data);
					}

					if (!Token::is_op(n, Token::Op::Semicolon))
						error("Expected ';'");
					return;
				} else
					goto base_member;
			} else
				error("Expected id");
		}
		base_member:
		if (!Token::is_op(n, Token::Op::Semicolon))
			error("Expected ';'");
		cont_insert(m_cur, id);
		alloc(sizeof(t_data));
		store_chunk(t_data);
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
			} else if (o == Token::Op::Semicolon) {
				return;
			} else if (o == Token::Op::Typedef) {
				n = next_exp();
				alloc(1);
				m_buffer[m_size++] = static_cast<char>(ContType::Using);
				char id[256];
				uint32_t t;
				n = parse_type_id(n, id, t, base_off + 1);

				load_chunk(t_data, t, m_size);
				m_size = t;
				cont_insert(m_cur, id);
				alloc(sizeof(t_data));
				store_chunk(t_data);

				if (!Token::is_op(n, Token::Op::Semicolon))
					error("Expected ';'");
				return;
			} else if (o == Token::Op::Using) {
				n = next_exp();
				if (Token::type(n) != Token::Type::Identifier)
					error("Expected identifier");
				token_nter(nn, n);
				n = next_exp();
				if (!Token::is_op(n, Token::Op::Equal))
					error("Expected '='");
				n = next_exp();
				char id[256];
				uint32_t t;
				n = parse_type(n, id, t);

				load_chunk(t_data, t, m_size);
				m_size = t;
				cont_insert(m_cur, nn);
				alloc(1 + sizeof(t_data));
				store(ContType::Using);
				store_chunk(t_data);

				if (!Token::is_op(n, Token::Op::Semicolon))
					error("Expected ';'");
				return;
			} else if (o == Token::Op::Namespace) {
				n = next_exp();
				auto last_cur = m_cur;
				while (true) {
					if (Token::type(n) != Token::Type::Identifier)
						error("Expected identifier");
					token_nter(nn, n);
					if (cont_resolve(m_cur, nn, m_cur)) {
						if (cont_type(m_cur) != ContType::Namespace)
							error("Not a namespace");
					} else {
						if (cont_type(m_cur) != ContType::Namespace)
							error("Not a namespace");
						auto par = m_cur;
						cont_insert(par, nn);
						m_cur = m_size;
						alloc(Cont::cont_ovhead);
						store(static_cast<char>(ContType::Namespace));
						store_part<3>(par);
						create_root();
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
				m_cur = last_cur;
				return;
			} else if (o == Token::Op::Template) {
				parse_template();
				return;
			}
		}
		parse_memb(n, base_off);
	}

	inline uint32_t skip_template_args(uint32_t types, uint32_t args)
	{
		if (static_cast<uint8_t>(m_buffer[types++]) != 0)
			error("Not implemented");
		return args;
	}

	inline uint32_t skip_template_arg_type(uint32_t t)
	{
		t++;	// skip is_pack;
		if (m_buffer[t++])	// is_integral
			t += type_size(t);
		else {
			auto a = static_cast<Template::ArgType>(m_buffer[t++]);
			if (a == Template::ArgType::Template)
				t += template_args_types_size(t);
			t++;
		}
		return t;
	}

	inline uint32_t skip_template_args_types(uint32_t types)
	{
		auto c = static_cast<uint8_t>(m_buffer[types++]);
		for (uint8_t i = 0; i < c; i++)
			types = skip_template_arg_type(types);
		return types;
	}

	inline uint32_t template_args_types_size(uint32_t types)
	{
		return skip_template_args_types(types) - types;
	}

	const char* parse_template(void)
	{
		auto n = next_exp();
		if (!Token::is_op(n, Token::Op::Less))
			error("Expected '<'");
		while (true) {

		}
	}

public:
	Cpp(void)
	{
		m_cur = m_size;
		alloc(Cont::cont_ovhead);
		store(static_cast<char>(ContType::Namespace));
		store_part<3>(0);
		create_root();

		cont_insert(0, "__builtin_va_list");
		alloc(3);
		store(ContType::Using);
		store(Type::Prim::Ptr);
		store(Type::Prim::S8);
	}
	~Cpp(void)
	{
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

	inline size_t get_size(void) const
	{
		return m_size;
	}

	inline const char* get_buffer(void) const
	{
		return m_buffer;
	}

	struct Pressure
	{
		size_t buffer;
		size_t macros;

		size_t total(void) const
		{
			return buffer + macros;
		}
	};

	inline Pressure get_pressure(void) const
	{
		Pressure res;
		res.buffer = m_size;
		res.macros = m_pp.get_count();
		return res;
	}
};