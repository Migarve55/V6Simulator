#ifndef OPERATINGSYSTEM_H
#define OPERATINGSYSTEM_H

#include "ComputerSystem.h"
#include "Processor.h"
#include <stdio.h>

#define NUMBEROFQUEUES 2 
#define USERPROCESSQUEUE 0 
#define DAEMONSQUEUE 1 

#define SLEEPINGQUEUE

#define SUCCESS 1
#define PROGRAMDOESNOTEXIST -1
#define PROGRAMNOTVALID -2

#define USERPROGRAM (unsigned int) 0
#define DAEMONPROGRAM (unsigned int) 1

#define MAXLINELENGTH 150

#define PROCESSTABLEMAXSIZE 4

#define INITIALPID 3

// In this version, every process occupies a 60 positions main memory chunk 
// so we can use 60 positions for OS code and the system stack
#define MAINMEMORYSECTIONSIZE (MAINMEMORYSIZE / (PROCESSTABLEMAXSIZE+1))

#define MEMCONFIG "MemConfig" 

#define NOFREEENTRY -3
#define TOOBIGPROCESS -4
#define MEMORYFULL -5 

#define NOPROCESS -1

#define BAM "before allocating memory"
#define AAM "after allocating memory"
#define BRM "before releasing memory"
#define ARM "after releasing memory"

// Enumerated type containing all the possible process states
enum ProcessStates { NEW, READY, EXECUTING, BLOCKED, EXIT};

// Enumerated type containing the list of system calls and their numeric identifiers
enum SystemCallIdentifiers { SYSCALL_IO=1, SYSCALL_END=3, SYSCALL_YIELD=4, SYSCALL_PRINTEXECPID=5, SYSCALL_PRINTCPUREG=6, SYSCALL_SLEEP=7};

// A PCB contains all of the information about a process that is needed by the OS
typedef struct {
	int busy;
	int initialPhysicalAddress;
	int processSize;
	int state;
	int priority;
	int copyOfPCRegister;
	int copyOfAccRegister;
	int copyOfRegisters[REGISTERS]; // Copy of all registers
	unsigned int copyOfPSWRegister;
	int programListIndex;
	int queueID;
	int whenToWakeUp; // Exercise 5-a of V2
	int partitionId;
} PCB;

// These "extern" declaration enables other source code files to gain access
// to the variable listed
extern PCB processTable[PROCESSTABLEMAXSIZE];
extern int OS_address_base;
extern int sipID;

// Functions prototypes
void OperatingSystem_Initialize();
void OperatingSystem_InterruptLogic(int);
int OperatingSystem_GetExecutingProcessID();
int OperatingSystem_ObtainAnEntryInTheProcessTable();
int OperatingSystem_ObtainProgramSize(FILE **, char *);
int OperatingSystem_ObtainPriority(FILE *);
int OperatingSystem_LoadProgram(FILE *, int, int);
void OperatingSystem_ReadyToShutdown();
void OperatingSystem_ShowTime(char);
void OperatingSystem_PrintStatus();
void OperatingSystem_PrintReadyToRunQueue();
void OperatingSystem_PrepareTeachersDaemons();
int OperatingSystem_IsThereANewProgram();
int OperatingSystem_InitializePartitionTable();
void OperatingSystem_ShowPartitionTable(char *); 

extern int sleepingProcessesQueue[PROCESSTABLEMAXSIZE];
extern int numberOfSleepingProcesses; 
// Begin indes for daemons in programList
extern int baseDaemonsInProgramList; 

#ifdef MEMCONFIG
typedef struct {
     int occupied;
     int initAddress; // Lowest physical address of the partition
     int size; // Size of the partition in memory positions
     int PID; // PID of the process using the partition, if occupied
} PARTITIONDATA;

#define PARTITIONTABLEMAXSIZE PROCESSTABLEMAXSIZE*2
extern PARTITIONDATA partitionsTable[PARTITIONTABLEMAXSIZE];
#endif
#endif
