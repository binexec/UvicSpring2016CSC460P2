#include "os.h"
#include "kernel.h"

#define DEBUG

extern void a_main();							//External entry point for application once kernel and OS has initialized


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


/*main is used for kernel only! use a_main for application and testing!*/
void main() 
{
   //Enable STDIN/OUT to UART redirection for debugging
   #ifdef DEBUG
	uart_init();
	uart_setredir();
	printf("STDOUT->UART!\n");
   #endif  
   
   a_main();
   
}

