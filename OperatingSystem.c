#include "OperatingSystem.h"
#include "OperatingSystemBase.h"
#include "MMU.h"
#include "Processor.h"
#include "Buses.h"
#include "Heap.h"
#include "Clock.h"
#include "QueueFIFO.h"
#include "Device.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

// Functions prototypes
void OperatingSystem_PrepareDaemons();
void OperatingSystem_PCBInitialization(int, int, int, int, int, int);
void OperatingSystem_MoveToTheREADYState(int);
void OperatingSystem_Dispatch(int);
void OperatingSystem_RestoreContext(int);
void OperatingSystem_SaveContext(int);
void OperatingSystem_TerminateProcess();
int OperatingSystem_LongTermScheduler();
void OperatingSystem_PreemptRunningProcess();
int OperatingSystem_CreateProcess(int);
int OperatingSystem_ObtainMainMemory(int, int, int);
void OperatingSystem_ReleaseMainMemory(int);
int OperatingSystem_ShortTermScheduler();
int OperatingSystem_ExtractFromReadyToRun();
void OperatingSystem_HandleException();
void OperatingSystem_HandleSystemCall();
//Extra functions
void OperatingSystem_PrintReadyToRunQueue();
int OperatingSystem_GetProcessWithSamePriorityToRun(int);
// In OperatingSystem.c Exercise 2-b of V2
void OperatingSystem_HandleClockInterrupt();
//Sleeping process queue 
int OperatingSystem_ExtractFromSleepingProcessesQueue();
int OperatingSystem_GetFirstFromSleepingProcessesQueue();
void OperatingSystem_AddToSleepingProcessesQueue(int);
//Sleep call
void OperatingSystem_UpdateWhenToWakeUp(int);
void OperatingSystem_MoveToTheBLOCKEDState(int);
int OperatingSystem_CompareProcessPrivileges(int,int);
int OperatingSystem_ExtractFromArrivalQueue();
// I/O System
void OperatingSystem_HandleIOEndInterrupt();
void OperatingSystem_IOScheduler(int);
void OperatingSystem_DeviceControlerStartIOOperation();
int OperatingSystem_DeviceControlerEndIOOperation();
int OperatingSystem_AddToIOWaiting(int);
int OperatingSystem_ExtractFromIOWaiting();
int OperatingSystem_GetFirstFromIOWaiting();

// The process table
PCB processTable[PROCESSTABLEMAXSIZE];

// Address base for OS code in this version
int OS_address_base = PROCESSTABLEMAXSIZE * MAINMEMORYSECTIONSIZE;

// Identifier of the current executing process
int executingProcessID=NOPROCESS;

// Identifier of the System Idle Process
int sipID;

// Begin indes for daemons in programList
int baseDaemonsInProgramList; 

// Array that contains the identifiers of the READY processes
int readyToRunQueue[NUMBEROFQUEUES][PROCESSTABLEMAXSIZE];
int numberOfReadyToRunProcesses[NUMBEROFQUEUES]={0,0};

// Variable containing the number of not terminated user processes
int numberOfNotTerminatedUserProcesses=0;

//State definitions
char * statesNames[5]={"NEW","READY","EXECUTING","BLOCKED","EXIT"};

//Queues info
char * queuesNames[] = {"USER", "DAEMONS"};
int queuesPriorities[] = {0, 1};

//Number of clock interrupts
int numberOfClockInterrupts = 0;

// Heap with blocked processes sorted by when to wakeup
int sleepingProcessesQueue[PROCESSTABLEMAXSIZE];
int numberOfSleepingProcesses = 0;

int numOfPartitions = 0;

//I/O System data structures

int IOWaitingProcessesQueue[PROCESSTABLEMAXSIZE];
int numberOfIOWaitingProcesses=0;

