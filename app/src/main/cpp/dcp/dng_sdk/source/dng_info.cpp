

#include "dng_info.h"

#include "dng_camera_profile.h"
#include "dng_exceptions.h"
#include "dng_globals.h"
#include "dng_host.h"
#include "dng_tag_codes.h"
#include "dng_parse_utils.h"
#include "dng_safe_arithmetic.h"
#include "dng_sdk_limits.h"
#include "dng_tag_types.h"
#include "dng_tag_values.h"
#include "dng_utils.h"

static bool ApplyOffsetDelta (uint64 offset,
							  int64 offsetDelta,
							  uint64 *adjustedOffset)
	{

	if (offsetDelta >= 0)
		{

		const uint64 positiveDelta = (uint64) offsetDelta;

		if (offset > 0xFFFFFFFFFFFFFFFFull - positiveDelta)
			{
			return false;
			}

		*adjustedOffset = offset + positiveDelta;

		}

	else
		{

		
		

		const uint64 negativeDelta = (uint64) (-(offsetDelta + 1)) + 1;

		if (offset < negativeDelta)
			{
			return false;
			}

		*adjustedOffset = offset - negativeDelta;

		}

	return true;

	}

dng_info::dng_info ()

	:	fTIFFBlockOffset		 (0)
	,	fTIFFBlockOriginalOffset (0)
	,	fBigEndian				 (false)
	,	fMagic					 (0)
	,	fExif					 ()
	,	fShared					 ()
	,	fMainIndex				 (-1)
	,	fMaskIndex				 (-1)
	,	fDepthIndex				 (-1)
	,	fEnhancedIndex			 (-1)
	,	fIFD					 ()
	,	fChainedIFD				 ()
	,	fChainedSubIFD			 ()
	,	fMakerNoteNextIFD		 (0)
	
	{
	
	}
	

dng_info::~dng_info ()
	{

	for (size_t index = 0; index < fIFD.size (); index++)
		{

		if (fIFD [index])
			{
			delete fIFD [index];
			fIFD [index] = NULL;
			}

		}
	
	for (size_t index2 = 0; index2 < fChainedIFD.size (); index2++)
		{

		if (fChainedIFD [index2])
			{
			delete fChainedIFD [index2];
			fChainedIFD [index2] = NULL;
			}

		}

	for (size_t index3 = 0; index3 < fChainedSubIFD.size (); index3++)
		{

		for (size_t index4 = 0; index4 < fChainedSubIFD [index3].size (); index4++)
			{

			if (fChainedSubIFD [index3] [index4])
				{
				delete fChainedSubIFD [index3] [index4];
				fChainedSubIFD [index3] [index4] = NULL;
				}

			}

		}
	
	}

void dng_info::ValidateMagic ()
	{
	
	switch (fMagic)
		{
		
		case magicTIFF:
		case magicBigTIFF:
		case magicExtendedProfile:
		case magicRawCache:
		case magicPanasonic:
		case magicOlympusA:
		case magicOlympusB:
			{
			
			return;
			
			}
			
		default:
			{
			
			#if qDNGValidate
			
			ReportError ("Invalid TIFF magic number");
			
			#endif
			
			ThrowBadFormat ();
			
			}
			
		}
	
	}

void dng_info::ParseTag (dng_host &host,
						 dng_stream &stream,
						 dng_exif *exif,
						 dng_shared *shared,
						 dng_ifd *ifd,
						 uint32 parentCode,
						 uint32 tagCode,
						 uint32 tagType,
						 uint32 tagCount,
						 uint64 tagOffset,
						 int64 offsetDelta)
	{
	
	bool isSubIFD = parentCode >= tcFirstSubIFD &&
					parentCode <= tcLastSubIFD;
					  
	bool isMainIFD = (parentCode == 0 || isSubIFD) &&
					 ifd &&
					 ifd->fUsesNewSubFileType &&
					 ifd->fNewSubFileType == sfMainImage;
					 
	
	
	
	
	
					 
	if (fMagic == 85 && parentCode == 0 && (tagCode < tcNewSubFileType ||
											(tagCode >= 280 && tagCode <= 283)))
		{
		
		parentCode = tcPanasonicRAW;
		
		ifd = NULL;
		
		}
	
	stream.SetReadPosition (tagOffset);
		
	if (ifd && ifd->ParseTag (host,
							  stream,
							  parentCode,
							  tagCode,
							  tagType,
							  tagCount,
							  tagOffset))
		{
		
		return;
		
		}
		
	stream.SetReadPosition (tagOffset);
		
	if (exif && shared && exif->ParseTag (stream,
										  *shared,
										  parentCode,
										  isMainIFD,
										  tagCode,
										  tagType,
										  tagCount,
										  tagOffset))
		{
		
		return;
		
		}
		
	stream.SetReadPosition (tagOffset);
		
	if (shared && exif && shared->ParseTag (stream,
											*exif,
											parentCode,
											isMainIFD,
											tagCode,
											tagType,
											tagCount,
											tagOffset,
											offsetDelta))
		{
		
		return;
		
		}

	if (parentCode == tcLeicaMakerNote &&
		tagType == ttUndefined &&
		tagCount >= 14)
		{
		
		if (ParseMakerNoteIFD (host,
							   stream,
							   tagCount,
							   tagOffset,
							   offsetDelta,
							   tagOffset,
							   stream.Length (),
							   tcLeicaMakerNote))
			{
				
			return;
				
			}
		
		}
		
	if (parentCode == tcOlympusMakerNote &&
		tagType == ttUndefined &&
		tagCount >= 14)
		{
		
		uint32 olympusMakerParent = 0;
		
		switch (tagCode)
			{
			
			case 8208:
				olympusMakerParent = tcOlympusMakerNote8208;
				break;
				
			case 8224:
				olympusMakerParent = tcOlympusMakerNote8224;
				break; 
		
			case 8240:
				olympusMakerParent = tcOlympusMakerNote8240;
				break; 
		
			case 8256:
				olympusMakerParent = tcOlympusMakerNote8256;
				break; 
		
			case 8272:
				olympusMakerParent = tcOlympusMakerNote8272;
				break; 
		
			case 12288:
				olympusMakerParent = tcOlympusMakerNote12288;
				break;
				
			default:
				break;
				
			}
			
		if (olympusMakerParent)
			{
			
			
			
			
			if (ParseMakerNoteIFD (host,
								   stream,
								   stream.Length () - tagOffset,
								   tagOffset,
								   offsetDelta,
								   tagOffset,
								   stream.Length (),
								   olympusMakerParent))
				{
				
				return;
				
				}
			
			}
			
		}

	if (parentCode == tcRicohMakerNote &&
		tagCode == 0x2001 &&
		tagType == ttUndefined &&
		tagCount > 22)
		{
		
		char header [20];
		
		stream.SetReadPosition (tagOffset);
		
		stream.Get (header, sizeof (header));
		
		if (memcmp (header, "[Ricoh Camera Info]", 19) == 0)
			{
		
			ParseMakerNoteIFD (host,
							   stream,
							   tagCount - 20,
							   tagOffset + 20,
							   offsetDelta,
							   tagOffset + 20,
							   tagOffset + tagCount,
							   tcRicohMakerNoteCameraInfo);

			return;
			
			}
			
		}
		
	#if qDNGValidate
	
		{
		
		stream.SetReadPosition (tagOffset);
		
		if (gVerbose)
			{
					
			printf ("*");
				
			DumpTagValues (stream,
						   LookupTagType (tagType),
						   parentCode,
						   tagCode,
						   tagType,
						   tagCount);
			
			}
			
		
		
			
		else if (tagType == ttAscii)
			{
			
			dng_string s;
			
			ParseStringTag (stream,
							parentCode,
							tagCode,
							tagCount,
							s,
							false);

			}
			
		}
	
	#endif
	
	}

