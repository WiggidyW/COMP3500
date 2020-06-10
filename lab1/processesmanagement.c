
/*****************************************************************************\
* Laboratory Exercises COMP 3500                                              *
* Author: Saad Biaz                                                           *
* Updated 5/16/2019 to distribute to students to do Lab 1                     *
\*****************************************************************************/

/*****************************************************************************\
*                             Global system headers                           *
\*****************************************************************************/


#include "common.h"

/*****************************************************************************\
*                             Global data types                               *
\*****************************************************************************/

typedef enum {TAT,RT,CBT,THGT,WT} Metric;


/*****************************************************************************\
*                             Global definitions                              *
\*****************************************************************************/
#define MAX_QUEUE_SIZE 10 
#define FCFS            1 
#define SJF             2
#define RR              3 


#define MAXMETRICS      5 



/*****************************************************************************\
*                            Global data structures                           *
\*****************************************************************************/

ProcessControlBlock *PrevProcess;

/*****************************************************************************\
*                                  Global data                                *
\*****************************************************************************/

Quantity NumberofJobs[MAXMETRICS]; // Number of Jobs for which metric was collected
Average  SumMetrics[MAXMETRICS]; // Sum for each Metrics

/*****************************************************************************\
*                               Function prototypes                           *
\*****************************************************************************/

void                 ManageProcesses(void);
void                 NewJobIn(ProcessControlBlock whichProcess);
void                 BookKeeping(void);
Flag                 ManagementInitialization(void);
void                 LongtermScheduler(void);
void                 IO();
void                 CPUScheduler(Identifier whichPolicy);
ProcessControlBlock *SJF_Scheduler();
ProcessControlBlock *FCFS_Scheduler();
ProcessControlBlock *RR_Scheduler();
void                 Dispatcher();

/*****************************************************************************\
* function: main()                                                            *
* usage:    Create an artificial environment operating systems. The parent    *
*           process is the "Operating Systems" managing the processes using   *
*           the resources (CPU and Memory) of the system                      *
*******************************************************************************
* Inputs: ANSI flat C command line parameters                                 *
* Output: None                                                                *
*                                                                             *
* INITIALIZE PROGRAM ENVIRONMENT                                              *
* START CONTROL ROUTINE                                                       *
\*****************************************************************************/

int main (int argc, char **argv) {
   if (Initialization(argc,argv)){
     ManageProcesses();
   }
} /* end of main function */

/***********************************************************************\
* Input : none                                                          *
* Output: None                                                          *
* Function: Monitor Sources and process events (written by students)    *
\***********************************************************************/

void ManageProcesses(void){
  ManagementInitialization();
  while (1) {
    IO();
    CPUScheduler(PolicyNumber);
    Dispatcher(PolicyNumber);
  }
}

/* XXXXXXXXX Do Not Change IO() Routine XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */
/***********************************************************************\
* Input : none                                                          *          
* Output: None                                                          *        
* Function:                                                             *
*    1) if CPU Burst done, then move process on CPU to Waiting Queue    *
*         otherwise (for RR) return Process to Ready Queue              *                           
*    2) scan Waiting Queue to find processes with complete I/O          *
*           and move them to Ready Queue                                *         
\***********************************************************************/
void IO() {
  ProcessControlBlock *currentProcess = DequeueProcess(RUNNINGQUEUE); 
  if (currentProcess){
    if (currentProcess->RemainingCpuBurstTime <= 0) { // Finished current CPU Burst
      currentProcess->TimeEnterWaiting = Now(); // Record when entered the waiting queue
      EnqueueProcess(WAITINGQUEUE, currentProcess); // Move to Waiting Queue
      currentProcess->TimeIOBurstDone = Now() + currentProcess->IOBurstTime; // Record when IO completes
      currentProcess->state = WAITING;
    } else { // Must return to Ready Queue                
      currentProcess->JobStartTime = Now();                                               
      EnqueueProcess(READYQUEUE, currentProcess); // Mobe back to Ready Queue
      currentProcess->state = READY; // Update PCB state 
    }
  }

  /* Scan Waiting Queue to find processes that got IOs  complete*/
  ProcessControlBlock *ProcessToMove;
  /* Scan Waiting List to find processes that got complete IOs */
  ProcessToMove = DequeueProcess(WAITINGQUEUE);
  if (ProcessToMove){
    Identifier IDFirstProcess =ProcessToMove->ProcessID;
    EnqueueProcess(WAITINGQUEUE,ProcessToMove);
    ProcessToMove = DequeueProcess(WAITINGQUEUE);
    while (ProcessToMove){
      if (Now()>=ProcessToMove->TimeIOBurstDone){
        ProcessToMove->RemainingCpuBurstTime = ProcessToMove->CpuBurstTime;
        ProcessToMove->JobStartTime = Now();
        EnqueueProcess(READYQUEUE,ProcessToMove);
      } else {
        EnqueueProcess(WAITINGQUEUE,ProcessToMove);
      }
      if (ProcessToMove->ProcessID == IDFirstProcess){
        break;
      }
      ProcessToMove =DequeueProcess(WAITINGQUEUE);
    } // while (ProcessToMove)
  } // if (ProcessToMove)
}

