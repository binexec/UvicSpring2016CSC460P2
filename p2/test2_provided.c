//#include <avr/io.h>
//#include <util/delay.h>
#include "os.h"
#include "kernel.h"
//
// LAB - TEST 2
//  Noah Spriggs, Murray Dunne, Daniel McIlvaney

// EXPECTED RUNNING ORDER: P1,P2,P3,P1,P2,P3,P1
//
// P1 sleep              lock(attept)            lock
// P2      sleep                     resume
// P3           lock suspend               unlock

MUTEX mut;
EVENT evt;
volatile PID pid;

void Idle() {
    for(;;) {
    }
}

void Task_P1(int parameter)
{
	PORTB |= (1<<PB1);	//pin 52 on
	PORTB &= ~(1<<PB1);	//pin 52 off
    Task_Sleep(10); // sleep 100ms
	PORTB |= (1<<PB1);	//pin 52 on
	PORTB &= ~(1<<PB1);	//pin 52 off
	printf("p1: I woke up\n");
    Mutex_Lock(mut);
	printf("p1: locked mut\n");
	PORTB |= (1<<PB1);	//pin 52 on
	PORTB &= ~(1<<PB1);	//pin 52 off
    for(;;);
}

void Task_P2(int parameter)
{
	PORTB |= (1<<PB2);	//pin 51 on
	PORTB &= ~(1<<PB2);	//pin 51 off
	printf("p2: gonna sleep\n");
    Task_Sleep(20); // sleep 200ms
	printf("p2: woke up\n");
	PORTB |= (1<<PB2);	//pin 51 on
	PORTB &= ~(1<<PB2);	//pin 51 off
    Task_Resume(pid);
	PORTB |= (1<<PB2);	//pin 51 on
	PORTB &= ~(1<<PB2);	//pin 51 off
    for(;;);
}

void Task_P3(int parameter)
{
	PORTB |= (1<<PB3);	//pin 50 on
	PORTB &= ~(1<<PB3);	//pin 50 off
    Mutex_Lock(mut);
	PORTB |= (1<<PB3);	//pin 50 on
	PORTB &= ~(1<<PB3);	//pin 50 off
    Task_Suspend(pid);
	PORTB |= (1<<PB3);	//pin 50 on
	printf("p3: failed to suspend\n");
	PORTB &= ~(1<<PB3);	//pin 50 off
    Mutex_Unlock(mut);
	printf("p3: unlocked mutex\n");
	PORTB |= (1<<PB3);	//pin 50 on
	PORTB &= ~(1<<PB3);	//pin 50 off
    for(;;);
}

void a_main()
{
	OS_Init();
	//initialize pins
	DDRB |= (1<<PB1);	//pin 52
	DDRB |= (1<<PB2);	//pin 51
	DDRB |= (1<<PB3);	//pin 50
	
    mut = Mutex_Init();
    evt = Event_Init();

    Task_Create(Task_P1, 1, 0);
    Task_Create(Task_P2, 2, 0);
    pid = Task_Create(Task_P3, 3, 0);

    Task_Create(Idle, 10, 0);
	OS_Start();
}
