.text
addi t0, zero, 10
addi t1, zero, 25
addi t2, zero, 20
addi t3, zero, 19
addi t4, zero, 18
sw t0, 256(zero)
add t5, t0, t1
and t6, t2, t3
or s0, t4, t1
sll s1, t1, t4
slt s2, t5, t3
srl s3, t6, t5
sub s4, s0, t0
xor s5, s1, t1
andi s6, s2, 778
lw s7, 256(s2)
ori t0, s0, 232
slli t1, s1, 2
srli t2, s2, 1
xori t3, s3, 213
lui s4, 501
sw t4, 257(s2)
addi, zero, zero, 1  # $zero register should never be updated, so detect this change and quit simulator

.data
256: .word 4302
257: .word 3666


