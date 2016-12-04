#pragma once

#include <stdint.h>

#define countof(x) (sizeof(x) / sizeof((x)[0]))
#define NULL nullptr
typedef uint64_t size_t;

void putc(char c);
void putc_(char c);
void puts(const char *s);
void flush(void);
size_t strlen(const char *str);
void fail(void) __attribute__((noreturn));
inline void fail(const char *msg) __attribute__((noreturn));
inline void fail(const char *msg) {
	puts(msg);
	fail();
}
inline void assert(int assertion, const char *message) {
	if (!assertion) {
		puts("Assertion failed: ");
		puts(message);
		fail();
	}
}
template<typename t>
void puti_loop(t i) {
	int digit = i % 10, rest = i / 10;
	if (rest > 0) puti_loop(rest);
	putc('0' + digit);
}

template<typename t>
void puti(t i) {
	if (i == 0) putc('0');
	else if (i < 0) {
		putc('-');
		puti_loop(-i);
	} else {
		puti_loop(i);
	}
}

template<typename t>
void putu(t i) {
	if (i == 0) putc('0');
	else {
		puti_loop(i);
	}
}

void memcpy(uint8_t *dst, const uint8_t *src, size_t len);
void memset(uint8_t *dst, uint8_t val, size_t len);
int memcmp(const uint8_t *lhs, const uint8_t *rhs, size_t len);
size_t strlen(const char *str);