bool dng_info::ValidateIFD (dng_stream &stream,
							uint64 ifdOffset,
							int64 offsetDelta)
	{
	
	bool isBigTIFF = (fMagic == magicBigTIFF);

	const uint64 kCountBytes	= isBigTIFF ? 8 : 2;
	const uint64 kHeaderBytes	= isBigTIFF ? 8 : 2;
	const uint64 kEntryBytes	= isBigTIFF ? 20 : 12;
	const uint64 kNextIFDBytes	= isBigTIFF ? 8 : 4;
	const uint64 kMaxUint64		= ~uint64 (0);
	
	
	
	if (ifdOffset > stream.Length () ||
		kCountBytes > stream.Length () - ifdOffset)
		{
		return false;
		}
		
	
		
	stream.SetReadPosition (ifdOffset);
	
	uint64 ifdEntries = isBigTIFF ? stream.Get_uint64 ()
								  : stream.Get_uint16 ();
	
	if (ifdEntries < 1)
		{
		return false;
		}
		
	

	if (ifdEntries > (kMaxUint64 - kHeaderBytes - kNextIFDBytes) / kEntryBytes)
		{
		return false;
		}

	const uint64 ifdSpan = kHeaderBytes + ifdEntries * kEntryBytes + kNextIFDBytes;
		
	if (ifdOffset > stream.Length () ||
		ifdSpan > stream.Length () - ifdOffset)
		{
		return false;
		}
		
	
	
	for (uint64 tag_index = 0; tag_index < ifdEntries; tag_index++)
		{
		
		stream.SetReadPosition (ifdOffset + kHeaderBytes + tag_index * kEntryBytes);
		
		stream.Skip (2);		
		
		uint32 tagType = stream.Get_uint16 ();
		
		uint64 tagCount = isBigTIFF ? stream.Get_uint64 ()
									: stream.Get_uint32 ();
		
		uint64 tag_type_size = (uint64) TagTypeSize (tagType);
		
		if (tag_type_size == 0)
			{
			return false;
			}

		uint64 tag_data_size = tagCount * tag_type_size;

		
		
		if (tag_data_size < tagCount ||
			tag_data_size < tag_type_size)
			return false;
		
		if (tag_data_size > (isBigTIFF ? 8 : 4))
			{
			
			uint64 tagOffset = isBigTIFF ? stream.Get_uint64 ()
										 : stream.Get_uint32 ();
							
			if (!ApplyOffsetDelta (tagOffset, offsetDelta, &tagOffset))
				{
				return false;
				}

			if (SafeUint64Add (tagOffset,
							   tag_data_size) > stream.Length ())
				{
				return false;
				}
			
			}
			
		}
		
	return true;
	
	}

void dng_info::ParseIFD (dng_host &host,
						 dng_stream &stream,
						 dng_exif *exif,
						 dng_shared *shared,
						 dng_ifd *ifd,
						 uint64 ifdOffset,
						 int64 offsetDelta,
						 uint32 parentCode)
	{
	
	#if qDNGValidate

	bool isMakerNote = (parentCode >= tcFirstMakerNoteIFD &&
						parentCode <= tcLastMakerNoteIFD);
	
	#endif
	
	bool isBigTIFF = (fMagic == magicBigTIFF);
	
	
	
	
	
	
	dng_stream_double_buffered ifdStream (stream);

	ifdStream.SetReadPosition (ifdOffset);
	
	if (ifd)
		{
		ifd->fThisIFD = ifdOffset;
		}
	
	uint64 ifdEntries = isBigTIFF ? ifdStream.Get_uint64 ()
								  : ifdStream.Get_uint16 ();

	
	
	
	
	
	

	const uint64 kEntryStride  = isBigTIFF ? 20 : 12;
	const uint64 kEntryHeader  = isBigTIFF ?  8 :  2;
	const uint64 kNextIFDBytes = isBigTIFF ?  8 :  4;
	const uint64 kMaxUint64    = ~uint64 (0);

	if (ifdEntries > (kMaxUint64 - kEntryHeader - kNextIFDBytes) / kEntryStride)
		{
		ThrowBadFormat ();
		}

	#if qDNGValidate
		
	bool generateOddOffsetWarnings = !gImagecore;
		
	if (gVerbose)
		{
		
		printf ("%s: Offset = %llu, Entries = %llu\n\n",
				LookupParentCode (parentCode),
				(unsigned long long) ifdOffset,
				(unsigned long long) ifdEntries);
		
		}
		
	if (generateOddOffsetWarnings && (ifdOffset & 1) && !isMakerNote)
		{
		
		char message [256];
	
		snprintf (message,
				  256,
				  "%s has odd offset (%u)",
				  LookupParentCode (parentCode),
				  (unsigned) ifdOffset);
					 
		ReportWarning (message);
		
		}
		
	uint32 prev_tag_code = 0;
		
	#endif
		
	for (uint64 tag_index = 0; tag_index < ifdEntries; tag_index++)
		{

		
		
		

		ifdStream.SetReadPosition (SafeUint64Add (ifdOffset,
												  kEntryHeader,
												  tag_index * kEntryStride));
		
		uint32 tagCode	= ifdStream.Get_uint16 ();
		uint32 tagType	= ifdStream.Get_uint16 ();
		
		
		
		
		
		if (tagCode == 0 && tagType == 0)
			{
			
			#if qDNGValidate
			
			char message [256];
	
			snprintf (message,
					  256,
					  "%s had zero/zero tag code/type entry",
					  LookupParentCode (parentCode));
					 
			ReportWarning (message);
			
			#endif
			
			return;
			
			}
		
		uint64 tagCount = isBigTIFF ? ifdStream.Get_uint64 ()
									: ifdStream.Get_uint32 ();
		
		#if qDNGValidate

			{
		
			if (tag_index > 0 && tagCode <= prev_tag_code && !isMakerNote)
				{
				
				char message [256];
		
				snprintf (message,
						  256,
						  "%s tags are not sorted in ascending numerical order",
						  LookupParentCode (parentCode));
						 
				ReportWarning (message);
				
				}
				
			}
			
		prev_tag_code = tagCode;
		
		#endif
			
		uint32 tag_type_size = TagTypeSize (tagType);
		
		if (tag_type_size == 0)
			{
			
			#if qDNGValidate
			
				{
			
				char message [256];
		
				snprintf (message,
						  256,
						  "%s %s has unknown type (%u)",
						  LookupParentCode (parentCode),
						  LookupTagCode (parentCode, tagCode),
						  (unsigned) tagType);
						 
				ReportWarning (message);
							 
				}
				
			#endif
					 
			continue;
			
			}
			
		bool localTag = true;
			
		uint64 tagOffset = isBigTIFF ? ifdOffset + 8 + tag_index * 20 + 12
									 : ifdOffset + 2 + tag_index * 12 +	 8;

		const uint64 tag_data_size = tagCount * (uint64) tag_type_size;

		
		
		if (tag_data_size < tagCount)
			{
			ThrowBadFormat ("overflow in tag_data_size");
			}
		
		if (tag_data_size > (isBigTIFF ? 8 : 4))
			{
			
			tagOffset = isBigTIFF ? ifdStream.Get_uint64 ()
								  : ifdStream.Get_uint32 ();
			
			#if qDNGValidate
			
				{
			
				if (generateOddOffsetWarnings &&
					!(ifdOffset & 1) &&
					 (tagOffset & 1) &&
					!isMakerNote	 &&
					parentCode != tcKodakDCRPrivateIFD &&
					parentCode != tcKodakKDCPrivateIFD)
					{
					
					char message [256];
		
					snprintf (message,
							  256,
							  "%s %s has odd data offset (%u)",
							  LookupParentCode (parentCode),
							  LookupTagCode (parentCode, tagCode),
							  (unsigned) tagOffset);
							 
					ReportWarning (message);
						 
					}
					
				}
				
			#endif
				
			if (!ApplyOffsetDelta (tagOffset, offsetDelta, &tagOffset))
				{
				ThrowBadFormat ("tag offset adjustment overflow");
				}

			if (SafeUint64Add (tagOffset, tag_data_size) > stream.Length ())
				{

				
				
				
				
				
				

				if (parentCode != tcExifIFD &&
					!(parentCode == 0 && tagCode == 0xC519))
					{

					ThrowBadFormat ("tag payload past stream end");

					}

				#if qDNGValidate

					{

					char message [256];

					snprintf (message,
							  256,
							  "%s %s payload extends past stream end (offset=%llu, size=%llu); skipping",
							  LookupParentCode (parentCode),
							  LookupTagCode (parentCode, tagCode),
							  (unsigned long long) tagOffset,
							  (unsigned long long) tag_data_size);

					ReportWarning (message);

					}

				#endif

				continue;

				}

			localTag = ifdStream.DataInBuffer (tag_data_size,
											   tagOffset);
				
			if (localTag)
				ifdStream.SetReadPosition (tagOffset);
			else
				stream.SetReadPosition (tagOffset);
			
			}
			
		
		
			
		if (tagCount <= 0x0FFFFFFFF)
			{
			
			ParseTag (host,
					  localTag ? ifdStream : stream,
					  exif,
					  shared,
					  ifd,
					  parentCode,
					  tagCode,
					  tagType,
					  (uint32) tagCount,
					  tagOffset,
					  offsetDelta);
					  
			}
			
		#if qDNGValidate
		
		else
			{
			
			char message [256];

			snprintf (message,
					  256,
					  "%s %s has larger than 32-bit tag count (%llu)",
					  LookupParentCode (parentCode),
					  LookupTagCode (parentCode, tagCode),
					  (unsigned long long) tagCount);
					 
			ReportWarning (message);
								 
			}
		
		#endif
			
		}
		
	ifdStream.SetReadPosition (SafeUint64Add (ifdOffset,
											  kEntryHeader,
											  ifdEntries * kEntryStride));
	
	uint64 nextIFD = isBigTIFF ? ifdStream.Get_uint64 ()
							   : ifdStream.Get_uint32 ();
	
	#if qDNGValidate
		
	if (gVerbose)
		{
		printf ("NextIFD = %llu\n", (unsigned long long) nextIFD);
		}
		
	#endif
		
	if (ifd)
		{
		ifd->fNextIFD = nextIFD;
		}
		
	#if qDNGValidate

	if (nextIFD)
		{
		
		if (parentCode != 0 &&
				(parentCode < tcFirstChainedIFD ||
				 parentCode > tcLastChainedIFD	))
			{

			char message [256];

			snprintf (message,
					  256,
					  "%s has an unexpected non-zero NextIFD (%llu)",
					  LookupParentCode (parentCode),
					  (unsigned long long) nextIFD);
					 
			ReportWarning (message);
					 
			}

		}
		
	if (gVerbose)
		{
		printf ("\n");
		}
		
	stream.SetReadPosition (ifdStream.Position ());

	#endif
		
	}
						 

