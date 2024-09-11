.text 
addi t0, t0, 999    # t0 = 0x000003E7
addi t1, t1, 2133   # t1 = 0xFFFFF855
and t2, t0, t1      # t2 = 0x00000045
addi zero, zero, 1 # detect this change and quit simulator
.data
2048: .word 4302 # Welcome
2049: .word 5402 # to
2050: .word 2023 # CompArch