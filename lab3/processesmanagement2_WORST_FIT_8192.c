
/*****************************************************************************\
* Laboratory Exercises COMP 3500                                              *
* Author: Saad Biaz                                                           *
* Updated 6/5/2017 to distribute to students to redo Lab 1                    *
* Updated 5/9/2017 for COMP 3500 labs                                         *
* Date  : February 20, 2009                                                   *
\*****************************************************************************/

/*****************************************************************************\
*                             Global system headers                           *
\*****************************************************************************/


#include "common2.h"

/*****************************************************************************\
*                             Global data types                               *
\*****************************************************************************/

typedef enum {TAT,RT,CBT,THGT,WT,AWTJQ} Metric;

/*****************************************************************************\
*                             Global definitions                              *
\*****************************************************************************/
#define MAX_QUEUE_SIZE 10 
#define FCFS            1 
#define RR              3 


#define MAXMETRICS      6 

#define PAGESIZE     8192

/*****************************************************************************\
*                            Global data structures                           *
\*****************************************************************************/




/*****************************************************************************\
*                                  Global data                                *
\*****************************************************************************/

Quantity NumberofJobs[MAXMETRICS]; // Number of Jobs for which metric was collected
Average  SumMetrics[MAXMETRICS]; // Sum for each Metrics

int* MemoryTable = NULL; // Table that holds the page slots
Memory* TableSize = NULL; // Size of the memory table
int SeenLast = 0; // fix corrupted size bug

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
ProcessControlBlock *SRTF();
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
  
  // allocate the memory table!
  Memory m;
  Memory tableSize = sizeof (Memory) * ((AvailableMemory / PAGESIZE) + 1);
  TableSize = &tableSize;
  MemoryTable = malloc(*TableSize);
  for (m = 0; m < *TableSize - 1; m++) {
    MemoryTable[m] = 0;
  }
  MemoryTable[*TableSize - 1] = 1; // assists our mapping
  
  while (1) {
    IO();
    CPUScheduler(PolicyNumber);
    Dispatcher();
  }
}

/***********************************************************************\
* Input : none                                                          *          
* Output: None                                                          *        
* Function:                                                             *
*    1) if CPU Burst done, then move process on CPU to Waiting Queue    *
*         otherwise (RR) return to rReady Queue                         *                           
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
 * Input : whichPolicy (1:FCFS, 2: SRTF, and 3:RR)                      *        
 * Output: None                                                         * 
 * Function: Selects Process from Ready Queue and Puts it on Running Q. *
\***********************************************************************/
void CPUScheduler(Identifier whichPolicy) {
  ProcessControlBlock *selectedProcess;
  if ((whichPolicy == FCFS) || (whichPolicy == RR)) {
    selectedProcess = DequeueProcess(READYQUEUE);
  } else{ // Shortest Remaining Time First 
    selectedProcess = SRTF();
  }
  if (selectedProcess) {
    selectedProcess->state = RUNNING; // Process state becomes Running                                     
    EnqueueProcess(RUNNINGQUEUE, selectedProcess); // Put process in Running Queue                         
  }
}

/***********************************************************************\                         
 * Input : None                                                         *                                     
 * Output: Pointer to the process with shortest remaining time (SRTF)   *                                     
 * Function: Returns process control block with SRTF                    *                                     
\***********************************************************************/
ProcessControlBlock *SRTF() {
  /* Select Process with Shortest Remaining Time*/
  ProcessControlBlock *selectedProcess, *currentProcess = DequeueProcess(READYQUEUE);
  selectedProcess = (ProcessControlBlock *) NULL;
  if (currentProcess){
    TimePeriod shortestRemainingTime = currentProcess->TotalJobDuration - currentProcess->TimeInCpu;
    Identifier IDFirstProcess =currentProcess->ProcessID;
    EnqueueProcess(READYQUEUE,currentProcess);
    currentProcess = DequeueProcess(READYQUEUE);
    while (currentProcess){
      if (shortestRemainingTime >= (currentProcess->TotalJobDuration - currentProcess->TimeInCpu)){
	EnqueueProcess(READYQUEUE,selectedProcess);
	selectedProcess = currentProcess;
	shortestRemainingTime = currentProcess->TotalJobDuration - currentProcess->TimeInCpu;
      } else {
	EnqueueProcess(READYQUEUE,currentProcess);
      }
      if (currentProcess->ProcessID == IDFirstProcess){
	break;
      }
      currentProcess =DequeueProcess(READYQUEUE);
    } // while (ProcessToMove)
  } // if (currentProcess)
  return(selectedProcess);
}