bool dng_info::ParseMakerNoteIFD (dng_host &host,
								  dng_stream &stream,
								  uint64 ifdSize,
								  uint64 ifdOffset,
								  int64 offsetDelta,
								  uint64 minOffset,
								  uint64 maxOffset,
								  uint32 parentCode)
	{

	

	if (fParseDepth > kMaxParseDepth)
		{		
		return false;
		}

	RecursionProtector depthProtect (fParseDepth);
	
	uint32 tagIndex;
	uint32 tagCode;
	uint32 tagType;
	uint32 tagCount;
	
	
	
	fMakerNoteNextIFD = 0;
	
	
	
	if (ifdSize < 14)
		{
		return false;
		}
		
	
	
	dng_stream_double_buffered ifdStream (stream);
	
	ifdStream.SetReadPosition (ifdOffset);
	
	uint32 ifdEntries = ifdStream.Get_uint16 ();

	
	
	if (ifdEntries < 1 || 2 + ifdEntries * 12 > ifdSize)
		{
		return false;
		}
		
	
		
	for (tagIndex = 0; tagIndex < ifdEntries; tagIndex++)
		{
		
		ifdStream.SetReadPosition (ifdOffset + 2 + tagIndex * 12 + 2);
		
		tagType = ifdStream.Get_uint16 ();
		
		
		
		
		if (parentCode == tcCanonMakerNote && tagType == 0)
			{
			continue;
			}

		
		
		if (parentCode == tcAppleMakerNote && tagType == 0)
			{
			continue;
			}
		
		if (TagTypeSize (tagType) == 0)
			{
			return false;
			}
		
		}
		
	
	
	#if qDNGValidate
	
	if (gVerbose)
		{
		
		printf ("%s: Offset = %u, Entries = %u\n\n",
				LookupParentCode (parentCode),
				(unsigned) ifdOffset, 
				(unsigned) ifdEntries);
		
		}
		
	#endif
		
	for (tagIndex = 0; tagIndex < ifdEntries; tagIndex++)
		{
		
		ifdStream.SetReadPosition (ifdOffset + 2 + tagIndex * 12);
		
		tagCode	 = ifdStream.Get_uint16 ();
		tagType	 = ifdStream.Get_uint16 ();
		tagCount = ifdStream.Get_uint32 ();
		
		if (tagType == 0)
			{
			continue;
			}
		
		uint32 tagSize = 0;

		try
			{
		
			tagSize = SafeUint32Mult (tagCount,
									  TagTypeSize (tagType));

			}

		catch (...)
			{
			
			
			continue;
			}
		
		uint64 tagOffset = ifdOffset + 2 + tagIndex * 12 + 8;
		
		bool localTag = true;
		
		if (tagSize > 4)
			{
			
			if (!ApplyOffsetDelta (ifdStream.Get_uint32 (),
								   offsetDelta,
								   &tagOffset))
				{
				continue;
				}

			try
				{
			
				if (tagOffset < minOffset ||
					SafeUint64Add (tagOffset, tagSize) > maxOffset)
					{

					
					

					continue;

					}

				}

			catch (...)
				{
				
				continue;
				}
				
			localTag = ifdStream.DataInBuffer (tagSize, tagOffset);
			
			ifdStream.SetReadPosition (tagOffset);
			
			stream.SetReadPosition (tagOffset);
			
			}
			
		
		
		if (parentCode == tcOlympusMakerNote &&
			tagType == ttIFD &&
			tagCount == 1)
			{
			
			uint32 olympusMakerParent = 0;
			
			switch (tagCode)
				{
				
				case 8208:
					olympusMakerParent = tcOlympusMakerNote8208;
					break;
					
				case 8224:
					olympusMakerParent = tcOlympusMakerNote8224;
					break; 
			
				case 8240:
					olympusMakerParent = tcOlympusMakerNote8240;
					break; 
			
				case 8256:
					olympusMakerParent = tcOlympusMakerNote8256;
					break; 
			
				case 8272:
					olympusMakerParent = tcOlympusMakerNote8272;
					break; 
			
				case 12288:
					olympusMakerParent = tcOlympusMakerNote12288;
					break;
					
				default:
					break;
					
				}
				
			if (olympusMakerParent)
				{
				
				stream.SetReadPosition (tagOffset);
			
				uint64 subMakerNoteOffset = 0;

				if (!ApplyOffsetDelta (stream.Get_uint32 (),
									   offsetDelta,
									   &subMakerNoteOffset))
					{
					continue;
					}

				if (subMakerNoteOffset >= minOffset &&
					subMakerNoteOffset <  maxOffset)
					{
				
					if (ParseMakerNoteIFD (host,
										   stream,
										   maxOffset - subMakerNoteOffset,
										   subMakerNoteOffset,
										   offsetDelta,
										   minOffset,
										   maxOffset,
										   olympusMakerParent))
						{
						
						continue;
						
						}
						
					}
				
				}
				
			stream.SetReadPosition (tagOffset);
			
			}
		
		ParseTag (host,
				  localTag ? ifdStream : stream,
				  fExif.Get (),
				  fShared.Get (),
				  NULL,
				  parentCode,
				  tagCode,
				  tagType,
				  tagCount,
				  tagOffset,
				  offsetDelta);
			
		}
		
	
	
	if (ifdSize >= 2 + ifdEntries * 12 + 4)
		{
		
		ifdStream.SetReadPosition (ifdOffset + 2 + ifdEntries * 12);
		
		fMakerNoteNextIFD = ifdStream.Get_uint32 ();
		
		}
		
	#if qDNGValidate
		
	if (gVerbose)
		{
		printf ("\n");
		}
		
	#endif
		
	return true;
		
	}
						 

