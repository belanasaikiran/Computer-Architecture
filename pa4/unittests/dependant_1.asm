.text
#Storing initial data
addi s0, zero, 0x123 # set s0 = 0x123

#Adding nop to clear the pipeline
addi zero, zero, 0 # nop
addi zero, zero, 0 # nop
addi zero, zero, 0 # nop
addi zero, zero, 0 # nop


#interlock signal coming from decode
lui s2, 45678      
slli s3, s2, 10    

addi zero, zero, 1 # detect this change and quit simulator

.data
256: .word 20
257: .word 5402
258: .word 5302