// Initial set of tasks of the OS
void OperatingSystem_Initialize(int daemonsIndex) {
	
	int i, selectedProcess;
	FILE *programFile; // For load Operating System Code

	// Obtain the memory requirements of the program
	int processSize=OperatingSystem_ObtainProgramSize(&programFile, "OperatingSystemCode");

	// Load Operating System Code
	OperatingSystem_LoadProgram(programFile, OS_address_base, processSize);
	
	//Load memory configuration
	numOfPartitions = OperatingSystem_InitializePartitionTable() ;
	
	// Process table initialization (all entries are free)
	for (i=0; i<PROCESSTABLEMAXSIZE;i++)
		processTable[i].busy=0;
	
	// Initialization of the interrupt vector table of the processor
	Processor_InitializeInterruptVectorTable(OS_address_base+1);
		
	// Create all system daemon processes
	OperatingSystem_PrepareDaemons(daemonsIndex);
	//Initialization of the arrivalTime queue
	ComputerSystem_FillInArrivalTimeQueue();
	OperatingSystem_PrintStatus();
	//Initialization of I/O Devices
	Device_Initialize ("OutputDevice-2018", 7);
	// Create all user processes from the information given in the command line
	OperatingSystem_LongTermScheduler(); 
	//No user process where created 
	if (numberOfNotTerminatedUserProcesses <= 0 && numberOfProgramsInArrivalTimeQueue <= 0)
		OperatingSystem_ReadyToShutdown();
	if (strcmp(programList[processTable[sipID].programListIndex]->executableName,"SystemIdleProcess")) {
		// Show message "ERROR: Missing SIP program!\n"
		OperatingSystem_ShowTime(SHUTDOWN);
		ComputerSystem_DebugMessage(21,SHUTDOWN);
		exit(1);		
	}

	// At least, one user process has been created
	// Select the first process that is going to use the processor
	selectedProcess=OperatingSystem_ShortTermScheduler();

	// Assign the processor to the selected process
	OperatingSystem_Dispatch(selectedProcess);

	// Initial operation for Operating System
	Processor_SetPC(OS_address_base);
}

// Daemon processes are system processes, that is, they work together with the OS.
// The System Idle Process uses the CPU whenever a user process is able to use it
void OperatingSystem_PrepareDaemons(int programListDaemonsBase) {

        // Include a entry for SystemIdleProcess at 0 position
        programList[0]=(PROGRAMS_DATA *) malloc(sizeof(PROGRAMS_DATA));

        programList[0]->executableName="SystemIdleProcess";
        programList[0]->arrivalTime=0;
        programList[0]->type=DAEMONPROGRAM; // daemon program

        sipID=INITIALPID%PROCESSTABLEMAXSIZE; // first PID for sipID

        // Prepare aditionals daemons here
        // index for aditionals daemons program in programList
        baseDaemonsInProgramList=programListDaemonsBase;

}



// The LTS is responsible of the admission of new processes in the system.
// Initially, it creates a process from each program specified in the 
// command line and daemons programs
int OperatingSystem_LongTermScheduler() {
  
	int PID, i,
		numberOfSuccessfullyCreatedProcesses=0;
	while (OperatingSystem_IsThereANewProgram() > 0) {
		i = OperatingSystem_ExtractFromArrivalQueue();
		if ((PID = OperatingSystem_CreateProcess(i)) < 0) {
			//Switch to show the correct error
			OperatingSystem_ShowTime(ERROR);
			switch (PID) {
			case NOFREEENTRY:
				ComputerSystem_DebugMessage(103,ERROR,programList[i]->executableName);
				break;
			case PROGRAMDOESNOTEXIST:
				ComputerSystem_DebugMessage(104,ERROR,programList[i]->executableName, "it does not exist");
				break;
			case PROGRAMNOTVALID:
				ComputerSystem_DebugMessage(104,ERROR,programList[i]->executableName, "invalid priority or size");
				break;
			case TOOBIGPROCESS:
				ComputerSystem_DebugMessage(105,ERROR,programList[i]->executableName);
				break;
			case MEMORYFULL:
				ComputerSystem_DebugMessage(144,ERROR,programList[i]->executableName);
				break;
			default:
				ComputerSystem_DebugMessage(104,ERROR,programList[i]->executableName, "unknown error");
			}
		}
		else { //It was created
			numberOfSuccessfullyCreatedProcesses++;
			if (programList[i]->type==USERPROGRAM) 
				numberOfNotTerminatedUserProcesses++;
			// Move process to the ready state
			OperatingSystem_MoveToTheREADYState(PID);
		}
	}
	//Print status
	if (numberOfSuccessfullyCreatedProcesses > 0) {
		OperatingSystem_PrintStatus();
	}
	// Return the number of succesfully created processes
	return numberOfSuccessfullyCreatedProcesses;
}


// This function creates a process from an executable program
int OperatingSystem_CreateProcess(int indexOfExecutableProgram) {
  
	int PID;
	int processSize;
	int partitionId;
	int priority;
	FILE *programFile;
	PROGRAMS_DATA *executableProgram=programList[indexOfExecutableProgram];

	// Obtain a process ID
	if ((PID = OperatingSystem_ObtainAnEntryInTheProcessTable()) == NOFREEENTRY)
		return NOFREEENTRY;

	// Obtain the memory requirements of the program
	if((processSize = OperatingSystem_ObtainProgramSize(&programFile, executableProgram->executableName)) < 0)
		return processSize; //In this case this will be the error code

	// Obtain the priority for the process
	if((priority = OperatingSystem_ObtainPriority(programFile)) < 0)
		return PROGRAMNOTVALID;
	
	// Obtain enough memory space
 	if ((partitionId = OperatingSystem_ObtainMainMemory(processSize, PID, indexOfExecutableProgram)) < 0)
		return partitionId;
		
	// Load program in the allocated memory
	if (OperatingSystem_LoadProgram(programFile, partitionsTable[partitionId].initAddress, processSize) == TOOBIGPROCESS) 
		return TOOBIGPROCESS;
	
	// PCB initialization
	OperatingSystem_PCBInitialization(PID, partitionId, partitionsTable[partitionId].initAddress, processSize, priority, indexOfExecutableProgram);
	
	// Assign memory
	OperatingSystem_ShowPartitionTable(BAM);
	partitionsTable[partitionId].occupied = 1; //true
	partitionsTable[partitionId].PID = PID;
	OperatingSystem_ShowPartitionTable(AAM);
	
	// Show message "Process [PID] created from program [executableName]\n"
	OperatingSystem_ShowTime(INIT);
	ComputerSystem_DebugMessage(22,INIT,PID,executableProgram->executableName);
	
	return PID;
}


// Main memory is assigned in chunks. All chunks are the same size. A process
// always obtains the chunk whose position in memory is equal to the processor identifier
int OperatingSystem_ObtainMainMemory(int processSize, int pid, int indexOfExecutableProgram) {
	int i, partitionId = -1;
	int minSize = MAINMEMORYSIZE;
	int candidateWasFound = 0;
	//Debug message
	OperatingSystem_ShowTime(SYSMEM);
	ComputerSystem_DebugMessage(142,SYSMEM,
	pid,
	programList[indexOfExecutableProgram]->executableName,
	processSize);
	//Find the ideal partition using best fit
	for (i = 0;i < numOfPartitions;i++) {
		if (partitionsTable[i].size >= processSize) { 	//Partition can hold the process
			candidateWasFound = 1;
			if (!partitionsTable[i].occupied) { 		//Partition is not occupied
				if (partitionsTable[i].size < minSize) { 
					minSize = partitionsTable[i].size;
					partitionId = i;
				}
			}
		}
	}
	//Handle errors
	if (partitionId < 0) { //No partition was found
		if (!candidateWasFound)
			return TOOBIGPROCESS;
		if (minSize == MAINMEMORYSIZE) //Candidate was found, but the memory was occupied
			return MEMORYFULL;
	}
	//Debug
	OperatingSystem_ShowTime(SYSMEM);
	ComputerSystem_DebugMessage(143,SYSMEM,
	partitionId,
	partitionsTable[partitionId].initAddress,
	partitionsTable[partitionId].size,
	pid,
	programList[indexOfExecutableProgram]->executableName
	);
 	return partitionId;
}