void dng_info::ParseMakerNote (dng_host &host,
							   dng_stream &stream,
							   uint32 makerNoteCount,
							   uint64 makerNoteOffset,
							   int64 offsetDelta,
							   uint64 minOffset,
							   uint64 maxOffset)
	{
	
	uint8 firstBytes [16];
	
	memset (firstBytes, 0, sizeof (firstBytes));
	
	stream.SetReadPosition (makerNoteOffset);
	
	stream.Get (firstBytes, (uint32) Min_uint64 (sizeof (firstBytes),
												 makerNoteCount));
	
	
	
	if (memcmp (firstBytes, "Apple iOS", 9) == 0)
		{

		if (makerNoteCount < 14)
			{
			return;
			}
		
		stream.SetReadPosition (SafeUint64Add (makerNoteOffset, 12));
		
		bool bigEndian = false;
		
		uint16 endianMark = stream.Get_uint16 ();
		
		if (endianMark == byteOrderMM)
			{
			bigEndian = true;
			}
			
		else if (endianMark != byteOrderII)
			{
			return;
			}

		TempBigEndian temp_endian (stream, bigEndian);
		
		if (makerNoteCount > 14)
			{
		
			ParseMakerNoteIFD (host,
							   stream,
							   makerNoteCount - 14,
							   SafeUint64Add (makerNoteOffset, 14),
							   makerNoteOffset,
							   minOffset,
							   maxOffset,
							   tcAppleMakerNote);
							   
			}
			
		return;
					
		}
		
	
	
	if (memcmp (firstBytes, "EPSON\000\001\000", 8) == 0)
		{
		
		if (makerNoteCount > 8)
			{
		
			ParseMakerNoteIFD (host,
							   stream,
							   makerNoteCount - 8,
							   makerNoteOffset + 8,
							   offsetDelta,
							   minOffset,
							   maxOffset,
							   tcEpsonMakerNote);
							   
			}
			
		return;
		
		}
		
	
	
	if (memcmp (firstBytes, "FUJIFILM", 8) == 0)
		{

		if (makerNoteCount < 12)
			{
			return;
			}
		
		stream.SetReadPosition (SafeUint64Add (makerNoteOffset, 8));
		
		TempLittleEndian tempEndian (stream);
		
		uint32 ifd_offset = stream.Get_uint32 ();
		
		if (ifd_offset >= 12 && ifd_offset < makerNoteCount)
			{
			
			ParseMakerNoteIFD (host,
							   stream,
							   makerNoteCount - ifd_offset,
							   SafeUint64Add (makerNoteOffset, ifd_offset),
							   makerNoteOffset,
							   minOffset,
							   maxOffset,
							   tcFujiMakerNote);
			
			}
			
		return;
					
		}
		
	
	
	
	if ((memcmp (firstBytes, "LEICA\000\000\000", 8) == 0) ||
		(memcmp (firstBytes, "LEICA0\003\000",	  8) == 0) ||
		(memcmp (firstBytes, "LEICA\000\001\000", 8) == 0) ||
		(memcmp (firstBytes, "LEICA\000\004\000", 8) == 0) ||
		(memcmp (firstBytes, "LEICA\000\005\000", 8) == 0) ||
		(memcmp (firstBytes, "LEICA\000\006\000", 8) == 0))
		{

		if (makerNoteCount > 8)
			{
		
			ParseMakerNoteIFD (host,
							   stream,
							   makerNoteCount - 8,
							   makerNoteOffset + 8,
							   makerNoteOffset,
							   minOffset,
							   maxOffset,
							   tcLeicaMakerNote);
							   
			}
		
		return;

		}

	
	

	if ((memcmp (firstBytes, "LEICA\000\002\377", 8) == 0) ||
		(memcmp (firstBytes, "LEICA\000\002\000", 8) == 0))
		{
		
		if (makerNoteCount > 8)
			{
		
			ParseMakerNoteIFD (host,
							   stream,
							   makerNoteCount - 8,
							   makerNoteOffset + 8,
							   offsetDelta,
							   minOffset,
							   maxOffset,
							   tcLeicaMakerNote);
							   
			}
		
		return;
		
		}
		
	
	
	if (memcmp (firstBytes, "Nikon\000\002", 7) == 0)
		{

		if (makerNoteCount < 18)
			{
			return;
			}

		stream.SetReadPosition (makerNoteOffset + 10);
		
		bool bigEndian = false;
		
		uint16 endianMark = stream.Get_uint16 ();
		
		if (endianMark == byteOrderMM)
			{
			bigEndian = true;
			}
			
		else if (endianMark != byteOrderII)
			{
			return;
			}
			
		TempBigEndian temp_endian (stream, bigEndian);
		
		uint16 magic = stream.Get_uint16 ();
		
		if (magic != 42)
			{
			return;
			}
			
		uint32 ifd_offset = stream.Get_uint32 ();
		
		if (ifd_offset >= 8 && ifd_offset < makerNoteCount - 10)
			{
			
			ParseMakerNoteIFD (host,
							   stream,
							   makerNoteCount - 10 - ifd_offset,
							   makerNoteOffset + 10 + ifd_offset,
							   makerNoteOffset + 10,
							   minOffset,
							   maxOffset,
							   tcNikonMakerNote);
			
			}
			
		return;
					
		}
		
	
	
	if (memcmp (firstBytes, "OLYMPUS\000", 8) == 0)
		{

		if (makerNoteCount < 12)
			{
			return;
			}
		
		stream.SetReadPosition (SafeUint64Add (makerNoteOffset, 8));
		
		bool bigEndian = false;
		
		uint16 endianMark = stream.Get_uint16 ();
		
		if (endianMark == byteOrderMM)
			{
			bigEndian = true;
			}
			
		else if (endianMark != byteOrderII)
			{
			return;
			}
			
		TempBigEndian temp_endian (stream, bigEndian);
		
		uint16 version = stream.Get_uint16 ();
		
		if (version != 3)
			{
			return;
			}
		
		if (makerNoteCount > 12)
			{
		
			ParseMakerNoteIFD (host,
							   stream,
							   makerNoteCount - 12,
							   SafeUint64Add (makerNoteOffset, 12),
							   makerNoteOffset,
							   minOffset,
							   maxOffset,
							   tcOlympusMakerNote);
							   
			}
			
		return;
		
		}
		
	
	
	if (memcmp (firstBytes, "OLYMP", 5) == 0)
		{
		
		if (makerNoteCount > 8)
			{
		
			ParseMakerNoteIFD (host,
							   stream,
							   makerNoteCount - 8,
							   makerNoteOffset + 8,
							   offsetDelta,
							   minOffset,
							   maxOffset,
							   tcOlympusMakerNote);
							   
			}
			
		return;
		
		}
	
	
	
	
	
	if (memcmp (firstBytes, "OM SYSTEM\000", 10) == 0)
		{

		if (makerNoteCount < 16)
			{
			return;
			}
		
		stream.SetReadPosition (SafeUint64Add (makerNoteOffset, 12));
		
		bool bigEndian = false;
		
		uint16 endianMark = stream.Get_uint16 ();
		
		if (endianMark == byteOrderMM)
			{
			bigEndian = true;
			}
			
		else if (endianMark != byteOrderII)
			{
			return;
			}
			
		TempBigEndian temp_endian (stream, bigEndian);
		
		uint16 version = stream.Get_uint16 ();
		
		if (version != 4)
			{
			return;
			}
		
		if (makerNoteCount > 16)
			{
		
			ParseMakerNoteIFD (host,
							   stream,
							   makerNoteCount - 16,
							   SafeUint64Add (makerNoteOffset, 16),
							   makerNoteOffset,
							   minOffset,
							   maxOffset,
							   tcOlympusMakerNote);
							   
			}
			
		return;
		
		}
		
	
	
	if (memcmp (firstBytes, "Panasonic\000\000\000", 12) == 0)
		{
		
		if (makerNoteCount > 12)
			{
		
			ParseMakerNoteIFD (host,
							   stream,
							   makerNoteCount - 12,
							   makerNoteOffset + 12,
							   offsetDelta,
							   minOffset,
							   maxOffset,
							   tcPanasonicMakerNote);
							   
			}
		
		return;
		
		}
		
	
	
	if (memcmp (firstBytes, "AOC", 4) == 0)
		{
		
		if (makerNoteCount > 6)
			{
					
			stream.SetReadPosition (makerNoteOffset + 4);
			
			bool bigEndian = stream.BigEndian ();
			
			uint16 endianMark = stream.Get_uint16 ();
			
			if (endianMark == byteOrderMM)
				{
				bigEndian = true;
				}
				
			else if (endianMark == byteOrderII)
				{
				bigEndian = false;
				}
				
			TempBigEndian temp_endian (stream, bigEndian);
		
			ParseMakerNoteIFD (host,
							   stream,
							   makerNoteCount - 6,
							   makerNoteOffset + 6,
							   offsetDelta,
							   minOffset,
							   maxOffset,
							   tcPentaxMakerNote);
			
			}
			
		return;
		
		}

	
					
	if (memcmp (firstBytes, "PENTAX", 6) == 0)
		{
		
		
		

		if (makerNoteCount >= 10)
			{
					
			stream.SetReadPosition (SafeUint64Add (makerNoteOffset, 8));
			
			bool bigEndian = stream.BigEndian ();
			
			uint16 endianMark = stream.Get_uint16 ();
			
			if (endianMark == byteOrderMM)
				{
				bigEndian = true;
				}
				
			else if (endianMark == byteOrderII)
				{
				bigEndian = false;
				}
				
			TempBigEndian temp_endian (stream, bigEndian);
		
			ParseMakerNoteIFD (host,
							   stream,
							   makerNoteCount - 10,
							   SafeUint64Add (makerNoteOffset, 10),
							   makerNoteOffset,		
							   minOffset,
							   maxOffset,
							   tcPentaxMakerNote);
			
			}
			
		return;
		
		}
					
	
	
	if (memcmp (firstBytes, "RICOH", 5) == 0 ||
		memcmp (firstBytes, "Ricoh", 5) == 0)
		{
		
		if (makerNoteCount > 8)
			{
			
			TempBigEndian tempEndian (stream);
		
			ParseMakerNoteIFD (host,
							   stream,
							   makerNoteCount - 8,
							   makerNoteOffset + 8,
							   offsetDelta,
							   minOffset,
							   maxOffset,
							   tcRicohMakerNote);
							   
			}
			
		return;
		
		}
		
	
	
	if (fExif->fMake.StartsWith ("NIKON"))
		{
		
		ParseMakerNoteIFD (host,
						   stream,
						   makerNoteCount,
						   makerNoteOffset,
						   offsetDelta,
						   minOffset,
						   maxOffset,
						   tcNikonMakerNote);
						   
		return;
			
		}
	
	
	
	if (fExif->fMake.StartsWith ("CANON"))
		{
		
		ParseMakerNoteIFD (host,
						   stream,
						   makerNoteCount,
						   makerNoteOffset,
						   offsetDelta,
						   minOffset,
						   maxOffset,
						   tcCanonMakerNote);
			
		return;
		
		}
		
	
	
	if (fExif->fMake.StartsWith ("MINOLTA"		 ) ||
		fExif->fMake.StartsWith ("KONICA MINOLTA"))
		{

		ParseMakerNoteIFD (host,
						   stream,
						   makerNoteCount,
						   makerNoteOffset,
						   offsetDelta,
						   minOffset,
						   maxOffset,
						   tcMinoltaMakerNote);
			
		return;
		
		}
	
	
	
	if (fExif->fMake.StartsWith ("SONY"))
		{

		ParseMakerNoteIFD (host,
						   stream,
						   makerNoteCount,
						   makerNoteOffset,
						   offsetDelta,
						   minOffset,
						   maxOffset,
						   tcSonyMakerNote);
			
		return;
		
		}
	
	
	
	if (fExif->fMake.StartsWith ("EASTMAN KODAK"))
		{
		
		ParseMakerNoteIFD (host,
						   stream,
						   makerNoteCount,
						   makerNoteOffset,
						   offsetDelta,
						   minOffset,
						   maxOffset,
						   tcKodakMakerNote);
						   
		return;
			
		}
	
	
	
	if (fExif->fMake.StartsWith ("Mamiya"))
		{
		
		if (!ParseMakerNoteIFD (host,
								stream,
								makerNoteCount,
								makerNoteOffset,
								offsetDelta,
								minOffset,
								maxOffset,
								tcMamiyaMakerNote))
			{
			return;
			}
						   
		

		uint64 visitedOffsets [kMaxChainedIFDs];
		uint32 visitedCount = 0;

		visitedOffsets [visitedCount++] = makerNoteOffset;

		while (fMakerNoteNextIFD)
			{

			if (visitedCount >= kMaxChainedIFDs)
				{
				ThrowBadFormat ("Mamiya MakerNote chain too long");
				}

			uint64 nextIFDOffset = 0;

			if (!ApplyOffsetDelta (fMakerNoteNextIFD,
								   offsetDelta,
								   &nextIFDOffset))
				{
				ThrowBadFormat ("Invalid Mamiya MakerNote chain offset");
				}

			if (nextIFDOffset < minOffset ||
				nextIFDOffset >= maxOffset)
				{
				ThrowBadFormat ("Mamiya MakerNote chain out of bounds");
				}

			for (uint32 index = 0; index < visitedCount; index++)
				{

				if (visitedOffsets [index] == nextIFDOffset)
					{
					ThrowBadFormat ("Mamiya MakerNote chain cycle");
					}

				}

			visitedOffsets [visitedCount++] = nextIFDOffset;

			if (!ParseMakerNoteIFD (host,
									stream,
									Min_uint64 (makerNoteCount,
												maxOffset - nextIFDOffset),
									nextIFDOffset,
									offsetDelta,
									minOffset,
									maxOffset,
									tcMamiyaMakerNote))
				{
				ThrowBadFormat ("Invalid Mamiya MakerNote chain");
				}
							   
			}
						   
		return;
			
		}
	
	
	
	if (fExif->fMake.StartsWith ("Hasselblad"))
		{
		
		ParseMakerNoteIFD (host,
						   stream,
						   makerNoteCount,
						   makerNoteOffset,
						   offsetDelta,
						   minOffset,
						   maxOffset,
						   tcHasselbladMakerNote);
						   
		return;
			
		}

	

	if (fExif->fMake.StartsWith ("Samsung"))
		{
		
		ParseMakerNoteIFD (host,
						   stream,
						   makerNoteCount,
						   makerNoteOffset,
						   makerNoteOffset,
						   minOffset,
						   maxOffset,
						   tcSamsungMakerNote);
		
		return;
		
		}
	
	
	
	
	

	if (fExif->fMake.StartsWith ("CASIO COMPUTER") &&
		makerNoteCount >= 6 &&
		memcmp (firstBytes, "QVC\000\000\000", 6) == 0)
		{
		
		ParseMakerNoteIFD (host,
						   stream,
						   makerNoteCount - 6,
						   SafeUint64Add (makerNoteOffset, 6),
						   makerNoteOffset,
						   minOffset,
						   maxOffset,
						   tcCasioMakerNote);
						   
		return;
			
		}
	
	}
									 