/***********************************************************************\    
* Input : whichPolicy (1:FCFS, 2: SJF, and 3:RR)                       *        
* Output: None                                                         * 
* Function: Selects Process from Ready Queue and Puts it on Running Q. *
\***********************************************************************/
void CPUScheduler(Identifier whichPolicy) {
  ProcessControlBlock *selectedProcess;
  switch(whichPolicy){
    case FCFS : selectedProcess = FCFS_Scheduler();
      break;
    case SJF : selectedProcess = SJF_Scheduler();
      break;
    case RR   : selectedProcess = RR_Scheduler();
  }
  if (selectedProcess) {
    selectedProcess->state = RUNNING; // Process state becomes Running                                     
    EnqueueProcess(RUNNINGQUEUE, selectedProcess); // Put process in Running Queue                         
  }
}
/***********************************************************************\                                               
* Input : None                                                         *                                               
* Output: Pointer to the process based on First Come First Serve (FCFS)*
* Function: Returns process control block based on FCFS                *
\***********************************************************************/
ProcessControlBlock *FCFS_Scheduler() {
  /* Select Process based on FCFS */
  
  // simply return the tail of the queue
  return(DequeueProcess(READYQUEUE));
}



/***********************************************************************\                         
* Input : None                                                          *                                     
* Output: Pointer to the process with shortest remaining time (SJF)     *                                     
* Function: Returns process control block with SJF                      *                                     
\***********************************************************************/
ProcessControlBlock *SJF_Scheduler() {
  /* Select Process with Shortest Remaining Time*/

  // Get a pointer to the shortest process in the queue
  ProcessControlBlock *shortestProcess = Queues[READYQUEUE].Tail;
  ProcessControlBlock *currentProcess = shortestProcess->previous;
  while (currentProcess != NULL) {
    if (currentProcess->RemainingCpuBurstTime < shortestProcess->RemainingCpuBurstTime) {
      shortestProcess = currentProcess;
    }
    currentProcess = currentProcess->previous;
  }

  // Dequeue and Requeue the queue until we have the shortest process
  ProcessControlBlock *selectedProcess = DequeueProcess(READYQUEUE);
  while (selectedProcess != shortestProcess) {
    EnqueueProcess(READYQUEUE, selectedProcess);
    selectedProcess = DequeueProcess(READYQUEUE);
  }
  
  // return it
  return(selectedProcess);
}


/***********************************************************************\                                               
* Input : None                                                          *                                               
* Output: Pointer to the process based on Round Robin (RR)              *                                               
* Function: Returns process control block based on RR                   *
\***********************************************************************/
ProcessControlBlock *RR_Scheduler() {
  /* Select Process based on RR*/                                             

  // simply return the tail of the queue
  return(DequeueProcess(READYQUEUE));
}

