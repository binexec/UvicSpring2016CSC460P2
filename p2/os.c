#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include "os_internal.h"

#define DEBUG

#ifdef DEBUG
#include "uart/uart.h"
#endif 

extern void CSwitch();
extern void Exit_Kernel();
extern void Enter_Kernel();
void Task_Terminate(void);

static PD Process[MAXPROCESS];					//Contains the process descriptor for all tasks, regardless of their current state.
volatile static PD* Cp;							//The process descriptor of the currently RUNNING task. CP is used to pass information to the kernel telling it what to do.
volatile unsigned char *KernelSp;				//Pointer to the Kernel's own stack location.
volatile unsigned char *CurrentSp;				//Pointer to the stack location of the current running task. Used for saving into PD during ctxswitch.
volatile static unsigned int NextP;				//Which task in the process queue to dispatch next.
volatile static unsigned int KernelActive;		//Indicates if kernel has been initialzied by OS_Start().
volatile static unsigned int Tasks;				//Number of tasks created so far .
volatile static unsigned int last_PID = 0;		//Highest PID value created so far.
volatile static unsigned int err = NO_ERR;		//Error code for the previous kernel operation (if any)

/************************************************************************/
/*                           KERNEL FUNCTIONS                           */
/************************************************************************/

/* This internal kernel function is a part of the "scheduler". It chooses the next task to run, i.e., Cp. */
static void Dispatch()
{
   //Find the next READY task.
   //Note: if there is no READY task, then this will loop forever!.
   while(Process[NextP].state != READY) {
      NextP = (NextP + 1) % MAXPROCESS;
   }

   Cp = &(Process[NextP]);
   CurrentSp = Cp->sp;
   Cp->state = RUNNING;

   NextP = (NextP + 1) % MAXPROCESS;
}

/* Handles all low level operations for creating a new task */
static void Kernel_Create_Task(voidfuncptr f, PRIORITY py, int arg)
{
	int x;
	unsigned char *sp;
	PD *p;

	#ifdef DEBUG
	int counter = 0;
	#endif
	
	//Make sure the system can still have enough resources to create more tasks
	if (Tasks == MAXPROCESS)
	{
		err = MAX_PROCESS_ERR;
		return;
	}

	//Find a dead or empty PD slot to allocate our new task
	for (x = 0; x < MAXPROCESS; x++)
	if (Process[x].state == DEAD) break;
	
	++Tasks;
	p = &(Process[x]);
	
	/*The code below was agglomerated from Kernel_Create_Task_At;*/
	
	//Initializing the workspace memory for the new task
	sp = (unsigned char *) &(p->workSpace[WORKSPACE-1]);
	memset(&(p->workSpace),0,WORKSPACE);

	//Store terminate at the bottom of stack to protect against stack underrun.
	*(unsigned char *)sp-- = ((unsigned int)Task_Terminate) & 0xff;
	*(unsigned char *)sp-- = (((unsigned int)Task_Terminate) >> 8) & 0xff;
	*(unsigned char *)sp-- = 0x00;

	//Place return address of function at bottom of stack
	*(unsigned char *)sp-- = ((unsigned int)f) & 0xff;
	*(unsigned char *)sp-- = (((unsigned int)f) >> 8) & 0xff;
	*(unsigned char *)sp-- = 0x00;

	//Allocate the stack with enough memory spaces to save the registers needed for ctxswitch
	#ifdef DEBUG
	 //Fill stack with initial values for development debugging
	 for (counter = 0; counter < 34; counter++)
	 {
		 *(unsigned char *)sp-- = counter;
	 }
	#else
	 //Place stack pointer at top of stack
	 sp = sp - 34;
	#endif
	
	//Build the process descriptor for the new task
	p->pid = ++last_PID;
	p->pri = py;
	p->arg = arg;
	p->request = NONE;
	p->state = READY;
	p->sp = sp;				/* stack pointer into the "workSpace" */
	p->code = f;				/* function to be executed as a task */
	
	//No errors occured
	err = NO_ERR;
}

/**
  * This internal kernel function is the "main" driving loop of this full-served
  * model architecture. Basically, on OS_Start(), the kernel repeatedly
  * requests the next user task's next system call and then invokes the
  * corresponding kernel function on its behalf.
  *
  * This is the main loop of our kernel, called by OS_Start().
  */
static void Next_Kernel_Request() 
{
   Dispatch();  /* select a new task to run */

   while(1) {
       Cp->request = NONE; /* clear its request */

       /* activate this newly selected task */
       CurrentSp = Cp->sp;
       Exit_Kernel();    /* or CSwitch() */

       /* if this task makes a system call, it will return to here! */

        /* save the Cp's stack pointer */
       Cp->sp = CurrentSp;

       switch(Cp->request){
       case CREATE:
           Kernel_Create_Task(Cp->code, Cp->pri, Cp->arg);
           break;
       case YIELD:
	   case NONE:
           /* NONE could be caused by a timer interrupt */
          Cp->state = READY;
          Dispatch();
          break;
       case TERMINATE:
          /* deallocate all resources used by this task */
          Cp->state = DEAD;
          Dispatch();
          break;
       default:
          /* Houston! we have a problem here! */
          break;
       }
    } 
}

/************************************************************************/
/*						   RTOS API FUNCTIONS                           */
/************************************************************************/

/*This function initializes the RTOS and must be called before any othersystem calls.*/
void OS_Init() 
{
   int x;

   Tasks = 0;
   KernelActive = 0;
   NextP = 0;
	//Reminder: Clear the memory for the task on creation.
   for (x = 0; x < MAXPROCESS; x++) {
      memset(&(Process[x]),0,sizeof(PD));
      Process[x].state = DEAD;
   }
}

/* This function starts the RTOS after creating a few tasks.*/
void OS_Start() 
{   
   if ( (! KernelActive) && (Tasks > 0)) {
       Disable_Interrupt();
      /* we may have to initialize the interrupt vector for Enter_Kernel() here. */

      /* here we go...  */
      KernelActive = 1;
      Next_Kernel_Request();
      /* NEVER RETURNS!!! */
   }
}

/* OS call to create a new task */
PID Task_Create(voidfuncptr f, PRIORITY py, int arg)
{
   //Run the task creation through kernel if it's running already
   if (KernelActive ) 
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
   
   return last_PID;
}

/* The calling task terminates itself. */
void Task_Terminate()
{
	if (KernelActive) {
		Disable_Interrupt();
		Cp -> request = TERMINATE;
		Enter_Kernel();
		/* never returns here! */
	}
}

/* The calling task gives up its share of the processor voluntarily. Previously Task_Next() */
void Task_Yield() 
{
   if (KernelActive) {
     Disable_Interrupt();
     Cp ->request = YIELD;
     Enter_Kernel();
  }
}

int Task_GetArg()
{
	return 0;
}

void Task_Suspend(PID p)
{
	
}

void Task_Resume(PID p)
{
	
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
		_delay_ms(100);
		Task_Yield();
	}
}

void Pong()
{
	for(;;)
	{
		PORTB &= ~LED_PIN_MASK;		//Turn off onboard LED
		_delay_ms(100);
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
   Task_Create(Ping, 10, 0);
   Task_Create(Pong, 10, 0);
   OS_Start();
}