void dng_info::ParseSonyPrivateData (dng_host & ,
									 dng_stream & ,
									 uint64 ,
									 uint64 ,
									 uint64 )
	{
	
	
	
	}
									 

void dng_info::ParseDNGPrivateData (dng_host &host,
									dng_stream &stream)
	{
	
	if (fShared->fDNGPrivateDataCount < 2)
		{
		return;
		}
	
	
	
			
	dng_string privateName;
			
		{
			
		char buffer [64];
		
		stream.SetReadPosition (fShared->fDNGPrivateDataOffset);
	
		uint32 readLength = Min_uint32 (fShared->fDNGPrivateDataCount,
										sizeof (buffer) - 1);
		
		stream.Get (buffer, readLength);
		
		buffer [readLength] = 0;
		
		privateName.Set (buffer);
		
		}
		
	
	
	if (privateName.StartsWith ("PENTAX" ) ||
		privateName.StartsWith ("SAMSUNG"))
		{
		
		#if qDNGValidate
		
		if (gVerbose)
			{
			printf ("Parsing Pentax/Samsung DNGPrivateData\n\n");
			}
			
		#endif

		
		

		if (fShared->fDNGPrivateDataCount < 10)
			{
			return;
			}

		const uint64 privateDataEnd =
			SafeUint64Add (fShared->fDNGPrivateDataOffset,
						   fShared->fDNGPrivateDataCount);

		stream.SetReadPosition
			(SafeUint64Add (fShared->fDNGPrivateDataOffset, 8));
		
		bool bigEndian = stream.BigEndian ();
		
		uint16 endianMark = stream.Get_uint16 ();
		
		if (endianMark == byteOrderMM)
			{
			bigEndian = true;
			}
			
		else if (endianMark == byteOrderII)
			{
			bigEndian = false;
			}
			
		TempBigEndian temp_endian (stream, bigEndian);
	
		ParseMakerNoteIFD (host,
						   stream,
						   fShared->fDNGPrivateDataCount - 10,
						   SafeUint64Add (fShared->fDNGPrivateDataOffset, 10),
						   fShared->fDNGPrivateDataOffset,
						   fShared->fDNGPrivateDataOffset,
						   privateDataEnd,
						   tcPentaxMakerNote);
						   
		return;
		
		}
	
	else if (privateName.StartsWith ("RICOH"))
		  {
		  
		  #if qDNGValidate
		  
		  if (gVerbose)
			  {
			  printf ("Parsing RICOH-PENTAX DNGPrivateData\n\n");
			  }
			  
		  #endif

		  
		  

		  if (fShared->fDNGPrivateDataCount < 8)
			  {
			  return;
			  }

		  const uint64 privateDataEnd =
			  SafeUint64Add (fShared->fDNGPrivateDataOffset,
							 fShared->fDNGPrivateDataCount);

		  stream.SetReadPosition
			  (SafeUint64Add (fShared->fDNGPrivateDataOffset, 8));
			  
		  TempBigEndian temp_endian (stream, false);
		  
		  ParseMakerNoteIFD (host,
							 stream,
							 fShared->fDNGPrivateDataCount - 8,
							 SafeUint64Add (fShared->fDNGPrivateDataOffset, 8),
							 fShared->fDNGPrivateDataOffset,
							 fShared->fDNGPrivateDataOffset,
							 privateDataEnd,
							 tcPentaxMakerNote);
							 
		  return;
		  
		  }
				
	
	
	if (!privateName.Matches ("Adobe"))
		{
		return;
		}
	
	TempBigEndian temp_order (stream);
	
	uint32 section_offset = 6;
	
	while (SafeUint32Add (section_offset, 8) < fShared->fDNGPrivateDataCount)
		{
		
		stream.SetReadPosition (SafeUint64Add (fShared->fDNGPrivateDataOffset,
											   section_offset));
		
		uint32 section_key	 = stream.Get_uint32 ();
		uint32 section_count = stream.Get_uint32 ();

		const uint32 section_data_offset = SafeUint32Add (section_offset, 8);

		if (section_data_offset > fShared->fDNGPrivateDataCount ||
			section_count > fShared->fDNGPrivateDataCount - section_data_offset)
			{
			ThrowBadFormat ("DNGPrivateData section extends past tag data");
			}

		const uint32 section_end_offset =
			SafeUint32Add (section_data_offset, section_count);

		const uint64 section_data_start =
			SafeUint64Add (fShared->fDNGPrivateDataOffset,
						   section_data_offset);

		const uint64 section_end =
			SafeUint64Add (fShared->fDNGPrivateDataOffset,
						   section_end_offset);

		const auto requireSectionRange =
			[section_data_start,
			 section_end] (uint64 range_offset,
						   uint64 range_count)
			{

			if (range_offset < section_data_start ||
				range_offset > section_end ||
				range_count > section_end - range_offset)
				{
				ThrowBadFormat ("DNGPrivateData section read out of bounds");
				}

			};
		
		if (section_key == DNG_CHAR4 ('M','a','k','N') && section_count > 6)
			{
			
			#if qDNGValidate
			
			if (gVerbose)
				{
				printf ("Found MakerNote inside DNGPrivateData\n\n");
				}
				
			#endif
				
			requireSectionRange (stream.Position (), 6);

			uint16 order_mark = stream.Get_uint16 ();

			
			
			
			
			

			const uint32 raw_old_offset = stream.Get_uint32 ();
			const int64 old_offset = static_cast<int64> (raw_old_offset);

			uint32 tempSize = SafeUint32Sub (section_count, 6);
			
			AutoPtr<dng_memory_block> tempBlock (host.Allocate (tempSize));
			
			uint64 positionInOriginalFile = stream.PositionInOriginalFile();
			
			stream.Get (tempBlock->Buffer (), tempSize);
			
			dng_stream tempStream (tempBlock->Buffer (),
								   tempSize,
								   positionInOriginalFile);
								   
			tempStream.SetBigEndian (order_mark == byteOrderMM);
			
			ParseMakerNote (host,
							tempStream,
							tempSize,
							0,
							-old_offset,
							0,
							tempSize);
	
			}
			
		else if (section_key == DNG_CHAR4 ('S','R','2',' ') && section_count > 6)
			{
			
			#if qDNGValidate
			
			if (gVerbose)
				{
				printf ("Found Sony private data inside DNGPrivateData\n\n");
				}
				
			#endif
			
			requireSectionRange (stream.Position (), 6);

			uint16 order_mark = stream.Get_uint16 ();
			uint64 old_offset = stream.Get_uint32 ();

			uint64 new_offset =
				SafeUint64Add (fShared->fDNGPrivateDataOffset,
							   (uint64) section_offset,
							   14ull);
			
			TempBigEndian sr2_order (stream, order_mark == byteOrderMM);
			
			ParseSonyPrivateData (host,
								  stream,
								  section_count - 6,
								  old_offset,
								  new_offset);
				
			}

		else if (section_key == DNG_CHAR4 ('R','A','F',' ') && section_count > 4)
			{
			
			#if qDNGValidate
			
			if (gVerbose)
				{
				printf ("Found Fuji RAF tags inside DNGPrivateData\n\n");
				}
				
			#endif
			
			requireSectionRange (stream.Position (), 6);

			uint16 order_mark = stream.Get_uint16 ();
			
			uint32 tagCount = stream.Get_uint32 ();
			
			uint64 tagOffset = stream.Position ();
				
			if (tagCount)
				{
				
				TempBigEndian raf_order (stream, order_mark == byteOrderMM);

				requireSectionRange (tagOffset, tagCount);
				
				ParseTag (host,
						  stream,
						  fExif.Get (),
						  fShared.Get (),
						  NULL,
						  tcFujiRAF,
						  tcFujiHeader,
						  ttUndefined,
						  tagCount,
						  tagOffset,
						  0);
						  
				stream.SetReadPosition (SafeUint64Add (tagOffset, tagCount));
				
				}
			
			requireSectionRange (stream.Position (), 4);

			tagCount = stream.Get_uint32 ();
			
			tagOffset = stream.Position ();
				
			if (tagCount)
				{
				
				TempBigEndian raf_order (stream, order_mark == byteOrderMM);

				requireSectionRange (tagOffset, tagCount);
				
				ParseTag (host,
						  stream,
						  fExif.Get (),
						  fShared.Get (),
						  NULL,
						  tcFujiRAF,
						  tcFujiRawInfo1,
						  ttUndefined,
						  tagCount,
						  tagOffset,
						  0);
						  
				stream.SetReadPosition (SafeUint64Add (tagOffset, tagCount));
				
				}
			
			requireSectionRange (stream.Position (), 4);

			tagCount = stream.Get_uint32 ();
			
			tagOffset = stream.Position ();
				
			if (tagCount)
				{
				
				TempBigEndian raf_order (stream, order_mark == byteOrderMM);

				requireSectionRange (tagOffset, tagCount);
				
				ParseTag (host,
						  stream,
						  fExif.Get (),
						  fShared.Get (),
						  NULL,
						  tcFujiRAF,
						  tcFujiRawInfo2,
						  ttUndefined,
						  tagCount,
						  tagOffset,
						  0);
						  
				stream.SetReadPosition (SafeUint64Add (tagOffset, tagCount));
				
				}
			
			}

		else if (section_key == DNG_CHAR4 ('C','n','t','x') && section_count > 4)
			{
			
			#if qDNGValidate
			
			if (gVerbose)
				{
				printf ("Found Contax Raw header inside DNGPrivateData\n\n");
				}
				
			#endif
			
			requireSectionRange (stream.Position (), 6);

			uint16 order_mark = stream.Get_uint16 ();
			
			uint32 tagCount	 = stream.Get_uint32 ();
			
			uint64 tagOffset = stream.Position ();
				
			if (tagCount)
				{
				
				TempBigEndian contax_order (stream, order_mark == byteOrderMM);

				requireSectionRange (tagOffset, tagCount);
				
				ParseTag (host,
						  stream,
						  fExif.Get (),
						  fShared.Get (),
						  NULL,
						  tcContaxRAW,
						  tcContaxHeader,
						  ttUndefined,
						  tagCount,
						  tagOffset,
						  0);
						  
				}
			
			}
			
		else if (section_key == DNG_CHAR4 ('C','R','W',' ') && section_count > 4)
			{
			
			#if qDNGValidate
			
			if (gVerbose)
				{
				printf ("Found Canon CRW tags inside DNGPrivateData\n\n");
				}
				
			#endif
				
			requireSectionRange (stream.Position (), 4);

			uint16 order_mark = stream.Get_uint16 ();
			uint32 entries	  = stream.Get_uint16 ();
			
			uint64 crwTagStart = stream.Position ();
			
			for (uint32 parsePass = 1; parsePass <= 2; parsePass++)
				{
				
				stream.SetReadPosition (crwTagStart);
			
				for (uint32 index = 0; index < entries; index++)
					{

					requireSectionRange (stream.Position (), 6);
					
					uint32 tagCode = stream.Get_uint16 ();
											 
					uint32 tagCount = stream.Get_uint32 ();
					
					uint64 tagOffset = stream.Position ();
					
					
					
					
					if ((parsePass == 1) == (tagCode == 0x5834))
						{
				
						TempBigEndian tag_order (stream, order_mark == byteOrderMM);

						requireSectionRange (tagOffset, tagCount);
					
						ParseTag (host,
								  stream,
								  fExif.Get (),
								  fShared.Get (),
								  NULL,
								  tcCanonCRW,
								  tagCode,
								  ttUndefined,
								  tagCount,
								  tagOffset,
								  0);

						}
					
					requireSectionRange (tagOffset, tagCount);

					stream.SetReadPosition (SafeUint64Add (tagOffset, tagCount));
					
					}
					
				}
			
			}

		else if (section_count > 4)
			{
			
			uint32 parentCode = 0;
			
			bool code32	 = false;
			bool hasType = true;
			
			switch (section_key)
				{
				
				case DNG_CHAR4 ('M','R','W',' '):
					{
					parentCode = tcMinoltaMRW;
					code32	   = true;
					hasType	   = false;
					break;
					}
				
				case DNG_CHAR4 ('P','a','n','o'):
					{
					parentCode = tcPanasonicRAW;
					break;
					}
					
				case DNG_CHAR4 ('L','e','a','f'):
					{
					parentCode = tcLeafMOS;
					break;
					}
					
				case DNG_CHAR4 ('K','o','d','a'):
					{
					parentCode = tcKodakDCRPrivateIFD;
					break;
					}
					
				case DNG_CHAR4 ('K','D','C',' '):
					{
					parentCode = tcKodakKDCPrivateIFD;
					break;
					}
					
				default:
					break;
					
				}

			if (parentCode)
				{
			
				#if qDNGValidate
				
				if (gVerbose)
					{
					printf ("Found %s tags inside DNGPrivateData\n\n",
							LookupParentCode (parentCode));
					}
					
				#endif
				
				requireSectionRange (stream.Position (), 4);

				uint16 order_mark = stream.Get_uint16 ();
				uint32 entries	  = stream.Get_uint16 ();
				
				for (uint32 index = 0; index < entries; index++)
					{

					const uint32 entryHeaderSize =
						SafeUint32Add (code32 ? 4u : 2u,
									   hasType ? 2u : 0u,
									   4u);

					requireSectionRange (stream.Position (), entryHeaderSize);
					
					uint32 tagCode = code32 ? stream.Get_uint32 ()
											: stream.Get_uint16 ();
											 
					uint32 tagType	= hasType ? stream.Get_uint16 () 
											  : ttUndefined;
					
					uint32 tagCount = stream.Get_uint32 ();
					
					uint32 tagSize = SafeUint32Mult (tagCount, TagTypeSize (tagType));
					
					uint64 tagOffset = stream.Position ();

					requireSectionRange (tagOffset, tagSize);
					
					TempBigEndian tag_order (stream, order_mark == byteOrderMM);
				
					ParseTag (host,
							  stream,
							  fExif.Get (),
							  fShared.Get (),
							  NULL,
							  parentCode,
							  tagCode,
							  tagType,
							  tagCount,
							  tagOffset,
							  0);
					
					stream.SetReadPosition (SafeUint64Add (tagOffset, tagSize));
					
					}
					
				}
			
			}
		
		section_offset = section_end_offset;
		
		if (section_offset & 1)
			{
			if (section_offset >= fShared->fDNGPrivateDataCount)
				{
				break;
				}

			section_offset = SafeUint32Add (section_offset, 1);
			}
		
		}
		
	}
	

