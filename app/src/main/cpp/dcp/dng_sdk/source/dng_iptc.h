

#ifndef __dng_iptc__
#define __dng_iptc__

#include "dng_date_time.h"
#include "dng_string.h"
#include "dng_string_list.h"

class dng_iptc
	{
	
	public:
	
		dng_string fTitle;

		int32 fUrgency;
		
		dng_string fCategory;
		
		dng_string_list fSupplementalCategories;
		
		dng_string_list fKeywords;
		
		dng_string fInstructions;
		
		dng_date_time_info fDateTimeCreated;
		
		dng_date_time_info fDigitalCreationDateTime;
		
		dng_string_list fAuthors;
		
		dng_string fAuthorsPosition;
		
		dng_string fCity;
		dng_string fState;
		dng_string fCountry;
		dng_string fCountryCode;
		
		dng_string fLocation;
		
		dng_string fTransmissionReference;
		
		dng_string fHeadline;
		
		dng_string fCredit;
		
		dng_string fSource;
		
		dng_string fCopyrightNotice;
		
		dng_string fDescription;
		dng_string fDescriptionWriter;
		
	protected:
		
		enum DataSet
			{
			kRecordVersionSet					= 0,
			kObjectNameSet						= 5,
			kUrgencySet							= 10,
			kCategorySet						= 15,
			kSupplementalCategoriesSet			= 20,
			kKeywordsSet						= 25,
			kSpecialInstructionsSet				= 40,
			kDateCreatedSet						= 55,
			kTimeCreatedSet						= 60,
			kDigitalCreationDateSet				= 62,
			kDigitalCreationTimeSet				= 63,
			kBylineSet							= 80,
			kBylineTitleSet						= 85,
			kCitySet							= 90,
			kSublocationSet						= 92,
			kProvinceStateSet					= 95,
			kCountryCodeSet						= 100,
			kCountryNameSet						= 101,
			kOriginalTransmissionReferenceSet	= 103,
			kHeadlineSet						= 105,
			kCreditSet							= 110,
			kSourceSet							= 115,
			kCopyrightNoticeSet					= 116,
			kCaptionSet							= 120,
			kCaptionWriterSet					= 122
			};
			
		enum CharSet
			{
			kCharSetUnknown						= 0,
			kCharSetUTF8						= 1
			};
	
	public:
	
		dng_iptc ();
		
		virtual ~dng_iptc ();

		
		

		bool IsEmpty () const;
		
		
		

		bool NotEmpty () const
			{
			return !IsEmpty ();
			}

		
		
		
		

		void Parse (const void *blockData,
					uint32 blockSize,
					uint64 offsetInOriginalFile);

		
		
		
		

		dng_memory_block * Spool (dng_memory_allocator &allocator,
								  bool padForTIFF);
					
	protected:
	
		void ParseString (dng_stream &stream,
						  dng_string &s,
						  CharSet charSet);
		
		void SpoolString (dng_stream &stream,
						  const dng_string &s,
						  uint8 dataSet,
						  uint32 maxChars,
						  CharSet charSet);
						  
	};

#endif
	

