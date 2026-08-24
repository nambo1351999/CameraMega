

#ifndef __dng_update_meta__
#define __dng_update_meta__

#include "dng_classes.h"

void CleanUpMetadataForUpdate (dng_host &host,
							   dng_metadata &metadata,
							   bool wantsIPTC);

void DNGUpdateMetadata (dng_host &host,
						dng_stream &stream,
						const dng_negative &negative,
						const dng_metadata &metadata);

#endif	
	

