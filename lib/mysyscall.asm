%include "sconst.inc"

_NR_xia		equ 0
INT_VECTOR_SYS_CALL		equ 0x90

global  xia

bits 32
[section .text]

xia:
	mov	eax,_NR_xia
	int	INT_VECTOR_SYS_CALL
	ret
