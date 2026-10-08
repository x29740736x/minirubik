.section .text.start
.globl _start
_start:
    la sp, stack_top
    call main
    li a7, 93
    ecall
.section .bss
.balign 16
call_stack: .skip 1024
stack_top:
