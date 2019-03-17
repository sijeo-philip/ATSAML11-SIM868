#ifndef _YUTU_FW_SCHEDULER_H_
#define _YUTU_FW_SCHEDULER_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// The Maximum number of Tasks
#define SCHEDULER_MAX_TASKS 4
//#define SCHEDULER_MAX_STACK_SIZE 256
#define SCHEDULER_MAX_STACK_SIZE 264

// Enable Debugging
//#define OS_CONFIG_DEBUG

typedef uint32_t os_stack_t;

void scheduler_os_init(void);

bool scheduler_os_task_init(void (*handler)(void));

bool scheduler_os_start(uint32_t quanta);

void scheduler_os_thread_yield(void);

void delay_ms(uint32_t ms);

#endif
