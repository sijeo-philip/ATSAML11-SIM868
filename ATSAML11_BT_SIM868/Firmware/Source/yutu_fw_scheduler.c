#include "yutu_fw_scheduler.h"
#include "sam.h"                        // Device header
#define BUS_FREQ 	16000000

// Do not enable as this scheduling is yet to be implemented and tested.
//#define COOPERATIVE_SCHEDULER 1
	
//Variables for Response time out delay	
volatile bool delayTimerStart=false;
volatile int32_t respTimeOut;
volatile bool timeOutFlag = false;

volatile static uint32_t msTicks=0;
/*******************************************/


typedef enum {
	OS_TASK_IDLE =1,
	OS_TASK_ACTIVE
}os_task_status_t;


os_stack_t taskStack[SCHEDULER_MAX_TASKS][SCHEDULER_MAX_STACK_SIZE];

typedef struct{
	/*The stack pointer is the first element so that it is located at the same addres as the structure itself
		--which makes it possible to locate it safely from assembly implementation of PendSV_Handler. The compiler
		might add padding between other structure elements */
	volatile uint32_t stackPtr;
	void(*handler)(void);
	volatile os_task_status_t status;
}os_task_t;

static struct {
	os_task_t tasks[SCHEDULER_MAX_TASKS];
	volatile uint32_t currentTask;
	uint32_t size;
}m_task_table;

volatile os_task_t *osCurrentTask;
volatile os_task_t *osNextTask;

uint32_t MILLISEC_PRESCALER=0;

static void task_finished(void)
{
	/*This function is called when some task handler returns*/
	volatile uint32_t i=0;
	while(1)
		i++;
}

void scheduler_os_init(void)
{
	memset(&m_task_table,0,sizeof(m_task_table));
}

bool scheduler_os_task_init(void(*handler)(void))
{
	if(m_task_table.size >= SCHEDULER_MAX_TASKS-1)
		return false;
	__disable_irq();
	/*Initialize the task structure and set SP to the top of the stack minus 16 words (64 bytes) to 
	leave space for storing 16 registers */
	os_task_t *pTask =&m_task_table.tasks[m_task_table.size];
	pTask->handler = handler;
	pTask->stackPtr = (uint32_t)&taskStack[m_task_table.size][SCHEDULER_MAX_STACK_SIZE-16];
	pTask->status = OS_TASK_IDLE;
	
	/* Save special registers which will be restored on exc. return:
	-XPSR: Default value is 0x01000000
	-PC: Pointer to the handler function
	-LR: Pointer to a function to be called when the handler returns */
	taskStack[m_task_table.size][SCHEDULER_MAX_STACK_SIZE-1] = 0x01000000;
	taskStack[m_task_table.size][SCHEDULER_MAX_STACK_SIZE-2] = (uint32_t)handler;
	taskStack[m_task_table.size][SCHEDULER_MAX_STACK_SIZE-3] = (uint32_t)&task_finished;
	
	#ifdef OS_CONFIG_DEBUG
		uint32_t base = (m_task_table.size + 1)*1000;
		taskStack[m_task_table.size][SCHEDULER_MAX_STACK_SIZE-4]= base+12;   /*R12*/
		taskStack[m_task_table.size][SCHEDULER_MAX_STACK_SIZE-5]= base+3;		/*R3*/
		taskStack[m_task_table.size][SCHEDULER_MAX_STACK_SIZE-6]= base+2;		/*R2*/
		taskStack[m_task_table.size][SCHEDULER_MAX_STACK_SIZE-7]= base+1;		/*R1*/
		taskStack[m_task_table.size][SCHEDULER_MAX_STACK_SIZE-8]= base+0;		/*R0*/
		taskStack[m_task_table.size][SCHEDULER_MAX_STACK_SIZE-9]= base+7; 	/*R7*/
		taskStack[m_task_table.size][SCHEDULER_MAX_STACK_SIZE-10]= base+6;	/*R6*/
		taskStack[m_task_table.size][SCHEDULER_MAX_STACK_SIZE-11]= base+5;	/*R5*/
		taskStack[m_task_table.size][SCHEDULER_MAX_STACK_SIZE-12]= base+4;	/*R4*/
		taskStack[m_task_table.size][SCHEDULER_MAX_STACK_SIZE-13]= base+11;	/*R11*/
		taskStack[m_task_table.size][SCHEDULER_MAX_STACK_SIZE-14]= base+10;	/*R10*/
		taskStack[m_task_table.size][SCHEDULER_MAX_STACK_SIZE-15]= base+9;	/*R9*/
		taskStack[m_task_table.size][SCHEDULER_MAX_STACK_SIZE-16]= base+8;	/*R8*/
	#endif
	
	m_task_table.size++;
	__enable_irq();
	return true;
}


