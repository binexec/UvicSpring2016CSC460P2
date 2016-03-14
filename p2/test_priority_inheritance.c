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
	for(int i=0;i<3;i++)
	{
		printf("Hello from R!\n");
	}
	Task_Terminate();
}

void task_q()
{
	printf("q: hello, gonna create R\n");
	Task_Create(task_r, 2, 0);
	printf("q: gonna try to lock mut\n");
	Mutex_Lock(mut);
	printf("q: I got into the mutex yeah! But I will let it go\n");
	Mutex_Lock(mut);
	printf("q: I am gonna die, good bye world\n");
	Task_Terminate();
}

void task_p()
{
	mut = Mutex_Init();
	printf("p:hello, gonna lock mut\n");
	Mutex_Lock(mut);
	printf("p: gonna create q\n");
	Task_Create(task_q, 1, 0);
	Task_Yield();
	for(int i=0;i<10;i++){
		printf("p: yeah I got the priority of q, gonna pray for %d times before unlock mutex\n", (10-i));
	}
	Mutex_Unlock(mut);
	Task_Yield();
	printf("p: if I appear at last, I lost the priority :(\n");
	Task_Terminate();
}

void a_main() {
	OS_Init();
	Task_Create(task_p, 3, 0);
	OS_Start();
}
