.text
addi t0, t0, 999    # t0 = 0x000003E7 
slti t1, t0, 1000   # t1 = 0x00000001
addi zero, zero, 1 # detect this change and quit simulator
.data
2048: .word 4302 # Welcome
2049: .word 5402 # to
2050: .word 2023 # CompArch