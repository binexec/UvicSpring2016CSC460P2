#include "kernel.h"

volatile static PD Process[MAXTHREAD];			//Contains the process descriptor for all tasks, regardless of their current state.
volatile static EVENT_TYPE Event[MAXEVENT];		//Contains all the event "counters" 
volatile static unsigned int NextP;				//Which task in the process queue to dispatch next.
volatile static unsigned int Tasks;				//Number of tasks created so far .

/*Variables accessed by OS*/
volatile  PD* Cp;		
volatile unsigned char *KernelSp;				//Pointer to the Kernel's own stack location.
volatile unsigned char *CurrentSp;				//Pointer to the stack location of the current running task. Used for saving into PD during ctxswitch.						//The process descriptor of the currently RUNNING task. CP is used to pass information from OS calls to the kernel telling it what to do.
volatile unsigned int last_PID = 0;				//Last (also highest) PID value created so far.
volatile unsigned int last_EVENT = 0;			//Last (also highest) EVENT value created so far.
volatile ERROR_TYPE err = NO_ERR;				//Error code for the previous kernel operation (if any)
volatile unsigned int KernelActive;				//Indicates if kernel has been initialzied by OS_Start().


/************************************************************************/
/*						  KERNEL-ONLY HELPERS                           */
/************************************************************************/

/*Returns the pointer of a process descriptor in the global process list, by searching for its PID*/
PD* findProcessByPID(int pid)
{
	int i;
	
	//Valid PIDs must be greater than 0.
	if(pid <=0)
	return NULL;
	
	for(i=0; i<MAXTHREAD; i++)
	{
		if (Process[i].pid == pid)
		return &(Process[i]);
	}
	
	//No process with such PID
	return NULL;
}

EVENT_TYPE* findEventByEventID(EVENT e)
{
	int i;
	
	//Ensure the request event ID is > 0
	if(e <= 0)
	{
		#ifdef DEBUG
		printf("findEventByID: The specified event ID is invalid!\n");
		#endif
		err = INVALID_ARG_ERR;
		return NULL;
	}
	
	//Find the requested Event
	for(i=0; i<MAXEVENT; i++)
	if(Event[i].id == e) break;
	
	//Ensure the event was actually found within the event list
	if(i == MAXEVENT && Event[MAXEVENT].id != Cp->request_arg)
	{
		#ifdef DEBUG
		printf("findEventByEventID: The requested event was not found!\n");
		#endif
		err = EVENT_NOT_FOUND_ERR;
		return NULL;
	}
	
	return &Event[i];
}

/************************************************************************/
/*				   		       OS HELPERS                               */
/************************************************************************/

/*Returns the PID associated with a function's memory address*/
int findPIDByFuncPtr(voidfuncptr f)
{
	int i;
	
	for(i=0; i<MAXTHREAD; i++)
	{
		if (Process[i].code == f)
			return Process[i].pid;
	}
	
	//No process with such PID
	return -1;
}

int getEventCount(EVENT e)
{
	EVENT_TYPE* e1 = findEventByEventID(e);
	
	if(e1 == NULL) 
		return 0;
		
	return e1->count;	
}

void clearEventCount(EVENT e)
{
	EVENT_TYPE* e1 = findEventByEventID(e);
	
	if(e1 == NULL)
		return;
		
	e1->count = 0;
}


/************************************************************************/
/*                    INTERNAL KERNEL ROUTINES                          */
/************************************************************************/

//This ISR processes all tasks that are currently sleeping and waking them up when their tick expires
ISR(TIMER1_COMPA_vect)
{
	int i;
	for(i=0; i<MAXTHREAD; i++)
	{
		//Process any active tasks that are sleeping
		if(Process[i].state == SLEEPING)
		{
			//If the current sleeping task's tick count expires, put it back into its READY state
			if(--Process[i].request_arg <= 0)
				Process[i].state = READY;
		}
		
		//Process any SUSPENDED tasks that were previously sleeping
		else if(Process[i].last_state == SLEEPING)
		{
			//When task_resume is called again, the task will be back into its READY state instead if its sleep ticks expired.
			if(--Process[i].request_arg <= 0)
				Process[i].last_state = READY;
		}
	}
}

/* Handles all low level operations for creating a new task */
void Kernel_Create_Task(voidfuncptr f, PRIORITY py, int arg)
{
	int x;
	unsigned char *sp;
	PD *p;

	#ifdef DEBUG
	int counter = 0;
	#endif
	
	//Make sure the system can still have enough resources to create more tasks
	if (Tasks == MAXTHREAD)
	{
		#ifdef DEBUG
		printf("Task_Create: Failed to create task. The system is at its process threshold.\n");
		#endif
		
		err = MAX_PROCESS_ERR;
		return;
	}

	//Find a dead or empty PD slot to allocate our new task
	for (x = 0; x < MAXTHREAD; x++)
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
	p->sp = sp;					/* stack pointer into the "workSpace" */
	p->code = f;				/* function to be executed as a task */
	
	//No errors occured
	err = NO_ERR;
}

