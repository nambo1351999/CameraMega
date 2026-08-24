

#ifndef __dng_bmff__
#define __dng_bmff__

#include "dng_classes.h"
#include "dng_memory.h"
#include "dng_string.h"
#include "dng_types.h"

#include <memory>
#include <vector>

class dng_bmff_box
	{
		
	public:

		
		
		
		
		

		uint32 fStoredLength = 0xFFFFFFFF;

		
		
		
		uint64 fRealLength = 0ULL;

		
		

		uint64 fOffset = 0ULL;

		

		dng_string fName;

		
		

		std::shared_ptr<dng_memory_block> fContent;

	};

typedef std::shared_ptr<dng_bmff_box> dng_bmff_box_sptr;

typedef std::vector<dng_bmff_box_sptr> dng_bmff_box_list;

class dng_bmff_io
	{

	public:

		dng_bmff_box_list fBoxes;
		
	public:

		virtual ~dng_bmff_io ();

		void Read (dng_host &host,
				   dng_stream &stream);

		void Write (dng_host &host,
					dng_stream &stream) const;

		void UpdateBigTables (dng_host &host,
							  const dng_big_table_dictionary &newTables,
							  bool deleteUnused);

		void Add (dng_bmff_box_sptr box)
			{
			if (box)
				fBoxes.push_back (box);
			}
		
	protected:

		virtual bool ShouldReadBox (const dng_string & ,
									uint64 ) const
			{
			return true;
			}
		
	};

#endif	
	

