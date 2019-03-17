		AREA |.text|, CODE, READONLY, ALIGN=2
        THUMB
		EXTERN  osCurrentTask
		EXTERN	osNextTask
		EXPORT	PendSV_Handler


PendSV_Handler
		CPSID 	I			;DISABLE GLOBAL INTERRUPTS
		MRS 	R0, PSP		;READ THE CURRENT STACK POINTER
		SUBS	R0, #16 	; SUBSTRACTS 16 BYTES TO R4 REGISTER
		STMIA	R0!, {R4-R7}; SAVES THE CURRENT REGISTER VALUES FROM R4 TO R7
		MOV		R4,	R8		; 
		MOV		R5, R9
		MOV		R6, R10
		MOV		R7, R11
		SUBS	R0, #32
		STMIA	R0!, {R4-R7}
		SUBS	R0, #16
		
									;Save current task's SP:
		LDR 	R2, =osCurrentTask
		LDR		R1,	[R2]
		STR		R0, [R1]
		
									;Load next task's SP:
		LDR 	R2, =osNextTask
		LDR		R1, [R2]
		LDR		R0, [R1]
		
									;Load registers R4-R11 (32 bytes) from new PSP and make the PSP point to the end 
									;of the exception stack frame
									;. The NVIC hardware will restore remaining registers after returning form exception 
		LDMIA	R0!, {R4-R7}
		MOV		R8, R4
		MOV 	R9, R5
		MOV		R10, R6
		MOV		R11, R7
		LDMIA	R0!,{R4-R7}
		MSR		PSP, R0
									;EXC_RETURN -THREAD MODE WITH PSP
		LDR		R0, =0xFFFFFFFD
		
									;Enable Interrupts 
		CPSIE	I
		
		BX		R0
		
		ALIGN
		END
		
		
		
		
		