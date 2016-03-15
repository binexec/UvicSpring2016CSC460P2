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
	PORTB |= (1<<PB1);	//pin 52 on
	//printf("p1: started, gonna sleep\n");
	PORTB &= ~(1<<PB1);	//pin 52 off
	Task_Sleep(10); // sleep 100ms
	PORTB |= (1<<PB1);	//pin 52 on
	//printf("p1: awake, gonna lock mutex\n");
	PORTB &= ~(1<<PB1);	//pin 52 off
	Mutex_Lock(mut);
	PORTB |= (1<<PB1);	//pin 52 on
	//printf("p1: mutex locked\n");
	PORTB &= ~(1<<PB1);	//pin 52 off
	for(;;);
}

void Task_P2(int parameter)
{
	PORTB |= (1<<PB2);	//pin 51 on
	//printf("p2: started, gonna sleep\n");
	PORTB &= ~(1<<PB2);	//pin 51 off
	Task_Sleep(20); // sleep 200ms
	PORTB |= (1<<PB2);	//pin 51 on
	//printf("p2: awake, gonna signal event\n");
	PORTB &= ~(1<<PB2);	//pin 51 off
	Event_Signal(evt);
	PORTB |= (1<<PB2);	//pin 51 on
	PORTB &= ~(1<<PB2);	//pin 51 off
	for(;;);
}

void Task_P3(int parameter)
{
	PORTB |= (1<<PB3);	//pin 50 on
	//printf("p3: started, gonna lock mutex\n");
	PORTB &= ~(1<<PB3);	//pin 50 off
	Mutex_Lock(mut);
	PORTB |= (1<<PB3);	//pin 50 on
	//printf("p3: locked mutex, wait on evt\n");
	PORTB &= ~(1<<PB3);	//pin 50 off
	Event_Wait(evt);
	PORTB |= (1<<PB3);	//pin 50 on
	//printf("p3: gonna unlock mutex\n");
	PORTB &= ~(1<<PB3);	//pin 50 off
	Mutex_Unlock(mut);
	PORTB |= (1<<PB3);	//pin 50 on
	PORTB &= ~(1<<PB3);	//pin 50 off
	for(;;);
}

/*Entry point for application*/

void a_main()
{
	//initialize pins
	DDRB |= (1<<PB1);	//pin 52
	DDRB |= (1<<PB2);	//pin 51
	DDRB |= (1<<PB3);	//pin 50
	
	OS_Init();						//init os	
	mut = Mutex_Init();
	evt = Event_Init();
	Task_Create(Task_P1, 1, 0);
	Task_Create(Task_P2, 2, 0);
	Task_Create(Task_P3, 3, 0);
	OS_Start();						//start os
}