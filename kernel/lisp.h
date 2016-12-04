#pragma once

#include <stdint.h>

enum tag : uint8_t {
	tnil, tfixnum, tsymbol, tcons, tcfunc, tclosure
};

struct object;
struct symbol;

struct binding {
	symbol *sym;
	object *val;
	int is_macro;
};
struct env {
	binding bind;
	env *next;
};

struct object {
	tag type;
};
struct fixnum : object {
	int64_t val;
	static const tag ttag = tfixnum;
};
struct symbol : object {
	uint8_t length;
	uint8_t chars[];
	static const tag ttag = tsymbol;
};
struct cons : object {
	object *car, *cdr;
	static const tag ttag = tcons;
};
typedef object * (*primop)(int argc, object **argv);
struct cfunc : object {
	primop func;
	static const tag ttag = tcfunc;
};
struct closure : object {
	env *bindings;
	static const tag ttag = tclosure;
};

void lisp_init(uint8_t *memory, size_t memlen);
object *read_string(const char **str);
void write(object *obj);
object *apply(object *func, object *args, env *env);
object *eval(object *obj, env *env);

extern char lispcode, endlispcode;
