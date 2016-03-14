#include "os.h"
#include "kernel.h"

#define LED_PIN_MASK 0x80			//Pin 13 = PB7
EVENT e1;
MUTEX m1;
EVENT evt;
MUTEX mut;

/************************************************************************/
/*							CODE FOR TESTING                            */
/************************************************************************/

void Ping()
{
	for(;;)
	{
		PORTB |= LED_PIN_MASK;		//Turn on onboard LED
		printf("PING!\n");
		Task_Sleep(30);
		Task_Yield();
	}
}

void Pong()
{
	for(;;)
	{
		PORTB &= ~LED_PIN_MASK;		//Turn off onboard LED
		printf("PONG!\n");
		Task_Sleep(30);
		Task_Yield();
	}
}

void suspend_pong()
{
	for(;;)
	{
		Task_Sleep(100);
		printf("SUSPENDING PONG!\n");
		Task_Suspend(findPIDByFuncPtr(Pong));
		Task_Yield();
		
		Task_Sleep(100);
		printf("RESUMING PONG!\n");
		Task_Resume(findPIDByFuncPtr(Pong));
		Task_Yield();
	}
	
}

void Task_P1(int parameter)
{	
	//mut = Mutex_Init();
	//evt = Event_Init();
	printf("last mutex id is %d\n", Last_MutexID);
	printf("task 1 started and gonna lock the mut\n");
	Mutex_Lock(mut);
	printf("task 1 will sleep\n");
	Task_Sleep(100); // sleep 100ms
	printf("task 1 is awake, and will unlock mut\n");
	Mutex_Unlock(mut);
	for(;;);
}

void Task_P2(int parameter)
{	
	printf("task 2 started and gonna lock the mut\n");
	Mutex_Lock(mut);
	printf("task 2 got into mutex");
	for(;;);
}

void Task_P3(int parameter)
{
	for(;;);
}


void event_wait_test()
{

	//Test normal signaling
	e1 = Event_Init();
	printf("Waiting for event %d...\n", e1);
	Event_Wait(e1);
	printf("Signal for event %d received! Total events: %d\n\n\n", e1, getEventCount(e1));
	e1 = Event_Init();
	Task_Sleep(100);
	//Task_Yield();
	
	//Test pre signaling
	printf("Waiting for event %d...\n", e1);
	Event_Wait(e1);
	printf("Signal for event %d received! Total events: %d\n", e1, getEventCount(e1));
	Task_Yield();
}

void event_signal_test()
{
	printf("Signalling event %d...\n", e1);
	Event_Signal(e1);
	printf("SIGNALED!\n");
	Task_Yield();
	
	printf("Signalling event %d...\n", e1);
	Event_Signal(e1);
	Event_Signal(e1);
	printf("SIGNALED!\n");
	Task_Yield();
}

void priority1()
{
	for(;;)
	{
		printf("Hello from 1!\n");
		Task_Sleep(300);
	}
}

void priority2()
{
	for(;;)
	{
		printf("Hello from 2!\n");
		Task_Sleep(200);
	}
}

void priority3()
{
	for(;;)
	{
		printf("Hello from 3!\n");
		Task_Sleep(100);
	}
}

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
	for(;;){
		printf("hello from p\n");
	}
}


/*Entry point for application*/
void a_main()
{
	int test_set = 4;				//Which set of tests to run?

	OS_Init();
	
	/*These tasks tests ctxswitching, suspension, resume, and sleep*/
	if(test_set == 0)
	{
		DDRB = LED_PIN_MASK;			//Set pin 13 as output
		Task_Create(Ping, 6, 210);
		Task_Create(Pong, 6, 205);
		Task_Create(suspend_pong, 4, 0);
	}
	else if(test_set == 1)
	{
		/*These tasks tests events*/
		Task_Create(event_wait_test, 4, 0);
		Task_Create(event_signal_test, 5, 0);
	}
	else if(test_set == 2)
	{
		/*These tasks tests priority scheduling*/
		Task_Create(priority1, 1, 0);
		Task_Create(priority2, 2, 0);
		Task_Create(priority3, 3, 0);
	} else if (test_set == 3)
	{
		mut = Mutex_Init();
		evt = Event_Init();
		Task_Create(Task_P1, 1, 0);
		Task_Create(Task_P2, 2, 0);
		//Task_Create(Task_P3, 3, 0);
	} else if (test_set == 4)
	{
		Task_Create(task_p, 3, 0);
	}
	
	//mut = Mutex_Init();
	//evt = Event_Init();
	OS_Start();
}