void OperatingSystem_ReleaseMainMemory(int pid) {
	int partitionId = processTable[pid].partitionId;
	//Find the correct partition
	OperatingSystem_ShowPartitionTable(BRM);
	//Change the partition table
	OperatingSystem_ShowTime(SYSMEM);
	ComputerSystem_DebugMessage(145,SYSMEM,
	partitionId,
	partitionsTable[partitionId].initAddress,
	partitionsTable[partitionId].size,
	executingProcessID,
	programList[processTable[executingProcessID].programListIndex]->executableName
	);
	partitionsTable[partitionId].occupied = 0; //false
	partitionsTable[partitionId].PID = NOPROCESS;
	OperatingSystem_ShowPartitionTable(ARM);
}

// Assign initial values to all fields inside the PCB
void OperatingSystem_PCBInitialization(int PID, int partitionId, int initialPhysicalAddress, int processSize, int priority, int processPLIndex) {
	processTable[PID].busy=1;
	processTable[PID].initialPhysicalAddress=initialPhysicalAddress;
	processTable[PID].processSize=processSize;
	processTable[PID].state=NEW;
	OperatingSystem_ShowTime(SYSPROC);
	ComputerSystem_DebugMessage(111, SYSPROC, PID);
	processTable[PID].priority = priority;
	processTable[PID].programListIndex=processPLIndex;
	processTable[PID].copyOfAccumulatorRegister = 0;
	processTable[PID].whenToWakeUp = 0;
	processTable[PID].partitionId = partitionId;
	// Daemons run in protected mode and MMU use real address
	if (programList[processPLIndex]->type == DAEMONPROGRAM) {
		processTable[PID].copyOfPCRegister = initialPhysicalAddress;
		processTable[PID].copyOfPSWRegister = ((unsigned int) 1) << EXECUTION_MODE_BIT;
		processTable[PID].queueID = DAEMONSQUEUE;
	} 
	else {
		processTable[PID].copyOfPCRegister=0;
		processTable[PID].copyOfPSWRegister=0;
		processTable[PID].queueID = USERPROCESSQUEUE;
	}
}


// Move a process to the READY state: it will be inserted, depending on its priority, in
// a queue of identifiers of READY processes
void OperatingSystem_MoveToTheREADYState(int PID) {
	int lastState;
	int queueID = processTable[PID].queueID;
	if (Heap_add(PID, readyToRunQueue[queueID],QUEUE_PRIORITY ,&numberOfReadyToRunProcesses[queueID] ,PROCESSTABLEMAXSIZE)>=0) {
		lastState = processTable[PID].state;
		processTable[PID].state=READY;
		OperatingSystem_ShowTime(SYSPROC);
		ComputerSystem_DebugMessage(110, SYSPROC, PID, statesNames[lastState], statesNames[READY]);
	} 
	//OperatingSystem_PrintReadyToRunQueue();
}


// The STS is responsible of deciding which process to execute when specific events occur.
// It uses processes priorities to make the decission. Given that the READY queue is ordered
// depending on processes priority, the STS just selects the process in front of the READY queue
int OperatingSystem_ShortTermScheduler() {
	int selectedProcess;
	selectedProcess=OperatingSystem_ExtractFromReadyToRun();
	return selectedProcess;
}


// Return PID of more priority process in the READY queue
// This method has been modified so it searches in all queues
int OperatingSystem_ExtractFromReadyToRun() {
	int i;
	int selectedProcess = NOPROCESS;
	for (i = 0;i < NUMBEROFQUEUES;i++) {
		if ((selectedProcess = Heap_poll(readyToRunQueue[i],QUEUE_PRIORITY,&numberOfReadyToRunProcesses[i])) >= 0) //If it has found a process
			return selectedProcess;
	}
	// Return most priority process or NOPROCESS if empty queue
	return selectedProcess; 
}

