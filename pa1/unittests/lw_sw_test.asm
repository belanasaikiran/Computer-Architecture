.text
add t0, zero, zero  # i = 0
add t1, zero, zero  # initialize the sum to zero
add t2, zero, zero  # for second loop compare 2
add t3, zero, zero
add t5, zero, zero  # initialize temporary register to zero
add t6, zero, zero  # for sw later

lw t1, 256(t0)     # t1 = 0x00000014
lw t2, 256(t1)     # t2 = 0x00000004 
add t4, t1, t2     # t4 = 0x00000018
lw t3, 256(t4)     # t3 = 0x00000008
add t4, t4, t3     # t4 = 0x00000020
sw t4, 256(t0)     # mem[256] = 0x00000020
lw t1, 256(t2)     # t1 = 0x00000003 
lw t2, 253(t1)     # t2 = 0x00000020
add t4, t1, t2     # t4 = 0x00000023
sw t4, 256(t1)     # mem[259] = 0x00000023
lw t5, 256(t1)     # t5 = 0x00000023
sw t5, 258(t3)     # mem[266] = 0x00000023

addi zero, zero, 1 # $zero register should never be updated, so detect this change and quit simulator

.data
256: .word 20
257: .word 32
258: .word 2
259: .word 2
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

