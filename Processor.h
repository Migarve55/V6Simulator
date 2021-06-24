#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "MainMemory.h"

#define INTERRUPTTYPES 10

//Registers
#define REGISTERS 10
#define ACCUMULATOR 0
#define REG_A 1
#define REG_B 2

// Enumerated type that connects bit positions in the PSW register with
// processor events and status
enum PSW_BITS {POWEROFF_BIT=0, ZERO_BIT=1, NEGATIVE_BIT=2, OVERFLOW_BIT=3, EXECUTION_MODE_BIT=7, INTERRUPT_MASKED_BIT=15};

// Enumerated type that connects bit positions in the interruptLines with
// interrupt types 
enum INT_BITS {SYSCALL_BIT=2, EXCEPTION_BIT=6, IOEND_BIT=8, CLOCK_BIT=9};

// Exception types
enum EXCEPTIONS {DIVISIONBYZERO, INVALIDPROCESSORMODE, INVALIDADDRESS, INVALIDINSTRUCTION};

// Functions prototypes
void Processor_InitializeInterruptVectorTable();
void Processor_InstructionCycleLoop();
void Processor_CopyInSystemStack(int, int);
int Processor_CopyFromSystemStack(int);
unsigned int Processor_PSW_BitState(const unsigned int);
char * Processor_ShowPSW();
void Processor_PrintRegisters();

// The OS needs to access MAR and MBR registers to save the context of
// the process to which the processor is being assigned
// Buses needs to access MAR and MBR
int Processor_GetMAR();
void Processor_SetMAR(int);
void Processor_GetMBR(MEMORYCELL *);
void Processor_SetMBR(MEMORYCELL *);
void Processor_RaiseInterrupt(const unsigned int);
// int Processor_GetMBR_Value();
 
// The OS needs to access the accumulator register to restore the context of
// the process to which the processor is being assigned and to save the context
// of the process being preempted for another ready process
// void Processor_SetAccumulator(int);
// int Processor_GetAccumulator();

// The OS needs to access the PC register to restore the context of
// the process to which the processor is being assigned
void Processor_SetPC(int);

// The OS needs to access register A to when executing the system call management
// routine, so it will be able to know the invoked system call identifier
int Processor_GetRegister(int);
void Processor_SetRegister(int,int);

int Processor_GetAccumulator();
void Processor_SetAccumulator(int);
int Processor_GetRegisterA();
int Processor_GetRegisterB();
// The OS needs to access the PSW register to restore the context of
// the process to which the processor is being assigned
void Processor_SetPSW(unsigned int);

unsigned int Processor_GetPSW();
void Processor_RaiseException(int);

#endif
