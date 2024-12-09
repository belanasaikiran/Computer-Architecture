.text

addi s11, zero, 10 # Number of iterations around this to train BHSR
iterations:
addi s0, zero, 0
addi s1, zero, 0
addi s2, zero, 0 
addi s3, zero, 0
addi s4, zero, 0 
addi s5, zero, 0 
addi s6, zero, 0 
addi s7, zero, 0
addi s8, zero, 0 
addi s9, zero, 0
addi t0, zero, 0
addi t1, zero, 0
addi t2, zero, 0 
addi t3, zero, 0 
addi a0, zero, 0
addi a1, zero, 0
addi a2, zero, 0 
addi a3, zero, 0 
addi a4, zero, 0 
addi s10, s10, 1     # itr_id ++

addi s0, zero, 8   # outer loop upper bound
addi s1, zero, 3   # inner loop upper bound
addi s2, zero, 256 # memory address to keep track of 
add s3, zero, zero # i = 0  

outer_loop:
add s4, zero, zero # j = 0
    inner_loop:
    add t0, s3, s4 # i + j
    sw  t0, 0(s2)  # store i + j in memory
    addi s2, s2, 1 # increase memory pointer
    addi s4, s4, 1 # j++
    bne s4, s1, inner_loop
addi s3, s3, 1 # i++
bne s3, s0, outer_loop

bne s10, s11, iterations
addi zero, zero, 1 # terminate

.data
256: .word 4302
257: .word 5402
258: .word 5302