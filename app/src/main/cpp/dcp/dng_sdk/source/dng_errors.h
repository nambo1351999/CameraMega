

#ifndef __dng_errors__
#define __dng_errors__

#include "dng_types.h"

#include <atomic>

typedef int32 dng_error_code;

typedef std::atomic<dng_error_code> dng_atomic_error_code;

enum
	{
		
	dng_error_none       = 0,		            

	dng_error_sdk_first  = 100000,
	
	dng_error_unknown    = dng_error_sdk_first,	
	dng_error_not_yet_implemented,				
	dng_error_silent,							
	dng_error_user_canceled,					
	dng_error_host_insufficient,				
	dng_error_memory,							
	dng_error_bad_format,						
	dng_error_matrix_math,						
	dng_error_open_file,						
	dng_error_read_file,						
	dng_error_write_file,						
	dng_error_end_of_file,						
	dng_error_file_is_damaged,					
	dng_error_image_too_big_dng,				
	dng_error_image_too_big_tiff,				
	dng_error_unsupported_dng,					
	dng_error_overflow,							
	dng_error_jxl_encoder,						
	dng_error_jxl_decoder,						

	
	
	dng_error_sdk_end,
	dng_error_sdk_count = dng_error_sdk_end - dng_error_sdk_first + 1,
	
	};
	

#endif
	

