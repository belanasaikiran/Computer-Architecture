.text
#Storing initial data
addi s0, zero, 0x123 
addi s1, zero, 0x456 


#three dependant stalls
and s5, s0, s1      
lui s6, 333         
xor s7, s5, s6      
ori s8, s7, 111     

addi zero, zero, 1 # detect this change and quit simulator

.data
256: .word 20
257: .word 5402
258: .word 5302