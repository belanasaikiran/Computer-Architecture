.text
lui t1, 999         # t1 = 0x003E7000
addi zero, zero, 1  # detect this change and quit simulator
.data
2048: .word 4302 # Welcome
2049: .word 5402 # to
2050: .word 2023 # CompArch