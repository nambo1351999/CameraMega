

#ifndef __dng_memory__
#define __dng_memory__

#include "dng_classes.h"
#include "dng_exceptions.h"
#include "dng_flags.h"
#include "dng_safe_arithmetic.h"
#include "dng_types.h"
#include "dng_uncopyable.h"

#include <cstdlib>
#include <vector>

#if qDNGAVXSupport
	#define DNG_ALIGN_SIMD(x) ((((uintptr) (x)) + 31) & ~((uintptr) 31))
#else
	#define DNG_ALIGN_SIMD(x) ((((uintptr) (x)) + 15) & ~((uintptr) 15))
#endif

class dng_memory_data: private dng_uncopyable
	{
	
	private:
	
		char *fBuffer;
		
	public:
	
		
		

		dng_memory_data ();
		
		
		
		

		dng_memory_data (uint32 size);
		
		dng_memory_data (const dng_safe_uint32 &size);
		
		
		
		
		
		
		
		

		dng_memory_data (uint32 count, 
						 std::size_t elementSize);

		

		~dng_memory_data ();

		
		
		

		void Allocate (uint32 size);

		void Allocate (const dng_safe_uint32 &size);

		
		
		
		
		
		
		
		

		void Allocate (uint32 count, 
					   std::size_t elementSize);

		void Allocate (const dng_safe_uint32 &count, 
					   std::size_t elementSize);

		
		
		
		void Clear ();
		
		
		

		void * Buffer ()
			{
			return fBuffer;
			}
		
		
		

		const void * Buffer () const
			{
			return fBuffer;
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
	

class dng_memory_block: private dng_uncopyable
	{
	
	private:
	
		uint32 fLogicalSize;
		
		char *fBuffer;
		
	protected:
	
		dng_memory_block (uint32 logicalSize)
			:	fLogicalSize (logicalSize)
			,	fBuffer (NULL)
			{
			}
		
		uint32 PhysicalSize ()
			{
			
			
			
			
			
			
			
			
			
			
			
			
			

			dng_safe_uint32 safeLogicalSize (fLogicalSize);

			#if qDNGAVXSupport

			
			

			return (safeLogicalSize + 160u).Get ();

			#else

			

			return (safeLogicalSize + 64u).Get ();

			#endif	

			}
		
		void SetBuffer (void *p)
			{
			fBuffer = (char *) DNG_ALIGN_SIMD (p);
			}
		
	public:
	
		virtual ~dng_memory_block ()
			{
			}

		dng_memory_block * Clone (dng_memory_allocator &allocator) const;
	
		
		

		uint32 LogicalSize () const
			{
			return fLogicalSize;
			}

		
		

		void * Buffer ()
			{
			return fBuffer;
			}
		
		
		

		const void * Buffer () const
			{
			return fBuffer;
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

class dng_memory_allocator
	{
	
	public:
	
		virtual ~dng_memory_allocator () 
			{
			}
				
		
		
		
		

		virtual dng_memory_block * Allocate (uint32 size);

		
		
		
		
		
		

		virtual void * Malloc (size_t size);

		
		
	
		virtual void Free (void *ptr);
	
	};

class dng_malloc_block : public dng_memory_block
	{
	
	private:
	
		void *fMalloc;
	
	public:
	
		dng_malloc_block (uint32 logicalSize);
		
		virtual ~dng_malloc_block ();
		
	};
	

extern dng_memory_allocator gDefaultDNGMemoryAllocator;

template <typename T>
class dng_std_allocator
	{
	
	public:

		typedef T value_type;
		
		#if defined(_MSC_VER) && _MSC_VER >= 1900

		
		

		dng_std_allocator () = default;

		

		template<class U> dng_std_allocator (const dng_std_allocator<U> &) {}
		
		#endif

		T * allocate (size_t n)
			{
			const size_t size = SafeSizetMult (n, sizeof (T));
			T *retval = static_cast<T *> (malloc (size));
			if (!retval) 
				{
				ThrowMemoryFull ();
				}
			return retval;
			}
		
		void deallocate (T *ptr, 
						 size_t )
			{
			free (ptr);
			}

	};

template <class T>
bool operator== (const dng_std_allocator<T> & ,
				 const dng_std_allocator<T> & )
	{
	return true;
	}

template <class T>
bool operator!= (const dng_std_allocator<T> & ,
				 const dng_std_allocator<T> & )
	{
	return false;
	}

#if 0

#define dng_std_vector std::vector
#else

template <class T> using dng_std_vector = std::vector<T, dng_std_allocator<T> >;
#endif

#endif
	