void dng_info::Parse (dng_host &host,
					  dng_stream &stream)
	{
	
	fTIFFBlockOffset = stream.Position ();
	
	fTIFFBlockOriginalOffset = stream.PositionInOriginalFile ();
	
	
	
	uint16 byteOrder = stream.Get_uint16 ();
	
	if (byteOrder == byteOrderII)
		{
		
		fBigEndian = false;
		
		#if qDNGValidate
		
		if (gVerbose)
			{
			printf ("\nUses little-endian byte order\n");
			}
			
		#endif
			
		stream.SetLittleEndian ();
		
		}
		
	else if (byteOrder == byteOrderMM)
		{

		fBigEndian = true;
		
		#if qDNGValidate
		
		if (gVerbose)
			{
			printf ("\nUses big-endian byte order\n");
			}
			
		#endif
			
		stream.SetBigEndian ();
		
		}
		
	else
		{
		
		#if qDNGValidate
		
		ReportError ("Unknown byte order");
					 
		#endif
					 
		ThrowBadFormat ();

		}
		
	
		
	fMagic = stream.Get_uint16 ();
	
	#if qDNGValidate
	
	if (gVerbose)
		{
		printf ("Magic number = %u\n\n", (unsigned) fMagic);
		}
		
	#endif
	
	ValidateMagic ();
	
	
	
	if (fMagic == magicBigTIFF)
		{
		
		uint16 byteSize = stream.Get_uint16 ();
		uint16 zeroPad	= stream.Get_uint16 ();
		
		if (byteSize != 8 || zeroPad != 0)
			{
			
			#if qDNGValidate
			
			ReportError ("Invalid BigTIFF header");
			
			#endif
			
			ThrowBadFormat ();
			
			}
		
		}
	
	
	
	uint64 next_offset = (fMagic == magicBigTIFF) ? stream.Get_uint64 ()
												  : stream.Get_uint32 ();
	
	fExif.Reset (host.Make_dng_exif ());
	
	fShared.Reset (host.Make_dng_shared ());
	
	fIFD.push_back (host.Make_dng_ifd ());
	
	ParseIFD (host,
			  stream,
			  fExif.Get (),
			  fShared.Get (),
			  fIFD [0],
			  fTIFFBlockOffset + next_offset,
			  fTIFFBlockOffset,
			  0);
				
	next_offset = fIFD [0]->fNextIFD;
	
	
	
	while (next_offset)
		{
		
		if (next_offset >= stream.Length ())
			{
			
			#if qDNGValidate
			
				{
				
				ReportWarning ("Chained IFD offset past end of stream");

				}
				
			#endif
			
			break;
			
			}
		
		
		
		
		if (!ValidateIFD (stream,
						  fTIFFBlockOffset + next_offset,
						  fTIFFBlockOffset))
			{
			
			#if qDNGValidate
			
				{
				
				ReportWarning ("Chained IFD is not valid");

				}
				
			#endif
			
			break;
			
			}

		if (ChainedIFDCount () == kMaxChainedIFDs)
			{
			
			#if qDNGValidate
			
				{
				
				ReportWarning ("Chained IFD count exceeds DNG SDK parsing limit");

				}
				
			#endif
			
			break;
			
			}
			
		fChainedIFD.push_back (host.Make_dng_ifd ());
		
		fChainedSubIFD.push_back (std::vector <dng_ifd *> ());
			
		ParseIFD (host,
				  stream,
				  fExif.Get (),
				  fShared.Get (),
				  fChainedIFD [ChainedIFDCount () - 1],
				  fTIFFBlockOffset + next_offset,
				  fTIFFBlockOffset,
				  tcFirstChainedIFD + ChainedIFDCount () - 1);
											   
		next_offset = fChainedIFD [ChainedIFDCount () - 1]->fNextIFD;
		
		}
		
	
	
	uint32 searchedIFDs = 0;
	
	bool tooManySubIFDs = false;
	
	while (searchedIFDs < IFDCount () && !tooManySubIFDs)
		{
		
		uint32 searchLimit = IFDCount ();
		
		for (uint32 searchIndex = searchedIFDs;
			 searchIndex < searchLimit && !tooManySubIFDs;
			 searchIndex++)
			{
			
			for (uint32 subIndex = 0;
				 subIndex < fIFD [searchIndex]->fSubIFDsCount;
				 subIndex++)
				{
				
				if (IFDCount () == kMaxSubIFDs + 1)
					{
					
					tooManySubIFDs = true;
					
					break;
					
					}
					
				uint32 subIFDType = fIFD [searchIndex]->fSubIFDsType;
				
				stream.SetReadPosition (fIFD [searchIndex]->fSubIFDsOffset +
										subIndex * TagTypeSize (subIFDType));
				
				uint64 sub_ifd_offset = stream.TagValue_uint64 (subIFDType);
				
				fIFD.push_back (host.Make_dng_ifd ());
				
				ParseIFD (host,
						  stream,
						  fExif.Get (),
						  fShared.Get (),
						  fIFD [IFDCount () - 1],
						  fTIFFBlockOffset + sub_ifd_offset,
						  fTIFFBlockOffset,
						  tcFirstSubIFD + IFDCount () - 2);
				
				}
									
			searchedIFDs = searchLimit;
			
			}
		
		}
		
	#if qDNGValidate

		{
		
		if (tooManySubIFDs)
			{
			
			ReportWarning ("SubIFD count exceeds DNG SDK parsing limit");

			}
		
		}
		
	#endif

	
	

	for (uint32 chainedIndex = 0;
		 chainedIndex < ChainedIFDCount ();
		 chainedIndex++)
		{

		for (uint32 subIndex = 0;
			 subIndex < fChainedIFD [chainedIndex]->fSubIFDsCount;
			 subIndex++)
			{
			
			if (subIndex == kMaxSubIFDs)
				{
				
				#if qDNGValidate

				ReportWarning ("Chained SubIFD count exceeds DNG SDK parsing limit");

				#endif

				break;
				
				}
			
			uint32 subIFDType = fChainedIFD [chainedIndex]->fSubIFDsType;
			
			stream.SetReadPosition (fChainedIFD [chainedIndex]->fSubIFDsOffset +
									subIndex * TagTypeSize (subIFDType));
			
			uint64 sub_ifd_offset = stream.TagValue_uint64 (subIFDType);
			
			fChainedSubIFD [chainedIndex].push_back (host.Make_dng_ifd ());
			
			ParseIFD (host,
					  stream,
					  fExif.Get (),
					  fShared.Get (),
					  fChainedSubIFD [chainedIndex] [subIndex],
					  fTIFFBlockOffset + sub_ifd_offset,
					  fTIFFBlockOffset,
					  tcFirstSubIFD + subIndex);
			
			}

		}
		
	
		
	if (fShared->fExifIFD)
		{
		
		ParseIFD (host,
				  stream,
				  fExif.Get (),
				  fShared.Get (),
				  NULL,
				  fTIFFBlockOffset + fShared->fExifIFD,
				  fTIFFBlockOffset,
				  tcExifIFD);
		
		}

	
		
	if (fShared->fGPSInfo)
		{
		
		ParseIFD (host,
				  stream,
				  fExif.Get (),
				  fShared.Get (),
				  NULL,
				  fTIFFBlockOffset + fShared->fGPSInfo,
				  fTIFFBlockOffset,
				  tcGPSInfo);
		
		}

	
		
	if (fShared->fInteroperabilityIFD)
		{
		
		
		
		
		if (ValidateIFD (stream,
						 fTIFFBlockOffset + fShared->fInteroperabilityIFD,
						 fTIFFBlockOffset))
			{
		
			ParseIFD (host,
					  stream,
					  fExif.Get (),
					  fShared.Get (),
					  NULL,
					  fTIFFBlockOffset + fShared->fInteroperabilityIFD,
					  fTIFFBlockOffset,
					  tcInteroperabilityIFD);
					  
			}
			
		#if qDNGValidate
		
		else
			{
			
			ReportWarning ("The Interoperability IFD is not a valid IFD");
		
			}
			
		#endif
						 
		}

	
		
	if (fShared->fKodakDCRPrivateIFD)
		{
		
		ParseIFD (host,
				  stream,
				  fExif.Get (),
				  fShared.Get (),
				  NULL,
				  fTIFFBlockOffset + fShared->fKodakDCRPrivateIFD,
				  fTIFFBlockOffset,
				  tcKodakDCRPrivateIFD);
		
		}

	
		
	if (fShared->fKodakKDCPrivateIFD)
		{
		
		ParseIFD (host,
				  stream,
				  fExif.Get (),
				  fShared.Get (),
				  NULL,
				  fTIFFBlockOffset + fShared->fKodakKDCPrivateIFD,
				  fTIFFBlockOffset,
				  tcKodakKDCPrivateIFD);
		
		}

	
	
	if (fShared->fMakerNoteCount)
		{
		
		ParseMakerNote (host,
						stream,
						fShared->fMakerNoteCount,
						fShared->fMakerNoteOffset,
						fTIFFBlockOffset,
						0,
						stream.Length ());
		
		}

	
	
	if (fShared->fDNGPrivateDataCount &&
		fShared->fDNGVersion)
		{
		
		ParseDNGPrivateData (host, stream);
				
		}

	#if qDNGValidate
	
	
	
	
	if (fMagic == magicExtendedProfile)
		{
		
		dng_camera_profile_info &profileInfo = fShared->fCameraProfile;
		
		dng_camera_profile profile;
		
		profile.Parse (stream, profileInfo);
		
		if (profileInfo.fColorPlanes < 3 || !profile.IsValid (profileInfo.fColorPlanes))
			{
			
			ReportError ("Invalid camera profile file");
		
			}
			
		}
		
	#endif
		
	}
	

