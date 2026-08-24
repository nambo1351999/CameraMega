

#include "dng_globals.h"
#include "dng_simd_type.h"

#if qDNGValidate || qDNGDebug

bool gVerbose = false;

uint32 gDumpLineLimit = 100;

#endif

bool gDNGUseFakeTimeZonesInXMP = false;

bool gDNGShowTimers =
#if qDNGValidate
	true
#else
	false
#endif
	;

uint32 gDNGStreamBlockSize = 4096;

uint32 gDNGMaxStreamBufferSize = 1024 * 1024;

bool gImagecore = false;

bool gPrintTimings = false;

bool gPrintAsserts = true;

bool gBreakOnAsserts = false;

SIMDType gDNGMaxSIMD = Scalar;

