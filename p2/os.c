#include "os.h"
#include "kernel.h"

#define DEBUG

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
   {
		#ifdef DEBUG
			printf("Task_Create: Failed to create task. The system is at its process threshold.\n");
		#endif
		return 0;
   }
   
   #ifdef DEBUG
	printf("Created PID: %d\n", last_PID);
   #endif
   
   return last_PID;
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

EVENT Event_Init(void)
{
	if(!KernelActive){
		err = KERNEL_INACTIVE_ERR;
		return;
	}
	Disable_Interrupt();
	
	Cp->request = CREATE_E;
	Enter_Kernel();
	
	//Return zero as PID if the task creation process gave errors. Note that the smallest valid PID is 1
	if (err == MAX_EVENT_ERR)
	{
		#ifdef DEBUG
		printf("Event_Init: Failed to create Event. The system is at its max event threshold.\n");
		#endif
		return 0;
	}
	
	#ifdef DEBUG
	printf("Created Event: %d\n", last_EVENT);
	#endif
	
	return last_EVENT;
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
		//_delay_ms(100);
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
		//_delay_ms(100);
		Task_Sleep(30);
		Task_Yield();
	}
}

void suspend_pong()
{
	for(;;)
	{
		//_delay_ms(1000);
		Task_Sleep(100);
		printf("SUSPENDING PONG!\n");
		Task_Suspend(findPIDByFuncPtr(Pong));
		Task_Yield();
		
		//_delay_ms(1000);
		Task_Sleep(100);
		printf("RESUMING PONG!\n");
		Task_Resume(findPIDByFuncPtr(Pong));
		Task_Yield();
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
   Task_Create(Ping, 10, 210);
   Task_Create(Pong, 10, 205);
   Task_Create(suspend_pong, 10, 0);
   OS_Start();
   
}

