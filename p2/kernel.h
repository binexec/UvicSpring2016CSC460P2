#ifndef OS_INTERNAL_H_
#define OS_INTERNAL_H_

//Global configurations
#define WORKSPACE     256
#define MAXPROCESS   4

//Misc macros
#define Disable_Interrupt()		asm volatile ("cli"::)
#define Enable_Interrupt()		asm volatile ("sei"::)
  
//Definitions for potential errors the RTOS may come across
typedef enum error_codes
{
	NO_ERR  = 0,
	INVALID_KERNET_REQUEST_ERR,
	KERNEL_INACTIVE_ERR,
	MAX_PROCESS_ERR,
	PID_NOT_FOUND_ERR,
	SUSPEND_NONRUNNING_TASK_ERR,
	RESUME_NONSUSPENDED_TASK_ERR
} ERROR_TYPE;
  
typedef enum process_states 
{ 
   DEAD = 0, 
   READY, 
   RUNNING,
   SUSPENDED 
} PROCESS_STATES;

typedef enum kernel_request_type 
{
   NONE = 0,
   CREATE,
   YIELD,
   TERMINATE,
   SUSPEND,
   RESUME
} KERNEL_REQUEST_TYPE;


/*Process descriptor for a task*/
typedef struct ProcessDescriptor 
{
   PID pid;									//An unique process ID for this task.
   PRIORITY pri;							//The priority of this task, from 0 (highest) to 10 (lowest).
   PROCESS_STATES state;					//What's the current state of this task?
   KERNEL_REQUEST_TYPE request;				//What the task want the kernel to do (when needed).
   int request_arg;							//What value is needed for the specified kernel request.
   int arg;									//Initial argument for the task (if specified).
   unsigned char *sp;						//stack pointer into the "workSpace".
   unsigned char workSpace[WORKSPACE];		//Data memory allocated to this process.
   voidfuncptr  code;						//The function to be executed when this process is running.
} PD;


#endif /* OS_INTERNAL_H_ */