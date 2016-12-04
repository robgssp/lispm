.PHONY: all
all: kernel.bin kernel.elf

SOUT:=start.o lispcode.o
COUT:=kernel.o lisp.o
HDRS:=$(wildcard *.h)

kernel.bin: $(SOUT) $(COUT)
	ld -T kernel.ld --oformat=binary $(SOUT) $(COUT) -o kernel.bin

kernel.elf: $(SOUT) $(COUT)
	ld -T kernel.ld $(SOUT) $(COUT) -o kernel.elf

%.o: %.cpp $(HDRS)
	g++ -Wall -Og -ggdb -std=gnu++14 -c -o $@ $<

%.o: %.s $(HDRS)
	nasm -f elf64 -o $@ $<

lispcode.o: code.lisp

.PHONY: clean
clean:
	rm kernel.bin kernel.elf $(COUT) $(SOUT)
