set architecture i386:x86-64
file kernel/kernel.elf
target remote localhost:1234
layout src
b read_string