// Function that assigns the processor to a process
void OperatingSystem_Dispatch(int PID) {

	// The process identified by PID becomes the current executing process
	executingProcessID=PID;
	// Change the process' state
	int lastState = processTable[PID].state;
	processTable[PID].state=EXECUTING;
	OperatingSystem_ShowTime(SYSPROC);
	ComputerSystem_DebugMessage(110, SYSPROC, PID, statesNames[lastState], statesNames[EXECUTING]);
	// Modify hardware registers with appropriate values for the process identified by PID
	OperatingSystem_RestoreContext(PID);
}


// Modify hardware registers with appropriate values for the process identified by PID
void OperatingSystem_RestoreContext(int PID) {
  
	// New values for the CPU registers are obtained from the PCB
	Processor_CopyInSystemStack(MAINMEMORYSIZE-1,processTable[PID].copyOfPCRegister);
	Processor_CopyInSystemStack(MAINMEMORYSIZE-2,processTable[PID].copyOfPSWRegister);
	// Same thing for the MMU registers
	MMU_SetBase(processTable[PID].initialPhysicalAddress);
	MMU_SetLimit(processTable[PID].processSize);
	//Restore accumulator
	Processor_SetAccumulator(processTable[PID].copyOfAccumulatorRegister);
}


// Function invoked when the executing process leaves the CPU 
void OperatingSystem_PreemptRunningProcess() {

	// Save in the process' PCB essential values stored in hardware registers and the system stack
	OperatingSystem_SaveContext(executingProcessID);
	// Change the process' state
	OperatingSystem_MoveToTheREADYState(executingProcessID);
	// The processor is not assigned until the OS selects another process
	executingProcessID=NOPROCESS;
}


// Save in the process' PCB essential values stored in hardware registers and the system stack
void OperatingSystem_SaveContext(int PID) {
	
	// Load PC saved for interrupt manager
	processTable[PID].copyOfPCRegister = Processor_CopyFromSystemStack(MAINMEMORYSIZE-1);
	
	// Load PSW saved for interrupt manager
	processTable[PID].copyOfPSWRegister = Processor_CopyFromSystemStack(MAINMEMORYSIZE-2);
	
	// Load accumulator saved for interrupt manager
	processTable[PID].copyOfAccumulatorRegister = Processor_GetAccumulator();
}


// Exception management routine
void OperatingSystem_HandleException() {
	OperatingSystem_ShowTime(INTERRUPT);
	char* name = programList[processTable[executingProcessID].programListIndex]->executableName;
	switch (Processor_GetRegisterB()) {
		case DIVISIONBYZERO:
			ComputerSystem_DebugMessage(140,INTERRUPT,executingProcessID,name,"division by zero");
			break;
		case INVALIDPROCESSORMODE:
			ComputerSystem_DebugMessage(140,INTERRUPT,executingProcessID,name,"invalid processor mode");
			break;
		case INVALIDADDRESS:
			ComputerSystem_DebugMessage(140,INTERRUPT,executingProcessID,name,"invalid address");
			break;
		case INVALIDINSTRUCTION:
			ComputerSystem_DebugMessage(140,INTERRUPT,executingProcessID,name,"invalid instruction");
			break;
		default:
			ComputerSystem_DebugMessage(140,INTERRUPT,executingProcessID,name,"unknown exception");
	}
	OperatingSystem_TerminateProcess();
	OperatingSystem_PrintStatus();
}


// All tasks regarding the removal of the process
void OperatingSystem_TerminateProcess() {
  
	int selectedProcess;
  	
	int lastState = processTable[executingProcessID].state;
	processTable[executingProcessID].state=EXIT;
	OperatingSystem_ShowTime(SYSPROC);
	ComputerSystem_DebugMessage(110, SYSPROC, executingProcessID, statesNames[lastState], statesNames[EXIT]);
	OperatingSystem_ReleaseMainMemory(executingProcessID);
	//Release the PCB page
	processTable[executingProcessID].busy = 0;
	// One more process that has terminated
	numberOfNotTerminatedUserProcesses--;
	if (numberOfNotTerminatedUserProcesses <= 0 && numberOfProgramsInArrivalTimeQueue <= 0) {
		// Simulation must finish 
		OperatingSystem_ReadyToShutdown();
	}
	// Select the next process to execute (sipID if no more user processes)
	selectedProcess = OperatingSystem_ShortTermScheduler();
	// Assign the processor to that process
	OperatingSystem_Dispatch(selectedProcess);
}


