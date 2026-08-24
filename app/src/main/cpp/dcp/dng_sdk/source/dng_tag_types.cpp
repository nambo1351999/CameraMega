

#include "dng_tag_types.h"

uint32 TagTypeSize (uint32 tagType)
	{
	
	switch (tagType)
		{
		
		case ttByte:
		case ttAscii:
		case ttSByte:
		case ttUndefined:
			{
			return 1;
			}

		case ttShort:
		case ttSShort:
		case ttUnicode:
		case ttHalfFloat:
			{
			return 2;
			}

		case ttLong:
		case ttSLong:
		case ttFloat:
		case ttIFD:
			{
			return 4;
			}

		case ttRational:
		case ttDouble:
		case ttSRational:
		case ttComplex:
		case ttLong8:
		case ttSLong8:
		case ttIFD8:
			{
			return 8;
			}

		default:
			break;
			
		}
		
	return 0;
	
	}

