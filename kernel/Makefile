.PHONY: all
all: kernel.bin kernel.elf

SOUT:=start.o
COUT:=kernel.o lisp.o
HDRS:=$(wildcard *.h)

kernel.bin: $(SOUT) $(COUT)
	ld -T kernel.ld --oformat=binary $(OUTS) -o kernel.bin

kernel.elf: $(SOUT) $(COUT)
	ld -T kernel.ld $(OUTS) -o kernel.elf

%.o: %.cpp $(HDRS)
	g++ -Og -ggdb -std=gnu++14 -c -o $@ $<

%.o: %.s $(HDRS)
	nasm -f elf64 -o $@ $<

.PHONY: clean
clean:
	rm kernel.bin kernel.elf $(OUTS)
