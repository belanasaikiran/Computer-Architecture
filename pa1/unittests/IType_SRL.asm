.text
addi t0, t0, 999    # t0 = 0x000003E7
srli t1, t0, 1      # t1 = 0x000001F3
addi zero, zero, 1 # detect this change and quit simulator
.data
2048: .word 4302 # Welcome
2049: .word 5402 # to
2050: .word 2023 # CompArch