bool scheduler_os_start(uint32_t quanta)
{
	NVIC_SetPriority(PendSV_IRQn, 0xff); //Lowest Possible priority
	NVIC_SetPriority(SysTick_IRQn, 0x00); //Highest Possible priority
	MILLISEC_PRESCALER = BUS_FREQ/1000;
	//Start the SysTick Timer
	
	#ifdef COOPERATIVE_SCHEDULER
	SysTick->VAL   = 0UL;                                             /* Load the SysTick Counter Value */
  
	#else
	uint32_t ret_val = SysTick_Config((quanta*MILLISEC_PRESCALER)-1);
	if(ret_val !=0)
		return false;
	#endif
	//Start the first task
	osCurrentTask = &m_task_table.tasks[m_task_table.currentTask];
	
	__set_PSP(osCurrentTask->stackPtr+64); //Set PSP to the top of the task's stack
	__set_CONTROL(0x03); //Switch to PSP, unpriviledged mode;
	
	__ISB(); //Exec. ISB after changing control.
	osCurrentTask->handler();
	
	return true;
}

void SysTick_Handler(void)
{
	osCurrentTask = &m_task_table.tasks[m_task_table.currentTask];
	osCurrentTask->status = OS_TASK_IDLE;
	
	//Select Next Task
	m_task_table.currentTask++;
	if(m_task_table.currentTask >= m_task_table.size)
		m_task_table.currentTask=0;
	
	osNextTask = &m_task_table.tasks[m_task_table.currentTask];
	osNextTask->status = OS_TASK_ACTIVE;
	
	//Response Time out Implementation
	 if((delayTimerStart==true)&&(timeOutFlag==false))
	 {
		 respTimeOut--;
		 if(respTimeOut<=0){
			 timeOutFlag = true;
			 delayTimerStart = false;
		 }
	 }
	if(msTicks !=0)
		msTicks--;
	//Trigger PendSV which performs the actual context switch.
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk|SCB_ICSR_STTNS_Msk;
}

void delay_ms(uint32_t ms)
{
	msTicks = ms;
	while(msTicks);
}

void scheduler_os_thread_yield(void)
{
	#ifdef COOPERATIVE_SCHEDULER
	  osCurrentTask = &m_task_table.tasks[m_task_table.currentTask];
	osCurrentTask->status = OS_TASK_IDLE;
	
	//Select Next Task
	m_task_table.currentTask++;
	if(m_task_table.currentTask >= m_task_table.size)
		m_task_table.currentTask=0;
	
	osNextTask = &m_task_table.tasks[m_task_table.currentTask];
	osNextTask->status = OS_TASK_ACTIVE;
		NVIC_SetPendingIRQ(PendSV_IRQn);
	#else
	NVIC_SetPendingIRQ(SysTick_IRQn);
	#endif
  
	//SCB->ICSR |=SCB_ICSR_PENDSTSET_Msk;
}

//void PendSV_Handler(){
//	
//	__asm("CPSID 	I\n\t"			
//		"MRS 	R0, PSP\n\t"
//		".syntax unified\n\t"	
//		"SUBS	R0, #16\n\t" 	
//		"STMIA	R0!, {R4-R7}\n\t"
//		"MOV	R4,	R8\n\t"
//		"MOV	R5, R9\n\t"
//		"MOV	R6, R10\n\t"
//		"MOV	R7, R11\n\t"
//		"SUBS	R0, #32\n\t"
//		"STMIA	R0!, {R4-R7}\n\t"
//		"SUBS	R0, #16\n\t"
//		".syntax divided\n\t"
//	    "LDR 	R2, =osCurrentTask\n\t"
//		"LDR	R1,	[R2]\n\t"
//		"STR	R0, [R1]\n\t"
//		"LDR 	R2, =osNextTask\n\t"
//		"LDR	R1, [R2]\n\t"
//		"LDR	R0, [R1]\n\t"
//		"LDMIA	R0!, {R4-R7}\n\t"
//		"MOV	R8, R4\n\t"
//		"MOV 	R9, R5\n\t"
//		"MOV	R10, R6\n\t"
//		"MOV	R11, R7\n\t"
//		"LDMIA	R0!,{R4-R7}\n\t"
//		"MSR	PSP, R0\n\t"
//		"LDR	R0, =0xFFFFFFFD\n\t"
//		"CPSIE	I\n\t"
//		"BX		R0\n\t");
//}

