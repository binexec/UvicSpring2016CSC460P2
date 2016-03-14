#include "LED_Test.h"
#include <avr/io.h>
#include <util/delay.h>
#include "os.h"
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
    Task_Sleep(10); // sleep 100ms
    Mutex_Lock(mut);
    for(;;);
}

void Task_P2(int parameter)
{
    Task_Sleep(20); // sleep 200ms
    Task_Resume(pid);
    for(;;);
}

void Task_P3(int parameter)
{
    Mutex_Lock(mut);
    Task_Suspend(pid);
    Mutex_Unlock(mut);
    for(;;);
}

void a_main()
{
    mut = Mutex_Init();
    evt = Event_Init();

    Task_Create(Task_P1, 1, 0);
    Task_Create(Task_P2, 2, 0);
    pid = Task_Create(Task_P3, 3, 0);

    Task_Create(Idle, 10, 0);

    disable_LED(PORTL0);
    disable_LED(PORTL2);
    disable_LED(PORTL5);
    disable_LED(PORTL6);

    enable_LED(PORTL0);
    disable_LED(PORTL0);

    Task_Terminate();
}
