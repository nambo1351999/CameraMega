

#ifndef __dng_simple_image__
#define __dng_simple_image__

#include "dng_auto_ptr.h"
#include "dng_image.h"
#include "dng_memory.h"
#include "dng_pixel_buffer.h"

 
class dng_simple_image : public dng_image
	{
	
	protected:
	
		dng_pixel_buffer fBuffer;
		
		AutoPtr<dng_memory_block> fMemory;
		
		dng_memory_allocator &fAllocator;
		
	public:
	
		dng_simple_image (const dng_rect &bounds,
						  uint32 planes,
						  uint32 pixelType,
						  dng_memory_allocator &allocator = gDefaultDNGMemoryAllocator);
		
		dng_simple_image (dng_pixel_buffer &buffer,
						  dng_memory_allocator &allocator = gDefaultDNGMemoryAllocator);
		
		virtual ~dng_simple_image ();
	
		virtual dng_image * Clone () const;

		
		
		virtual void SetPixelType (uint32 pixelType);
		
		

		virtual void Trim (const dng_rect &r);

		
		
		virtual void Rotate (const dng_orientation &orientation);
		
		
		
		virtual void Offset (const dng_point &offset);
		
		
		
		void GetPixelBuffer (dng_pixel_buffer &buffer)
			{
			buffer = fBuffer;
			}

	protected:
	
		virtual void AcquireTileBuffer (dng_tile_buffer &buffer,
										const dng_rect &area,
										bool dirty) const;
		
	};

#endif
	

