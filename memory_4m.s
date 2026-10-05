.text
.globl main

main:
    li t0, 0x20000000    
    li t1, 4194304         
    li t2, 1             

loop:
    sw t2, 0(t0)         
    addi t0, t0, 4       
    addi t1, t1, -4      
    bne t1, zero, loop

    li a0, 0
    li a7, 93
    ecall                