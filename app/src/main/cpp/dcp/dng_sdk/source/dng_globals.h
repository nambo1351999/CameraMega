

#ifndef __dng_globals__
#define __dng_globals__

#include "dng_flags.h"
#include "dng_types.h"

#if qDNGValidate || qDNGDebug

extern bool gVerbose;

extern uint32 gDumpLineLimit;

#endif

extern bool gDNGShowTimers;

extern bool gDNGUseFakeTimeZonesInXMP;

extern uint32 gDNGStreamBlockSize;

extern uint32 gDNGMaxStreamBufferSize;

extern bool gImagecore;

extern bool gPrintTimings;

extern bool gPrintAsserts;

extern bool gBreakOnAsserts;

#endif
	

