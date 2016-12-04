#include "kernel.h"
#include "lisp.h"

// TODO roots on the C stack need to not be there

template<typename t, typename q>
struct pair {
	t fst; q snd;
};

template<typename t, typename q>
pair<t, q> make_pair(t l, q r) {
	return { l, r };
}

void putsym(symentry *sym) {
	for (int i = 0; i < sym->length; ++i) {
		putc_(sym->chars[i]);
	}
	flush();
}

constexpr size_t SYMTBLSZ = 4096;
uint8_t symtbl[SYMTBLSZ];
int32_t symind = 0;

symentry *intern_strl(const char *str, size_t len) {
	assert(len < 256, "string too long for interning");
	uint8_t *loc = symtbl;

	while (loc < symtbl + symind) {
		uint8_t len1 = loc[0];
		if (len == len1 && memcmp((uint8_t*)str,loc+1, len) == 0) {
			return (symentry*)loc;
		} else {
			loc += len1 + 1;
		}
	}

	assert(symind + len + 1 < SYMTBLSZ, "too many symbols in symtbl");
	loc[0] = len;
	memcpy(loc+1, (uint8_t*)str, len);
	symind += len + 1;
	return (symentry*)loc;	
}

symentry *intern_cstr(const char *str) {
	return intern_strl(str, strlen(str));
}

symentry *sym_def;

uint8_t *leftspace;
uint8_t *rightspace;
uint64_t spacesize;

uint8_t *spaceend;
uint8_t *spacepos;

void collect(void) {
	fail("PHBBBBBBBBT");
}

object *gcalloc_(size_t sz) {
	assert(sz % 8 == 0, "tried to gcalloc unaligned");
	if (spacepos + sz > spaceend) {
		collect();
		assert(spacepos + sz > spaceend, "No GC space after collecting!");
	}
	uint8_t *ret = spacepos;
	spacepos += sz;
	return (object*)ret;
}

template<typename t>
inline t *gcalloc() {
	static_assert(sizeof(t)%8 == 0); // ensure we stay pointer-aligned
	t *ret = (t*)gcalloc_(sizeof(t));
	ret->type = t::ttag;
	return ret;
}

cfunc *alloc_cfunc(primop prim) {
	cfunc *ret = gcalloc<cfunc>();
	ret->func = prim;
	return ret;
}

cons *alloc_cons(object *lhs, object *rhs) {
	cons *ret = gcalloc<cons>();
	ret->car = lhs; ret->cdr = rhs;
	return ret;
}

fixnum *alloc_fixnum(int64_t val) {
	fixnum *ret = gcalloc<fixnum>();
	ret->val = val;
	return ret;
}

symbol *alloc_symbol(symentry *sym) {
	symbol *ret = gcalloc<symbol>();
	ret->sym = sym;
	return ret;
}

void setup_arena(uint8_t *memory, size_t memlen) {
	spacesize = (memlen / 2) & ~0x7;
	leftspace = memory;
	rightspace = memory + spacesize;
	spacepos = leftspace;
	spaceend = leftspace + spacesize;
}	

binding globals[1024];
uint64_t globalind = 0;

void add_global(symentry *sym, object *val, bool is_macro) {
	assert(globalind < countof(globals), "too many globals!");
	globals[globalind++] = { sym, val, is_macro };
}

void add_primop(const char *name, primop prim) {
	add_global(intern_cstr(name), alloc_cfunc(prim), 0);
}

inline tag type_of(object *obj) {
	return obj == nullptr ? tnil : obj->type;
}

// proper floored div/mod
int64_t div(int64_t num, int64_t den) {
	return (num >= 0 ? num : num-den+1) / den;
}
int64_t mod(int64_t num, int64_t den) {
	return num - div(num, den) * den;
}

namespace prim {
#define DYAD(name, rest) object *name(int argc, object **argv) {	\
		assert(argc == 2, "wrong #args to prim");				\
		object *lhs = argv[0], *rhs = argv[1];					\
		rest;													\
	}
#define MONAD(name, rest) object *name(int argc, object **argv) {	\
		assert(argc == 1, "wrong #args to prim");				\
		object *arg = argv[0];									\
		rest;													\
	}
#define INT_DYAD(name, exp) object *name(int argc, object **argv) {		\
		assert(argc == 2, "wrong #args to prim");						\
		assert(type_of(argv[0]) == tfixnum && type_of(argv[1]) == tfixnum, "non-numeric args to fixnum"); \
		int64_t x = ((fixnum*)argv[0])->val, y = ((fixnum*)argv[1])->val; \
		return alloc_fixnum(exp);										\
	}
	INT_DYAD(plus, x + y)
	INT_DYAD(minus, x - y)
	INT_DYAD(mul, x * y)
	INT_DYAD(div_, div(x, y))
	INT_DYAD(mod_, mod(x, y))
	DYAD(cons, { return alloc_cons(lhs, rhs); })
	MONAD(write_, { write(arg); return NULL; })
}

inline object *car(object *obj) {
	assert(type_of(obj) == tcons, "car on non-cons");
	return ((cons*)obj)->car;
}

inline object *cdr(object *obj) {
	assert(type_of(obj) == tcons, "cdr on non-cons");
	return ((cons*)obj)->cdr;
}

object *lookup(symentry *sym) {
	for (size_t i = 0; i < globalind; ++i) {
		if (globals[i].sym == sym) {
			return globals[i].val;
		}
	}
	puts("No such symbol: ");
	putsym(sym);
	fail();
}

