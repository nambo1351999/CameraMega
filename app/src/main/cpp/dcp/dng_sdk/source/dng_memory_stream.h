

#ifndef __dng_memory_stream__
#define __dng_memory_stream__

#include "dng_stream.h"

class dng_memory_stream: public dng_stream
	{
	
	protected:
	
		dng_memory_allocator &fAllocator;
		
		uint32 fPageSize;
		
		uint32 fPageCount;
		uint32 fPagesAllocated;
		
		dng_memory_block **fPageList;
		
		uint64 fMemoryStreamLength;
		
		uint64 fLengthLimit;
		
	public:

		
		
		
		

		dng_memory_stream (dng_memory_allocator &allocator,
						   dng_abort_sniffer *sniffer = NULL,
						   uint32 pageSize = 64 * 1024);
						   
		virtual ~dng_memory_stream ();
		
		
		
		void SetLengthLimit (uint64 limit)
			{
			fLengthLimit = limit;
			}

		
		
		
		
		virtual void CopyToStream (dng_stream &dstStream,
								   uint64 count);
		
	protected:
		
		virtual uint64 DoGetLength ();
	
		virtual void DoRead (void *data,
							 uint32 count,
							 uint64 offset);
							 
		virtual void DoSetLength (uint64 length);
							 
		virtual void DoWrite (const void *data,
							  uint32 count,
							  uint64 offset);
		
	};

#endif
	

