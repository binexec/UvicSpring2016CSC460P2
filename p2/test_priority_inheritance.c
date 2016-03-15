/*
 * test3_priority_inheritance.c
 *
 * Created: 2016-03-14 11:35:36 AM
 *  Author: Haoyan
 */ 
/*
 * priority q > r > p
 * 
 * expected order
 * p q p q r p
 *
 * q                 create(r)  lock(attempt)					   lock(switch in)   terminate   
 * r																						  runs  terminate
 * p  lock creates(q)                         (gain priority)unlock                                           terminate
*/
#include "os.h"
#include "kernel.h"

MUTEX mut;

void task_r()
{
	PORTB |= (1<<PB2);	//pin 51 on
	PORTB &= ~(1<<PB2);	//pin 51 off
	Task_Terminate();
}

void task_q()
{
	//printf("q: hello, gonna create R\n");
	PORTB |= (1<<PB1);	//pin 52 on
	PORTB &= ~(1<<PB1);	//pin 52 off
	Task_Create(task_r, 2, 0);
	//printf("q: gonna try to lock mut\n");
	PORTB |= (1<<PB1);	//pin 52 on
	PORTB &= ~(1<<PB1);	//pin 52 off
	Mutex_Lock(mut);
	PORTB |= (1<<PB1);	//pin 52 on
	PORTB &= ~(1<<PB1);	//pin 52 off
	//printf("q: I got into the mutex yeah! But I will let it go\n");
	Mutex_Lock(mut);
	PORTB |= (1<<PB1);	//pin 52 on
	PORTB &= ~(1<<PB1);	//pin 52 off
	//printf("q: I am gonna die, good bye world\n");
	Task_Terminate();
}

void task_p()
{
	//printf("p:hello, gonna lock mut\n");
	PORTB |= (1<<PB3);	//pin 50 on
	PORTB &= ~(1<<PB3);	//pin 50 off
	Mutex_Lock(mut);
	//printf("p: gonna create q\n");
	PORTB |= (1<<PB3);	//pin 50 on
	PORTB &= ~(1<<PB3);	//pin 50 off
	Task_Create(task_q, 1, 0);
	Task_Yield();
	PORTB |= (1<<PB3);	//pin 50 on
	PORTB &= ~(1<<PB3);	//pin 50 off
	Mutex_Unlock(mut);
	Task_Yield();
	PORTB |= (1<<PB3);	//pin 50 on
	PORTB &= ~(1<<PB3);	//pin 50 off
	Task_Terminate();
}

void a_main() {
	//initialize pins
	DDRB |= (1<<PB1);	//pin 52
	DDRB |= (1<<PB2);	//pin 51
	DDRB |= (1<<PB3);	//pin 50
	
	OS_Init();
	mut = Mutex_Init();
	Task_Create(task_p, 3, 0);
	OS_Start();
}