// System call management routine
void OperatingSystem_HandleSystemCall() {
  
	int systemCallID;
	int oldPID, newPID;

	// Register A contains the identifier of the issued system call
	systemCallID=Processor_GetRegisterA();
	
	switch (systemCallID) {
		case SYSCALL_PRINTEXECPID:
			// Show message: "Process [executingProcessID] has the processor assigned\n"
			OperatingSystem_ShowTime(SYSPROC);
			ComputerSystem_DebugMessage(24,SYSPROC,executingProcessID);
			break;
		case SYSCALL_YIELD:
			if ((newPID = OperatingSystem_GetProcessWithSamePriorityToRun(executingProcessID)) != NOPROCESS) {
				oldPID = executingProcessID;
				OperatingSystem_ExtractFromReadyToRun(processTable[oldPID].queueID); //Remove from queue (the OS already knows this PID: newPID)
				OperatingSystem_PreemptRunningProcess();
				OperatingSystem_Dispatch(newPID);
				//Print debug message
				OperatingSystem_ShowTime(SYSPROC);
				ComputerSystem_DebugMessage(115,SYSPROC,oldPID,executingProcessID);
				OperatingSystem_PrintStatus();
			}
			break;
		case SYSCALL_END:
			// Show message: "Process [executingProcessID] has requested to terminate\n"
			OperatingSystem_ShowTime(SYSPROC);
			ComputerSystem_DebugMessage(25,SYSPROC,executingProcessID);
			OperatingSystem_TerminateProcess();
			OperatingSystem_PrintStatus();
			break;
		case SYSCALL_SLEEP:
			OperatingSystem_UpdateWhenToWakeUp(executingProcessID);
			OperatingSystem_MoveToTheBLOCKEDState(executingProcessID);
			OperatingSystem_AddToSleepingProcessesQueue(executingProcessID);
			OperatingSystem_SaveContext(executingProcessID);
			// The processor is not assigned until the OS selects another process
			executingProcessID=NOPROCESS;
			// Select the next process to execute 
			newPID = OperatingSystem_ShortTermScheduler();
			OperatingSystem_Dispatch(newPID);
			//Print the status
			OperatingSystem_PrintStatus();
			break;
		case SYSCALL_IO: 
			//Block the current one
			OperatingSystem_MoveToTheBLOCKEDState(executingProcessID);
			OperatingSystem_SaveContext(executingProcessID);
			oldPID=executingProcessID;
			executingProcessID=NOPROCESS;
			newPID = OperatingSystem_ShortTermScheduler();
			OperatingSystem_Dispatch(newPID);
			//Call the independent handler
			OperatingSystem_IOScheduler(oldPID);
			OperatingSystem_PrintStatus();
			break;
		default:
			OperatingSystem_ShowTime(INTERRUPT);
			ComputerSystem_DebugMessage(141,INTERRUPT,executingProcessID,programList[processTable[executingProcessID].programListIndex]->executableName,systemCallID);
			OperatingSystem_TerminateProcess();
			OperatingSystem_PrintStatus();
	}
}

