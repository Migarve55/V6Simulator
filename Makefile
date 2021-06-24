#
# Makefile for the Linux Simulator
#
########################################################

PROGRAM = sim

# Compilation Details
SHELL = /bin/sh
CC = cc
STDCFLAGS = -g -c -Wall -std=gnu90 
INCLUDES =
LIBRERIAS =

${PROGRAM}: Simulator.o Buses.o Clock.o ComputerSystem.o  Device.o Heap.o MainMemory.o Messages.o MMU.o OperatingSystem.o  Processor.o QueueFIFO.o
	$(CC) -o ${PROGRAM} Simulator.o Buses.o Clock.o ComputerSystem.o Device.o Heap.o MainMemory.o Messages.o MMU.o OperatingSystem.o Processor.o QueueFIFO.o $(LIBRERIAS)

Simulator.o: Simulator.c Simulator.h
	$(CC) $(STDCFLAGS) $(INCLUDES) Simulator.c

Buses.o: Buses.c Buses.h
	$(CC) $(STDCFLAGS) $(INCLUDES) Buses.c

Clock.o: Clock.c Clock.h
	$(CC) $(STDCFLAGS) $(INCLUDES) Clock.c

ComputerSystem.o: ComputerSystem.c ComputerSystem.h
	$(CC) $(STDCFLAGS) $(INCLUDES) ComputerSystem.c

Device.o: Device.c Device.h
	$(CC) $(STDCFLAGS) $(INCLUDES) Device.c	
	
Heap.o: Heap.c Heap.h
	$(CC) $(STDCFLAGS) $(INCLUDES) Heap.c

MainMemory.o: MainMemory.c MainMemory.h
	$(CC) $(STDCFLAGS) $(INCLUDES) MainMemory.c

Messages.o: Messages.c Messages.h
	$(CC) $(STDCFLAGS) $(INCLUDES) Messages.c

MMU.o: MMU.c MMU.h
	$(CC) $(STDCFLAGS) $(INCLUDES) MMU.c

OperatingSystem.o: OperatingSystem.c OperatingSystem.h
	$(CC) $(STDCFLAGS) $(INCLUDES) OperatingSystem.c

Processor.o: ProcessorV2.c ProcessorV2.h
	$(CC) $(STDCFLAGS) $(INCLUDES) Processor.c

QueueFIFO.o: QueueFIFO.c QueueFIFO.h
	$(CC) $(STDCFLAGS) $(INCLUDES) QueueFIFO.c
	
clean:
	rm -f $(PROGRAM) *.o *~ core
