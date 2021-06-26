#include "Clock.h"
#include "Processor.h"

int tics = 0;

void Clock_Update() {
	if (++tics % INTERVALBETWEENINTERRUPS == 0) 
		Processor_RaiseInterrupt(CLOCK_BIT);
}

int Clock_GetTime() {
	return tics;
}
