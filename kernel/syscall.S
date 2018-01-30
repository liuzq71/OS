.global syscall
syscall:
	push	{ r0, r1, r2, r3 }
	stmdb	sp, { r4-r7,lr }
	ldmia	sp!, { r7 }
	ldmia   sp, { r0-r6 }
	sub		sp, sp, #20+4	@20为r4-r7,lr,4为r0(上上条指令)
    swi #0
	pop		{ r4-r7,lr }
	add	sp, sp, #16
	mov pc, lr