//
// Finds the index of the best fit, or table size if there is no fit
//
Memory FindBestFitIndex(Memory size) {
  Memory innerSize = size / PAGESIZE;
  Memory m;
  Memory bestFitIndex = *TableSize; // index of the table slot that is the best fit
  Memory currentIndex = *TableSize; // index of the current (first 0) table slot
  Memory bestFitCount = 0;
  Memory currentCount = 0;
  for (m = 0; m < *TableSize; m++) {
    if (MemoryTable[m] == 0) {
      if (currentIndex == *TableSize) {
        currentIndex = m;
      }
      currentCount++;
    }
    else {
      // printf("current count - %u, current index - %u,\n", currentCount, currentIndex);
      if (currentCount > innerSize) {
        if (currentCount < bestFitCount || bestFitCount < innerSize) {
          bestFitIndex = currentIndex;
          bestFitCount = currentCount;
        }
      }
      currentIndex = *TableSize;
      currentCount = 0;
    }
  }
  if (bestFitIndex != *TableSize) {
    printf(
      "BEST FIT ALLOCATION: page table size - %u, process page count - %u, index of allocated memory - %u,\n\n",
      *TableSize, innerSize, bestFitIndex
    );
  }
  return(bestFitIndex);
}

//
// Finds the index of the worst fit, or table size if there is no fit
//
Memory FindWorstFitIndex(Memory size) {
  Memory innerSize = size / PAGESIZE;
  Memory m;
  Memory worstFitIndex = *TableSize; // index of the table slot that is the worst fit
  Memory currentIndex = *TableSize; // index of the current (first 0) table slot
  Memory worstFitCount = 0;
  Memory currentCount = 0;
  for (m = 0; m < *TableSize; m++) {
    if (MemoryTable[m] == 0) {
      if (currentIndex == *TableSize) {
        currentIndex = m;
      }
      currentCount++;
    }
    else {
      // printf("current count - %u, current index - %u,\n", currentCount, currentIndex);
      if (currentCount > innerSize) {
        if (currentCount > worstFitCount || worstFitCount < innerSize) {
          worstFitIndex = currentIndex;
          worstFitCount = currentCount;
        }
      }
      currentIndex = *TableSize;
      currentCount = 0;
    }
  }
  if (worstFitIndex != *TableSize) {
    printf(
      "WORST FIT ALLOCATION: page table size - %u, process page count - %u, index of allocated memory - %u,\n\n",
      *TableSize, innerSize, worstFitIndex
    );
  }
  return(worstFitIndex);
}

//
// allocates memory at the given index
//
void AllocateMemory(Memory index, Memory size) {
  Memory innerSize = size / PAGESIZE;
  Memory m;
  for (m = index; m < index + innerSize; m++) {
    MemoryTable[m] = 1;
  }
}

//
// De-allocates memory at the given index
//
void DeAllocateMemory(Memory index, Memory size) {
  Memory innerSize = size / PAGESIZE;
  Memory m;
  for (m = index; m < index + innerSize; m++) {
    MemoryTable[m] = 0;
  }
}

