#include "kernel.h"

struct vgaslot {
	uint8_t chr, color;
};

static_assert(sizeof(vgaslot) == 2);

vgaslot * const VGAMEM = (vgaslot*)0xB8000;

void memcpy(uint8_t *dst, const uint8_t *src, size_t len) {
	for (uint64_t i = 0; i < len; ++i) {
		dst[i] = src[i];
	}
}

void memset(uint8_t *dst, uint8_t val, size_t len) {
	for (uint64_t i = 0; i < len; ++i) {
		dst[i] = val;
	}
}

int memcmp(const uint8_t *lhs, const uint8_t *rhs, size_t len) {
	for (uint64_t i = 0; i < len; ++i) {
		if (lhs[i] != rhs[i])
			return 1;
	}
	return 0;
}

size_t strlen(const char *str) {
	size_t ret = 0;
	while (str[ret] != '\0') ret += 1;
	return ret;
}

int cursor = 0;

const int maxline = 25;

vgaslot vgabuf[80*25];

void flush(void) {
	memcpy((uint8_t*)VGAMEM, (uint8_t*)&vgabuf, sizeof(vgabuf));
}

void scroll(void) {
	for (int i = 0; i < maxline-1; ++i) {
		memcpy((uint8_t*)(vgabuf+80*i), (uint8_t*)(vgabuf+80*(i+1)), 80*sizeof(vgaslot));
	}
	for (int i = 0; i < 80; ++i) {
		vgabuf[80*(maxline-1)+i].chr = ' ';
	}
	cursor = 0;
}

void putc_(char c) {
	if (c == '\n') {
		scroll();
	} else {
		if (cursor == 79) {
			cursor = 0;
			scroll();
		}
		vgabuf[80*(maxline-1)+cursor++] = { .chr = (uint8_t)c, .color = 0x07 };
	}
}

void putc(char c) {
	putc_(c);
	flush();
}

void puts(const char *s) {
	while (*s != '\0')
		putc_(*(s++));
	flush();
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

// unsigned specialization
template<typename t>
void putu(t i) {
	if (i == 0) putc('0');
	else {
		puti_loop(i);
	}
}

struct memblock {
	uint8_t *start;
	uint64_t length;
	uint32_t type, xattr;
	uint64_t pad;
};

static_assert(sizeof(memblock) == 32);

memblock * const MEMMAP = (memblock*)0x4000;

void usable_memory(void) {
	memblock zero = { 0 };
	memblock *map = MEMMAP;

	while (memcmp((uint8_t*)&zero, (uint8_t*)map, sizeof(memblock)) != 0) {
		puts("found memory at ");
		puti((uint64_t)map->start);
		puts(" to ");
		puti((uint64_t)map->start + map->length);
		puts(" type ");
		puti(map->type);
		puts("\n");
		map += 1;
	}
}

void fail(void) __attribute__((noreturn)) {
	puts("\nDEAD");
	while(1);
}

// arena is located immediately after BSS
extern uint8_t arenastart;
uint8_t *arena = &arenastart;
uint64_t arenalen;

void find_arena(void) {
	memblock zero = { 0 };
	memblock *map = MEMMAP;
	arenalen = 0;

	while (memcmp((uint8_t*)&zero, (uint8_t*)map, sizeof(memblock)) != 0) {
		if (map->start < arena && arena < map->start + map->length) {
			arenalen = (uint64_t)map->start + map->length - (uint64_t)arena;
			break;
		}
		map += 1;
	}

	assert(arenalen != 0, "Suitable arena not found!");
	puts("arena found at ");
	puti((uint64_t)arena);
	puts(" to ");
	puti((uint64_t)arena + arenalen);
	puts("\n");
}
		
volatile int dump;
void delay() {
	for (int i = 0; i < 1000000; ++i) {
		dump += 1;
	}
}

void spin() {
	const uint8_t chars[] = "/-\\|";
	while (true) {
		for (int i = 0; i < 4; ++i) {
			VGAMEM[0] = { chars[i], 0x07 };
			delay();
		}
	}
}

volatile int pause = 1;
extern "C"
void kmain() {
	puts("kernel started\n");
//	while (pause);
	find_arena();
	usable_memory();
	spin();
	while(1);
}
