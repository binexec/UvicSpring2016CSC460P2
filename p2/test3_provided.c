#include <avr/io.h>
#include <util/delay.h>
#include "os.h"
//
// LAB - TEST 3
//  Noah Spriggs, Murray Dunne, Daniel McIlvaney
//
// EXPECTED RUNNING ORDER: P1,P2,P3,P2,P1,P3,P2,P1
//
// P1 sleep                                              lock 1                               unlock 1
// P2        lock 1  sleep                       lock 2                    unlock 2 unlock 1
// P3                       sleep  lock 2  sleep                 unlock 2
// 1 2 3 3 2 1 3 2 3 2 1

MUTEX mut1;
MUTEX mut2;

void Idle() {
    for(;;) {
    }
}

void Task_P1()
{
    Task_Sleep(30);
    Mutex_Lock(mut1);
    Mutex_Unlock(mut1);
    for(;;);
}

void Task_P2()
{
    Mutex_Lock(mut1);
    Task_Sleep(20);
    Mutex_Lock(mut2);
    Task_Sleep(10);
    Mutex_Unlock(mut2);
    Mutex_Unlock(mut1);
    for(;;);
}

void Task_P3()
{
    Task_Sleep(10);
    Mutex_Lock(mut2);
    Task_Sleep(20);
    Mutex_Unlock(mut2);
    for(;;);
}

void a_main()
{
    mut1 = Mutex_Init();
    mut2 = Mutex_Init();

    Task_Create(Task_P1, 1, 0);
    Task_Create(Task_P2, 2, 0);
    Task_Create(Task_P3, 3, 0);

    Task_Create(Idle, 10, 0);

	/*
    disable_LED(PORTL0);
    disable_LED(PORTL2);
    disable_LED(PORTL5);
    disable_LED(PORTL6);

    enable_LED(PORTL0);
    disable_LED(PORTL0);
	*/
    Task_Terminate();
}