/*TODO: Check for mutex ownership. If PID owns any mutex, ignore this request*/
static void Kernel_Suspend_Task() 
{
	//Finds the process descriptor for the specified PID
	PD* p = findProcessByPID(Cp->request_arg);
	
	//Ensure the PID specified in the PD currently exists in the global process list
	if(p == NULL)
	{
		#ifdef DEBUG
			printf("Kernel_Suspend_Task: PID not found in global process list!\n");
		#endif
		err = PID_NOT_FOUND_ERR;
		return;
	}
	
	//Ensure the task is not in a unsuspendable state
	if(p->state == DEAD || p->state == SUSPENDED)
	{
		#ifdef DEBUG
		printf("Kernel_Suspend_Task: Trying to suspend a task that's in an unsuspendable state %d!\n", p->state);
		#endif
		err = SUSPEND_NONRUNNING_TASK_ERR;
		return;
	}
	
	//Save its current state and set it to SUSPENDED
	p->last_state = p->state;
	p->state = SUSPENDED;
	err = NO_ERR;
}

static void Kernel_Resume_Task()
{
	//Finds the process descriptor for the specified PID
	PD* p = findProcessByPID(Cp->request_arg);
	
	//Ensure the PID specified in the PD currently exists in the global process list
	if(p == NULL)
	{
		#ifdef DEBUG
			printf("Kernel_Resume_Task: PID not found in global process list!\n");
		#endif
		err = PID_NOT_FOUND_ERR;
		return;
	}
	
	//Ensure the task is currently in the SUSPENDED state
	if(p->state != SUSPENDED)
	{
		#ifdef DEBUG
		printf("Kernel_Resume_Task: Trying to resume a task that's not SUSPENDED!\n");
		printf("CURRENT STATE: %d\n", p->state);
		#endif
		err = RESUME_NONSUSPENDED_TASK_ERR;
		return;
	}
	
	//Restore the previous state of the task
	p->state = p->last_state;
	p->last_state = SUSPENDED;			
	err = NO_ERR;
}

static void Kernel_Create_Event(void)
{
	int i;
	
	//Make sure the system's events are not at max
	if(last_EVENT >= MAXEVENT+1)
	{
		#ifdef DEBUG
		printf("Event_Init: Failed to create Event. The system is at its max event threshold.\n");
		#endif
		
		err = MAX_EVENT_ERR;
		return;
	}
	
	//Find an uninitialized Event slot
	for(i=0; i<MAXEVENT; i++)
		if(Event[i].id == 0) break;
	
	//Assign a new unique ID to the event. Note that the smallest valid Event ID is 1.
	Event[i].id = ++last_EVENT;
	Event[i].owner = 0;
	err = NO_ERR;
}

static void Kernel_Wait_Event(void)
{
	EVENT_TYPE* e = findEventByEventID(Cp->request_arg);
	
	if(e == NULL)
	{
		#ifdef DEBUG
		printf("Kernel_Wait_Event: Error finding requested event!\n");
		#endif
		return;
	}
	
	//Ensure no one else is waiting for this same event
	if(e->owner > 0 && e->owner != Cp->pid)
	{
		#ifdef DEBUG
			printf("Kernel_Wait_Event: The requested event is already being waited by PID %d\n", e->owner);
		#endif
		err = EVENT_NOT_FOUND_ERR;
		return;
	}
	
	//Set the owner of the requested event to the current task and put it into the WAIT EVENT state
	e->owner = Cp->pid;
	Cp->state = WAIT_EVENT;
	err = NO_ERR;
}

static void Kernel_Signal_Event(void)
{
	EVENT_TYPE* e = findEventByEventID(Cp->request_arg);
	PD *e_owner;
	
	if(e == NULL)
	{
		#ifdef DEBUG
		printf("Kernel_Wait_Event: Error finding requested event!\n");
		#endif
		return;
	}
	
	//Ensure the event is waited by someone
	if(e->owner == 0)
	{
		#ifdef DEBUG
		printf("Kernel_Signal_Event: The requested event is not being waited by anyone!\n");
		#endif
		err = SIGNAL_UNOWNED_EVENT_ERR;
		return;
	}
	
	//Increment the event counter
	e->count++;
	
	//Get the PD of the event owner
	e_owner = findProcessByPID(e->owner);
	
	//Signal the owner of the event by setting its state to READY if it's waiting actively
	if(e_owner->state == WAIT_EVENT)
		e_owner->state = READY;
	//Only other possibility is if the owner that is waiting has been suspended. In this case, do nothing and just let the counter increment.
	else if (!(e_owner->state == SUSPENDED && e_owner->last_state == WAIT_EVENT))
	{
		#ifdef DEBUG
		printf("Kernel_Signal_Event: Event Owner doesn't' seem to be expecting this event! Current state: %d!\n", e_owner->state);
		#endif
		err = EVENT_OWNER_NO_LONGER_WAITING_ERR;
	}
}

