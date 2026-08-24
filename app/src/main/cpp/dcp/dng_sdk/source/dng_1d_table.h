

#ifndef __dng_1d_table__
#define __dng_1d_table__

#include "dng_assertions.h"
#include "dng_auto_ptr.h"
#include "dng_classes.h"
#include "dng_types.h"
#include "dng_uncopyable.h"

class dng_1d_table: private dng_uncopyable
	{

	public:

		

		static const uint32 kMinTableSize = 512;
	
	private:
	
		

		static const uint32 kDefaultTableSize = 4096;
			
	protected:
	
		AutoPtr<dng_memory_block> fBuffer;
		
		real32 *fTable;

		const uint32 fTableCount;
		
	private:
		
		const real32 fTableCount32;
	
	public:

		
		
	
		explicit dng_1d_table (uint32 count = kDefaultTableSize);
			
		virtual ~dng_1d_table ();

		

		uint32 Count () const
			{
			return fTableCount;
			}

		
		
		
		
		

		void Initialize (dng_memory_allocator &allocator,
						 const dng_1d_function &function,
						 bool subSample = false);

		
		
		

		real32 Interpolate (real32 x) const
			{

			real32 y = x * fTableCount32;

			
			
			
			
			

			if (!(y >= 0.0f && y <= fTableCount32))
				{

				ThrowBadFormat ("Index out of range.");

				}

			int32 index = (int32) y;

			
			
			DNG_ASSERT (!(index < 0 || index > (int32) fTableCount), "dng_1d_table::Interpolate parameter out of range");
			
			real32 z = (real32) index;
			
			real32 fract = y - z;
			
			return fTable [index	] * (1.0f - fract) +
				   fTable [index + 1] * (		fract);
			
			}
			
		
		
			
		DNG_ALWAYS_INLINE real32 UnsafeInterpolate (real32 x) const
			{
			
			real32 y = x * fTableCount32;
			
			int32 index = (int32) y;
			
			real32 z = (real32) index;
			
			real32 fract = y - z;
			
			return fTable [index	] * (1.0f - fract) +
				   fTable [index + 1] * (		fract);
			
			}
			
		
			
		const real32 * Table () const
			{
			return fTable;
			}
			
		
		
		void Expand16 (uint16 *table16) const;
			
	private:
			
		void SubDivide (const dng_1d_function &function,
						uint32 lower,
						uint32 upper,
						real32 maxDelta);
	
	};

#endif
	