size_t length(object *list) {
	size_t len = 0;
	while (list != NULL) {
		assert(type_of(list) == tcons, "length on non-list");
		list = cdr(list);
		len += 1;
	}
	return len;
}

object *nth(size_t i, object *list) {
	while(i-- > 0) {
		assert(type_of(list) == tcons, "nth of a non-list");
		list = cdr(list);
	}
	return car(list);
}

bool builtin(symentry *name, object *args, env *env, object **ret) {
	if (name == sym_def) {
		assert(length(args) == 2 && type_of(nth(0, args)) == tsymbol, "Bad def");
		symentry *sym = ((symbol*)nth(0, args))->sym;
		add_global(sym, eval(nth(1, args), env), 0);
		*ret = nullptr;
		return true;
	} else {
		return false;
	}
}

object *apply(object *func, object *args, env *env) {
	// puts("applying ");
	// write(func);
	// puts("\n");
	object *ret;
	if (type_of(func) == tsymbol && builtin(((symbol*)func)->sym, args, env, &ret)) {
		return ret;
	} else {
		object *func1 = eval(func, env);
		if (type_of(func1) == tcfunc) {
			object *argv[8];
			int argc = 0;
			while (args != nullptr) {
				assert(type_of(args) == tcons, "arguments aren't a list");
				assert(argc < 8, "too many args!");
				argv[argc++] = eval(car(args), env);
				args = cdr(args);
			}
			return ((cfunc*)func1)->func(argc, argv);
		} else if (type_of(func1) == tclosure) {
			// TODO user functions
			fail("how do");
		} else { 
			puts("Bad function: ");
			write(func);
			fail();
		}
	}
}

object *eval(object *val, env *env) {
	// puts("evaluating ");
	// write(val);
	// puts("\n");

	switch (type_of(val)) {
	case tnil:
	case tfixnum:
	case tcfunc:
	case tclosure: return val;
	case tsymbol: return lookup(((symbol*)val)->sym);
	case tcons: return apply(car(val), cdr(val), env);
	default:
		fail("Bad object in eval");
	}
}

inline char next_tok(const char **str) {
	char chr;
	while (chr = **str, chr == '\n' || chr == ' ' || chr == '\t') {
		*str += 1;
	}
	return chr;
}
const char symchars[] = "!$%&*+-/0123456789:<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[]^_abcdefghijklmnopqrstuvwxyz~";
bool issymchar(char c) {
	for (size_t i = 0; i < sizeof(symchars); ++i) {
		if (c == symchars[i]) return true;
	}
	return false;
}

fixnum *read_number(const char **str) {
	int64_t val = 0;
	bool negative = false;
	char dig;

	if (**str == '-') {
		negative = true;
		*str += 1;
	}

	while (dig = **str, dig >= '0' && dig <= '9') {
		val = val * 10 + (dig - '0');
		*str += 1;
	}

	return alloc_fixnum(negative ? -val : val);
}

symbol *read_sym(const char **str) {
	int length = 0;

	while (issymchar((*str)[length])) {
		length += 1;
	}
	const char *begin = *str;
	*str += length;
	symentry *sym = intern_strl(begin, length);
	return alloc_symbol(sym);
}

object *read_string(const char **str) {
	char chr = next_tok(str);
	assert(chr != '\0', "reading past end of string");
	if (chr == '(') {
		object *list = nullptr;
		object **dst = &list;

		*str += 1;
		while (chr = next_tok(str), chr != ')' && chr != '.') {
			*dst = alloc_cons(read_string(str), nullptr);
			dst = &(((cons*)*dst)->cdr);
		}
		if (chr == '.') {
			assert(list != nullptr, "(. blah) is not a valid sexp");
			*dst = read_string(str);
			assert(next_tok(str) == ')', "junk after (a . b");
		}
		(*str)++;
		return list;
	} else if (chr == '-' || (chr >= '0' && chr <= '9')) {
		char chr1 = *(*str + 1);
		if (chr == '-' && !(chr1 >= '0' && chr1 <= '9')) {
			return read_sym(str);
		} else {
			return read_number(str);
		}
	} else if (issymchar(chr)) {
		return read_sym(str);
	} else {
		fail("tried to read something weird");
	}
}

void write_list(object *list) {
	putc('(');
	goto begin;

	while (type_of(list) == tcons) {
		putc(' ');
	begin:
		write(car(list));
		list = cdr(list);
	}

	if (type_of(list) != tnil) {
		puts(" . ");
		write(list);
	}
	putc(')');
}

void write(object *obj) {
	switch (type_of(obj)) {
	case tnil:
		puts("()");
		break;
	case tfixnum:
		puti(((fixnum*)obj)->val);
		break;
	case tsymbol:
		putsym(((symbol*)obj)->sym);
		break;
	case tcons:
		write_list(obj);
		break;
	case tcfunc:
		puts("<CFUNC>");
		break;
	case tclosure:
		puts("<CLOSURE>");
		break;
	default:
		fail("I'm printing what now?");
	}
}

void lisp_init(uint8_t *memory, size_t memlen) {
	setup_arena(memory, memlen);

	sym_def = intern_cstr("def");
	add_primop("+", prim::plus);
	add_primop("-", prim::minus);
	add_primop("*", prim::mul);
	add_primop("/", prim::div_);
	add_primop("%", prim::mod_);
	add_primop("cons", prim::cons);
	add_primop("write", prim::write_);

	const char *str = &lispcode;
	do {
		eval(read_string(&str), NULL);
		next_tok(&str);
	} while (str < &endlispcode);
}