void dng_info::PostParse (dng_host &host)
	{
	
	uint32 index;
	
	fExif->PostParse (host, *fShared.Get ());
	
	fShared->PostParse (host, *fExif.Get ());
	
	for (index = 0; index < IFDCount (); index++)
		{
		
		fIFD [index]->PostParse ();
		
		}
		
	for (index = 0; index < ChainedIFDCount (); index++)
		{
		
		fChainedIFD [index]->PostParse ();
		
		}
		
	for (size_t i = 0; i < fChainedSubIFD.size (); i++)
		{

		std::vector <dng_ifd *> &chain = fChainedSubIFD [i];

		for (size_t j = 0; j < chain.size (); j++)
			{

			if (chain [j])
				{
				chain [j]->PostParse ();
				}

			}
		
		}
		
	if (fShared->fDNGVersion != 0)
		{
	
		
		
		fMainIndex = -1;
		
		for (index = 0; index < IFDCount (); index++)
			{
			
			if (fIFD [index]->fUsesNewSubFileType &&
				fIFD [index]->fNewSubFileType == sfMainImage)
				{
				
				if (fMainIndex == -1)
					{
					
					fMainIndex = index;
					
					}
					
				#if qDNGValidate
					
				else
					{

					ReportError ("Multiple IFDs marked as main image");
					
					}
					
				#endif
						
				}
				
			else if (fIFD [index]->fNewSubFileType == sfPreviewImage ||
					 fIFD [index]->fNewSubFileType == sfAltPreviewImage)
				{
				
				
				
				if (fIFD [index]->fPreviewInfo.fColorSpace == previewColorSpace_MaxEnum)
					{
					
					if (fIFD [index]->fSamplesPerPixel == 1)
						{
						
						fIFD [index]->fPreviewInfo.fColorSpace = previewColorSpace_GrayGamma22;
						
						}
						
					else
						{
						
						fIFD [index]->fPreviewInfo.fColorSpace = previewColorSpace_sRGB;
						
						}
					
					}
					
				}
				
			}
			
		
		
		if (fShared->fDNGVersion < dngVersion_1_1_0_0)
			{
			
			if (fMainIndex != -1)
				{
				
				fIFD [fMainIndex]->fLosslessJPEGBug16 = true;
				
				}
				
			}
			
		
		
		for (index = 0; index < IFDCount (); index++)
			{
			
			if (fIFD [index]->fNewSubFileType == sfTransparencyMask)
				{
				
				if (fMaskIndex == -1)
					{
					
					fMaskIndex = index;
					
					}
					
				#if qDNGValidate
					
				else
					{

					ReportError ("Multiple IFDs marked as transparency mask image");
					
					}
					
				#endif
						
				}
				
			}
   
		
		
		for (index = 0; index < IFDCount (); index++)
			{
			
			if (fIFD [index]->fNewSubFileType == sfDepthMap)
				{
				
				if (fDepthIndex == -1)
					{
					
					fDepthIndex = index;
					
					}
					
				#if qDNGValidate
					
				else
					{

					ReportError ("Multiple IFDs marked as depth map image");
					
					}
					
				#endif
					
				}
				
			}
			
		
		
		for (index = 0; index < IFDCount (); index++)
			{
			
			if (fIFD [index]->fNewSubFileType == sfEnhancedImage)
				{
				
				if (fEnhancedIndex == -1)
					{
					
					fEnhancedIndex = index;
					
					}
					
				#if qDNGValidate
					
				else
					{

					ReportError ("Multiple IFDs marked as enhanced image");
					
					}
					
				#endif
					
				}
				
			}

		

		for (index = 0; index < IFDCount (); index++)
			{
			
			if (fIFD [index]->fNewSubFileType == sfSemanticMask)
				{

				fSemanticMaskIndices.push_back (index);
					
				}
				
			}

		
			
		#if qDNGValidate
					
		if (ChainedIFDCount () > 0)
			{
			
			ReportWarning ("This file has Chained IFDs, which will be ignored by DNG readers");
			
			}
			
		#endif
		
		}
		
	}
	