/***********************************************************************\  
* Input : None                                                          *   
* Output: None                                                          *   
* Function:                                                             *
*  1)If process in Running Queue needs computation, put it on CPU       *
*              else move process from running queue to Exit Queue       *     
\***********************************************************************/
void Dispatcher(Identifier whichPolicy) {
  #define min(a,b) (((a) < (b)) ? (a) : (b))

  ProcessControlBlock *process = DequeueProcess(RUNNINGQUEUE);

  // Do nothing if queue is empty
  if (process == NULL) {
    return;
  }

  printf("---\n");
  printf("TotalJobDuration - %f\n", process->TotalJobDuration);
  printf("CpuBurstTime - %f\n", process->CpuBurstTime);
  printf("RemainingCpuBurstTime - %f\n", process->RemainingCpuBurstTime);
  printf("TimeInCpu - %f\n", process->TimeInCpu);
  printf("IOBurstTime - %f\n", process->IOBurstTime);
  printf("TimeIOBurstDone - %f\n", process->TimeIOBurstDone);
  printf("---\n");

  // Place process on CPU if it needs to run
  if (process->RemainingCpuBurstTime > 0) {
    TimePeriod burstTime;
    switch (whichPolicy) {
      case FCFS: burstTime = process->RemainingCpuBurstTime; break;
      case SJF: burstTime = process->RemainingCpuBurstTime; break;
      case RR: burstTime = min(process->RemainingCpuBurstTime, Quantum);
    }
    // This seems to not mutate the process at all
    OnCPU(process, burstTime);
    // so we will mutate it manually!
    process->RemainingCpuBurstTime = process->RemainingCpuBurstTime - burstTime;
    process->TimeInCpu = process->TimeInCpu + burstTime;
    // place it back in the running queue for IO to handle
    EnqueueProcess(process, RUNNINGQUEUE);
  }
  
  else {
    process = DequeueProcess(RUNNINGQUEUE);
    process->state = DONE;
    EnqueueProcess(EXITQUEUE, process);
    // Book-keeping
    NumberofJobs[TAT]++;
    NumberofJobs[RT]++;
    NumberofJobs[WT]++;
    NumberofJobs[THGT]++;
    SumMetrics[TAT] = SumMetrics[TAT] + (process->JobExitTime - process->JobArrivalTime);
    SumMetrics[RT] = SumMetrics[RT] + (process->StartCpuTime - process->JobArrivalTime);
    SumMetrics[CBT] = SumMetrics[CBT] + (process->TimeInCpu);
    SumMetrics[WT] = SumMetrics[WT] + (process->TimeInReadyQueue);
    // End Book-keeping
  }
}

/***********************************************************************\
* Input : None                                                          *
* Output: None                                                          *
* Function: This routine is run when a job is added to the Job Queue    *
\***********************************************************************/
void NewJobIn(ProcessControlBlock whichProcess){
  ProcessControlBlock *NewProcess;
  /* Add Job to the Job Queue */
  NewProcess = (ProcessControlBlock *) malloc(sizeof(ProcessControlBlock));
  memcpy(NewProcess,&whichProcess,sizeof(whichProcess));
  EnqueueProcess(JOBQUEUE,NewProcess);
  DisplayQueue("Job Queue in NewJobIn",JOBQUEUE);
  LongtermScheduler(); /* Job Admission  */
}


/***********************************************************************\
* Input : None                                                          *
* Output: None                                                          *                
* Function:                                                             *
* 1) BookKeeping is called automatically when 250 arrived               *
* 2) Computes and display metrics: average turnaround  time, throughput *
*     average response time, average waiting time in ready queue,       *
*     and CPU Utilization                                               *                                                     
\***********************************************************************/
void BookKeeping(void){
  double end = Now(); // Total time for all processes to arrive
  Metric m;

  printf("\n********* Processes Managemenent Numbers ******************************\n");
  printf("Policy Number = %d, Quantum = %.6f   Show = %d\n", PolicyNumber, Quantum, Show);
  printf("Number of Completed Processes = %d\n", NumberofJobs[THGT]);
  printf("ATAT=%f   ART=%f  CBT = %f  T=%f AWT=%f\n", 
	  SumMetrics[TAT] / NumberofJobs[TAT],
    SumMetrics[RT] / NumberofJobs[RT],
    SumMetrics[CBT] / Now(),
	  NumberofJobs[THGT] / Now(),
    SumMetrics[WT] / NumberofJobs[WT]
  );

  exit(0);
}

/***********************************************************************\
* Input : None                                                          *
* Output: None                                                          *
* Function: Decides which processes should be admitted in Ready Queue   *
*           If enough memory and within multiprogramming limit,         *
*           then move Process from Job Queue to Ready Queue             *
\***********************************************************************/
void LongtermScheduler(void){
  ProcessControlBlock *currentProcess = DequeueProcess(JOBQUEUE);
  while (currentProcess) {
    currentProcess->TimeInJobQueue = Now() - currentProcess->JobArrivalTime; // Set TimeInJobQueue
    currentProcess->JobStartTime = Now(); // Set JobStartTime
    EnqueueProcess(READYQUEUE,currentProcess); // Place process in Ready Queue
    currentProcess->state = READY; // Update process state
    currentProcess = DequeueProcess(JOBQUEUE);
  }
}


/***********************************************************************\
* Input : None                                                          *
* Output: TRUE if Intialization successful                              *
\***********************************************************************/
Flag ManagementInitialization(void){
  Metric m;
  for (m = TAT; m < MAXMETRICS; m++){
     NumberofJobs[m] = 0;
     SumMetrics[m]   = 0.0;
  }
  return TRUE;
}
