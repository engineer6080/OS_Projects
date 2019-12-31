;;kernel.asm
bits 32				;nasm directive - 32 bit
section .text

global start
extern kmain			;kmain defined in c file

start:
	cli			;block interrupt
	mov esp, stack_space	;set stack pointer
	call kmain
	hlt			;halt CPU

section .bss
resb 8192			;8KB for stack
stack_space:
