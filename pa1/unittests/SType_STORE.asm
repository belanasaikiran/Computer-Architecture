.text 
lw t0, 256(t0)     # t0 = 20
sw t0, 257(t2)     # mem[257] = 20
addi zero, zero, 1 # detect this change and quit simulator
.data
256: .word 20
257: .word 0
258: .word 0