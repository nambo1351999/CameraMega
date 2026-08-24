

#ifndef __dng_file_stream__
#define __dng_file_stream__

#include "dng_stream.h"

class dng_file_stream: public dng_stream
	{
	
	private:
	
		FILE *fFile;
	
	public:
	
		
		
		
		
		

		dng_file_stream (const char *filename,
						 bool output = false,
						 uint32 bufferSize = kDefaultBufferSize);
	
		
		
		
		
		
		
		dng_file_stream (FILE *file,
						 uint32 bufferSize = kDefaultBufferSize);
		
		#if qAndroid
		
		dng_file_stream (int fileDescriptor,
						 bool output = false,
						 uint32 bufferSize = kDefaultBufferSize);

		dng_file_stream (int fileDescriptor,
						 const char *mode,
						 uint32 bufferSize = kDefaultBufferSize);

		#endif	

		#if qWinOS

		dng_file_stream (const wchar_t *filename,
						 bool output = false,
						 uint32 bufferSize = kDefaultBufferSize);

		#endif	
		
		virtual ~dng_file_stream ();
	
	protected:
	
		virtual uint64 DoGetLength ();
	
		virtual void DoRead (void *data,
							 uint32 count,
							 uint64 offset);
		
		virtual void DoWrite (const void *data,
							  uint32 count,
							  uint64 offset);
		
	};
		

#endif
	

