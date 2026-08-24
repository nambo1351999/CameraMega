

#ifndef __dng_ref_counted_block__
#define __dng_ref_counted_block__

#include "dng_types.h"
#include "dng_mutex.h"

#include <mutex>

class dng_ref_counted_block
	{
	
	private:
	
		struct header
			{

			dng_std_mutex fMutex;

			uint32 fRefCount;
				
			uint32 fSize;

			header (uint32 size)
				:	fMutex	  ()
				,	fRefCount (1)
				,	fSize	  (size)
				{
				}

			~header ()
				{
				}

			};

		void *fBuffer;
		
	public:
	
		
		

		dng_ref_counted_block ();
		
		
		
		

		dng_ref_counted_block (uint32 size);
		
		

		~dng_ref_counted_block ();

		

		dng_ref_counted_block (const dng_ref_counted_block &data);
		
		
		
		dng_ref_counted_block & operator= (const dng_ref_counted_block &data);

		
		
		

		void Allocate (uint32 size);

		
		
		
		void Clear ();
		
		

		void EnsureWriteable ();

		
		

		uint32 LogicalSize () const
			{
			return fBuffer ? ((header *) fBuffer)->fSize : 0;
			}

		void * Buffer ()
			{
			return fBuffer ? (void *) ((char *) fBuffer + sizeof (header)) : NULL;
			}
		
		
		

		const void * Buffer () const
			{
			return fBuffer ? (const void *) ((char *) fBuffer + sizeof (header)) : NULL;
			}
		
		
		

		char * Buffer_char ()
			{
			return (char *) Buffer ();
			}
			
		
		

		const char * Buffer_char () const
			{
			return (const char *) Buffer ();
			}
			
		
		

		uint8 * Buffer_uint8 ()
			{
			return (uint8 *) Buffer ();
			}
			
		
		

		const uint8 * Buffer_uint8 () const
			{
			return (const uint8 *) Buffer ();
			}
	
		
		

		uint16 * Buffer_uint16 ()
			{
			return (uint16 *) Buffer ();
			}
			
		
		

		const uint16 * Buffer_uint16 () const
			{
			return (const uint16 *) Buffer ();
			}
	
		
		

		int16 * Buffer_int16 ()
			{
			return (int16 *) Buffer ();
			}
			
		
		

		const int16 * Buffer_int16 () const
			{
			return (const int16 *) Buffer ();
			}
	
		
		

		uint32 * Buffer_uint32 ()
			{
			return (uint32 *) Buffer ();
			}
			
		
		

		const uint32 * Buffer_uint32 () const
			{
			return (const uint32 *) Buffer ();
			}
	
		
		

		int32 * Buffer_int32 ()
			{
			return (int32 *) Buffer ();
			}
			
		
		

		const int32 * Buffer_int32 () const
			{
			return (const int32 *) Buffer ();
			}
	
		
		

		uint64 * Buffer_uint64 ()
			{
			return (uint64 *) Buffer ();
			}
			
		
		

		const uint64 * Buffer_uint64 () const
			{
			return (const uint64 *) Buffer ();
			}
	
		
		

		int64 * Buffer_int64 ()
			{
			return (int64 *) Buffer ();
			}
			
		
		

		const int64 * Buffer_int64 () const
			{
			return (const int64 *) Buffer ();
			}
	
		
		

		real32 * Buffer_real32 ()
			{
			return (real32 *) Buffer ();
			}
			
		
		

		const real32 * Buffer_real32 () const
			{
			return (const real32 *) Buffer ();
			}
			
		
		

		real64 * Buffer_real64 ()
			{
			return (real64 *) Buffer ();
			}
			
		
		

		const real64 * Buffer_real64 () const
			{
			return (const real64 *) Buffer ();
			}
			
	};

#endif
	

