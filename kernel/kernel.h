#pragma once

#include <stdint.h>

#define countof(x) (sizeof(x) / sizeof((x)[0]))
typedef uint64_t size_t;

void putc(char c);
void putc_(char c);
void puts(const char *s);
void flush(void);
inline void assert(int assertion, const char *message) {
	if (!assertion) {
		puts("Assertion failed: ");
		puts(message);
		fail();
	}
}
size_t strlen(const char *str);
void fail(void) __attribute__((noreturn));
void fail(const char *msg) __attribute__((noreturn)) {
	puts(msg);
	fail();
}
