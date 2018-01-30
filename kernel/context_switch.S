.global __switch_to
__switch_to:
	@保存context开始
		.extern debug_task

		mov ip,r0

		mrs r0, spsr
		stmia ip!, {r0}

		mrs	r2, cpsr
		@切换到svc模式下，加载SP,LR
		msr	cpsr_c, #0x53
		mov	r3,sp
		mov	r4,lr
		@切换回当前模式
		msr	cpsr_cxsf, r2

		stmia ip!, {r3}
		stmia ip!, {r4}

		ldr sp,=0x33d00000-14*4	@sp指向r0的位置
	
		ldmia sp!, {r2 - r8}	@ 加载sp中保存的R0~R6
		stmia ip!, {r2 - r8}	@ 存入ip指向的结构体中
		ldmia sp!, {r2 - r8}	@ 加载sp中保存的R7~R12,PC
		stmia ip!, {r2 - r8}	@ 存入ip指向的结构体中
		
	@保存context结束
	
	@恢复context开始
		mov ip,r1

		ldmia ip!, {r1}
		msr spsr, r1

		ldmia ip!, {r3}
		ldmia ip!, {r4}

		mrs	r2, cpsr
		@切换到svc模式下，加载SP,LR
		msr	cpsr_c, #0x53
		mov	sp,r3
		mov	lr,r4
		@切换回当前模式
		msr	cpsr_cxsf, r2

		ldr sp,=0x33d00000-14*4	@sp指向r0的位置
		
		ldmia ip!, {r2 - r8}	@ 加载iP中保存的R0~R6
		stmia sp!, {r2 - r8}	@ 存入sp指向的结构体中
		ldmia ip!, {r2 - r8}	@ 加载ip中保存的R7~R12,PC
		stmia sp!, {r2 - r8}	@ 存入sp指向的结构体中
		
		ldr sp,=0x33d00000-14*4	@sp指向r0的位置
		@bl debug_task
		ldmia sp!, {r0 - r12, pc}^	@ Load all regs saved previously
	
	@恢复context结束