bool dng_info::IsValidDNG ()
	{
	
	
	
	if (!fShared->IsValidDNG ())
		{
		
		return false;
		
		}
	
	
		
	if (fMagic != magicTIFF && fMagic != magicBigTIFF)
		{
		
		#if qDNGValidate
		
		ReportError ("Invalid TIFF magic number");
					 
		#endif
					 
		return false;
			
		}

	
		
	if (fMainIndex == -1)
		{
		
		#if qDNGValidate
		
		ReportError ("Unable to find main image IFD");
					 
		#endif
					 
		return false;
					 
		}
		
	
	
	for (uint32 index = 0; index < IFDCount (); index++)
		{
		
		uint32 parentCode = (index == 0 ? 0 : tcFirstSubIFD + index - 1);
		
		if (!fIFD [index]->IsValidDNG (*fShared.Get (),
									   parentCode))
			{
			
			
			
			if (index == (uint32) fMainIndex ||
				index == (uint32) fMaskIndex)
				{
				
				return false;
				
				}
	
			
			
			if (index == (uint32) fDepthIndex)
				{
				
				return false;
				
				}
			
			
			
			if (index == (uint32) fEnhancedIndex)
				{
				
				return false;
				
				}

			
			

			if (fIFD [index]->fNewSubFileType == sfSemanticMask)
				{

				return false;

				}
				
			}
		
		}
			
	return true;
	
	}

