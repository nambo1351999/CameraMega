#include "dng_jpeg_memory_source.h"

#if qDNGUseLibJPEG

#include "dng_safe_arithmetic.h"

namespace {

void InitSource(j_decompress_ptr )
	{
	
	}

boolean FillInputBuffer(j_decompress_ptr cinfo)
	{
	
	
	ERREXIT(cinfo, JERR_INPUT_EOF);
	return FALSE;
	}

void SkipInputData(j_decompress_ptr cinfo, long num_bytes)
	{
	if (num_bytes > 0)
		{
		
		
		
		size_t num_bytes_as_size_t = 0;
		try
			{
			ConvertUnsigned(static_cast<unsigned long>(num_bytes),
							&num_bytes_as_size_t);
			}

		catch (const dng_exception &e)
			{
			ERREXIT(cinfo, JERR_INPUT_EOF);
			return;
			}

		jpeg_source_mgr *source_manager =
			reinterpret_cast<jpeg_source_mgr *>(cinfo->src);

		
		if (num_bytes_as_size_t <= source_manager->bytes_in_buffer)
			{
			source_manager->bytes_in_buffer -= num_bytes_as_size_t;
			source_manager->next_input_byte += num_bytes_as_size_t;
			}
		else
			{
			
			ERREXIT(cinfo, JERR_INPUT_EOF);
			return;
			}
		}
	}

boolean ResyncToRestart(j_decompress_ptr , int )
	{
	
	return FALSE;
	}

void TermSource(j_decompress_ptr )
	{
	
	}

}  

jpeg_source_mgr CreateJpegMemorySource(const uint8 *buffer, size_t size)
	{
	jpeg_source_mgr source;

	source.next_input_byte = reinterpret_cast<const JOCTET *>(buffer);
	source.bytes_in_buffer = size;

	
	source.init_source = InitSource;
	source.fill_input_buffer = FillInputBuffer;
	source.skip_input_data = SkipInputData;
	source.resync_to_restart = ResyncToRestart;
	source.term_source = TermSource;

	return source;
	}

#endif	
