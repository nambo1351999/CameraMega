

#ifndef __dng_tag_types__
#define __dng_tag_types__

#include "dng_types.h"

enum
	{
	
	ttByte = 1,
	ttAscii,
	ttShort,
	ttLong,
	ttRational,
	ttSByte,
	ttUndefined,
	ttSShort,
	ttSLong,
	ttSRational,
	ttFloat,
	ttDouble,
	ttIFD,
	ttUnicode,
	ttComplex,
	
	

	ttLong8,
	ttSLong8,
	ttIFD8,
	
	
	
	
	ttHalfFloat
	
	};

uint32 TagTypeSize (uint32 tagType);

#endif
	