// Handle clock interrupt
void OperatingSystem_HandleClockInterrupt() {
	numberOfClockInterrupts++;
	//Print interrupt message
	OperatingSystem_ShowTime(INTERRUPT);
	ComputerSystem_DebugMessage(120,INTERRUPT,numberOfClockInterrupts);
	//Select process to wakeup
	int pid, wakenup = 0;
	//Introduce processes if any
	if(OperatingSystem_LongTermScheduler() > 0) {
		OperatingSystem_PrintStatus();
	}
	//Find a process
	pid = OperatingSystem_GetFirstFromSleepingProcessesQueue();
	while (numberOfClockInterrupts == processTable[pid].whenToWakeUp) {
		wakenup++;
		OperatingSystem_ExtractFromSleepingProcessesQueue();
		OperatingSystem_MoveToTheREADYState(pid);
		pid = OperatingSystem_GetFirstFromSleepingProcessesQueue();
	}
	//If at least one has been processed
	if (wakenup > 0) {
		OperatingSystem_PrintStatus();
	}
	//If one of the woken processes has a higher prioriy than the current executing process
	if ((pid = OperatingSystem_ShortTermScheduler()) != NOPROCESS) {
		if (OperatingSystem_CompareProcessPrivileges(pid, executingProcessID) > 0) {
			//Print debug info
			OperatingSystem_ShowTime(SHORTTERMSCHEDULE);
			ComputerSystem_DebugMessage(121,SHORTTERMSCHEDULE,executingProcessID,pid);
			//Swap processes
			OperatingSystem_PreemptRunningProcess();
			OperatingSystem_Dispatch(pid);
			OperatingSystem_PrintStatus();
		} else {
			//Add it back again
			Heap_add(pid, readyToRunQueue[processTable[pid].queueID],QUEUE_PRIORITY ,&numberOfReadyToRunProcesses[processTable[pid].queueID] ,PROCESSTABLEMAXSIZE);
		}
	}
	//Check if the simulation must end 
	if (numberOfNotTerminatedUserProcesses <= 0 && numberOfProgramsInArrivalTimeQueue <= 0) {
		// Simulation must finish 
		OperatingSystem_ReadyToShutdown();
	}
}

//Compares two process priorities, based also in their queues
//Returns positive if pid1 has more priority than pid2, negative if viceversa
int OperatingSystem_CompareProcessPrivileges(int pid1, int pid2) {
	if (processTable[pid1].queueID == processTable[pid2].queueID)
		return processTable[pid2].priority - processTable[pid1].priority;
	else
		return queuesPriorities[processTable[pid2].queueID] - queuesPriorities[processTable[pid1].queueID];
}
	
//	Implement interrupt logic calling appropriate interrupt handle
void OperatingSystem_InterruptLogic(int entryPoint){
	switch (entryPoint) {
		case IOEND_BIT: // CLOCK_BIT=1
			OperatingSystem_HandleIOEndInterrupt();
			break;
		case SYSCALL_BIT: // SYSCALL_BIT=2
			OperatingSystem_HandleSystemCall();
			break;
		case EXCEPTION_BIT: // EXCEPTION_BIT=6
			OperatingSystem_HandleException();
			break;
		case CLOCK_BIT: // CLOCK_BIT=9
			OperatingSystem_HandleClockInterrupt();
			break;
	}
}

void OperatingSystem_HandleIOEndInterrupt()  {
	//Unblock a process
	int pid;
	if ((pid = OperatingSystem_DeviceControlerEndIOOperation()) < 0)
		return;
	//Notify the device dependent handler
	OperatingSystem_DeviceControlerStartIOOperation();
	//Unblock
	OperatingSystem_PrintStatus();
	//Swap the processes if necesary
	if (OperatingSystem_CompareProcessPrivileges(pid, executingProcessID) > 0) {
		//Print debug info
		OperatingSystem_ShowTime(INTERRUPT);
		ComputerSystem_DebugMessage(121,INTERRUPT,executingProcessID,pid);
		//Swap processes
		OperatingSystem_PreemptRunningProcess();
		OperatingSystem_Dispatch(pid);
	} else
		OperatingSystem_MoveToTheREADYState(pid);
	OperatingSystem_PrintStatus();
}

//Device independent handler
void OperatingSystem_IOScheduler(int pid)  {
	if (OperatingSystem_AddToIOWaiting(pid) >= 0)
		OperatingSystem_DeviceControlerStartIOOperation();
} 

