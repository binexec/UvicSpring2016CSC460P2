#include "os.h"
#include "kernel.h"

#define DEBUG

/*test variables*/
EVENT e1;

/************************************************************************/
/*						   RTOS API FUNCTIONS                           */
/************************************************************************/

/* OS call to create a new task */
PID Task_Create(voidfuncptr f, PRIORITY py, int arg)
{
   //Run the task creation through kernel if it's running already
   if (KernelActive) 
   {
     Disable_Interrupt();
	 
	 //Fill in the parameters for the new task into CP
	 Cp->pri = py;
	 Cp->arg = arg;
     Cp->request = CREATE;
     Cp->code = f;

     Enter_Kernel();
   } 
   else 
	   Kernel_Create_Task(f,py,arg);		//If kernel hasn't started yet, manually create the task
   
   //Return zero as PID if the task creation process gave errors. Note that the smallest valid PID is 1
   if (err == MAX_PROCESS_ERR)
		return 0;
   
   #ifdef DEBUG
	printf("Created PID: %d\n", Last_PID);
   #endif
   
   return Last_PID;
}

/* The calling task terminates itself. */
/*TODO: CLEAN UP EVENTS AND MUTEXES*/
void Task_Terminate()
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}

	Disable_Interrupt();
	Cp -> request = TERMINATE;
	Enter_Kernel();			
}

/* The calling task gives up its share of the processor voluntarily. Previously Task_Next() */
void Task_Yield() 
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}

    Disable_Interrupt();
    Cp ->request = YIELD;
    Enter_Kernel();
}

int Task_GetArg()
{
	return 0;
}

void Task_Suspend(PID p)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	Disable_Interrupt();
	
	//Sets up the kernel request fields in the PD for this task
	Cp->request = SUSPEND;
	Cp->request_arg = p;
	//printf("SUSPENDING: %u\n", Cp->request_arg);
	Enter_Kernel();
}

void Task_Resume(PID p)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	Disable_Interrupt();
	
	//Sets up the kernel request fields in the PD for this task
	Cp->request = RESUME;
	Cp->request_arg = p;
	//printf("RESUMING: %u\n", Cp->request_arg);
	Enter_Kernel();
}

/*Puts the calling task to sleep for AT LEAST t ticks.*/
void Task_Sleep(TICK t)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	Disable_Interrupt();
	
	Cp->request = SLEEP;
	Cp->request_arg = t;

	Enter_Kernel();
}

/*Initialize an event object*/
EVENT Event_Init(void)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return 0;
	}
	Disable_Interrupt();
	
	Cp->request = CREATE_E;
	Enter_Kernel();
	
	//Return zero as PID if the event creation process gave errors. Note that the smallest valid event ID is 1
	if (err == MAX_EVENT_ERR)
		return 0;
	
	#ifdef DEBUG
	printf("Created Event: %d\n", Last_EventID);
	#endif
	
	return Last_EventID;
}

void Event_Wait(EVENT e)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	Disable_Interrupt();
	
	Cp->request = WAIT_E;
	Cp->request_arg = e;
	Enter_Kernel();
	
}

void Event_Signal(EVENT e)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	Disable_Interrupt();
	
	Cp->request = SIGNAL_E;
	Cp->request_arg = e;
	Enter_Kernel();	
}


/************************************************************************/
/*							CODE FOR TESTING                            */
/************************************************************************/

#define LED_PIN_MASK 0x80			//Pin 13 = PB7

void testSetup()
{
	DDRB = LED_PIN_MASK;			//Set pin 13 as output
}

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

void event_wait_test()
{

		//Test normal signaling
		e1 = Event_Init();
		printf("Waiting for event %d...\n", e1);
		Event_Wait(e1);
		printf("Signal for event %d received! Total events: %d\n", e1, getEventCount(e1));
		e1 = Event_Init();
		Task_Yield();
		
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

void main() 
{
   //Enable STDIN/OUT to UART redirection for debugging
   #ifdef DEBUG
	uart_init();
	uart_setredir();
	printf("STDOUT->UART!\n");
   #endif  
   
   testSetup();
   
   OS_Init();
   //Task_Create(Ping, 6, 210);
   //Task_Create(Pong, 6, 205);
   //Task_Create(suspend_pong, 4, 0);
   
   //Task_Create(event_wait_test, 5, 0);
   //Task_Create(event_signal_test, 5, 0);
   
   Task_Create(priority1, 1, 0);
   Task_Create(priority2, 2, 0);
   Task_Create(priority3, 3, 0);
   OS_Start();
   
}

