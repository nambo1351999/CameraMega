

#ifndef __dng_info__
#define __dng_info__

#include "dng_auto_ptr.h"
#include "dng_classes.h"
#include "dng_errors.h"
#include "dng_exif.h"
#include "dng_ifd.h"
#include "dng_sdk_limits.h"
#include "dng_shared.h"
#include "dng_uncopyable.h"

#include <vector>

class dng_info: private dng_uncopyable
	{
	
	public:
	
		uint64 fTIFFBlockOffset;
		
		uint64 fTIFFBlockOriginalOffset;
	
		bool fBigEndian;
		
		uint32 fMagic;
		
		AutoPtr<dng_exif> fExif;
	
		AutoPtr<dng_shared> fShared;
		
		int32 fMainIndex;
		
		int32 fMaskIndex;
  
		int32 fDepthIndex;
			
		int32 fEnhancedIndex;

		int32 fGainMapIndex = -1;			 

		std::vector<uint32> fSemanticMaskIndices;
		
		std::vector <dng_ifd *> fIFD;

		std::vector <dng_ifd *> fChainedIFD;

		std::vector <std::vector <dng_ifd *> > fChainedSubIFD;

		AutoPtr<dng_memory_block> fXMPBlock;
		
	protected:
	
		uint32 fMakerNoteNextIFD;

		uint32 fParseDepth = 0;

	public:
	
		dng_info ();
		
		virtual ~dng_info ();

		

		uint32 IFDCount () const
			{
			return (uint32) fIFD.size ();
			}

		

		uint32 ChainedIFDCount () const
			{
			return (uint32) fChainedIFD.size ();
			}

		

		uint32 ChainedSubIFDCount (uint32 chainIndex) const
			{
			if (chainIndex >= fChainedSubIFD.size ())
				return 0;
			else
				return (uint32) fChainedSubIFD [chainIndex].size ();
			}

		
		
		

		virtual void Parse (dng_host &host,
							dng_stream &stream);

		

		virtual void PostParse (dng_host &host);

		
		

		virtual bool IsValidDNG ();
		
	protected:
		
		virtual void ValidateMagic ();

		virtual void ParseTag (dng_host &host,
							   dng_stream &stream,
							   dng_exif *exif,
							   dng_shared *shared,
							   dng_ifd *ifd,
							   uint32 parentCode,
							   uint32 tagCode,
							   uint32 tagType,
							   uint32 tagCount,
							   uint64 tagOffset,
							   int64 offsetDelta);

		virtual bool ValidateIFD (dng_stream &stream,
								  uint64 ifdOffset,
								  int64 offsetDelta);

		virtual void ParseIFD (dng_host &host,
							   dng_stream &stream,
							   dng_exif *exif,
							   dng_shared *shared,
							   dng_ifd *ifd,
							   uint64 ifdOffset,
							   int64 offsetDelta,
							   uint32 parentCode);

		virtual bool ParseMakerNoteIFD (dng_host &host,
										dng_stream &stream,
										uint64 ifdSize,
										uint64 ifdOffset,
										int64 offsetDelta,
										uint64 minOffset,
										uint64 maxOffset,
										uint32 parentCode);

		virtual void ParseMakerNote (dng_host &host,
									 dng_stream &stream,
									 uint32 makerNoteCount,
									 uint64 makerNoteOffset,
									 int64 offsetDelta,
									 uint64 minOffset,
									 uint64 maxOffset);
									 
		virtual void ParseSonyPrivateData (dng_host &host,
										   dng_stream &stream,
										   uint64 count,
										   uint64 oldOffset,
										   uint64 newOffset);
									 
		virtual void ParseDNGPrivateData (dng_host &host,
										  dng_stream &stream);

	protected:

		class RecursionProtector
			{
				
			private:

				uint32 &fDepth;

			public:

				RecursionProtector (uint32 &depth)
					:	fDepth (depth)
					{
					fDepth++;
					}

				~RecursionProtector ()
					{
					fDepth--;
					}
				
			};
		
	};
	

#endif
	

