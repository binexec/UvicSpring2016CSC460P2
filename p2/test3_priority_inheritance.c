/*
 * test3_priority_inheritance.c
 *
 * Created: 2016-03-14 11:35:36 AM
 *  Author: Haoyan
 */ 
#include "os.h"
#include "kernel.h"

MUTEX mut;

void task_r()
{
	for(;;)
	{
		printf("Hello from R!\n");
	}
}

void task_q()
{
	printf("q: hello, gonna create R\n");
	Task_Create(task_r, 2, 0);
	printf("q: gonna try to lock mut\n");
	Mutex_Lock(mut);
	for(;;) {
		printf("hello from q\n");
	}
}

void task_p()
{
	mut = Mutex_Init();
	printf("p:hello, gonna lock mut\n");
	Mutex_Lock(mut);
	printf("p: gonna create q\n");
	Task_Create(task_q, 1, 0);
	Task_Yield();
	for(;;){
		printf("hello from p\n");
	}
}

void a_main() {
	OS_Init();
	Task_Create(task_p, 3, 0);
	OS_Start();
}
