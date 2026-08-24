

#ifndef __dng_jpeg_memory_source__
#define __dng_jpeg_memory_source__

#if qDNGUseLibJPEG

#include "dng_tag_types.h"
#include "dng_jpeglib.h"

#include <limits>

jpeg_source_mgr CreateJpegMemorySource(const uint8 *buffer, size_t size);

#endif  

#endif  