//Device dependent handler
void OperatingSystem_DeviceControlerStartIOOperation()  {
	int value;
	if (Device_GetStatus() == BUSY)
		return;
	if ((value = OperatingSystem_GetFirstFromIOWaiting()) >= 0)
		Device_StartIO(value);
}

int OperatingSystem_DeviceControlerEndIOOperation()  {
	return OperatingSystem_ExtractFromIOWaiting();
}

//Auxilar for the IO fifo queue

int OperatingSystem_AddToIOWaiting(int pid) {
	return QueueFIFO_add(pid, IOWaitingProcessesQueue, &numberOfIOWaitingProcesses, PROCESSTABLEMAXSIZE);
}

int OperatingSystem_ExtractFromIOWaiting() {
	return QueueFIFO_poll(IOWaitingProcessesQueue, &numberOfIOWaitingProcesses);
}

int OperatingSystem_GetFirstFromIOWaiting() {
	return QueueFIFO_getFirst(IOWaitingProcessesQueue, numberOfIOWaitingProcesses);
}

//Prints the ReadyToRunQueue, all queus
void OperatingSystem_PrintReadyToRunQueue()  {
	int i, id, queueID;
	OperatingSystem_ShowTime(SHORTTERMSCHEDULE);
	ComputerSystem_DebugMessage(106, SHORTTERMSCHEDULE);
	for (queueID = 0;queueID < NUMBEROFQUEUES;queueID++) {
		ComputerSystem_DebugMessage(112, SHORTTERMSCHEDULE, queuesNames[queueID]);
		for (i = 0;i < numberOfReadyToRunProcesses[queueID];i++) {
			id = readyToRunQueue[queueID][i];
			ComputerSystem_DebugMessage(107, SHORTTERMSCHEDULE, id, processTable[id].priority);
			if (i < numberOfReadyToRunProcesses[queueID] - 1)
				ComputerSystem_DebugMessage(108, SHORTTERMSCHEDULE);
		}
	}
	ComputerSystem_DebugMessage(109, SHORTTERMSCHEDULE);
}

//Used in the SYSCALL_YIELD call
int OperatingSystem_GetProcessWithSamePriorityToRun(int pid) {
	int newPid, queueID;
	queueID = processTable[pid].queueID;
	newPid = Heap_getFirst(readyToRunQueue[queueID],numberOfReadyToRunProcesses[queueID]);
	if (processTable[pid].priority == processTable[newPid].priority)
		return newPid;
	return NOPROCESS;
}

// Used in the syscall sleep

void OperatingSystem_UpdateWhenToWakeUp(int pid) {
	int accumm = Processor_GetAccumulator();
	accumm = accumm < 0 ? -accumm : accumm; //Absolute value
	processTable[pid].whenToWakeUp = accumm + numberOfClockInterrupts + 1;
}

void OperatingSystem_MoveToTheBLOCKEDState(int pid) {
	int lastState = processTable[pid].state;
	processTable[pid].state=BLOCKED;
	OperatingSystem_ShowTime(SYSPROC);
	ComputerSystem_DebugMessage(110, SYSPROC, pid, statesNames[lastState], statesNames[BLOCKED]);
}

//Sleeping process queue methods 

void OperatingSystem_AddToSleepingProcessesQueue(int pid) {
	Heap_add(pid, sleepingProcessesQueue, QUEUE_WAKEUP, &numberOfSleepingProcesses, PROCESSTABLEMAXSIZE);
}

int OperatingSystem_GetFirstFromSleepingProcessesQueue() {
	return Heap_getFirst(sleepingProcessesQueue, numberOfSleepingProcesses);
}

int OperatingSystem_ExtractFromSleepingProcessesQueue() {
	return Heap_poll(sleepingProcessesQueue, QUEUE_WAKEUP, &numberOfSleepingProcesses);
}

int OperatingSystem_GetExecutingProcessID() {
	return executingProcessID;
}

//Arrival queue methods

int OperatingSystem_ExtractFromArrivalQueue() {
	return Heap_poll(arrivalTimeQueue, QUEUE_ARRIVAL, &numberOfProgramsInArrivalTimeQueue);
}