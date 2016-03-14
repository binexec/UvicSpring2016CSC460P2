/*
 * test2_mut_and_evt.c
 *
 * Created: 2016-03-14 11:26:49 AM
 * 
 */ 

//
// LAB - TEST 1
//	Noah Spriggs, Murray Dunne
//
//
// EXPECTED RUNNING ORDER: P1,P2,P3,P1,P2,P3,P1
//
// P1 sleep              lock(attept)            lock
// P2      sleep                     signal
// P3           lock wait                  unlock

#include "os.h"
#include "kernel.h"

EVENT evt;
MUTEX mut;

/************************************************************************/
/*							CODE FOR TESTING                            */
/************************************************************************/

void Task_P1(int parameter)
{
	printf("p1: started, gonna sleep\n");
	Task_Sleep(100); // sleep 100ms
	printf("p1: awake, gonna lock mutex\n");
	Mutex_Lock(mut);
	printf("p1: mutex locked\n");
	for(;;);
}

void Task_P2(int parameter)
{
	printf("p2: started, gonna sleep\n");
	Task_Sleep(200); // sleep 200ms
	printf("p2: awake, gonna signal event\n");
	Event_Signal(evt);
	for(;;);
}

void Task_P3(int parameter)
{
	printf("p3: started, gonna lock mutex\n");
	Mutex_Lock(mut);
	printf("p3: locked mutex, wait on evt\n");
	Event_Wait(evt);
	printf("p3: gonna unlock mutex\n");
	Mutex_Unlock(mut);
	for(;;);
}

/*Entry point for application*/
/*
void a_main()
{
	OS_Init();						//init os	
	mut = Mutex_Init();
	evt = Event_Init();
	Task_Create(Task_P1, 1, 0);
	Task_Create(Task_P2, 2, 0);
	Task_Create(Task_P3, 3, 0);
	OS_Start();						//start os
}*/