/************************************************************************/
/*                   CORE KERNEL SCHEDULING FUNCTIONS                   */
/************************************************************************/

/* This internal kernel function is a part of the "scheduler". It chooses the next task to run, i.e., Cp. */
static void Dispatch()
{
	int i = 0;
	
	//Find the next READY task by iterating through the process list
	while(Process[NextP].state != READY)
	{
		NextP = (NextP + 1) % MAXTHREAD;
		i++;
		
		//Not a single task is ready. We'll temporarily re-enable interrupt in case if one or more task is waiting on events/interrupts or sleeping
		if(i > MAXTHREAD) Enable_Interrupt();
	}
	
	//Now that we have a ready task, interrupts must be disabled for the kernel to function properly again.
	Disable_Interrupt();
	
	//Load the task's process descriptor into Cp
	Cp = &(Process[NextP]);
	CurrentSp = Cp->sp;
	Cp->state = RUNNING;
	
	//Increment NextP so the next dispatch will not run the same process (unless everything else isn't ready)
	NextP = (NextP + 1) % MAXTHREAD;
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
	Dispatch();	//Select an initial task to run

	//After OS initialization, this will be kernel's main loop
	while(1) 
	{
		//Clears the process' request fields
		Cp->request = NONE;
		//Cp->request_arg is not reset, because task_sleep uses it to keep track of remaining ticks

		//Load the current task's stack pointer and switch to its context
		CurrentSp = Cp->sp;
		Exit_Kernel();

		/* if this task makes a system call, it will return to here! */

		//Save the current task's stack pointer and proceed to handle its request
		Cp->sp = CurrentSp;

		switch(Cp->request)
		{
			case CREATE:
			Kernel_Create_Task(Cp->code, Cp->pri, Cp->arg);
			break;
			
			case TERMINATE:
			Cp->state = DEAD;			//Mark the task as DEAD so its resources will be recycled later when new tasks are created
			Dispatch();					//Dispatch is only needed if the syscall requires running a different task  after it's done
			break;
		   
			case SUSPEND:
			Kernel_Suspend_Task();
			break;
			
			case RESUME:
			Kernel_Resume_Task();
			break;
			
			case SLEEP:
			Cp->state = SLEEPING;
			Dispatch();					
			break;
			
			case CREATE_E:
			Kernel_Create_Event();
			break;
			
			case WAIT_E:
			Kernel_Wait_Event();
			Dispatch();
			break;
			
			case SIGNAL_E:
			Kernel_Signal_Event();
			break;
		   
			case YIELD:
			case NONE:					// NONE could be caused by a timer interrupt
			Cp->state = READY;
			Dispatch();
			break;
       
			//Invalid request code, just ignore
			default:
				err = INVALID_KERNET_REQUEST_ERR;
			break;
       }
    } 
}

	
/************************************************************************/
/* KERNEL BOOT                                                          */
/************************************************************************/

/*Sets up the timer needed for task_sleep*/
void Timer_init()
{
	/*Timer1 is configured for the task*/
	
	//Use Prescaler = 1024
	TCCR1B |= (1<<CS12)|(1<<CS10);
	TCCR1B &= ~(1<<CS11);
	
	//Use CTC mode (mode 4)
	TCCR1B |= (1<<WGM12);
	TCCR1B &= ~((1<<WGM13)|(1<<WGM11)|(1<<WGM10));
	
	OCR1A = TICK_LENG;			//Set timer top comparison value to ~10ms
	TCNT1 = 0;					//Load initial value for timer
	TIMSK1 |= (1<<OCIE1A);      //enable match for OCR1A interrupt
}

/*This function initializes the RTOS and must be called before any othersystem calls.*/
void OS_Init()
{
	int x;
	
	Tasks = 0;
	KernelActive = 0;
	NextP = 0;
	
	//Clear and initialize the memory used for tasks
	for (x = 0; x < MAXTHREAD; x++) {
		memset(&(Process[x]), 0, sizeof(PD));
		Process[x].state = DEAD;
	}
	
	//Clear and initialize the memory used for Events
	for (x = 0; x < MAXEVENT; x++) {
		memset(&(Event[x]), 0, sizeof(EVENT_TYPE));
		Event[x].id = 0;
	}
	
	/*Initialize and start Timer needed for sleep*/
	Timer_init();
}

/* This function starts the RTOS after creating a few tasks.*/
void OS_Start()
{
	if ( (! KernelActive) && (Tasks > 0))
	{
		Disable_Interrupt();
		
		/* we may have to initialize the interrupt vector for Enter_Kernel() here. */
			/* here we go...  */
		KernelActive = 1;
		Next_Kernel_Request();
		/* NEVER RETURNS!!! */
	}
}