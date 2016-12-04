section .data
global lispcode
global endlispcode
lispcode:
incbin "code.lisp"
endlispcode: db 0x00
