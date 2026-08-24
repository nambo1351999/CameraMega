

#include "dng_filter_task.h"

#include "dng_bottlenecks.h"
#include "dng_exceptions.h"
#include "dng_image.h"
#include "dng_memory.h"
#include "dng_tag_types.h"
#include "dng_tag_values.h"
#include "dng_utils.h"

dng_filter_task::dng_filter_task (const char *name,
								  const dng_image &srcImage,
								  dng_image &dstImage)

	:	dng_area_task (name)
	
	,	fSrcImage	  (srcImage)
	,	fDstImage	  (dstImage)
	
	,	fSrcPlane	  (0					)
	,	fSrcPlanes	  (srcImage.Planes	  ())
	,	fSrcPixelType (srcImage.PixelType ())
	
	,	fDstPlane	  (0					)
	,	fDstPlanes	  (dstImage.Planes	  ())
	,	fDstPixelType (dstImage.PixelType ())
	
	,	fSrcRepeat	  (1, 1)
	,	fSrcTileSize  (0, 0)
	
	{

	}
							  

dng_filter_task::~dng_filter_task ()
	{
	
	}
		

void dng_filter_task::Start (uint32 threadCount,
							 const dng_rect & ,
							 const dng_point &tileSize,
							 dng_memory_allocator *allocator,
							 dng_abort_sniffer * )
	{
	
	fSrcTileSize = SrcTileSize (tileSize);

	uint32 srcBufferSize = ComputeBufferSize (fSrcPixelType, 
											  fSrcTileSize,
											  fSrcPlanes, 
											  padSIMDBytes);

	uint32 dstBufferSize = ComputeBufferSize (fDstPixelType, 
											  tileSize,
											  fDstPlanes, 
											  padSIMDBytes);
						   
	for (uint32 threadIndex = 0; threadIndex < threadCount; threadIndex++)
		{
		
		fSrcBuffer [threadIndex] . Reset (allocator->Allocate (srcBufferSize));
		
		fDstBuffer [threadIndex] . Reset (allocator->Allocate (dstBufferSize));
		
		
		
		DoZeroBytes (fSrcBuffer [threadIndex]->Buffer	   (),
					 fSrcBuffer [threadIndex]->LogicalSize ());
		
		DoZeroBytes (fDstBuffer [threadIndex]->Buffer	   (),
					 fDstBuffer [threadIndex]->LogicalSize ());
		
		}
		
	}

void dng_filter_task::Process (uint32 threadIndex,
							   const dng_rect &area,
							   dng_abort_sniffer * )
	{
	
	
	
	dng_rect srcArea = SrcArea (area);

	
					  
	int32 src_area_w;
	int32 src_area_h;

	if (!ConvertUint32ToInt32 (srcArea.W (), 
							   &src_area_w) || 
		!ConvertUint32ToInt32 (srcArea.H (), 
							   &src_area_h) || 
		src_area_w > fSrcTileSize.h || 
		src_area_h > fSrcTileSize.v)
		{
	
		ThrowMemoryFull ("Area exceeds tile size.");
	
		}
	
	
	
	dng_pixel_buffer srcBuffer (srcArea, 
								fSrcPlane, 
								fSrcPlanes, 
								fSrcPixelType,
								pcRowInterleavedAlignSIMD,
								fSrcBuffer [threadIndex]->Buffer ());
	
	
	
	dng_pixel_buffer dstBuffer (area, 
								fDstPlane, 
								fDstPlanes, 
								fDstPixelType,
								pcRowInterleavedAlignSIMD,
								fDstBuffer [threadIndex]->Buffer ());
	
	
	
	fSrcImage.Get (srcBuffer,
				   dng_image::edge_repeat,
				   fSrcRepeat.v,
				   fSrcRepeat.h);
				   
	
	
	ProcessArea (threadIndex,
				 srcBuffer,
				 dstBuffer);

	
	
	fDstImage.Put (dstBuffer);
	
	}

