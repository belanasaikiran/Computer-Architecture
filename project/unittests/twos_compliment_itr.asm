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

lui s0, 0xFFFFF    # s0 = all '1' in 20 MSB
ori s0, s0, 0xFFF  # s1 = all '1' in MSB (constant)

# Words to test
addi s1, zero, 2023
addi s2, zero, -2023

# Convert t1 to +4302
xor t1, s2, s0     # flip all bits in s2
addi t2, t1, 1     # add 1 to t2

and t3, t2, s1     # Check mutual bits
bne t3, s1, fail
xori s0, s0, 0xAAA # Imprint pattern on s0 

fail:
bne s10, s11, iterations
addi zero, zero, 1 # terminate

.data
256: .word 10
257: .word 10