.text
#Storing initial data
addi s0, zero, 0x123 
addi s1, zero, 0x456 

#Adding nop to clear the pipeline
addi zero, zero, 0 # nop
addi zero, zero, 0 # nop
addi zero, zero, 0 # nop
addi zero, zero, 0 # nop


#two dependant stalls
slt s3, s0, s1      
addi s2, s2, 10     
xor s4, s3, s2      

#Adding nop to clear the pipeline
addi zero, zero, 0 # nop
addi zero, zero, 0 # nop
addi zero, zero, 0 # nop
addi zero, zero, 0 # nop

#three dependant stalls
srl s5, s4, s3      
andi s6, s1, 333    
xor s7, s5, s6      
ori s8, s7, 111     

addi zero, zero, 1 # detect this change and quit simulator

.data
256: .word 20
257: .word 5402
258: .word 5302