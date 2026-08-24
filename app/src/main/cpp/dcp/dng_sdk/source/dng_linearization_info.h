

#ifndef __dng_linearization_info__
#define __dng_linearization_info__

#include "dng_auto_ptr.h"
#include "dng_classes.h"
#include "dng_memory.h"
#include "dng_rational.h"
#include "dng_rect.h"
#include "dng_sdk_limits.h"

class dng_linearization_info
	{
	
	public:

		
		

		dng_rect fActiveArea;

		

		uint32 fMaskedAreaCount;

		
		
		
		
		
		
		
		

		dng_rect fMaskedArea [kMaxMaskedAreas];
		
		
		
		
		
		

		AutoPtr<dng_memory_block> fLinearizationTable;

		

		uint32 fBlackLevelRepeatRows;

		

		uint32 fBlackLevelRepeatCols;

		
		
		real64 fBlackLevel [kMaxBlackPattern] [kMaxBlackPattern] [kMaxColorPlanes];

		

		AutoPtr<dng_memory_block> fBlackDeltaH;

		

		AutoPtr<dng_memory_block> fBlackDeltaV;
		
		

		real64 fWhiteLevel [kMaxColorPlanes];
		
	protected:
	
		int32 fBlackDenom;

	public:
	
		dng_linearization_info ();
		
		virtual ~dng_linearization_info ();
		
		void RoundBlacks ();
		
		virtual void Parse (dng_host &host,
							dng_stream &stream,
							dng_info &info);
							
		virtual void PostParse (dng_host &host,
								dng_negative &negative);
							
		
		

		real64 MaxBlackLevel (uint32 plane) const;
		
		
		
		
		
		

		virtual void Linearize (dng_host &host,
								dng_negative &negative,
								const dng_image &srcImage,
								dng_image &dstImage);

		

		uint16 Stage3BlackLevel (dng_negative &negative,
								 uint32 numPlanes) const;

		
		
		
		

		dng_urational BlackLevel (uint32 row,
								  uint32 col,
								  uint32 plane) const;
							  
		

		uint32 RowBlackCount () const;

		
		
		

		dng_srational RowBlack (uint32 row) const;
		
		

		uint32 ColumnBlackCount () const;
		
		
		
		

		dng_srational ColumnBlack (uint32 col) const;
		
	};
	

#endif
	

