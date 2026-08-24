

#ifndef __dng_semantic_mask__
#define __dng_semantic_mask__

#include "dng_classes.h"
#include "dng_string.h"
#include "dng_types.h"

#include <memory>

class dng_semantic_mask
	{
		
	public:

		
		

		dng_string fName;

		
		

		dng_string fInstanceID;

		
		

		std::shared_ptr<const dng_memory_block> fXMP;

		
		

		std::shared_ptr<const dng_image> fMask;

		

		
		
		
		
		

		uint32 fMaskSubArea [4] = { 0, 0, 0, 0 };
		
		
		
		std::shared_ptr<const dng_lossy_compressed_image> fLossyCompressed;

	public:

		bool IsMaskSubAreaValid () const;

		void CalcMaskSubArea (dng_point &origin,
							  dng_rect &wholeImageArea) const;
		
	};

#endif	

