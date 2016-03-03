#ifndef OS_INTERNAL_H_
#define OS_INTERNAL_H_

#include "os.h"

//Global configurations
#define WORKSPACE     256
#define MAXPROCESS   4

//Error code definitions
#define NO_ERR 0
#define MAX_PROCESS_ERR 1

//Misc macros
#define Disable_Interrupt()		asm volatile ("cli"::)
#define Enable_Interrupt()		asm volatile ("sei"::)
  
typedef enum process_states 
{ 
   DEAD = 0, 
   READY, 
   RUNNING 
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
   PID pid;
   PRIORITY pri;
   PROCESS_STATES state;
   KERNEL_REQUEST_TYPE request;
   int arg;									//Initial argument for the task (if specified)
   unsigned char *sp;						//stack pointer into the "workSpace"
   unsigned char workSpace[WORKSPACE];		//Data memory allocated to this process
   voidfuncptr  code;						//function to be executed as a task 
} PD;


#endif /* OS_INTERNAL_H_ */