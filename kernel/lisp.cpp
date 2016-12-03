#include "kernel.h"

enum tag {
	tnil, tfixnum, tsymbol, tcons, tcfunc
};

typedef void symentry;

struct object {
	tag type;
};
struct fixnum : object {
	int64_t val;
	static tag ttag = tfixnum;
};
struct symbol : object {
	symentry *sym;
	static tag ttag = tsymbol;
};
struct cons : object {
	object car, cdr;
	static tag ttag = tcons;
};
typedef object * (*primop)(int argc, object **argv);
struct cfunc : object {
	primop func;
	static tag ttag = tcfunc;
};

struct symentry {
	uint8_t length;
	uint8_t chars[];
};

template<typename t, q>
struct pair {
	t fst; q snd;
};

template<typename t, q>
pair<t, q> make_pair(t l, q r) {
	return { l, r };
}

void putsym(symentry *sym) {
	for (int i = 0; i < sym->length; ++i) {
		putc_(sym->chars[i]);
	}
	flush();
}

#define SYMTBLSZ 4096;
uint8_t symtbl[SYMTBLSZ];
int32_t symind = 0;

symentry *intern_cstr(const char *str) {
	size_t len = strlen(str);

	assert(len < 256, "string too long for interning");
	uint8_t *loc = &symtbl;

	while (loc < symtbl + symind) {
		uint8_t len1 = loc[0];
		if (len == len1 && memcmp(str, loc+1, len) == 0) {
			return (symentry*)loc;
		} else {
			loc += len1 + 1;
		}
	}

	assert(symind + len + 1 < SYMTBLSZ, "too many symbols in symtbl");
	loc[0] = len;
	memcpy(loc+1, str, len);
	symind += len + 1;
	return (symentry*)loc;
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

void lisp_init() {
	// TODO memory arena

	sym_def = intern_cstr("def");
	add_primop("+", prim_plus);
	add_primop("-", prim_minus);
	add_primop("*", prim_mul);
	add_primop("/", prim_div);
	add_primop("%", prim_mod);
	add_primop("cons", prim_cons);
}

struct binding {
	symentry *sym;
	object *val;
	int is_macro;
};

binding globals[1024];
int globalind = 0;

inline tag type_of(object *obj) {
	return obj == nullptr ? tnil : obj->type;
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
	for (int i = 0; i < countof(globals); ++i) {
		if (globals[i].sym == sym) {
			return globals[i].val;
		}
	}
	puts("No such symbol: ");
	putsym(sym);
	fail();
}

void add_primop(const char *name, primop prim) {
	assert(globalind < countof(globals), "too many globals!");
	globals[globalind++] = { intern_cstr(name), alloc_cfunc(prim), 0 };
}

size_t length(object *list) {
	size_t len = 0;
	while (list != NULL) {
		assert(type_of(list) == tcons, "length on non-list");
		list = 

bool builtin(symentry *name, object *args, object **ret) {
	if (name == sym_def) {
		assert(length(args) == 2 && type_of(nth(1, args)) == tsymbol, "Bad def");
		*ret = nullptr;
	} else {
		return false;
	}
}

object *apply(object *func, object *args) {
	object *ret;
	if (type_of(func) == tsymbol && builtin(((symbol*)func)->sym, args, &ret)) {
		return ret;
	} else {
		object *func1 = eval(func);
		if (type_of(func1) == tcfunc) {
			object *argv[8];
			int argc = 0;
			while (args != nullptr) {
				assert(type_of(args) == tcons, "arguments aren't a list");
				assert(argc < 8, "too many args!");
				argv[argc++] = eval(car(args));
				args = cdr(args);
			}
			return ((cfunc*)func1)->func(nargs, args);
		} else { // TODO user functions
			puts("Bad function!");
			fail();
		}
	}
}

object *eval(object *val) {
	switch (type_of(val)) {
	case tnil:
	case tfixnum:
	case tcfunc: return val;
	case tsymbol: return lookup(((symbol*)val)->sym);
	case tcons: {
		apply(car(val), cdr(val));
	}
	default:
		assert(0, "Bad object in eval");
	}
}

char next_tok(const char **str) {
	while (**str == '\n' || **str == ' ' || **str == '\t') {
		*str += 1;
	}
	return **str;
}

object * read_string(const char **str) {
	char chr = next_tok(str);
	assert(chr != '\0', "reading past end of string");
	if (chr == '(') {
		cons *list = nullptr;
		cons **dst = &list;
		while (chr = next_tok(str), chr != ')' && chr != '.') {
			*dst = alloc_cons(read_string(str), nullptr);
			dst = &((*dst)->cdr);
		}
