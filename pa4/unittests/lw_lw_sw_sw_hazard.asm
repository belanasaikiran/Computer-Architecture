.text
addi t0, zero, 1    #t0 = 1
addi t1, zero, 2    #t1 = 2
addi t2, zero, 3    #t2 = 3
addi a3, zero, 4    #a3 = 4
addi t3, zero, 10   #t3 = 10
addi t4, zero, 20   #t4 = 20
addi t5, zero, 30   #t5 = 30
addi t6, zero, 40   #t6 = 40

#Potential WAW hazards
sw t3, 259(t0)     # addr[260] = 10 
sw t4, 259(t0)     # addr[260] = 20
sw t5, 259(t1)     # addr[261] = 30
sw t6, 259(t2)     # addr[262] = 40

addi zero, zero, 0 # nop
addi zero, zero, 0 # nop
addi zero, zero, 0 # nop
addi zero, zero, 0 # nop
addi zero, zero, 0 # nop
addi zero, zero, 0 # nop

#Potential RAW Register Hazard
lw s0, 255(t0)     # s0 = 20
lw s1, 255(s0)     # s1 = 3
lw s2, 255(a3)     # s2 = 50
lw s3, 255(t1)     # s3 = 30




addi zero, zero, 1 # $zero register should never be updated, so detect this change and quit simulator

.data
256: .word 20
257: .word 30
258: .word 40
259: .word 50
260: .word 3
261: .word 3
262: .word 3
263: .word 3
264: .word 3
265: .word 3
266: .word 3
267: .word 3
268: .word 3
269: .word 3
270: .word 3
271: .word 3
272: .word 3
273: .word 3
274: .word 3
275: .word 3
276: .word 4
277: .word 3
278: .word 3
279: .word 3
280: .word 8
281: .word 3
282: .word 3
283: .word 3
284: .word 3
285: .word 3
286: .word 3
287: .word 3
288: .word 5