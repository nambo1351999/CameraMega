

 

#ifndef __dng_filter_task__
#define __dng_filter_task__

#include "dng_area_task.h"
#include "dng_auto_ptr.h"
#include "dng_point.h"
#include "dng_rect.h"
#include "dng_sdk_limits.h"

class dng_filter_task: public dng_area_task
	{
	
	protected:
	
		const dng_image &fSrcImage;
		
		dng_image &fDstImage;
		
		uint32 fSrcPlane;
		uint32 fSrcPlanes;
		uint32 fSrcPixelType;
		
		uint32 fDstPlane;
		uint32 fDstPlanes;
		uint32 fDstPixelType;
		
		dng_point fSrcRepeat;
		dng_point fSrcTileSize;
		
		AutoPtr<dng_memory_block> fSrcBuffer [kMaxMPThreads];
		AutoPtr<dng_memory_block> fDstBuffer [kMaxMPThreads];
		
	public:
	
		
		
		

		dng_filter_task (const char *name,
						 const dng_image &srcImage,
						 dng_image &dstImage);
							   
		virtual ~dng_filter_task ();

		
		
		
		
		
		
		
		

		virtual dng_rect SrcArea (const dng_rect &dstArea)
			{
			return dstArea;
			}

		
		
		
		
		
		
		

		virtual dng_point SrcTileSize (const dng_point &dstTileSize)
			{
			return SrcArea (dng_rect (dstTileSize)).Size ();
			}

		
		
		
		
		
		
		
		
		
		

		virtual void ProcessArea (uint32 threadIndex,
								  dng_pixel_buffer &srcBuffer,
								  dng_pixel_buffer &dstBuffer) = 0;

		
		
		
		
		
		
		
		
		
		
		
		
		
		

		virtual void Start (uint32 threadCount,
							const dng_rect &dstArea,
							const dng_point &tileSize,
							dng_memory_allocator *allocator,
							dng_abort_sniffer *sniffer);
							
		
		
		
		
		
		
		
		
		
		
		
		
		
		

		virtual void Process (uint32 threadIndex,
							  const dng_rect &area,
							  dng_abort_sniffer *sniffer);
							  
	};

#endif
	

