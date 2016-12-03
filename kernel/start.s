[BITS 64]
BEGIN equ 0x100000
STK equ 0x200000

extern kmain
global start
	
start:
	;; mov rbp, STK
	;; mov rsp, STK
	jmp kmain