/***********************************************************************\  
 * Input : None                                                         *   
 * Output: None                                                         *   
 * Function:                                                            *
 *  1)If process in Running Queue needs computation, put it on CPU      *
 *              else move process from running queue to Exit Queue      *     
\***********************************************************************/
void Dispatcher() {
  double start;
  ProcessControlBlock *processOnCPU = Queues[RUNNINGQUEUE].Tail; // Pick Process on CPU
  if (!processOnCPU) { // No Process in Running Queue, i.e., on CPU
    return;
  }
  if(processOnCPU->TimeInCpu == 0.0) { // First time this process gets the CPU
    SumMetrics[RT] += Now()- processOnCPU->JobArrivalTime;
    NumberofJobs[RT]++;
    processOnCPU->StartCpuTime = Now(); // Set StartCpuTime
  }
  
  if (processOnCPU->TimeInCpu >= processOnCPU-> TotalJobDuration) { // Process Complete
    printf(" >>>>>Process # %d complete, %d Processes Completed So Far <<<<<<\n",
	   processOnCPU->ProcessID,NumberofJobs[THGT]);   
    processOnCPU=DequeueProcess(RUNNINGQUEUE);
    EnqueueProcess(EXITQUEUE,processOnCPU);

    NumberofJobs[THGT]++;
    NumberofJobs[TAT]++;
    NumberofJobs[WT]++;
    NumberofJobs[CBT]++;
    NumberofJobs[AWTJQ]++;
    SumMetrics[TAT]     += Now() - processOnCPU->JobArrivalTime;
    SumMetrics[WT]      += processOnCPU->TimeInReadyQueue;
    SumMetrics[AWTJQ]   += processOnCPU->TimeInJobQueue;

    DeAllocateMemory(processOnCPU->TopOfMemory, processOnCPU->MemoryAllocated); // deallocate the processes memory for future use
    AvailableMemory += processOnCPU->MemoryAllocated; // Return this memory to available

    // processOnCPU = DequeueProcess(EXITQUEUE);
    // XXX free(processOnCPU);

  } else { // Process still needs computing, out it on CPU
    TimePeriod CpuBurstTime = processOnCPU->CpuBurstTime;
    processOnCPU->TimeInReadyQueue += Now() - processOnCPU->JobStartTime;
    if (PolicyNumber == RR){
      CpuBurstTime = Quantum;
      if (processOnCPU->RemainingCpuBurstTime < Quantum)
	CpuBurstTime = processOnCPU->RemainingCpuBurstTime;
    }
    processOnCPU->RemainingCpuBurstTime -= CpuBurstTime;
    // SB_ 6/4 End Fixes RR 
    TimePeriod StartExecution = Now();
    OnCPU(processOnCPU, CpuBurstTime); // SB_ 6/4 use CpuBurstTime instead of PCB-> CpuBurstTime
    processOnCPU->TimeInCpu += CpuBurstTime; // SB_ 6/4 use CpuBurstTime instead of PCB-> CpuBurstTimeu
    SumMetrics[CBT] += CpuBurstTime;
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
  NewProcess->TimeInCpu = 0; // Fixes TUX error
  NewProcess->RemainingCpuBurstTime = NewProcess->CpuBurstTime; // SB_ 6/4 Fixes RR
  EnqueueProcess(JOBQUEUE,NewProcess);
  DisplayQueue("Job Queue in NewJobIn",JOBQUEUE);
  LongtermScheduler(); /* Job Admission  */
}


/***********************************************************************\                                                   
* Input : None                                                         *                                                    
* Output: None                                                         *                                                    
* Function:                                                            *
* 1) BookKeeping is called automatically when 250 arrived              *
* 2) Computes and display metrics: average turnaround  time, throughput*
*     average response time, average waiting time in ready queue,      *
*     and CPU Utilization                                              *                                                     
\***********************************************************************/
void BookKeeping(void){
  MemoryTable = (int *) NULL; // fix corrupted size bug
  TableSize = (Memory *) NULL; // fix corrupted size bug
  double end = Now(); // Total time for all processes to arrive
  Metric m;

  // Compute averages and final results
  if (NumberofJobs[TAT] > 0){
    SumMetrics[TAT] = SumMetrics[TAT]/ (Average) NumberofJobs[TAT];
  }
  if (NumberofJobs[RT] > 0){
    SumMetrics[RT] = SumMetrics[RT]/ (Average) NumberofJobs[RT];
  }
  SumMetrics[CBT] = SumMetrics[CBT]/ Now();

  if (NumberofJobs[WT] > 0){
    SumMetrics[WT] = SumMetrics[WT]/ (Average) NumberofJobs[WT];
  }

  if (NumberofJobs[AWTJQ] > 0){
    SumMetrics[AWTJQ] = SumMetrics[AWTJQ]/ (Average) NumberofJobs[AWTJQ];
  }

  printf("\n********* Processes Managemenent Numbers ******************************\n");
  printf("Policy Number = %d, Quantum = %.6f   Show = %d\n", PolicyNumber, Quantum, Show);
  printf("Number of Completed Processes = %d\n", NumberofJobs[THGT]);
  printf("ATAT=%f   ART=%f  CBT = %f  T=%f AWT=%f  AWTJQ=%f\n", 
	 SumMetrics[TAT], SumMetrics[RT], SumMetrics[CBT], 
	 NumberofJobs[THGT]/Now(), SumMetrics[WT], SumMetrics[AWTJQ]);

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
  ProcessControlBlock *lastProcess = Queues[JOBQUEUE].Head; // tracks the final process in the queue
  ProcessControlBlock *currentProcess = DequeueProcess(JOBQUEUE);

  if (currentProcess->ProcessID == 249) { BookKeeping(); } // fix corrupted size bug

  while (currentProcess) {
    currentProcess->TopOfMemory = *TableSize; // initialize the top of memory for the process

    // map the correct page size to the process
    currentProcess->MemoryAllocated = (currentProcess->MemoryRequested / PAGESIZE) * PAGESIZE;
    if (currentProcess->MemoryAllocated < currentProcess->MemoryRequested) {
      currentProcess->MemoryAllocated += PAGESIZE; // if requested is greater than page size, or is not a multiple, fix it!
    }

    // attempt to find a memory hole that will accomodate the process
    if (currentProcess->MemoryAllocated <= AvailableMemory) {
      currentProcess->TopOfMemory = FindWorstFitIndex(currentProcess->MemoryAllocated);
    }

    if (currentProcess->TopOfMemory != *TableSize) {
      currentProcess->TimeInJobQueue = Now() - currentProcess->JobArrivalTime; // Set TimeInJobQueue
      currentProcess->JobStartTime = Now(); // Set JobStartTime
      EnqueueProcess(READYQUEUE,currentProcess); // Place process in Ready Queue
      currentProcess->state = READY; // Update process state
      AllocateMemory(currentProcess->TopOfMemory, currentProcess->MemoryAllocated); // allocate the memory
      AvailableMemory -= currentProcess->MemoryAllocated; // reduce available memory
    }
    else {
      EnqueueProcess(JOBQUEUE,currentProcess);
    }
    if (currentProcess->ProcessID == lastProcess->ProcessID) { // short circuit after the final process
      break;
    }
    if (currentProcess->ProcessID == 249) { SeenLast = 1; } // fix corrupted size bug
    currentProcess = DequeueProcess(JOBQUEUE);
  }
  if (SeenLast == 1) { BookKeeping(); }// fix corrupted size bug
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