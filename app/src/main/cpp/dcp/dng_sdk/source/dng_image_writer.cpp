

#include "dng_image_writer.h"

#include "dng_abort_sniffer.h"
#include "dng_area_task.h"
#include "dng_big_table.h"
#include "dng_bottlenecks.h"
#include "dng_camera_profile.h"
#include "dng_color_space.h"
#include "dng_exceptions.h"
#include "dng_exif.h"
#include "dng_flags.h"
#include "dng_gain_map.h"
#include "dng_globals.h"
#include "dng_host.h"
#include "dng_ifd.h"
#include "dng_image.h"
#include "dng_jpeg_image.h"
#include "dng_jxl.h"
#include "dng_lossless_jpeg.h"
#include "dng_memory.h"
#include "dng_memory_stream.h"
#include "dng_negative.h"
#include "dng_pixel_buffer.h"
#include "dng_preview.h"
#include "dng_read_image.h"
#include "dng_safe_arithmetic.h"
#include "dng_shared.h"
#include "dng_stream.h"
#include "dng_string_list.h"
#include "dng_tag_codes.h"
#include "dng_tag_values.h"
#include "dng_utils.h"

#if qDNGUseXMP
#include "dng_xmp.h"
#endif

#include "zlib.h"

#if qDNGUseLibJPEG
#include "dng_jpeglib.h"
#endif

#include <atomic>
	

dng_resolution::dng_resolution ()

	:	fXResolution ()
	,	fYResolution ()
	
	,	fResolutionUnit (0)
	
	{
	
	}

static void SpoolAdobeData (dng_stream &stream,
							const dng_metadata *metadata,
							const dng_jpeg_preview *preview,
							const dng_memory_block *imageResources)
	{
	
	TempBigEndian tempEndian (stream);
	
	#if qDNGUseXMP

	if (metadata && metadata->GetXMP ())
		{
		
		bool marked = false;
		
		if (metadata->GetXMP ()->GetBoolean (XMP_NS_XAP_RIGHTS,
											 "Marked",
											 marked))
			{
			
			stream.Put_uint32 (DNG_CHAR4 ('8','B','I','M'));
			stream.Put_uint16 (1034);
			stream.Put_uint16 (0);
			
			stream.Put_uint32 (1);
			
			stream.Put_uint8 (marked ? 1 : 0);
			
			stream.Put_uint8 (0);
			
			}
			
		dng_string webStatement;
		
		if (metadata->GetXMP ()->GetString (XMP_NS_XAP_RIGHTS,
											"WebStatement",
											webStatement))
			{
			
			dng_memory_data buffer;
			
			uint32 size = webStatement.Get_SystemEncoding (buffer);
			
			if (size > 0)
				{
				
				stream.Put_uint32 (DNG_CHAR4 ('8','B','I','M'));
				stream.Put_uint16 (1035);
				stream.Put_uint16 (0);
				
				stream.Put_uint32 (size);
				
				stream.Put (buffer.Buffer (), size);
				
				if (size & 1)
					stream.Put_uint8 (0);
					
				}
			
			}
		
		}
	
	#endif	
	
	if (preview)
		{
		
		preview->SpoolAdobeThumbnail (stream);
				
		}
		
	if (metadata && metadata->IPTCLength ())
		{
		
		dng_fingerprint iptcDigest = metadata->IPTCDigest ();

		if (iptcDigest.IsValid ())
			{
			
			stream.Put_uint32 (DNG_CHAR4 ('8','B','I','M'));
			stream.Put_uint16 (1061);
			stream.Put_uint16 (0);
			
			stream.Put_uint32 (16);
			
			stream.Put (iptcDigest);
			
			}
			
		}
		
	if (imageResources)
		{
		
		uint32 size = imageResources->LogicalSize ();
		
		stream.Put (imageResources->Buffer (), size);
		
		if (size & 1)
			stream.Put_uint8 (0);
		
		}
		
	}

static dng_memory_block * BuildAdobeData (dng_host &host,
										  const dng_metadata *metadata,
										  const dng_jpeg_preview *preview,
										  const dng_memory_block *imageResources)
	{
	
	dng_memory_stream stream (host.Allocator ());
	
	SpoolAdobeData (stream,
					metadata,
					preview,
					imageResources);
					
	return stream.AsMemoryBlock (host.Allocator ());
	
	}

tag_string::tag_string (uint16 code,
						const dng_string &s,
						bool forceASCII)
						
	:	tiff_tag (code, ttAscii, 0)
	
	,	fString (s)
	
	{
			
	if (forceASCII)
		{
		
		
		
		
		
		
		
		
		
		}
		
	else if (!fString.IsASCII ())
		{
		
		fType = ttByte;
		
		}
		
	fCount = fString.Length () + 1;
	
	}

void tag_string::Put (dng_stream &stream) const
	{
	
	stream.Put (fString.Get (), Size ());
	
	}

tag_encoded_text::tag_encoded_text (uint16 code,
									const dng_string &text)
	
	:	tiff_tag (code, ttUndefined, 0)
	
	,	fText (text)
	
	,	fUTF16 ()
	
	{
			
	if (fText.IsASCII ())
		{
	
		fCount = SafeUint32Add (8, fText.Length ());
		
		}
		
	else
		{
		
		fCount = SafeUint32Add (8,
					SafeUint32Mult (fText.Get_UTF16 (fUTF16), 2));
		
		}
	
	}

void tag_encoded_text::Put (dng_stream &stream) const
	{
	
	if (fUTF16.Buffer ())
		{
		
		stream.Put ("UNICODE\000", 8);
		
		uint32 chars = (fCount - 8) >> 1;
		
		const uint16 *buf = fUTF16.Buffer_uint16 ();
		
		for (uint32 j = 0; j < chars; j++)
			{
			
			stream.Put_uint16 (buf [j]);
			
			}
		
		}
		
	else
		{
		
		stream.Put ("ASCII\000\000\000", 8);
		
		stream.Put (fText.Get (), fCount - 8);
		
		}
		
	}
	

void tag_data_ptr::Put (dng_stream &stream) const
	{
	
	
	
	
	if (stream.SwapBytes ())
		{
	
		switch (Type ())
			{
			
			
			
			case ttShort:
			case ttSShort:
			case ttUnicode:
				{
				
				const uint16 *p = (const uint16 *) fData;
				
				uint32 entries = (Size () >> 1);
				
				for (uint32 j = 0; j < entries; j++)
					{
					
					stream.Put_uint16 (p [j]);
					
					}
				
				return;
				
				}
			
			
			
			case ttLong:
			case ttSLong:
			case ttRational:
			case ttSRational:
			case ttIFD:
			case ttFloat:
			case ttComplex:
				{
				
				const uint32 *p = (const uint32 *) fData;
				
				uint32 entries = (Size () >> 2);
				
				for (uint32 j = 0; j < entries; j++)
					{
					
					stream.Put_uint32 (p [j]);
					
					}
				
				return;
				
				}
				
			
			
			case ttDouble:
			case ttLong8:
			case ttSLong8:
			case ttIFD8:
				{
				
				const real64 *p = (const real64 *) fData;
				
				uint32 entries = (Size () >> 3);
				
				for (uint32 j = 0; j < entries; j++)
					{
					
					stream.Put_real64 (p [j]);
					
					}
				
				return;
				
				}
			
			
			
				
			default:
				{
				
				break;
				
				}

			}
			
		}
		
	
		
	stream.Put (fData, Size ());
				
	}

void tag_big_uint::Put (dng_stream &stream) const
	{
	
	if (fType == ttLong)
		{
		
		if (fValue > 0xFFFFFFFF)
			{
			ThrowProgramError ("tag_big_uint overflow");
			}
			
		stream.Put_uint32 ((uint32) fValue);
		
		}
		
	else
		{
		stream.Put_uint64 (fValue);
		}
	
	}

void tag_big_uints::Put (dng_stream &stream) const
	{
	
	const uint64 *buffer = fData.Buffer_uint64 ();
	
	for (uint32 index = 0; index < fCount; index++)
		{
		
		uint64 x = buffer [index];
		
		if (fType == ttLong)
			{
			
			if (x > 0xFFFFFFFF)
				{
				ThrowProgramError ("tag_big_uints overflow");
				}
				
			stream.Put_uint32 ((uint32) x);
			
			}
			
		else
			{
			stream.Put_uint64 (x);
			}
			
		}
	
	}

tag_matrix::tag_matrix (uint16 code,
						const dng_matrix &m)
				   
	:	tag_srational_ptr (code, fEntry, m.Rows () * m.Cols ())
	
	{
	
	uint32 index = 0;
	
	for (uint32 r = 0; r < m.Rows (); r++)
		for (uint32 c = 0; c < m.Cols (); c++)
			{
			
			fEntry [index].Set_real64 (m [r] [c], 10000);
			
			index++;
			
			}
	
	}

tag_icc_profile::tag_icc_profile (const void *profileData,
								  uint32 profileSize)
			
	:	tag_data_ptr (tcICCProfile, 
					  ttUndefined,
					  0,
					  NULL)
	
	{
	
	if (profileData && profileSize)
		{
		
		SetCount (profileSize);
		SetData	 (profileData);
		
		}
	
	}
			

void tag_cfa_pattern::Put (dng_stream &stream) const
	{
	
	stream.Put_uint16 ((uint16) fCols);
	stream.Put_uint16 ((uint16) fRows);
	
	for (uint32 col = 0; col < fCols; col++)
		for (uint32 row = 0; row < fRows; row++)
			{
			
			stream.Put_uint8 (fPattern [row * kMaxCFAPattern + col]);
			
			}

	}

tag_exif_date_time::tag_exif_date_time (uint16 code,
										const dng_date_time &dt)
				   
	:	tag_data_ptr (code, ttAscii, 20, fData)
	
	{
	
	if (dt.IsValid ())
		{
	
		snprintf (fData,
				  20,
				  "%04d:%02d:%02d %02d:%02d:%02d",
				  (int) dt.fYear,
				  (int) dt.fMonth,
				  (int) dt.fDay,
				  (int) dt.fHour,
				  (int) dt.fMinute,
				  (int) dt.fSecond);
				 
		}
	
	}

tag_iptc::tag_iptc (const void *data,
					uint32 length)
	
	:	tiff_tag (tcIPTC_NAA, ttLong, (length + 3) >> 2)
	
	,	fData	(data  )
	,	fLength (length)
	
	{
	
	}

void tag_iptc::Put (dng_stream &stream) const
	{
	
	
	
	
	
	
	stream.Put (fData, fLength);
	
	
	
	uint32 extra = fCount * 4 - fLength;
	
	while (extra--)
		{
		stream.Put_uint8 (0);
		}

	}

tag_xmp::tag_xmp (const dng_xmp *xmp)
	
	:	tag_uint8_ptr (tcXMP, NULL, 0)
	
	,	fBuffer ()
	
	{
	
	#if qDNGUseXMP
	
	if (xmp)
		{
		
		fBuffer.Reset (xmp->Serialize (true));
		
		if (fBuffer.Get ())
			{
			
			SetData (fBuffer->Buffer_uint8 ());
			
			SetCount (fBuffer->LogicalSize ());
			
			}
		
		}
	
	#endif	
	
	}

void dng_tiff_directory::Add (tiff_tag *tag)
	{
	
	
	
	for (size_t j = 0; j < fTag.size (); j++)
		{
		
		if (tag->Code () < fTag [j]->Code ())
			{
			fTag.insert (fTag.begin () + j, tag);
			return;
			}
		
		}
		
	fTag.push_back (tag);
			
	}
			

void dng_tiff_directory::SetBigTIFF (bool isBigTIFF)
	{
	
	fBigTIFF = isBigTIFF;
	
	for (size_t index = 0; index < fTag.size (); index++)
		{
		
		fTag [index]->SetBigTIFF (isBigTIFF);
		
		}
	
	}
	

uint32 dng_tiff_directory::Size () const
	{
	
	if (!fTag.size ()) return 0;
	
	uint32 size = fBigTIFF ? (uint32) fTag.size () * 20 + 16
						   : (uint32) fTag.size () * 12 +  6;
	
	for (size_t index = 0; index < fTag.size (); index++)
		{
		
		uint32 tagSize = fTag [index]->Size ();
		
		if (tagSize > (uint32)(fBigTIFF ? 8 : 4))
			{
			
			size += (tagSize + 1) & ~1;
			
			}
		
		}
		
	return size;
	
	}
		

void dng_tiff_directory::Put (dng_stream &stream,
							  OffsetsBase offsetsBase,
							  uint64 explicitBase) const
	{
	
	if (!fTag.size ()) return;
	
	uint64 bigData = fBigTIFF ? fTag.size () * 20 + 16
							  : fTag.size () * 12 +	 6;
	
	if (offsetsBase == offsetsRelativeToStream)
		bigData += stream.Position ();

	else if (offsetsBase == offsetsRelativeToExplicitBase)
		bigData += explicitBase;

	if (fBigTIFF)
		stream.Put_uint64 ((uint64) fTag.size ());
	else
		stream.Put_uint16 ((uint16) fTag.size ());
	
	for (size_t index = 0; index < fTag.size (); index++)
		{
		
		const tiff_tag &tag = *fTag [index];
		
		stream.Put_uint16 (tag.Code	 ());
		stream.Put_uint16 (tag.Type	 ());
		
		if (fBigTIFF)
			stream.Put_uint64 ((uint64) tag.Count ());
		else
			stream.Put_uint32 ((uint32) tag.Count ());
		
		uint32 size = tag.Size ();
		
		if (size <= (uint32)(fBigTIFF ? 8 : 4))
			{
			
			tag.Put (stream);
			
			while (size < (uint32)(fBigTIFF ? 8 : 4))
				{
				stream.Put_uint8 (0);
				size++;
				}
				
			}
			
		else
			{
			
			if (fBigTIFF)
				stream.Put_uint64 (bigData);
			else
				stream.Put_uint32 ((uint32) bigData);
			
			bigData += (size + 1) & ~1;
						
			}
		
		}
		
	
		
	if (fBigTIFF)
		stream.Put_uint64 (fChained);
	else
		stream.Put_uint32 ((uint32) fChained);
	
	for (size_t index = 0; index < fTag.size (); index++)
		{
		
		const tiff_tag &tag = *fTag [index];
		
		uint32 size = tag.Size ();
		
		if (size > (uint32)(fBigTIFF ? 8 : 4))
			{
			
			tag.Put (stream);
			
			if (size & 1)
				stream.Put_uint8 (0);
			
			}
		
		}
	
	}

dng_basic_tag_set::dng_basic_tag_set (dng_tiff_directory &directory,
									  const dng_ifd &info)
						  
	:	fNewSubFileType (tcNewSubFileType, info.fNewSubFileType)
	
	,	fImageWidth	 (tcImageWidth , info.fImageWidth )
	,	fImageLength (tcImageLength, info.fImageLength)
	
	,	fPhotoInterpretation (tcPhotometricInterpretation,
							  (uint16) info.fPhotometricInterpretation)
	
	,	fFillOrder (tcFillOrder, 1)
	
	,	fSamplesPerPixel (tcSamplesPerPixel, (uint16) info.fSamplesPerPixel)
	
	,	fBitsPerSampleData (info.fSamplesPerPixel)
	
	,	fBitsPerSample (tcBitsPerSample,
						fBitsPerSampleData.data (),
						info.fSamplesPerPixel)
						
	,	fStrips (info.fUsesStrips)
						
	,	fTileWidth (tcTileWidth, info.fTileWidth)
	
	,	fTileLength (fStrips ? tcRowsPerStrip : tcTileLength, 
					 info.fTileLength)
	
	,	fTileOffsets (fStrips ? tcStripOffsets : tcTileOffsets,
					  info.TilesPerImage ())
					   
	,	fTileByteCounts (fStrips ? tcStripByteCounts : tcTileByteCounts,
						 info.TilesPerImage (),
						 info.fCompression != ccUncompressed)
						  
	,	fPlanarConfiguration (tcPlanarConfiguration, pcInterleaved)
	
	,	fCompression (tcCompression, (uint16) info.fCompression)
	,	fPredictor	 (tcPredictor  , (uint16) info.fPredictor  )
	
	,	fExtraSamplesData (info.fExtraSamplesCount)
	
	,	fExtraSamples (tcExtraSamples,
					   fExtraSamplesData.data (),
					   info.fExtraSamplesCount)
					   
	,	fSampleFormatData (info.fSamplesPerPixel)
					   
	,	fSampleFormat (tcSampleFormat,
					   fSampleFormatData.data (),
					   info.fSamplesPerPixel)
					   
	,	fRowInterleaveFactor (tcRowInterleaveFactor,
							  (uint16) info.fRowInterleaveFactor)

	,	fColumnInterleaveFactor (tcColumnInterleaveFactor,
								 (uint16) info.fColumnInterleaveFactor)

	,	fSubTileBlockSize (tcSubTileBlockSize,
						   fSubTileBlockSizeData,
						   2)
						   
	,	fJXLDistance    (tcJXLDistance)
	,	fJXLEffort      (tcJXLEffort)
	,	fJXLDecodeSpeed (tcJXLDecodeSpeed)
								 
	{
	
	uint32 j;
	
	for (j = 0; j < info.fSamplesPerPixel; j++)
		{
	
		fBitsPerSampleData [j] = (uint16) info.fBitsPerSample [0];
		
		}
	
	directory.Add (&fNewSubFileType);
	
	directory.Add (&fImageWidth);
	directory.Add (&fImageLength);
	
	directory.Add (&fPhotoInterpretation);
	
	directory.Add (&fSamplesPerPixel);
	
	directory.Add (&fBitsPerSample);
	
	if (info.fBitsPerSample [0] !=	8 &&
		info.fBitsPerSample [0] != 16 &&
		info.fBitsPerSample [0] != 32)
		{
	
		directory.Add (&fFillOrder);
		
		}
		
	if (!fStrips)
		{
		
		directory.Add (&fTileWidth);
	
		}

	directory.Add (&fTileLength);
	
	directory.Add (&fTileOffsets);
	directory.Add (&fTileByteCounts);
	
	directory.Add (&fPlanarConfiguration);
	
	directory.Add (&fCompression);
	
	if (info.fPredictor != cpNullPredictor)
		{
		
		directory.Add (&fPredictor);
		
		}
		
	if (info.fExtraSamplesCount != 0)
		{
		
		for (j = 0; j < info.fExtraSamplesCount; j++)
			{
			fExtraSamplesData [j] = (uint16) info.fExtraSamples [j];
			}
			
		directory.Add (&fExtraSamples);
		
		}
		
	if (info.fSampleFormat [0] != sfUnsignedInteger)
		{
		
		for (j = 0; j < info.fSamplesPerPixel; j++)
			{
			fSampleFormatData [j] = (uint16) info.fSampleFormat [j];
			}
			
		directory.Add (&fSampleFormat);
		
		}
		
	if (info.fRowInterleaveFactor != 1)
		{
		
		directory.Add (&fRowInterleaveFactor);
		
		}

	if (info.fColumnInterleaveFactor != 1)
		{
		
		directory.Add (&fColumnInterleaveFactor);
		
		}

	if (info.fSubTileBlockRows != 1 ||
		info.fSubTileBlockCols != 1)
		{
		
		fSubTileBlockSizeData [0] = (uint16) info.fSubTileBlockRows;
		fSubTileBlockSizeData [1] = (uint16) info.fSubTileBlockCols;
		
		directory.Add (&fSubTileBlockSize);
		
		}
		
	if (info.fCompression == ccJXL)
		{
		
		if (info.fJXLDistance >= 0.0f)
			{
			
			fJXLDistance.Set (info.fJXLDistance);
			
			directory.Add (&fJXLDistance);
			
			}
			
		if (info.fJXLEffort >= 1)
			{
			
			fJXLEffort.Set (info.fJXLEffort);
			
			directory.Add (&fJXLEffort);
			
			}
		
		if (info.fJXLDecodeSpeed >= 1)
			{
			
			fJXLDecodeSpeed.Set (info.fJXLDecodeSpeed);
			
			directory.Add (&fJXLDecodeSpeed);
			
			}
		
		}

	}

exif_tag_set::exif_tag_set (dng_tiff_directory &directory,
							const dng_exif &exif,
							bool makerNoteSafe,
							const void *makerNoteData,
							uint32 makerNoteLength,
							bool insideDNG)

	:	fExifIFD ()
	,	fGPSIFD	 ()
	
	,	fExifLink (tcExifIFD, 0)
	,	fGPSLink  (tcGPSInfo, 0)
	
	,	fAddedExifLink (false)
	,	fAddedGPSLink  (false)
	
	,	fExifVersion (tcExifVersion, ttUndefined, 4, fExifVersionData)
	
	,	fExposureTime	   (tcExposureTime	   , exif.fExposureTime		)
	,	fShutterSpeedValue (tcShutterSpeedValue, exif.fShutterSpeedValue)
	
	,	fFNumber	   (tcFNumber	   , exif.fFNumber		)
	,	fApertureValue (tcApertureValue, exif.fApertureValue)
	
	,	fBrightnessValue (tcBrightnessValue, exif.fBrightnessValue)
	
	,	fExposureBiasValue (tcExposureBiasValue, exif.fExposureBiasValue)
	
	,	fMaxApertureValue (tcMaxApertureValue , exif.fMaxApertureValue)
	
	,	fSubjectDistance (tcSubjectDistance, exif.fSubjectDistance)
	
	,	fFocalLength (tcFocalLength, exif.fFocalLength)
	
	
	
	
	

	,	fISOSpeedRatings (tcISOSpeedRatings, 
						  (uint16) Min_uint32 (65535, 
											   exif.fISOSpeedRatings [0]))

	,	fSensitivityType (tcSensitivityType, (uint16) exif.fSensitivityType)

	,	fStandardOutputSensitivity (tcStandardOutputSensitivity, exif.fStandardOutputSensitivity)
	
	,	fRecommendedExposureIndex (tcRecommendedExposureIndex, exif.fRecommendedExposureIndex)

	,	fISOSpeed (tcISOSpeed, exif.fISOSpeed)

	,	fISOSpeedLatitudeyyy (tcISOSpeedLatitudeyyy, exif.fISOSpeedLatitudeyyy)

	,	fISOSpeedLatitudezzz (tcISOSpeedLatitudezzz, exif.fISOSpeedLatitudezzz)

	,	fFlash (tcFlash, (uint16) exif.fFlash)
	
	,	fExposureProgram (tcExposureProgram, (uint16) exif.fExposureProgram)
	
	,	fMeteringMode (tcMeteringMode, (uint16) exif.fMeteringMode)
	
	,	fLightSource (tcLightSource, (uint16) exif.fLightSource)
	
	,	fSensingMethod (tcSensingMethodExif, (uint16) exif.fSensingMethod)
	
	,	fFocalLength35mm (tcFocalLengthIn35mmFilm, (uint16) exif.fFocalLengthIn35mmFilm)
	
	,	fFileSourceData ((uint8) exif.fFileSource)
	,	fFileSource		(tcFileSource, ttUndefined, 1, &fFileSourceData)

	,	fSceneTypeData ((uint8) exif.fSceneType)
	,	fSceneType	   (tcSceneType, ttUndefined, 1, &fSceneTypeData)
	
	,	fCFAPattern (tcCFAPatternExif,
					 exif.fCFARepeatPatternRows,
					 exif.fCFARepeatPatternCols,
					 &exif.fCFAPattern [0] [0])
	
	,	fCustomRendered		  (tcCustomRendered		 , (uint16) exif.fCustomRendered	  )
	,	fExposureMode		  (tcExposureMode		 , (uint16) exif.fExposureMode		  )
	,	fWhiteBalance		  (tcWhiteBalance		 , (uint16) exif.fWhiteBalance		  )
	,	fSceneCaptureType	  (tcSceneCaptureType	 , (uint16) exif.fSceneCaptureType	  )
	,	fGainControl		  (tcGainControl		 , (uint16) exif.fGainControl		  )
	,	fContrast			  (tcContrast			 , (uint16) exif.fContrast			  )
	,	fSaturation			  (tcSaturation			 , (uint16) exif.fSaturation		  )
	,	fSharpness			  (tcSharpness			 , (uint16) exif.fSharpness			  )
	,	fSubjectDistanceRange (tcSubjectDistanceRange, (uint16) exif.fSubjectDistanceRange)
		
	,	fDigitalZoomRatio (tcDigitalZoomRatio, exif.fDigitalZoomRatio)
	
	,	fExposureIndex (tcExposureIndexExif, exif.fExposureIndex)
	
	,	fImageNumber (tcImageNumber, exif.fImageNumber)
	
	,	fSelfTimerMode (tcSelfTimerMode, (uint16) exif.fSelfTimerMode)
	
	,	fBatteryLevelA (tcBatteryLevel, exif.fBatteryLevelA)
	,	fBatteryLevelR (tcBatteryLevel, exif.fBatteryLevelR)

	,	fColorSpace (tcColorSpace, (uint16) exif.fColorSpace)
	
	,	fFocalPlaneXResolution (tcFocalPlaneXResolutionExif, exif.fFocalPlaneXResolution)
	,	fFocalPlaneYResolution (tcFocalPlaneYResolutionExif, exif.fFocalPlaneYResolution)
	
	,	fFocalPlaneResolutionUnit (tcFocalPlaneResolutionUnitExif, (uint16) exif.fFocalPlaneResolutionUnit)
	
	,	fSubjectArea (tcSubjectArea, fSubjectAreaData, exif.fSubjectAreaCount)

	,	fLensInfo (tcLensInfo, fLensInfoData, 4)
	
	,	fDateTime		   (tcDateTime		   , exif.fDateTime			.DateTime ())
	,	fDateTimeOriginal  (tcDateTimeOriginal , exif.fDateTimeOriginal .DateTime ())
	,	fDateTimeDigitized (tcDateTimeDigitized, exif.fDateTimeDigitized.DateTime ())
	
	,	fSubsecTime			 (tcSubsecTime,			 exif.fDateTime			.Subseconds ())
	,	fSubsecTimeOriginal	 (tcSubsecTimeOriginal,	 exif.fDateTimeOriginal .Subseconds ())
	,	fSubsecTimeDigitized (tcSubsecTimeDigitized, exif.fDateTimeDigitized.Subseconds ())

	,	fOffsetTime			 (tcOffsetTime,			 exif.fDateTime			.OffsetTime ())
	,	fOffsetTimeOriginal	 (tcOffsetTimeOriginal,	 exif.fDateTimeOriginal .OffsetTime ())
	,	fOffsetTimeDigitized (tcOffsetTimeDigitized, exif.fDateTimeDigitized.OffsetTime ())

	,	fMake (tcMake, exif.fMake)

	,	fModel (tcModel, exif.fModel)
	
	,	fArtist (tcArtist, exif.fArtist)
	
	,	fSoftware (tcSoftware, exif.fSoftware)
	
	,	fCopyright (tcCopyright, exif.fCopyright)
	
	,	fImageDescription (tcImageDescription, exif.fImageDescription)
	
	,	fSerialNumber (tcCameraSerialNumber, exif.fCameraSerialNumber)

	,	fMakerNoteSafety (tcMakerNoteSafety, makerNoteSafe ? 1 : 0)

	,	fMakerNote (tcMakerNote, ttUndefined, makerNoteLength, makerNoteData)

	,	fUserComment (tcUserComment, exif.fUserComment)
	
	,	fImageUniqueID (tcImageUniqueID, ttAscii, 33, fImageUniqueIDData)

	

	,	fCameraOwnerName   (tcCameraOwnerNameExif,	  exif.fOwnerName		  )
	,	fBodySerialNumber  (tcCameraSerialNumberExif, exif.fCameraSerialNumber)
	,	fLensSpecification (tcLensSpecificationExif,  fLensInfoData, 4		  )
	,	fLensMake		   (tcLensMakeExif,			  exif.fLensMake		  )
	,	fLensModel		   (tcLensModelExif,		  exif.fLensName		  )
	,	fLensSerialNumber  (tcLensSerialNumberExif,	  exif.fLensSerialNumber  )

	

	,	fTemperature		  (tcTemperature,		   exif.fTemperature		 )
	,	fHumidity			  (tcHumidity,			   exif.fHumidity			 )
	,	fPressure			  (tcPressure,			   exif.fPressure			 )
	,	fWaterDepth			  (tcWaterDepth,		   exif.fWaterDepth			 )
	,	fAcceleration		  (tcAcceleration,		   exif.fAcceleration		 )
	,	fCameraElevationAngle (tcCameraElevationAngle, exif.fCameraElevationAngle)

	,	fGPSVersionID (tcGPSVersionID, fGPSVersionData, 4)
	
	,	fGPSLatitudeRef (tcGPSLatitudeRef, exif.fGPSLatitudeRef)
	,	fGPSLatitude	(tcGPSLatitude,	   exif.fGPSLatitude, 3)
	
	,	fGPSLongitudeRef (tcGPSLongitudeRef, exif.fGPSLongitudeRef)
	,	fGPSLongitude	 (tcGPSLongitude,	 exif.fGPSLongitude, 3)
	
	,	fGPSAltitudeRef (tcGPSAltitudeRef, (uint8) exif.fGPSAltitudeRef)
	,	fGPSAltitude	(tcGPSAltitude,			   exif.fGPSAltitude   )
	
	,	fGPSTimeStamp (tcGPSTimeStamp, exif.fGPSTimeStamp, 3)
		
	,	fGPSSatellites	(tcGPSSatellites , exif.fGPSSatellites )
	,	fGPSStatus		(tcGPSStatus	 , exif.fGPSStatus	   )
	,	fGPSMeasureMode (tcGPSMeasureMode, exif.fGPSMeasureMode)
	
	,	fGPSDOP (tcGPSDOP, exif.fGPSDOP)
		
	,	fGPSSpeedRef (tcGPSSpeedRef, exif.fGPSSpeedRef)
	,	fGPSSpeed	 (tcGPSSpeed   , exif.fGPSSpeed	  )
		
	,	fGPSTrackRef (tcGPSTrackRef, exif.fGPSTrackRef)
	,	fGPSTrack	 (tcGPSTrack   , exif.fGPSTrack	  )
		
	,	fGPSImgDirectionRef (tcGPSImgDirectionRef, exif.fGPSImgDirectionRef)
	,	fGPSImgDirection	(tcGPSImgDirection	 , exif.fGPSImgDirection   )
	
	,	fGPSMapDatum (tcGPSMapDatum, exif.fGPSMapDatum)
		
	,	fGPSDestLatitudeRef (tcGPSDestLatitudeRef, exif.fGPSDestLatitudeRef)
	,	fGPSDestLatitude	(tcGPSDestLatitude,	   exif.fGPSDestLatitude, 3)
	
	,	fGPSDestLongitudeRef (tcGPSDestLongitudeRef, exif.fGPSDestLongitudeRef)
	,	fGPSDestLongitude	 (tcGPSDestLongitude,	 exif.fGPSDestLongitude, 3)
	
	,	fGPSDestBearingRef (tcGPSDestBearingRef, exif.fGPSDestBearingRef)
	,	fGPSDestBearing	   (tcGPSDestBearing   , exif.fGPSDestBearing	)
		
	,	fGPSDestDistanceRef (tcGPSDestDistanceRef, exif.fGPSDestDistanceRef)
	,	fGPSDestDistance	(tcGPSDestDistance	 , exif.fGPSDestDistance   )
		
	,	fGPSProcessingMethod (tcGPSProcessingMethod, exif.fGPSProcessingMethod)
	,	fGPSAreaInformation	 (tcGPSAreaInformation , exif.fGPSAreaInformation )
	
	,	fGPSDateStamp (tcGPSDateStamp, exif.fGPSDateStamp)
	
	,	fGPSDifferential (tcGPSDifferential, (uint16) exif.fGPSDifferential)
		
	,	fGPSHPositioningError (tcGPSHPositioningError, exif.fGPSHPositioningError)
		
	{
	
	if (exif.fExifVersion)
		{
		
		fExifVersionData [0] = (uint8) (exif.fExifVersion >> 24);
		fExifVersionData [1] = (uint8) (exif.fExifVersion >> 16);
		fExifVersionData [2] = (uint8) (exif.fExifVersion >>  8);
		fExifVersionData [3] = (uint8) (exif.fExifVersion	   );
		
		fExifIFD.Add (&fExifVersion);

		}
	
	if (exif.fExposureTime.IsValid ())
		{
		fExifIFD.Add (&fExposureTime);
		}
		
	if (exif.fShutterSpeedValue.IsValid ())
		{
		fExifIFD.Add (&fShutterSpeedValue);
		}
		
	if (exif.fFNumber.IsValid ())
		{
		fExifIFD.Add (&fFNumber);
		}
		
	if (exif.fApertureValue.IsValid ())
		{
		fExifIFD.Add (&fApertureValue);
		}
		
	if (exif.fBrightnessValue.IsValid ())
		{
		fExifIFD.Add (&fBrightnessValue);
		}
		
	if (exif.fExposureBiasValue.IsValid ())
		{
		fExifIFD.Add (&fExposureBiasValue);
		}
		
	if (exif.fMaxApertureValue.IsValid ())
		{
		fExifIFD.Add (&fMaxApertureValue);
		}
		
	if (exif.fSubjectDistance.IsValid ())
		{
		fExifIFD.Add (&fSubjectDistance);
		}
		
	if (exif.fFocalLength.IsValid ())
		{
		fExifIFD.Add (&fFocalLength);
		}
	
	if (exif.fISOSpeedRatings [0] != 0)
		{
		fExifIFD.Add (&fISOSpeedRatings);
		}
		
	if (exif.fFlash <= 0x0FFFF)
		{
		fExifIFD.Add (&fFlash);
		}
		
	if (exif.fExposureProgram <= 0x0FFFF)
		{
		fExifIFD.Add (&fExposureProgram);
		}
		
	if (exif.fMeteringMode <= 0x0FFFF)
		{
		fExifIFD.Add (&fMeteringMode);
		}
		
	if (exif.fLightSource <= 0x0FFFF)
		{
		fExifIFD.Add (&fLightSource);
		}
		
	if (exif.fSensingMethod <= 0x0FFFF)
		{
		fExifIFD.Add (&fSensingMethod);
		}
		
	if (exif.fFocalLengthIn35mmFilm != 0)
		{
		fExifIFD.Add (&fFocalLength35mm);
		}
		
	if (exif.fFileSource <= 0x0FF)
		{
		fExifIFD.Add (&fFileSource);
		}
		
	if (exif.fSceneType <= 0x0FF)
		{
		fExifIFD.Add (&fSceneType);
		}
		
	if (exif.fCFARepeatPatternRows &&
		exif.fCFARepeatPatternCols)
		{
		fExifIFD.Add (&fCFAPattern);
		}
		
	if (exif.fCustomRendered <= 0x0FFFF)
		{
		fExifIFD.Add (&fCustomRendered);
		}
		
	if (exif.fExposureMode <= 0x0FFFF)
		{
		fExifIFD.Add (&fExposureMode);
		}
		
	if (exif.fWhiteBalance <= 0x0FFFF)
		{
		fExifIFD.Add (&fWhiteBalance);
		}
		
	if (exif.fSceneCaptureType <= 0x0FFFF)
		{
		fExifIFD.Add (&fSceneCaptureType);
		}
		
	if (exif.fGainControl <= 0x0FFFF)
		{
		fExifIFD.Add (&fGainControl);
		}
		
	if (exif.fContrast <= 0x0FFFF)
		{
		fExifIFD.Add (&fContrast);
		}
		
	if (exif.fSaturation <= 0x0FFFF)
		{
		fExifIFD.Add (&fSaturation);
		}
		
	if (exif.fSharpness <= 0x0FFFF)
		{
		fExifIFD.Add (&fSharpness);
		}
		
	if (exif.fSubjectDistanceRange <= 0x0FFFF)
		{
		fExifIFD.Add (&fSubjectDistanceRange);
		}
		
	if (exif.fDigitalZoomRatio.IsValid ())
		{
		fExifIFD.Add (&fDigitalZoomRatio);
		}
		
	if (exif.fExposureIndex.IsValid ())
		{
		fExifIFD.Add (&fExposureIndex);
		}
		
	if (insideDNG)	
		{
		
		if (exif.fImageNumber != 0xFFFFFFFF)
			{
			directory.Add (&fImageNumber);
			}
			
		if (exif.fSelfTimerMode <= 0x0FFFF)
			{
			directory.Add (&fSelfTimerMode);
			}
			
		if (exif.fBatteryLevelA.NotEmpty ())
			{
			directory.Add (&fBatteryLevelA);
			}
			
		else if (exif.fBatteryLevelR.IsValid ())
			{
			directory.Add (&fBatteryLevelR);
			}
		
		}
		
	if (exif.fColorSpace == 1 ||
		exif.fColorSpace == 0xFFFF)
		{
		fExifIFD.Add (&fColorSpace);
		}
	
	if (exif.fFocalPlaneXResolution.IsValid ())
		{
		fExifIFD.Add (&fFocalPlaneXResolution);
		}
	
	if (exif.fFocalPlaneYResolution.IsValid ())
		{
		fExifIFD.Add (&fFocalPlaneYResolution);
		}
	
	if (exif.fFocalPlaneResolutionUnit <= 0x0FFFF)
		{
		fExifIFD.Add (&fFocalPlaneResolutionUnit);
		}
		
	if (exif.fSubjectAreaCount)
		{
		
		fSubjectAreaData [0] = (uint16) exif.fSubjectArea [0];
		fSubjectAreaData [1] = (uint16) exif.fSubjectArea [1];
		fSubjectAreaData [2] = (uint16) exif.fSubjectArea [2];
		fSubjectAreaData [3] = (uint16) exif.fSubjectArea [3];
		
		fExifIFD.Add (&fSubjectArea);
		
		}
	
	if (exif.fLensInfo [0].IsValid () &&
		exif.fLensInfo [1].IsValid ())
		{
		
		fLensInfoData [0] = exif.fLensInfo [0];
		fLensInfoData [1] = exif.fLensInfo [1];
		fLensInfoData [2] = exif.fLensInfo [2];
		fLensInfoData [3] = exif.fLensInfo [3];

		if (insideDNG)
			{
			directory.Add (&fLensInfo);
			}
		
		}
		
	if (exif.fDateTime.IsValid ())
		{
		
		directory.Add (&fDateTime);
		
		if (exif.fDateTime.Subseconds ().NotEmpty ())
			{
			fExifIFD.Add (&fSubsecTime);
			}
		
		}
		
	if (exif.fDateTimeOriginal.IsValid ())
		{
		
		fExifIFD.Add (&fDateTimeOriginal);
		
		if (exif.fDateTimeOriginal.Subseconds ().NotEmpty ())
			{
			fExifIFD.Add (&fSubsecTimeOriginal);
			}
		
		}
		
	if (exif.fDateTimeDigitized.IsValid ())
		{
		
		fExifIFD.Add (&fDateTimeDigitized);
		
		if (exif.fDateTimeDigitized.Subseconds ().NotEmpty ())
			{
			fExifIFD.Add (&fSubsecTimeDigitized);
			}
		
		}
		
	if (exif.fMake.NotEmpty ())
		{
		directory.Add (&fMake);
		}
		
	if (exif.fModel.NotEmpty ())
		{
		directory.Add (&fModel);
		}
		
	if (exif.fArtist.NotEmpty ())
		{
		directory.Add (&fArtist);
		}
	
	if (exif.fSoftware.NotEmpty ())
		{
		directory.Add (&fSoftware);
		}
	
	if (exif.fCopyright.NotEmpty ())
		{
		directory.Add (&fCopyright);
		}
	
	if (exif.fImageDescription.NotEmpty ())
		{
		directory.Add (&fImageDescription);
		}
	
	if (exif.fCameraSerialNumber.NotEmpty () && insideDNG)
		{
		directory.Add (&fSerialNumber);
		}
		
	if (makerNoteSafe && makerNoteData)
		{
		
		directory.Add (&fMakerNoteSafety);
		
		fExifIFD.Add (&fMakerNote);
		
		}
		
	if (exif.fUserComment.NotEmpty ())
		{
		fExifIFD.Add (&fUserComment);
		}
		
	if (exif.fImageUniqueID.IsValid ())
		{
		
		for (uint32 j = 0; j < 16; j++)
			{
			
			snprintf (fImageUniqueIDData + j * 2,
					  33,
					  "%02X",
					  (unsigned) exif.fImageUniqueID.Data () [j]);
					 
			}
		
		fExifIFD.Add (&fImageUniqueID);
		
		}

	if (exif.AtLeastVersion0230 ())
		{

		if (exif.fSensitivityType != 0)
			{
			
			fExifIFD.Add (&fSensitivityType);
			
			}

		
		

		if (exif.fSensitivityType	  != 0 &&
			exif.fISOSpeedRatings [0] != 0)
			{

			

			if (exif.fStandardOutputSensitivity != 0)
				{
				fExifIFD.Add (&fStandardOutputSensitivity);	
				}

			

			if (exif.fRecommendedExposureIndex != 0)
				{
				fExifIFD.Add (&fRecommendedExposureIndex);
				}

			

			if (exif.fISOSpeed != 0)
				{

				fExifIFD.Add (&fISOSpeed);

				if (exif.fISOSpeedLatitudeyyy != 0 &&
					exif.fISOSpeedLatitudezzz != 0)
					{
						
					fExifIFD.Add (&fISOSpeedLatitudeyyy);
					fExifIFD.Add (&fISOSpeedLatitudezzz);
						
					}

				}

			}
		
		if (exif.fOwnerName.NotEmpty ())
			{
			fExifIFD.Add (&fCameraOwnerName);
			}
		
		if (exif.fCameraSerialNumber.NotEmpty ())
			{
			fExifIFD.Add (&fBodySerialNumber);
			}

		if (exif.fLensInfo [0].IsValid () &&
			exif.fLensInfo [1].IsValid ())
			{
			fExifIFD.Add (&fLensSpecification);
			}
		
		if (exif.fLensMake.NotEmpty ())
			{
			fExifIFD.Add (&fLensMake);
			}
		
		if (exif.fLensName.NotEmpty ())
			{
			fExifIFD.Add (&fLensModel);
			}
		
		if (exif.fLensSerialNumber.NotEmpty ())
			{
			fExifIFD.Add (&fLensSerialNumber);
			}
		
		}
  
	if (exif.AtLeastVersion0231 ())
		{
		
		if (exif.fDateTime.IsValid () &&
			exif.fDateTime.TimeZone ().IsValid ())
			{
			fExifIFD.Add (&fOffsetTime);
			}
		
		if (exif.fDateTimeOriginal.IsValid () &&
			exif.fDateTimeOriginal.TimeZone ().IsValid ())
			{
			fExifIFD.Add (&fOffsetTimeOriginal);
			}
		
		if (exif.fDateTimeDigitized.IsValid () &&
			exif.fDateTimeDigitized.TimeZone ().IsValid ())
			{
			fExifIFD.Add (&fOffsetTimeDigitized);
			}
			
		if (exif.fTemperature.IsValid ())
			{
			fExifIFD.Add (&fTemperature);
			}
		
		if (exif.fHumidity.IsValid ())
			{
			fExifIFD.Add (&fHumidity);
			}
		
		if (exif.fPressure.IsValid ())
			{
			fExifIFD.Add (&fPressure);
			}
		
		if (exif.fWaterDepth.IsValid ())
			{
			fExifIFD.Add (&fWaterDepth);
			}
		
		if (exif.fAcceleration.IsValid ())
			{
			fExifIFD.Add (&fAcceleration);
			}
		
		if (exif.fCameraElevationAngle.IsValid ())
			{
			fExifIFD.Add (&fCameraElevationAngle);
			}
		
		}
		
	if (exif.fGPSVersionID)
		{
		
		fGPSVersionData [0] = (uint8) (exif.fGPSVersionID >> 24);
		fGPSVersionData [1] = (uint8) (exif.fGPSVersionID >> 16);
		fGPSVersionData [2] = (uint8) (exif.fGPSVersionID >>  8);
		fGPSVersionData [3] = (uint8) (exif.fGPSVersionID	   );
		
		fGPSIFD.Add (&fGPSVersionID);
		
		}
		
	if (exif.fGPSLatitudeRef.NotEmpty () &&
		exif.fGPSLatitude [0].IsValid ())
		{
		fGPSIFD.Add (&fGPSLatitudeRef);
		fGPSIFD.Add (&fGPSLatitude	 );
		}
		
	if (exif.fGPSLongitudeRef.NotEmpty () &&
		exif.fGPSLongitude [0].IsValid ())
		{
		fGPSIFD.Add (&fGPSLongitudeRef);
		fGPSIFD.Add (&fGPSLongitude	  );
		}
		
	if (exif.fGPSAltitudeRef <= 0x0FF)
		{
		fGPSIFD.Add (&fGPSAltitudeRef);
		}
		
	if (exif.fGPSAltitude.IsValid ())
		{
		fGPSIFD.Add (&fGPSAltitude);
		}
		
	if (exif.fGPSTimeStamp [0].IsValid ())
		{
		fGPSIFD.Add (&fGPSTimeStamp);
		}
		
	if (exif.fGPSSatellites.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSSatellites);
		}
		
	if (exif.fGPSStatus.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSStatus);
		}
		
	if (exif.fGPSMeasureMode.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSMeasureMode);
		}
		
	if (exif.fGPSDOP.IsValid ())
		{
		fGPSIFD.Add (&fGPSDOP);
		}
		
	if (exif.fGPSSpeedRef.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSSpeedRef);
		}
		
	if (exif.fGPSSpeed.IsValid ())
		{
		fGPSIFD.Add (&fGPSSpeed);
		}
		
	if (exif.fGPSTrackRef.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSTrackRef);
		}
		
	if (exif.fGPSTrack.IsValid ())
		{
		fGPSIFD.Add (&fGPSTrack);
		}
		
	if (exif.fGPSImgDirectionRef.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSImgDirectionRef);
		}
		
	if (exif.fGPSImgDirection.IsValid ())
		{
		fGPSIFD.Add (&fGPSImgDirection);
		}

	if (exif.fGPSMapDatum.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSMapDatum);
		}
		
	if (exif.fGPSDestLatitudeRef.NotEmpty () &&
		exif.fGPSDestLatitude [0].IsValid ())
		{
		fGPSIFD.Add (&fGPSDestLatitudeRef);
		fGPSIFD.Add (&fGPSDestLatitude	 );
		}
		
	if (exif.fGPSDestLongitudeRef.NotEmpty () &&
		exif.fGPSDestLongitude [0].IsValid ())
		{
		fGPSIFD.Add (&fGPSDestLongitudeRef);
		fGPSIFD.Add (&fGPSDestLongitude	  );
		}
		
	if (exif.fGPSDestBearingRef.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSDestBearingRef);
		}
		
	if (exif.fGPSDestBearing.IsValid ())
		{
		fGPSIFD.Add (&fGPSDestBearing);
		}

	if (exif.fGPSDestDistanceRef.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSDestDistanceRef);
		}
		
	if (exif.fGPSDestDistance.IsValid ())
		{
		fGPSIFD.Add (&fGPSDestDistance);
		}
		
	if (exif.fGPSProcessingMethod.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSProcessingMethod);
		}

	if (exif.fGPSAreaInformation.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSAreaInformation);
		}
	
	if (exif.fGPSDateStamp.NotEmpty ())
		{
		fGPSIFD.Add (&fGPSDateStamp);
		}
	
	if (exif.fGPSDifferential <= 0x0FFFF)
		{
		fGPSIFD.Add (&fGPSDifferential);
		}

	if (exif.AtLeastVersion0230 ())
		{
		
		if (exif.fGPSHPositioningError.IsValid ())
			{
			fGPSIFD.Add (&fGPSHPositioningError);
			}

		}
		
	AddLinks (directory);
	
	}

void exif_tag_set::AddLinks (dng_tiff_directory &directory)
	{
	
	if (fExifIFD.Size () != 0 && !fAddedExifLink)
		{
		
		directory.Add (&fExifLink);
		
		fAddedExifLink = true;
		
		}
	
	if (fGPSIFD.Size () != 0 && !fAddedGPSLink)
		{
		
		directory.Add (&fGPSLink);
		
		fAddedGPSLink = true;
		
		}
	
	}
	

class range_tag_set
	{
	
	private:
	
		uint32 fActiveAreaData [4];
	
		tag_uint32_ptr fActiveArea;
	
		uint32 fMaskedAreaData [kMaxMaskedAreas * 4];
	
		tag_uint32_ptr fMaskedAreas;
							
		tag_uint16_ptr fLinearizationTable;
	
		uint16 fBlackLevelRepeatDimData [2];
		
		tag_uint16_ptr fBlackLevelRepeatDim;
		
		dng_urational fBlackLevelData [kMaxBlackPattern *
									   kMaxBlackPattern *
									   kMaxColorPlanes];
		
		tag_urational_ptr fBlackLevel;
		
		dng_memory_data fBlackLevelDeltaHData;
		dng_memory_data fBlackLevelDeltaVData;
		
		tag_srational_ptr fBlackLevelDeltaH;
		tag_srational_ptr fBlackLevelDeltaV;
		
		uint16 fWhiteLevelData16 [kMaxColorPlanes];
		uint32 fWhiteLevelData32 [kMaxColorPlanes];
		
		tag_uint16_ptr fWhiteLevel16;
		tag_uint32_ptr fWhiteLevel32;
		
	public:
	
		range_tag_set (dng_tiff_directory &directory,
					   const dng_negative &negative);
		
	};
	

range_tag_set::range_tag_set (dng_tiff_directory &directory,
							  const dng_negative &negative)
						  
	:	fActiveArea (tcActiveArea,
					 fActiveAreaData,
					 4)
	
	,	fMaskedAreas (tcMaskedAreas,
					  fMaskedAreaData,
					  0)
	
	,	fLinearizationTable (tcLinearizationTable,
							 NULL,
							 0)
						  
	,	fBlackLevelRepeatDim (tcBlackLevelRepeatDim,
							  fBlackLevelRepeatDimData,
							  2)
							 
	,	fBlackLevel (tcBlackLevel,
					 fBlackLevelData)
					 
	,	fBlackLevelDeltaHData ()
	,	fBlackLevelDeltaVData ()
					 
	,	fBlackLevelDeltaH (tcBlackLevelDeltaH)
	,	fBlackLevelDeltaV (tcBlackLevelDeltaV)
 
	,	fWhiteLevel16 (tcWhiteLevel,
					   fWhiteLevelData16)
					 
	,	fWhiteLevel32 (tcWhiteLevel,
					   fWhiteLevelData32)
					 
	{
	
	const dng_image &rawImage (negative.RawImage ());

	if (rawImage.Planes () > kMaxColorPlanes)
		{
		ThrowBadFormat ();
		}

	const dng_linearization_info *rangeInfo = negative.GetLinearizationInfo ();
	
	if (rangeInfo)
		{
		
		
		
			{
		
			const dng_rect &r = rangeInfo->fActiveArea;
			
			if (r.NotEmpty ())
				{
			
				fActiveAreaData [0] = r.t;
				fActiveAreaData [1] = r.l;
				fActiveAreaData [2] = r.b;
				fActiveAreaData [3] = r.r;
				
				directory.Add (&fActiveArea);
				
				}
				
			}
			
		
			
		if (rangeInfo->fMaskedAreaCount)
			{
			
			fMaskedAreas.SetCount (rangeInfo->fMaskedAreaCount * 4);
			
			for (uint32 index = 0; index < rangeInfo->fMaskedAreaCount; index++)
				{
				
				const dng_rect &r = rangeInfo->fMaskedArea [index];
				
				fMaskedAreaData [index * 4 + 0] = r.t;
				fMaskedAreaData [index * 4 + 1] = r.l;
				fMaskedAreaData [index * 4 + 2] = r.b;
				fMaskedAreaData [index * 4 + 3] = r.r;
				
				}
				
			directory.Add (&fMaskedAreas);
			
			}
			
		

		if (rangeInfo->fLinearizationTable.Get ())
			{
			
			fLinearizationTable.SetData	 (rangeInfo->fLinearizationTable->Buffer_uint16 ()	   );
			fLinearizationTable.SetCount (rangeInfo->fLinearizationTable->LogicalSize	() >> 1);
			
			directory.Add (&fLinearizationTable);
			
			}
			
		
		
			{
		
			fBlackLevelRepeatDimData [0] = (uint16) rangeInfo->fBlackLevelRepeatRows;
			fBlackLevelRepeatDimData [1] = (uint16) rangeInfo->fBlackLevelRepeatCols;
			
			directory.Add (&fBlackLevelRepeatDim);
			
			}
		
		
		
			{
		
			uint32 index = 0;
			
			for (uint16 v = 0; v < rangeInfo->fBlackLevelRepeatRows; v++)
				{
				
				for (uint32 h = 0; h < rangeInfo->fBlackLevelRepeatCols; h++)
					{
					
					for (uint32 c = 0; c < rawImage.Planes (); c++)
						{
					
						fBlackLevelData [index++] = rangeInfo->BlackLevel (v, h, c);
					
						}
						
					}
					
				}
				
			fBlackLevel.SetCount (rangeInfo->fBlackLevelRepeatRows *
								  rangeInfo->fBlackLevelRepeatCols * rawImage.Planes ());
			
			directory.Add (&fBlackLevel);
			
			}
		
		
				
		if (rangeInfo->ColumnBlackCount ())
			{
			
			uint32 count = rangeInfo->ColumnBlackCount ();
		
			fBlackLevelDeltaHData.Allocate (count, sizeof (dng_srational));
												 
			dng_srational *blacks = (dng_srational *) fBlackLevelDeltaHData.Buffer ();
			
			for (uint32 col = 0; col < count; col++)
				{
				
				blacks [col] = rangeInfo->ColumnBlack (col);
				
				}
												 
			fBlackLevelDeltaH.SetData  (blacks);
			fBlackLevelDeltaH.SetCount (count );
			
			directory.Add (&fBlackLevelDeltaH);
			
			}
		
		
				
		if (rangeInfo->RowBlackCount ())
			{
			
			uint32 count = rangeInfo->RowBlackCount ();
		
			fBlackLevelDeltaVData.Allocate (count, sizeof (dng_srational));
												 
			dng_srational *blacks = (dng_srational *) fBlackLevelDeltaVData.Buffer ();
			
			for (uint32 row = 0; row < count; row++)
				{
				
				blacks [row] = rangeInfo->RowBlack (row);
				
				}
												 
			fBlackLevelDeltaV.SetData  (blacks);
			fBlackLevelDeltaV.SetCount (count );
			
			directory.Add (&fBlackLevelDeltaV);
			
			}
			
		}
  
	else if (negative.RawImageBlackLevel ())
		{
		
		for (uint32 c = 0; c < rawImage.Planes (); c++)
			{
		
			fBlackLevelData [c] = dng_urational (negative.RawImageBlackLevel (), 1);
		
			}

		fBlackLevel.SetCount (rawImage.Planes ());
		
		directory.Add (&fBlackLevel);
			
		}
		
	
	
	
	
	
	bool needs32 = false;
		
	fWhiteLevel16.SetCount (rawImage.Planes ());
	fWhiteLevel32.SetCount (rawImage.Planes ());
	
	for (uint32 c = 0; c < fWhiteLevel16.Count (); c++)
		{
		
		fWhiteLevelData32 [c] = negative.WhiteLevel (c);
		
		if (fWhiteLevelData32 [c] > 0x0FFFF)
			{
			needs32 = true;
			}
			
		fWhiteLevelData16 [c] = (uint16) fWhiteLevelData32 [c];
		
		}
		
	if (needs32)
		{
		directory.Add (&fWhiteLevel32);
		}
		
	else
		{
		directory.Add (&fWhiteLevel16);
		}
	
	}

class mosaic_tag_set
	{
	
	private:
	
		uint16 fCFARepeatPatternDimData [2];
		
		tag_uint16_ptr fCFARepeatPatternDim;
										   
		uint8 fCFAPatternData [kMaxCFAPattern *
							   kMaxCFAPattern];
		
		tag_uint8_ptr fCFAPattern;
								 
		uint8 fCFAPlaneColorData [kMaxColorPlanes];
		
		tag_uint8_ptr fCFAPlaneColor;
									
		tag_uint16 fCFALayout;
		
		tag_uint32 fGreenSplit;
		
	public:
	
		mosaic_tag_set (dng_tiff_directory &directory,
						const dng_mosaic_info &info);
		
	};
	

mosaic_tag_set::mosaic_tag_set (dng_tiff_directory &directory,
								const dng_mosaic_info &info)

	:	fCFARepeatPatternDim (tcCFARepeatPatternDim,
							  fCFARepeatPatternDimData,
							  2)
							  
	,	fCFAPattern (tcCFAPattern,
					 fCFAPatternData)
					
	,	fCFAPlaneColor (tcCFAPlaneColor,
						fCFAPlaneColorData)
						
	,	fCFALayout (tcCFALayout,
					(uint16) info.fCFALayout)
	
	,	fGreenSplit (tcBayerGreenSplit,
					 info.fBayerGreenSplit)
	
	{
	
	if (info.IsColorFilterArray ())
		{
	
		
		
		fCFARepeatPatternDimData [0] = (uint16) info.fCFAPatternSize.v;
		fCFARepeatPatternDimData [1] = (uint16) info.fCFAPatternSize.h;
				
		directory.Add (&fCFARepeatPatternDim);
		
		
		
		fCFAPattern.SetCount (info.fCFAPatternSize.v *
							  info.fCFAPatternSize.h);
							  
		for (int32 r = 0; r < info.fCFAPatternSize.v; r++)
			{
			
			for (int32 c = 0; c < info.fCFAPatternSize.h; c++)
				{
				
				fCFAPatternData [r * info.fCFAPatternSize.h + c] = info.fCFAPattern [r] [c];
				
				}
				
			}
				
		directory.Add (&fCFAPattern);
		
		
		
		fCFAPlaneColor.SetCount (info.fColorPlanes);
		
		for (uint32 j = 0; j < info.fColorPlanes; j++)
			{
		
			fCFAPlaneColorData [j] = info.fCFAPlaneColor [j];
			
			}
		
		directory.Add (&fCFAPlaneColor);
		
		
		
		fCFALayout.Set ((uint16) info.fCFALayout);
		
		directory.Add (&fCFALayout);
		
		
			
		if (info.fCFAPatternSize == dng_point (2, 2) &&
			info.fColorPlanes	 == 3)
			{
			
			directory.Add (&fGreenSplit);
			
			}
			
		}

	}
	

class color_tag_set
	{
	
	private:
	
		uint32 fColorChannels;
		
		tag_matrix fCameraCalibration1;
		tag_matrix fCameraCalibration2;
		tag_matrix fCameraCalibration3;
										 
		tag_string fCameraCalibrationSignature;
		
		tag_string fAsShotProfileName;

		dng_urational fAnalogBalanceData [4];
	
		tag_urational_ptr fAnalogBalance;
									
		dng_urational fAsShotNeutralData [4];
		
		tag_urational_ptr fAsShotNeutral;
										
		dng_urational fAsShotWhiteXYData [2];
		
		tag_urational_ptr fAsShotWhiteXY;
										
		tag_urational fLinearResponseLimit;
										  
	public:
	
		color_tag_set (dng_tiff_directory &directory,
					   const dng_negative &negative);
		
	};
	

color_tag_set::color_tag_set (dng_tiff_directory &directory,
							  const dng_negative &negative)
						  
	:	fColorChannels (negative.ColorChannels ())
	
	,	fCameraCalibration1 (tcCameraCalibration1,
							 negative.CameraCalibration1 ())
						
	,	fCameraCalibration2 (tcCameraCalibration2,
							 negative.CameraCalibration2 ())
							 
	,	fCameraCalibration3 (tcCameraCalibration3,
							 negative.CameraCalibration3 ())
							 
	,	fCameraCalibrationSignature (tcCameraCalibrationSignature,
									 negative.CameraCalibrationSignature ())
									 
	,	fAsShotProfileName (tcAsShotProfileName,
							negative.AsShotProfileName ())

	,	fAnalogBalance (tcAnalogBalance,
						fAnalogBalanceData,
						fColorChannels)
						
	,	fAsShotNeutral (tcAsShotNeutral,
						fAsShotNeutralData,
						fColorChannels)
						
	,	fAsShotWhiteXY (tcAsShotWhiteXY,
						fAsShotWhiteXYData,
						2)
									
	,	fLinearResponseLimit (tcLinearResponseLimit,
							  negative.LinearResponseLimitR ())
							  
	{
	
	if (fColorChannels > 1)
		{
		
		uint32 channels2 = fColorChannels * fColorChannels;
		
		if (fCameraCalibration1.Count () == channels2)
			{
			
			directory.Add (&fCameraCalibration1);
		
			}
			
		if (fCameraCalibration2.Count () == channels2)
			{
			
			directory.Add (&fCameraCalibration2);
		
			}
			
		if (fCameraCalibration3.Count () == channels2)
			{
			
			directory.Add (&fCameraCalibration3);
		
			}
			
		if (fCameraCalibration1.Count () == channels2 ||
			fCameraCalibration2.Count () == channels2 ||
			fCameraCalibration3.Count () == channels2)
			{
			
			if (negative.CameraCalibrationSignature ().NotEmpty ())
				{
				
				directory.Add (&fCameraCalibrationSignature);
				
				}
			
			}
			
		if (negative.AsShotProfileName ().NotEmpty ())
			{
			
			directory.Add (&fAsShotProfileName);
				
			}
			
		for (uint32 j = 0; j < fColorChannels; j++)
			{
			
			fAnalogBalanceData [j] = negative.AnalogBalanceR (j);
			
			}
			
		directory.Add (&fAnalogBalance);
		
		if (negative.HasCameraNeutral ())
			{
			
			for (uint32 k = 0; k < fColorChannels; k++)
				{
				
				fAsShotNeutralData [k] = negative.CameraNeutralR (k);
				
				}
			
			directory.Add (&fAsShotNeutral);
			
			}
			
		else if (negative.HasCameraWhiteXY ())
			{
			
			negative.GetCameraWhiteXY (fAsShotWhiteXYData [0],
									   fAsShotWhiteXYData [1]);
									   
			directory.Add (&fAsShotWhiteXY);
			
			}
		
		directory.Add (&fLinearResponseLimit);
		
		}
	
	}

class profile_tag_set
	{
	
	private:
	
		tag_uint16 fCalibrationIlluminant1;
		tag_uint16 fCalibrationIlluminant2;
		tag_uint16 fCalibrationIlluminant3;

		tag_data_ptr fIlluminantData1;
		tag_data_ptr fIlluminantData2;
		tag_data_ptr fIlluminantData3;

		tag_matrix fColorMatrix1;
		tag_matrix fColorMatrix2;
		tag_matrix fColorMatrix3;
		
		tag_matrix fForwardMatrix1;
		tag_matrix fForwardMatrix2;
		tag_matrix fForwardMatrix3;

		tag_matrix fReductionMatrix1;
		tag_matrix fReductionMatrix2;
		tag_matrix fReductionMatrix3;
		
		tag_string fProfileName;
		tag_string fProfileGroupName;
		
		tag_string fProfileCalibrationSignature;
		
		tag_uint32 fEmbedPolicyTag;
		
		tag_string fCopyrightTag;
		
		uint32 fHueSatMapDimData [3];
		
		tag_uint32_ptr fHueSatMapDims;

		tag_data_ptr fHueSatData1;
		tag_data_ptr fHueSatData2;
		tag_data_ptr fHueSatData3;
		
		tag_uint32 fHueSatMapEncodingTag;
		
		uint32 fLookTableDimData [3];
		
		tag_uint32_ptr fLookTableDims;

		tag_data_ptr fLookTableData;
		
		tag_uint32 fLookTableEncodingTag;

		tag_srational fBaselineExposureOffsetTag;
		
		tag_uint32 fDefaultBlackRenderTag;

		dng_memory_data fToneCurveBuffer;
		
		tag_data_ptr fToneCurveTag;

		tag_uint32 fToneMethodTag;

		AutoPtr<dng_memory_block> fIlluminantBlock1;
		AutoPtr<dng_memory_block> fIlluminantBlock2;
		AutoPtr<dng_memory_block> fIlluminantBlock3;

		AutoPtr<tag_owned_data_ptr> fProfileGainTableMapTag;

		AutoPtr<tag_owned_data_ptr> fProfileDynamicRangeTag;

		AutoPtr<tag_owned_data_ptr> fRGBTablesTag;

		bool fProfileDidWritePGTM = false;

	public:
	
		profile_tag_set (dng_host &host,
						 dng_tiff_directory &directory,
						 const dng_camera_profile &profile);

		bool ProfileDidWritePGTMToMainIFD () const
			{
			return fProfileDidWritePGTM;
			}
		
	};
	

profile_tag_set::profile_tag_set (dng_host &host,
								  dng_tiff_directory &directory,
								  const dng_camera_profile &profile)
						  
	:	fCalibrationIlluminant1 (tcCalibrationIlluminant1,
								 (uint16) profile.CalibrationIlluminant1 ())
								 
	,	fCalibrationIlluminant2 (tcCalibrationIlluminant2,
								 (uint16) profile.CalibrationIlluminant2 ())
	
	,	fCalibrationIlluminant3 (tcCalibrationIlluminant3,
								 (uint16) profile.CalibrationIlluminant3 ())

	,	fIlluminantData1 (tcIlluminantData1,
						  ttUndefined,
						  profile.IlluminantData1 ().TagCount (),
						  nullptr)			 
		
	,	fIlluminantData2 (tcIlluminantData2,
						  ttUndefined,
						  profile.IlluminantData2 ().TagCount (),
						  nullptr)			 
		
	,	fIlluminantData3 (tcIlluminantData3,
						  ttUndefined,
						  profile.IlluminantData3 ().TagCount (),
						  nullptr)			 
		
	,	fColorMatrix1 (tcColorMatrix1,
					   profile.ColorMatrix1 ())
						
	,	fColorMatrix2 (tcColorMatrix2,
					   profile.ColorMatrix2 ())

	,	fColorMatrix3 (tcColorMatrix3,
					   profile.ColorMatrix3 ())

	,	fForwardMatrix1 (tcForwardMatrix1,
						 profile.ForwardMatrix1 ())
						
	,	fForwardMatrix2 (tcForwardMatrix2,
						 profile.ForwardMatrix2 ())
						
	,	fForwardMatrix3 (tcForwardMatrix3,
						 profile.ForwardMatrix3 ())
						
	,	fReductionMatrix1 (tcReductionMatrix1,
						   profile.ReductionMatrix1 ())
						
	,	fReductionMatrix2 (tcReductionMatrix2,
						   profile.ReductionMatrix2 ())
						   
	,	fReductionMatrix3 (tcReductionMatrix3,
						   profile.ReductionMatrix3 ())
						   
	,	fProfileName (tcProfileName,
					  profile.Name (),
					  false)
						   
	,	fProfileGroupName (tcProfileGroupName,
						   profile.GroupName (),
						   false)
						   
	,	fProfileCalibrationSignature (tcProfileCalibrationSignature,
									  profile.ProfileCalibrationSignature (),
									  false)
						
	,	fEmbedPolicyTag (tcProfileEmbedPolicy,
						 profile.EmbedPolicy ())
						 
	,	fCopyrightTag (tcProfileCopyright,
					   profile.Copyright (),
					   false)
					   
	,	fHueSatMapDims (tcProfileHueSatMapDims, 
						fHueSatMapDimData,
						3)
		
	,	fHueSatData1 (tcProfileHueSatMapData1,
					  ttFloat,
					  profile.HueSatDeltas1 ().DeltasCount () * 3,
					  profile.HueSatDeltas1 ().GetConstDeltas ())
					  
	,	fHueSatData2 (tcProfileHueSatMapData2,
					  ttFloat,
					  profile.HueSatDeltas2 ().DeltasCount () * 3,
					  profile.HueSatDeltas2 ().GetConstDeltas ())
					  
	,	fHueSatData3 (tcProfileHueSatMapData3,
					  ttFloat,
					  profile.HueSatDeltas3 ().DeltasCount () * 3,
					  profile.HueSatDeltas3 ().GetConstDeltas ())
					  
	,	fHueSatMapEncodingTag (tcProfileHueSatMapEncoding,
							   profile.HueSatMapEncoding ())
						 
	,	fLookTableDims (tcProfileLookTableDims,
						fLookTableDimData,
						3)
						
	,	fLookTableData (tcProfileLookTableData,
						ttFloat,
						profile.LookTable ().DeltasCount () * 3,
						profile.LookTable ().GetConstDeltas ())
					  
	,	fLookTableEncodingTag (tcProfileLookTableEncoding,
							   profile.LookTableEncoding ())
						 
	,	fBaselineExposureOffsetTag (tcBaselineExposureOffset,
									profile.BaselineExposureOffset ())
						 
	,	fDefaultBlackRenderTag (tcDefaultBlackRender,
								profile.DefaultBlackRender ())
						 
	,	fToneCurveBuffer ()
					  
	,	fToneCurveTag (tcProfileToneCurve,
					   ttFloat,
					   0,
					   NULL)
					   
	,	fToneMethodTag (tcProfileToneMethod,
						profile.ToneMethod ())

	{

	const uint32 illuminantModel = profile.IlluminantModel ();

	const bool isTripleIlluminantModel = (illuminantModel == 3);
	
	if (profile.HasColorMatrix1 ())
		{
	
		uint32 colorChannels = profile.ColorMatrix1 ().Rows ();
		
		directory.Add (&fCalibrationIlluminant1);

		if (profile.CalibrationIlluminant1 () == lsOther &&
			profile.IlluminantData1 ().IsValid ())
			{

			dng_memory_stream stream (host.Allocator ());

			auto &tag = fIlluminantData1;

			auto &block = fIlluminantBlock1;

			profile.IlluminantData1 ().Put (stream);

			block.Reset (stream.AsMemoryBlock (host.Allocator ()));

			DNG_REQUIRE (block->LogicalSize () >= tag.Count (),
						 "illuminant data block too small");
						 
			tag.SetData (block->Buffer ());
			
			directory.Add (&tag);
			
			}
		
		directory.Add (&fColorMatrix1);
		
		if (fForwardMatrix1.Count () == colorChannels * 3)
			{
			
			directory.Add (&fForwardMatrix1);

			}
		
		if (colorChannels > 3 && fReductionMatrix1.Count () == colorChannels * 3)
			{
			
			directory.Add (&fReductionMatrix1);
			
			}

		if (profile.HasColorMatrix2 ())
			{
		
			directory.Add (&fCalibrationIlluminant2);

			if (profile.CalibrationIlluminant2 () == lsOther &&
				profile.IlluminantData2 ().IsValid ())
				{

				dng_memory_stream stream (host.Allocator ());

				auto &tag = fIlluminantData2;

				auto &block = fIlluminantBlock2;

				profile.IlluminantData2 ().Put (stream);

				block.Reset (stream.AsMemoryBlock (host.Allocator ()));

				DNG_REQUIRE (block->LogicalSize () >= tag.Count (),
							 "illuminant data block too small");

				tag.SetData (block->Buffer ());

				directory.Add (&tag);

				}
		
			directory.Add (&fColorMatrix2);
				
			if (fForwardMatrix2.Count () == colorChannels * 3)
				{
				
				directory.Add (&fForwardMatrix2);

				}
		
			if (colorChannels > 3 && fReductionMatrix2.Count () == colorChannels * 3)
				{
				
				directory.Add (&fReductionMatrix2);
				
				}

			

			if (isTripleIlluminantModel)
				{
				
				directory.Add (&fCalibrationIlluminant3);

				if (profile.CalibrationIlluminant3 () == lsOther &&
					profile.IlluminantData3 ().IsValid ())
					{

					dng_memory_stream stream (host.Allocator ());

					auto &tag = fIlluminantData3;

					auto &block = fIlluminantBlock3;

					profile.IlluminantData3 ().Put (stream);

					block.Reset (stream.AsMemoryBlock (host.Allocator ()));

					DNG_REQUIRE (block->LogicalSize () >= tag.Count (),
								 "illuminant data block too small");

					tag.SetData (block->Buffer ());

					directory.Add (&tag);

					}

				directory.Add (&fColorMatrix3);

				if (fForwardMatrix3.Count () == colorChannels * 3)
					{

					directory.Add (&fForwardMatrix3);

					}

				if (colorChannels > 3 && fReductionMatrix3.Count () == colorChannels * 3)
					{

					directory.Add (&fReductionMatrix3);

					}

				} 
	
			}
			
		if (profile.Name ().NotEmpty ())
			{
			
			directory.Add (&fProfileName);

			}
			
		if (profile.GroupName ().NotEmpty ())
			{
			
			directory.Add (&fProfileGroupName);

			}
			
		if (profile.ProfileCalibrationSignature ().NotEmpty ())
			{
			
			directory.Add (&fProfileCalibrationSignature);
			
			}
			
		directory.Add (&fEmbedPolicyTag);
		
		if (profile.Copyright ().NotEmpty ())
			{
			
			directory.Add (&fCopyrightTag);
			
			}
		
		bool haveHueSat1 = profile.HueSatDeltas1 ().IsValid ();
		
		bool haveHueSat2 = profile.HueSatDeltas2 ().IsValid () &&
						   profile.HasColorMatrix2 ();

		bool haveHueSat3 = isTripleIlluminantModel &&
						   profile.HueSatDeltas3 ().IsValid ();

		if (haveHueSat1 || haveHueSat2 || haveHueSat3)
			{
			
			uint32 hueDivs = 0;
			uint32 satDivs = 0;
			uint32 valDivs = 0;

			
			
			
			

			if (haveHueSat1)
				{

				profile.HueSatDeltas1 ().GetDivisions (hueDivs,
													   satDivs,
													   valDivs);

				}

			else
				{

				profile.HueSatDeltas2 ().GetDivisions (hueDivs,
													   satDivs,
													   valDivs);

				}

			fHueSatMapDimData [0] = hueDivs;
			fHueSatMapDimData [1] = satDivs;
			fHueSatMapDimData [2] = valDivs;
			
			directory.Add (&fHueSatMapDims);

			
			

			if (profile.HueSatMapEncoding () != encoding_Linear)
				{

				directory.Add (&fHueSatMapEncodingTag);

				}
		
			}
			
		if (haveHueSat1)
			{
			
			directory.Add (&fHueSatData1);
			
			}
			
		if (haveHueSat2)
			{
			
			directory.Add (&fHueSatData2);
			
			}
			
		if (haveHueSat3)
			{
			
			directory.Add (&fHueSatData3);
			
			}
			
		if (profile.HasLookTable ())
			{
			
			uint32 hueDivs = 0;
			uint32 satDivs = 0;
			uint32 valDivs = 0;

			profile.LookTable ().GetDivisions (hueDivs,
											   satDivs,
											   valDivs);

			fLookTableDimData [0] = hueDivs;
			fLookTableDimData [1] = satDivs;
			fLookTableDimData [2] = valDivs;
			
			directory.Add (&fLookTableDims);
			
			directory.Add (&fLookTableData);
			
			
			

			if (profile.LookTableEncoding () != encoding_Linear)
				{

				directory.Add (&fLookTableEncodingTag);

				}
		
			}

		
		

		if (profile.BaselineExposureOffset ().IsValid ())
			{

			if (profile.BaselineExposureOffset ().As_real64 () != 0.0)
				{
			
				directory.Add (&fBaselineExposureOffsetTag);

				}
				
			}
			
		if (profile.DefaultBlackRender () != defaultBlackRender_Auto)
			{

			directory.Add (&fDefaultBlackRenderTag);

			}
		
		if (profile.ToneCurve ().IsValid ())
			{
			
			
			
			
			uint32 toneCurvePoints = (uint32) (profile.ToneCurve ().fCoord.size ());

			fToneCurveBuffer.Allocate (dng_safe_uint32 (toneCurvePoints) * 2u,
									   sizeof (real32));

			real32 *points = fToneCurveBuffer.Buffer_real32 ();
			
			fToneCurveTag.SetCount (toneCurvePoints * 2);
			fToneCurveTag.SetData  (points);
			
			for (uint32 i = 0; i < toneCurvePoints; i++)
				{

				
				

				points [i * 2	 ] = (real32) profile.ToneCurve ().fCoord [i].h;
				points [i * 2 + 1] = (real32) profile.ToneCurve ().fCoord [i].v;

				}

			directory.Add (&fToneCurveTag);

			}

		if (profile.ToneMethod () != profileToneMethod_Unspecified)
			{

			directory.Add (&fToneMethodTag);

			}

		

		if (profile.HasProfileGainTableMap ())
			{
			
			dng_memory_stream tempStream (host.Allocator (),
										  host.Sniffer ());

			auto pgtm = profile.ShareProfileGainTableMap ();

			

			pgtm->PutStream (tempStream, true);

			const_dng_memory_block_sptr block
				(tempStream.AsMemoryBlock (host.Allocator ()));
			
			fProfileGainTableMapTag.Reset
				(new tag_owned_data_ptr (tcProfileGainTableMap2,
										 ttUndefined,
										 block->LogicalSize (),
										 block));

			directory.Add (fProfileGainTableMapTag.Get ());

			fProfileDidWritePGTM = true;

			}

		

		if (profile.DynamicRangeInfo ().IsValid () &&
			(profile.DynamicRangeInfo () != dng_camera_profile_dynamic_range ()))
			{
			
			dng_memory_stream tempStream (host.Allocator (),
										  host.Sniffer ());

			profile.DynamicRangeInfo ().PutStream (tempStream);

			const_dng_memory_block_sptr block
				(tempStream.AsMemoryBlock (host.Allocator ()));
			
			fProfileDynamicRangeTag.Reset
				(new tag_owned_data_ptr (tcProfileDynamicRange,
										 ttUndefined,
										 block->LogicalSize (),
										 block));

			directory.Add (fProfileDynamicRangeTag.Get ());
			
			}

		
		
		if (profile.HasMaskedRGBTables () &&
			!profile.MaskedRGBTables ().IsNOP ())
			{

			dng_memory_stream tempStream (host.Allocator (),
										  host.Sniffer ());

			profile.MaskedRGBTables ().PutStream (tempStream);

			const_dng_memory_block_sptr block
				(tempStream.AsMemoryBlock (host.Allocator ()));
			
			fRGBTablesTag.Reset
				(new tag_owned_data_ptr (tcRGBTables,
										 ttUndefined,
										 block->LogicalSize (),
										 block));

			directory.Add (fRGBTablesTag.Get ());
			
			}

		} 
	
	}

tiff_dng_extended_color_profile::tiff_dng_extended_color_profile 
								 (const dng_camera_profile &profile,
								  bool includeModelRestriction)

	:	fProfile (profile)
	
	,	fProfileTagSet ()
	
	,	fCameraModelTag (tcUniqueCameraModel,
						 fProfile.UniqueCameraModelRestriction ())

	{
	
	
	
	dng_host host;
	
	fProfileTagSet.Reset (new profile_tag_set (host, *this, fProfile));
	
	

	if (includeModelRestriction)
		{
		
		if (fProfile.UniqueCameraModelRestriction ().NotEmpty ())
			{
			
			Add (&fCameraModelTag);
			
			}
					
		}
	
	}

tiff_dng_extended_color_profile::~tiff_dng_extended_color_profile ()
	{
		
	}

void tiff_dng_extended_color_profile::Put (dng_host & ,
										   dng_stream &stream)
	{
	
	

	stream.Put_uint16 (stream.BigEndian () ? byteOrderMM : byteOrderII);

	stream.Put_uint16 (magicExtendedProfile);

	stream.Put_uint32 (8);
	
	

	dng_tiff_directory::Put (stream, offsetsRelativeToExplicitBase, 8);

	}

uint64 tiff_dng_extended_color_profile::DataSize ()
	{
	
	return 8 + Size ();
	
	}
	

tag_dng_noise_profile::tag_dng_noise_profile (const dng_noise_profile &profile)

	:	tag_data_ptr (tcNoiseProfile,
					  ttDouble,
					  2 * profile.NumFunctions (),
					  fValues)

	{

	DNG_REQUIRE (profile.NumFunctions () <= kMaxColorPlanes,
				 "Too many noise functions in tag_dng_noise_profile.");

	for (uint32 i = 0; i < profile.NumFunctions (); i++)
		{

		fValues [(2 * i)	] = profile.NoiseFunction (i).Scale	 ();
		fValues [(2 * i) + 1] = profile.NoiseFunction (i).Offset ();

		}
	
	}
		

class big_table_tag_set
	{
	
	private:
	
		const dng_big_table_dictionary &fDictionary;
	
		const dng_big_table_group_index &fGroupIndex;
	
		tag_data_ptr fBigTableDigests;
		tag_data_ptr fBigTableOffsets;
		tag_data_ptr fBigTableByteCounts;
		tag_data_ptr fBigTableGroupIndex;
		
		AutoPtr<dng_memory_block> fDigestsBuffer;
		AutoPtr<dng_memory_block> fOffsetsBuffer;
		AutoPtr<dng_memory_block> fByteCountsBuffer;
		AutoPtr<dng_memory_block> fGroupBuffer;

	public:
	
		big_table_tag_set (dng_host &host,
						   dng_tiff_directory &directory,
						   const dng_big_table_dictionary &dictionary,
						   const dng_big_table_group_index &groupIndex);
						   
		void WriteData (dng_stream &stream);
		
		uint64 DataSize ();
		
	};
	

big_table_tag_set::big_table_tag_set (dng_host &host,
									  dng_tiff_directory &directory,
									  const dng_big_table_dictionary &dictionary,
									  const dng_big_table_group_index &groupIndex)
									  
	:	fDictionary (dictionary)

	,	fGroupIndex (groupIndex)
	
	,	fBigTableDigests (tcBigTableDigests,
						  ttByte,
						  0,
						  nullptr)
	
	,	fBigTableOffsets (tcBigTableOffsets,
						  ttLong,
						  0,
						  nullptr)
	
	,	fBigTableByteCounts (tcBigTableByteCounts,
							 ttLong,
							 0,
							 nullptr)
							 
	,	fBigTableGroupIndex (tcBigTableGroupIndex,
							 ttByte,
							 0,
							 nullptr)
							 
	{
	
	if (!fDictionary.IsEmpty ())
		{
		
		const size_t count64 = fDictionary.Map ().size ();

		if (count64 > size_t (0xFFFFFFFFu))
			{
			ThrowProgramError ("Too many BigTable entries");
			}

		uint32 count = (uint32) count64;

		const uint32 digestBytes = SafeUint32Mult (count, 16);
		
		fDigestsBuffer.Reset (host.Allocate (digestBytes));
		
		fBigTableDigests.SetCount (digestBytes);
		fBigTableDigests.SetData  (fDigestsBuffer->Buffer_uint8 ());
		
		directory.Add (&fBigTableDigests);

		const uint32 indexBytes = SafeUint32Mult (count,
												  (uint32) sizeof (uint32));
		
		fOffsetsBuffer.Reset (host.Allocate (indexBytes));
		
		fBigTableOffsets.SetCount (count);
		fBigTableOffsets.SetData  (fOffsetsBuffer->Buffer_uint32 ());
		
		directory.Add (&fBigTableOffsets);
		
		fByteCountsBuffer.Reset (host.Allocate (indexBytes));
		
		fBigTableByteCounts.SetCount (count);
		fBigTableByteCounts.SetData	 (fByteCountsBuffer->Buffer_uint32 ());
	
		directory.Add (&fBigTableByteCounts);

		

		if (!groupIndex.IsEmpty ())
			{

			const size_t groupCount64 = groupIndex.Map ().size ();

			if (groupCount64 > size_t (0xFFFFFFFFu))
				{
				ThrowProgramError ("Too many BigTable groups");
				}

			const uint32 groups = (uint32) groupCount64;

			constexpr uint32 bytesPerGroup = 2 * 16;

			const uint32 totalBytes = SafeUint32Mult (groups, bytesPerGroup);
			
			fGroupBuffer.Reset (host.Allocate (totalBytes));

			fBigTableGroupIndex.SetCount (totalBytes);
			fBigTableGroupIndex.SetData	 (fGroupBuffer->Buffer_uint8 ());

			directory.Add (&fBigTableGroupIndex);
			
			}

		}
	
	}
	

void big_table_tag_set::WriteData (dng_stream &stream)
	{
	
	if (!fDictionary.IsEmpty ())
		{
		
		uint32 index = 0;
		
		for (auto it = fDictionary.Map ().cbegin ();
			 it != fDictionary.Map ().cend ();
			 ++it)
			{
			
			fOffsetsBuffer->Buffer_uint32 () [index] = (uint32) stream.Position ();
			
			const dng_fingerprint &fingerprint = it->first;
			
			memcpy (fDigestsBuffer->Buffer_uint8 () + index * 16,
					fingerprint.Data (),
					16);
			
			const dng_ref_counted_block &block = it->second;
			
			uint32 count = block.LogicalSize ();
			
			fByteCountsBuffer->Buffer_uint32 () [index] = count;
			
			stream.Put (block.Buffer_uint8 (), count);
			
			stream.PadAlign2 ();
			
			index++;
			
			}

		

		if (!fGroupIndex.IsEmpty ())
			{

			uint8 *dPtr = fGroupBuffer->Buffer_uint8 ();
			
			for (const auto &group : fGroupIndex.Map ())			
				{
				
				memcpy (dPtr	 , group.first .Data (), 16);
				memcpy (dPtr + 16, group.second.Data (), 16);

				dPtr += 32;
				
				}
			
			}

		}
	
	}

uint64 big_table_tag_set::DataSize ()
	{
	
	uint64 result = 0;
	
	if (!fDictionary.IsEmpty ())
		{
		
		for (auto it = fDictionary.Map ().cbegin ();
			 it != fDictionary.Map ().cend ();
			 ++it)
			{
			
			const dng_ref_counted_block &block = it->second;
			
			uint32 count = block.LogicalSize ();
			
			result += RoundUp2 (count);
			
			}
			
		}
		
	return result;
	
	}

dng_image_writer::dng_image_writer ()
	{
	
	}

dng_image_writer::~dng_image_writer ()
	{
	
	}
							

uint32 dng_image_writer::CompressedBufferSize (const dng_ifd &ifd,
											   uint32 uncompressedSize)
	{

	const dng_safe_uint32 safeUncompressedSize (uncompressedSize);
	
	switch (ifd.fCompression)
		{
		
		case ccLZW:
			{
			
			
				
			return (safeUncompressedSize * 2u + 1024u).Get ();
			
			}
			
		case ccDeflate:
			{
		
			

			const dng_safe_uint32 temp (uncompressedSize >> 8);
			
			return (safeUncompressedSize + temp + 64u).Get ();

			}
			
		case ccJPEG:
			{
			
			
			
			
			if (ifd.fBitsPerSample [0] <= 8)
				{

				return (safeUncompressedSize * 2u).Get ();
				
				}
				
			break;
	
			}
			
		default:
			break;
		
		}
	
	return 0;
	
	}
							

static void EncodeDelta8 (uint8 *dPtr,
						  uint32 rows,
						  uint32 cols,
						  uint32 channels)
	{
	
	const uint32 dRowStep = SafeUint32Mult (cols, channels);
	
	for (uint32 row = 0; row < rows; row++)
		{
		
		for (uint32 col = cols - 1; col > 0; col--)
			{
			
			for (uint32 channel = 0; channel < channels; channel++)
				{
				
				dPtr [col * channels + channel] -= dPtr [(col - 1) * channels + channel];
				
				}
			
			}
		
		dPtr += dRowStep;
		
		}

	}

static void EncodeDelta16 (uint16 *dPtr,
						   uint32 rows,
						   uint32 cols,
						   uint32 channels)
	{
	
	const uint32 dRowStep = SafeUint32Mult (cols, channels);
	
	for (uint32 row = 0; row < rows; row++)
		{
		
		for (uint32 col = cols - 1; col > 0; col--)
			{
			
			for (uint32 channel = 0; channel < channels; channel++)
				{
				
				dPtr [col * channels + channel] -= dPtr [(col - 1) * channels + channel];
				
				}
			
			}
		
		dPtr += dRowStep;
		
		}

	}
	

static void EncodeDelta32 (uint32 *dPtr,
						   uint32 rows,
						   uint32 cols,
						   uint32 channels)
	{
	
	const uint32 dRowStep = SafeUint32Mult (cols, channels);
	
	for (uint32 row = 0; row < rows; row++)
		{
		
		for (uint32 col = cols - 1; col > 0; col--)
			{
			
			for (uint32 channel = 0; channel < channels; channel++)
				{
				
				dPtr [col * channels + channel] -= dPtr [(col - 1) * channels + channel];
				
				}
			
			}
		
		dPtr += dRowStep;
		
		}

	}
	

inline void EncodeDeltaBytes (uint8 *bytePtr, int32 cols, int32 channels)
	{
	
	if (cols <= 0 || channels <= 0)
		{
		ThrowProgramError ("Invalid EncodeDeltaBytes dimensions");
		}

	const int32 rowBytes = SafeInt32Mult (cols, channels);

	if (channels == 1)
		{
		
		bytePtr += (rowBytes - 1);
		
		uint8 this0 = bytePtr [0];
		
		for (int32 col = 1; col < cols; col++)
			{
			
			uint8 prev0 = bytePtr [-1];
			
			this0 -= prev0;
			
			bytePtr [0] = this0;
			
			this0 = prev0;
			
			bytePtr -= 1;

			}
	
		}
		
	else if (channels == 3)
		{
		
		bytePtr += (rowBytes - channels);
		
		uint8 this0 = bytePtr [0];
		uint8 this1 = bytePtr [1];
		uint8 this2 = bytePtr [2];
		
		for (int32 col = 1; col < cols; col++)
			{
			
			uint8 prev0 = bytePtr [-3];
			uint8 prev1 = bytePtr [-2];
			uint8 prev2 = bytePtr [-1];
			
			this0 -= prev0;
			this1 -= prev1;
			this2 -= prev2;
			
			bytePtr [0] = this0;
			bytePtr [1] = this1;
			bytePtr [2] = this2;
			
			this0 = prev0;
			this1 = prev1;
			this2 = prev2;
			
			bytePtr -= 3;

			}
	
		}
		
	else
		{
	
		bytePtr += rowBytes - 1;
		
		for (int32 col = channels; col < rowBytes; col++)
			{
			
			bytePtr [0] -= bytePtr [-channels];
				
			bytePtr--;

			}
			
		}

	}

static void EncodeFPDelta (uint8 *buffer,
						   uint8 *temp,
						   int32 cols,
						   int32 channels,
						   int32 bytesPerSample)
	{
	
	int32 rowIncrement = SafeInt32Mult (cols, channels);
	
	if (bytesPerSample == 2)
		{
		
		const uint8 *src = buffer;
		
		#if qDNGBigEndian
		uint8 *dst0 = temp;
		uint8 *dst1 = temp + rowIncrement;
		#else
		uint8 *dst1 = temp;
		uint8 *dst0 = temp + rowIncrement;
		#endif
				
		for (int32 col = 0; col < rowIncrement; ++col)
			{
			
			dst0 [col] = src [0];
			dst1 [col] = src [1];
			
			src += 2;
			
			}
			
		}
		
	else if (bytesPerSample == 3)
		{
		
		const uint8 *src = buffer;
		
		uint8 *dst0 = temp;
		uint8 *dst1 = temp + rowIncrement;
		uint8 *dst2 = temp + rowIncrement * 2;
				
		for (int32 col = 0; col < rowIncrement; ++col)
			{
			
			dst0 [col] = src [0];
			dst1 [col] = src [1];
			dst2 [col] = src [2];
			
			src += 3;
			
			}
			
		}
		
	else
		{
		
		const uint8 *src = buffer;
		
		#if qDNGBigEndian
		uint8 *dst0 = temp;
		uint8 *dst1 = temp + rowIncrement;
		uint8 *dst2 = temp + rowIncrement * 2;
		uint8 *dst3 = temp + rowIncrement * 3;
		#else
		uint8 *dst3 = temp;
		uint8 *dst2 = temp + rowIncrement;
		uint8 *dst1 = temp + rowIncrement * 2;
		uint8 *dst0 = temp + rowIncrement * 3;
		#endif
				
		for (int32 col = 0; col < rowIncrement; ++col)
			{
			
			dst0 [col] = src [0];
			dst1 [col] = src [1];
			dst2 [col] = src [2];
			dst3 [col] = src [3];
			
			src += 4;
			
			}
			
		}
		
	EncodeDeltaBytes (temp, cols*bytesPerSample, channels);
	
	memcpy (buffer, temp, cols*bytesPerSample*channels);
	
	}

void dng_image_writer::EncodePredictor (dng_host &host,
										const dng_ifd &ifd,
										dng_pixel_buffer &buffer,
										AutoPtr<dng_memory_block> &tempBuffer)
	{
	
	switch (ifd.fPredictor)
		{
		
		case cpHorizontalDifference:
		case cpHorizontalDifferenceX2:
		case cpHorizontalDifferenceX4:
			{
			
			int32 xFactor = 1;
			
			if (ifd.fPredictor == cpHorizontalDifferenceX2)
				{
				xFactor = 2;
				}
				
			else if (ifd.fPredictor == cpHorizontalDifferenceX4)
				{
				xFactor = 4;
				}
			
			switch (buffer.fPixelType)
				{
				
				case ttByte:
					{
					
					EncodeDelta8 ((uint8 *) buffer.fData,
								  buffer.fArea.H (),
								  buffer.fArea.W () / xFactor,
								  buffer.fPlanes	* xFactor);
					
					return;
					
					}
					
				case ttShort:
					{
					
					EncodeDelta16 ((uint16 *) buffer.fData,
								   buffer.fArea.H (),
								   buffer.fArea.W () / xFactor,
								   buffer.fPlanes	 * xFactor);
					
					return;
					
					}
					
				case ttLong:
					{
					
					EncodeDelta32 ((uint32 *) buffer.fData,
								   buffer.fArea.H (),
								   buffer.fArea.W () / xFactor,
								   buffer.fPlanes	 * xFactor);
					
					return;
					
					}
					
				default:
					break;
					
				}
			
			break;
			
			}
			
		case cpFloatingPoint:
		case cpFloatingPointX2:
		case cpFloatingPointX4:
			{
			
			int32 xFactor = 1;
			
			if (ifd.fPredictor == cpFloatingPointX2)
				{
				xFactor = 2;
				}
				
			else if (ifd.fPredictor == cpFloatingPointX4)
				{
				xFactor = 4;
				}
			
			if (buffer.fRowStep < 0)
				{
				ThrowProgramError ("Row step may not be negative");
				}

			dng_safe_uint32 tempBufferSize =
				dng_safe_uint32 (buffer.fPixelSize) * 
				static_cast<uint32> (buffer.fRowStep);

			if (!tempBuffer.Get () || 
				tempBuffer->LogicalSize () < tempBufferSize.Get ())
				{
				
				tempBuffer.Reset (host.Allocate (tempBufferSize.Get ()));
				
				}
				
			for (int32 row = buffer.fArea.t; row < buffer.fArea.b; row++)
				{
				
				EncodeFPDelta ((uint8 *) buffer.DirtyPixel (row, buffer.fArea.l, buffer.fPlane),
							   tempBuffer->Buffer_uint8 (),
							   buffer.fArea.W () / xFactor,
							   buffer.fPlanes	 * xFactor,
							   buffer.fPixelSize);
				
				}
			
			return;
			
			}
			
		default:
			break;
		
		}
	
	if (ifd.fPredictor != cpNullPredictor)
		{
		
		ThrowProgramError ();
		
		}
	
	}
							

void dng_image_writer::ByteSwapBuffer (dng_host & ,
									   dng_pixel_buffer &buffer)
	{

	
	

	DNG_REQUIRE (buffer.fArea.H () == 0 || buffer.fRowStep > 0,
				 "buffer.fRowStep");

	uint32 pixels = SafeUint32Mult ((uint32) buffer.fRowStep,
									buffer.fArea.H ());

	switch (buffer.fPixelSize)
		{
		
		case 2:
			{
			
			DoSwapBytes16 ((uint16 *) buffer.fData,
						   pixels);
						   
			break;
			
			}
			
		case 4:
			{
			
			DoSwapBytes32 ((uint32 *) buffer.fData,
						   pixels);
						   
			break;
			
			}
			
		default:
			break;
			
		}

	}
							

void dng_image_writer::ReorderSubTileBlocks (const dng_ifd &ifd,
											 dng_pixel_buffer &buffer,
											 AutoPtr<dng_memory_block> &uncompressedBuffer,
											 AutoPtr<dng_memory_block> &subTileBlockBuffer)
	{
	
	uint32 blockRows = ifd.fSubTileBlockRows;
	uint32 blockCols = ifd.fSubTileBlockCols;
	
	uint32 rowBlocks = buffer.fArea.H () / blockRows;
	uint32 colBlocks = buffer.fArea.W () / blockCols;
	
	int32 rowStep = SafeInt32Mult (buffer.fRowStep,
								   (int32) buffer.fPixelSize);
	
	int32 colStep = SafeInt32Mult (buffer.fColStep,
								   (int32) buffer.fPixelSize);
	
	int32 rowBlockStep = SafeInt32Mult (rowStep, (int32) blockRows);
	int32 colBlockStep = SafeInt32Mult (colStep, (int32) blockCols);
	
	uint32 blockColBytes = SafeUint32Mult (blockCols,
										   buffer.fPlanes,
										   buffer.fPixelSize);
	
	const uint8 *s0 = uncompressedBuffer->Buffer_uint8 ();
		  uint8 *d0 = subTileBlockBuffer->Buffer_uint8 ();
	
	for (uint32 rowBlock = 0; rowBlock < rowBlocks; rowBlock++)
		{
		
		const uint8 *s1 = s0;
		
		for (uint32 colBlock = 0; colBlock < colBlocks; colBlock++)
			{
			
			const uint8 *s2 = s1;
			
			for (uint32 blockRow = 0; blockRow < blockRows; blockRow++)
				{
				
				for (uint32 j = 0; j < blockColBytes; j++)
					{
					
					d0 [j] = s2 [j];
					
					}
					
				d0 += blockColBytes;
				
				s2 += rowStep;
				
				}
			
			s1 += colBlockStep;
			
			}
			
		s0 += rowBlockStep;
		
		}
		
	
		
	DoCopyBytes (subTileBlockBuffer->Buffer		 (),
				 uncompressedBuffer->Buffer		 (),
				 uncompressedBuffer->LogicalSize ());
	
	}
							

class dng_lzw_compressor: private dng_uncopyable
	{
	
	private:
	
		enum
			{
			kResetCode = 256,
			kEndCode   = 257,
			kTableSize = 4096
			};

		
		
		

		struct LZWCompressorNode
			{
			int16 final;
			int16 son0;
			int16 son1;
			int16 brother;
			};
			
		dng_memory_data fBuffer;

		LZWCompressorNode *fTable;
		
		uint8 *fDstPtr;
		
		int32 fBitOffset;

		int32 fNextCode;
		
		int32 fCodeSize;
		
	public:
	
		dng_lzw_compressor ();
		
		void Compress (const uint8 *sPtr,
					   uint8 *dPtr,
					   uint32 sCount,
					   uint32 &dCount);
 
	private:
		
		void InitTable ();
	
		int32 SearchTable (int32 w, int32 k) const
			{
			
			DNG_ASSERT ((w >= 0) && (w <= kTableSize),
						"Bad w value in dng_lzw_compressor::SearchTable");
			
			int32 son0 = fTable [w] . son0;
			int32 son1 = fTable [w] . son1;
			
			
			
			
			int32 code = son0 + ((-((int32) (k & 1))) & (son1 - son0));

			while (code > 0 && fTable [code].final != k)
				{
				code = fTable [code].brother;
				}

			return code;

			}

		void AddTable (int32 w, int32 k);
		
		void PutCodeWord (int32 code);

	};

dng_lzw_compressor::dng_lzw_compressor ()

	:	fBuffer	   ()
	,	fTable	   (NULL)
	,	fDstPtr	   (NULL)
	,	fBitOffset (0)
	,	fNextCode  (0)
	,	fCodeSize  (0)
	
	{
	
	fBuffer.Allocate (kTableSize, sizeof (LZWCompressorNode));
	
	fTable = (LZWCompressorNode *) fBuffer.Buffer ();
	
	}

void dng_lzw_compressor::InitTable ()
	{

	fCodeSize = 9;

	fNextCode = 258;
		
	LZWCompressorNode *node = &fTable [0];
	
	for (int32 code = 0; code < 256; ++code)
		{
		
		node->final	  = (int16) code;
		node->son0	  = -1;
		node->son1	  = -1;
		node->brother = -1;
		
		node++;
		
		}
		
	}

void dng_lzw_compressor::AddTable (int32 w, int32 k)
	{
	
	DNG_ASSERT ((w >= 0) && (w <= kTableSize),
				"Bad w value in dng_lzw_compressor::AddTable");

	LZWCompressorNode *node = &fTable [w];

	int32 nextCode = fNextCode;

	DNG_ASSERT ((nextCode >= 0) && (nextCode <= kTableSize),
				"Bad fNextCode value in dng_lzw_compressor::AddTable");
	
	LZWCompressorNode *node2 = &fTable [nextCode];
	
	fNextCode++;
	
	int32 oldSon;
	
	if( k&1 )
		{
		oldSon = node->son1;
		node->son1 = (int16) nextCode;
		}
	else
		{
		oldSon = node->son0;
		node->son0 = (int16) nextCode;
		}
	
	node2->final   = (int16) k;
	node2->son0	   = -1;
	node2->son1	   = -1;
	node2->brother = (int16) oldSon;
	
	if (nextCode == (1 << fCodeSize) - 1)
		{
		if (fCodeSize != 12)
			fCodeSize++;
		}
		
	}

void dng_lzw_compressor::PutCodeWord (int32 code)
	{
	
	int32 bit = (int32) (fBitOffset & 7);
	
	int32 offset1 = fBitOffset >> 3;
	int32 offset2 = (fBitOffset + fCodeSize - 1) >> 3;
		
	int32 shift1 = (fCodeSize + bit) -	8;
	int32 shift2 = (fCodeSize + bit) - 16;
	
	uint8 byte1 = (uint8) (code >> shift1);
	
	uint8 *dstPtr1 = fDstPtr + offset1;
	uint8 *dstPtr3 = fDstPtr + offset2;
	
	if (offset1 + 1 == offset2)
		{
		
		uint8 byte2 = (uint8) (code << (-shift2));
		
		if (bit)
			*dstPtr1 |= byte1;
		else
			*dstPtr1 = byte1;
		
		*dstPtr3 = byte2;
		
		}

	else
		{
		
		int32 shift3 = (fCodeSize + bit) - 24;
		
		uint8 byte2 = (uint8) (code >> shift2);
		uint8 byte3 = (uint8) (code << (-shift3));
		
		uint8 *dstPtr2 = fDstPtr + (offset1 + 1);
		
		if (bit)
			*dstPtr1 |= byte1;
		else
			*dstPtr1 = byte1;
		
		*dstPtr2 = byte2;
		
		*dstPtr3 = byte3;
		
		}
		
	fBitOffset += fCodeSize;
	
	}

void dng_lzw_compressor::Compress (const uint8 *sPtr,
								   uint8 *dPtr,
								   uint32 sCount,
								   uint32 &dCount)
	{
	
	fDstPtr = dPtr;
	
	fBitOffset = 0;
	
	InitTable ();
	
	PutCodeWord (kResetCode);
	
	int32 code = -1;
	
	int32 pixel;
	
	if (sCount > 0)
		{
		
		pixel = *sPtr;
		sPtr = sPtr + 1;
		code = pixel;

		sCount--;

		while (sCount--)
			{

			pixel = *sPtr;
			sPtr = sPtr + 1;
			
			int32 newCode = SearchTable (code, pixel);
			
			if (newCode == -1)
				{
				
				PutCodeWord (code);
				
				if (fNextCode < 4093)
					{
					AddTable (code, pixel);
					}
				else
					{
					PutCodeWord (kResetCode);
					InitTable ();
					}
					
				code = pixel;
				
				}
				
			else
				code = newCode;
				
			}
		
		}
		
	if (code != -1)
		{
		PutCodeWord (code);
		AddTable (code, 0);
		}
		
	PutCodeWord (kEndCode);

	dCount = (fBitOffset + 7) >> 3;

	}

#if qDNGUseLibJPEG

static void dng_error_exit (j_common_ptr cinfo)
	{
	
	
	
	(*cinfo->err->output_message) (cinfo);
	
	

	switch (cinfo->err->msg_code)
		{
		
		case JERR_OUT_OF_MEMORY:
			{
			ThrowMemoryFull ();
			break;
			}
			
		default:
			{
			ThrowBadFormat ();
			}
			
		}
			
	}

static void dng_output_message (j_common_ptr cinfo)
	{
	
	
	
	char buffer [JMSG_LENGTH_MAX];

	(*cinfo->err->format_message) (cinfo, buffer);
	
	
	
	ReportWarning ("libjpeg", buffer);

	}

struct dng_jpeg_stream_dest
	{
	
	struct jpeg_destination_mgr pub;
	
	dng_stream *fStream;
	
	uint8 fBuffer [4096];
	
	};

static void dng_init_destination (j_compress_ptr cinfo)
	{
	
	dng_jpeg_stream_dest *dest = (dng_jpeg_stream_dest *) cinfo->dest;

	dest->pub.next_output_byte = dest->fBuffer;
	dest->pub.free_in_buffer   = sizeof (dest->fBuffer);
	
	}

static boolean dng_empty_output_buffer (j_compress_ptr cinfo)
	{
	
	dng_jpeg_stream_dest *dest = (dng_jpeg_stream_dest *) cinfo->dest;
	
	dest->fStream->Put (dest->fBuffer, sizeof (dest->fBuffer));

	dest->pub.next_output_byte = dest->fBuffer;
	dest->pub.free_in_buffer   = sizeof (dest->fBuffer);

	return TRUE;
	
	}

static void dng_term_destination (j_compress_ptr cinfo)
	{
	
	dng_jpeg_stream_dest *dest = (dng_jpeg_stream_dest *) cinfo->dest;
	
	uint32 datacount = sizeof (dest->fBuffer) -
					   (uint32) dest->pub.free_in_buffer;
	
	if (datacount)
		{
		dest->fStream->Put (dest->fBuffer, datacount);
		}

	}

static void jpeg_set_adobe_quality (struct jpeg_compress_struct *cinfo,
									int32 quality)
	{
	
	
		
	if (quality < 0 || quality > 12)
		{
		quality = 10;
		}
		
	
	
	bool useChromaDownsampling = (quality <= 6);
		
	
	
	const int kLibJPEGQuality [13] =
		{
		5, 11, 23, 34, 46, 63, 76, 77, 86, 90, 94, 97, 99
		};
		
	quality = kLibJPEGQuality [quality];
	
	jpeg_set_quality (cinfo, quality, TRUE);
	
	
	
	
	if (!useChromaDownsampling)
		{
		
		cinfo->comp_info [0].h_samp_factor = 1;
		cinfo->comp_info [0].h_samp_factor = 1;
		
		}
				
	}

#endif	

void dng_image_writer::WriteData (dng_host &host,
								  const dng_ifd &ifd,
								  dng_stream &stream,
								  dng_pixel_buffer &buffer,
								  AutoPtr<dng_memory_block> &compressedBuffer,
								  bool usingMultipleThreads)
	{

	
	

	DNG_REQUIRE (buffer.fArea.H () == 0 || buffer.fRowStep > 0,
				 "buffer.fRowStep");

	(void) usingMultipleThreads;
	
	switch (ifd.fCompression)
		{
		
		case ccUncompressed:
			{
			
			
			
			
			if (ifd.fBitsPerSample [0] == 8 && buffer.fPixelType == ttShort)
				{
				
				uint32 count = SafeUint32Mult ((uint32) buffer.fRowStep,
											   buffer.fArea.H ());
							   
				const uint16 *sPtr = (const uint16 *) buffer.fData;
				
				for (uint32 j = 0; j < count; j++)
					{
					
					stream.Put_uint8 ((uint8) sPtr [j]);
					
					}
				
				}
				
			else
				{
	
				
				
				if (stream.SwapBytes ())
					{
					
					ByteSwapBuffer (host, buffer);
										
					}
			
				
				
				stream.Put (buffer.fData,
							SafeUint32Mult ((uint32) buffer.fRowStep,
											buffer.fArea.H (),
											buffer.fPixelSize));
										  
				}
			
			break;
			
			}
			
		case ccLZW:
		case ccDeflate:
			{
			
			
			
			
			
			if (stream.SwapBytes () && ifd.fPredictor != cpFloatingPoint)
				{
				
				ByteSwapBuffer (host,
								buffer);
								
				}
			
			
				
			uint32 sBytes = SafeUint32Mult ((uint32) buffer.fRowStep,
											buffer.fArea.H (),
											buffer.fPixelSize);
				
			uint8 *sBuffer = (uint8 *) buffer.fData;
				
			uint32 dBytes = 0;
				
			uint8 *dBuffer = compressedBuffer->Buffer_uint8 ();
			
			if (ifd.fCompression == ccLZW)
				{
				
				dng_lzw_compressor lzwCompressor;
				
				lzwCompressor.Compress (sBuffer,
										dBuffer,
										sBytes,
										dBytes);
										
				}
				
			else
				{
				
				uLongf dCount = compressedBuffer->LogicalSize ();
				
				int32 level = Z_DEFAULT_COMPRESSION;
				
				if (ifd.fCompressionQuality >= Z_BEST_SPEED &&
					ifd.fCompressionQuality <= Z_BEST_COMPRESSION)
					{
					
					level = ifd.fCompressionQuality;
					
					}
				
				int zResult = ::compress2 (dBuffer,
										   &dCount,
										   sBuffer,
										   sBytes,
										   level);
										  
				if (zResult != Z_OK)
					{
					
					ThrowMemoryFull ();
					
					}

				dBytes = (uint32) dCount;
				
				}
										
			if (dBytes > compressedBuffer->LogicalSize ())
				{
				
				ThrowOverflow ("Compression output buffer overflow");
				
				}
										
			stream.Put (dBuffer, dBytes);
				
			return;

			}
			
		case ccJPEG:
			{
			
			dng_pixel_buffer temp (buffer);
				
			if (buffer.fPixelType == ttByte)
				{
				
				
				
				
				temp.fData = compressedBuffer->Buffer ();
				
				temp.fPixelType = ttShort;
				temp.fPixelSize = 2;
				
				temp.CopyArea (buffer,
							   buffer.fArea,
							   buffer.fPlane,
							   buffer.fPlanes);
				
				}

			DoEncodeLosslessJPEG ((const uint16 *) temp.fData,
								  temp.fArea.H (),
								  temp.fArea.W (),
								  temp.fPlanes,
								  ifd.fBitsPerSample [0],
								  temp.fRowStep,
								  temp.fColStep,
								  stream);
										
			break;
			
			}
			
		#if qDNGUseLibJPEG
		
		case ccLossyJPEG:
			{
			
			struct jpeg_compress_struct cinfo;
	
			
			
			struct jpeg_error_mgr jerr;

			cinfo.err = jpeg_std_error (&jerr);
			
			jerr.error_exit		= dng_error_exit;
			jerr.output_message = dng_output_message;
	
			try
				{
				
				

				jpeg_create_compress (&cinfo);
				
				
				
				dng_jpeg_stream_dest dest;
				
				dest.fStream = &stream;
				
				dest.pub.init_destination	 = dng_init_destination;
				dest.pub.empty_output_buffer = dng_empty_output_buffer;
				dest.pub.term_destination	 = dng_term_destination;
				
				cinfo.dest = &dest.pub;
				
				
				
				cinfo.image_width	   = buffer.fArea.W ();
				cinfo.image_height	   = buffer.fArea.H ();
				cinfo.input_components = buffer.fPlanes;
				
				switch (buffer.fPlanes)
					{
					
					case 1:
						cinfo.in_color_space = JCS_GRAYSCALE;
						break;
						
					case 3:
						cinfo.in_color_space = JCS_RGB;
						break;
						
					case 4:
						cinfo.in_color_space = JCS_CMYK;
						break;
						
					default:
						ThrowProgramError ();
						
					}
					
				

				jpeg_set_defaults (&cinfo);
				
				jpeg_set_adobe_quality (&cinfo, ifd.fCompressionQuality);
				
				
				
				jpeg_start_compress (&cinfo, TRUE);
				
				
				
				for (int32 row = buffer.fArea.t; row < buffer.fArea.b; row++)
					{
					
					uint8 *sampArray [1];
		
					sampArray [0] = buffer.DirtyPixel_uint8 (row,
															 buffer.fArea.l,
															 0);

					jpeg_write_scanlines (&cinfo, sampArray, 1);
					
					}

				
					
				jpeg_finish_compress (&cinfo);

				jpeg_destroy_compress (&cinfo);
					
				}
				
			catch (...)
				{
				
				jpeg_destroy_compress (&cinfo);
				
				throw;
				
				}
				
			return;
			
			}
			
		#endif	

		case ccJXL:
			{

			dng_jxl_encode_settings settings;

			if (ifd.fJXLEncodeSettings)
				settings = *ifd.fJXLEncodeSettings;

			else if (host.JXLEncodeSettings ())
				settings = *host.JXLEncodeSettings ();

			else
				{
				DNG_REPORT ("No JXLEncodeSettings, encoding tile using default settings");
				}

			#if 0
			printf ("jxl encode tile: distance=%0.2f, effort=%d\n",
					settings.Distance (),
					settings.Effort ());
			#endif

			
			
			

			settings.SetSingleThread (usingMultipleThreads);

			

			dng_jxl_color_space_info colorSpaceInfoLocal;

			const dng_jxl_color_space_info *colorSpaceInfoPtr =
				&colorSpaceInfoLocal;

			
			
			if (host.JXLColorSpaceInfo ())
				{
				
				colorSpaceInfoPtr = host.JXLColorSpaceInfo ();

				}

			
			
			else if (ifd.fJXLColorEncoding)
				{
				
				colorSpaceInfoLocal.fJxlColorEncoding.Reset
					(new JxlColorEncoding (*ifd.fJXLColorEncoding));

				}

			else
				{
				
				PreviewColorSpaceToJXLEncoding (ifd.fPreviewInfo.fColorSpace,
												ifd.fSamplesPerPixel,
												colorSpaceInfoLocal);
				
				}

			#if 0
			printf ("jxl: strips=%s, tiles=%s, ta=%u, td=%u\n",
					ifd.fUsesStrips ? "yes" : "no",
					ifd.fUsesTiles ? "yes" : "no",
					ifd.TilesAcross (),
					ifd.TilesDown ());
			#endif

			
			
			
			

			if (ifd.TilesAcross () == 1 &&
				ifd.TilesDown   () == 1 &&
				(uint32 (ifd.fPreviewInfo.fColorSpace) <=
				 uint32 (previewColorSpace_LastValid)))
				{

				#if 0
				printf ("--- writing JXL container\n");
				#endif

				EncodeJXL_Container (host,
									 stream,
									 buffer,
									 settings,
									 *colorSpaceInfoPtr,
									 nullptr,
									 false,
									 false,
									 false,
									 nullptr);

				}

			
			

			else
				{

				EncodeJXL_Tile (host,
								stream,
								buffer,
								*colorSpaceInfoPtr,
								settings);

				}
			
			return;
			
			}

		default:
			{
			
			ThrowProgramError ();
			
			}
			
		}
	
	}
							

void dng_image_writer::EncodeJPEGPreview (dng_host &host,
										  const dng_image &image,
										  dng_jpeg_preview &preview,
										  int32 quality)
	{
	
	#if qDNGUseLibJPEG
		
	dng_memory_stream stream (host.Allocator ());
	
	struct jpeg_compress_struct cinfo;

	
	
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error (&jerr);
	
	jerr.error_exit		= dng_error_exit;
	jerr.output_message = dng_output_message;

	try
		{
		
		

		jpeg_create_compress (&cinfo);
		
		
		
		dng_jpeg_stream_dest dest;
		
		dest.fStream = &stream;
		
		dest.pub.init_destination	 = dng_init_destination;
		dest.pub.empty_output_buffer = dng_empty_output_buffer;
		dest.pub.term_destination	 = dng_term_destination;
		
		cinfo.dest = &dest.pub;
		
		
		
		cinfo.image_width	   = image.Bounds ().W ();
		cinfo.image_height	   = image.Bounds ().H ();
		cinfo.input_components = image.Planes ();
		
		switch (image.Planes ())
			{
			
			case 1:
				cinfo.in_color_space = JCS_GRAYSCALE;
				break;
				
			case 3:
				cinfo.in_color_space = JCS_RGB;
				break;
				
			default:
				ThrowProgramError ();
				
			}
			
		

		jpeg_set_defaults (&cinfo);
		
		jpeg_set_adobe_quality (&cinfo, quality);
		
		
		
		preview.SetIFDInfo (host, image);
	
		if (image.Planes () == 3)
			{
			
			preview.SetYCbCr (cinfo.comp_info [0].h_samp_factor,
							  cinfo.comp_info [0].v_samp_factor);
			
			}
		
		
		
		jpeg_start_compress (&cinfo, TRUE);
		
		
		
		dng_pixel_buffer buffer (image.Bounds (), 
								 0, 
								 image.Planes (), 
								 ttByte,
								 pcInterleaved, 
								 NULL);
		
		AutoPtr<dng_memory_block> bufferData (host.Allocate (buffer.fRowStep));
		
		buffer.fData = bufferData->Buffer ();
		
		for (uint32 row = 0; row < cinfo.image_height; row++)
			{
			
			buffer.fArea.t = row;
			buffer.fArea.b = row + 1;
			
			image.Get (buffer);
			
			uint8 *sampArray [1];

			sampArray [0] = buffer.DirtyPixel_uint8 (row,
													 buffer.fArea.l,
													 0);

			jpeg_write_scanlines (&cinfo, sampArray, 1);
			
			}

		
			
		jpeg_finish_compress (&cinfo);

		jpeg_destroy_compress (&cinfo);
			
		}
		
	catch (...)
		{
		
		jpeg_destroy_compress (&cinfo);
		
		throw;
		
		}
		
	AutoPtr<dng_memory_block> compressedData (stream.AsMemoryBlock (host.Allocator ()));
				   
	preview.SetCompressedData (compressedData);

	#else
	
	(void) host;
	(void) image;
	(void) preview;
	(void) quality;
	
	ThrowProgramError ("No JPEG encoder");
	
	#endif
		
	}
								

void dng_image_writer::WriteTile (dng_host &host,
								  const dng_ifd &ifd,
								  dng_stream &stream,
								  const dng_image &image,
								  const dng_rect &tileArea,
								  uint32 fakeChannels,
								  AutoPtr<dng_memory_block> &compressedBuffer,
								  AutoPtr<dng_memory_block> &uncompressedBuffer,
								  AutoPtr<dng_memory_block> &subTileBlockBuffer,
								  AutoPtr<dng_memory_block> &tempBuffer,
								  bool usingMultipleThreads)
	{
	
	
	
	dng_pixel_buffer buffer (tileArea, 
							 0, 
							 ifd.fSamplesPerPixel,
							 image.PixelType (), 
							 pcInterleaved, 
							 uncompressedBuffer->Buffer ());
							 
	
	
	bool isLossyCompression = (ifd.fCompression == ccLossyJPEG) ||
							  (ifd.fCompression == ccJXL &&
									(!ifd.fJXLEncodeSettings ||
									  ifd.fJXLEncodeSettings->Distance () != 0.0f));
	
	

	image.Get (buffer, isLossyCompression ? dng_image::edge_repeat
										  : dng_image::edge_zero);
	
	
	
	if (ifd.fSubTileBlockRows > 1)
		{
		
		ReorderSubTileBlocks (ifd,
							  buffer,
							  uncompressedBuffer,
							  subTileBlockBuffer);
		
		}
		
	
	
	if (ifd.fSampleFormat [0] == sfFloatingPoint)
		{
		
		if (ifd.fBitsPerSample [0] == 16)
			{
			
			uint32 *srcPtr = (uint32 *) buffer.fData;
			uint16 *dstPtr = (uint16 *) buffer.fData;
			
			uint32 pixels = SafeUint32Mult (tileArea.W (),
											tileArea.H (),
											buffer.fPlanes);
			
			for (uint32 j = 0; j < pixels; j++)
				{
				
				dstPtr [j] = DNG_FloatToHalf (srcPtr [j]);
				
				}
				
			buffer.fPixelSize = 2;
			
			}
			
		if (ifd.fBitsPerSample [0] == 24)
			{
			
			uint32 *srcPtr = (uint32 *) buffer.fData;
			uint8  *dstPtr = (uint8	 *) buffer.fData;
			
			uint32 pixels = SafeUint32Mult (tileArea.W (),
											tileArea.H (),
											buffer.fPlanes);
			
			if (stream.BigEndian () || ifd.fPredictor == cpFloatingPoint   ||
									   ifd.fPredictor == cpFloatingPointX2 ||
									   ifd.fPredictor == cpFloatingPointX4)
				{
			
				for (uint32 j = 0; j < pixels; j++)
					{
					
					DNG_FloatToFP24 (srcPtr [j], dstPtr);
					
					dstPtr += 3;
					
					}
					
				}
				
			else
				{
			
				for (uint32 j = 0; j < pixels; j++)
					{
					
					uint8 output [3];
					
					DNG_FloatToFP24 (srcPtr [j], output);
					
					dstPtr [0] = output [2];
					dstPtr [1] = output [1];
					dstPtr [2] = output [0];
					
					dstPtr += 3;
					
					}
					
				}
				
			buffer.fPixelSize = 3;
			
			}
		
		}
	
	
	
	EncodePredictor (host,
					 ifd,
					 buffer,
					 tempBuffer);
		
	
	
	if (fakeChannels > 1)
		{
		
		buffer.fPlanes	*= fakeChannels;
		buffer.fColStep *= fakeChannels;
		
		buffer.fArea.r = buffer.fArea.l + (buffer.fArea.W () / fakeChannels);
		
		}
		
	
	
	WriteData (host,
			   ifd,
			   stream,
			   buffer,
			   compressedBuffer,
			   usingMultipleThreads);
			   
	}

dng_write_tiles_task::dng_write_tiles_task 
	(dng_image_writer &imageWriter,
	 dng_host &host,
	 const dng_ifd &ifd,
	 dng_basic_tag_set &basic,
	 dng_stream &stream,
	 const dng_image &image,
	 uint32 fakeChannels,
	 uint32 tilesDown,
	 uint32 tilesAcross,
	 uint32 compressedSize,
	 uint32 uncompressedSize,
	 bool needDigest)

	:	dng_area_task ("dng_write_tiles_task")

	,	fImageWriter	  (imageWriter)
	,	fHost			  (host)
	,	fIFD			  (ifd)
	,	fBasic			  (basic)
	,	fStream			  (stream)
	,	fImage			  (image)
	,	fFakeChannels	  (fakeChannels)
	,	fTilesDown		  (tilesDown)
	,	fTilesAcross	  (tilesAcross)
	,	fTileCount		  (SafeUint32Mult (tilesDown, tilesAcross))
	,	fCompressedSize	  (compressedSize)
	,	fUncompressedSize (uncompressedSize)
	,	fNextTileIndex	  (0)
	,	fMutex			  ("dng_write_tiles_task")
	,	fCondition		  ()
	,	fTaskFailed		  (false)
	,	fWriteTileIndex	  (0)
	,	fNeedDigest		  (needDigest)

	{

	fMinTaskArea = 16 * 16;
	fUnitCell	 = dng_point (16, 16);
	fMaxTileSize = dng_point (16, 16);

	}

	
void dng_write_tiles_task::Process (uint32 ,
									const dng_rect & ,
									dng_abort_sniffer *sniffer)
	{

	try
		{

		AutoPtr<dng_memory_block> compressedBuffer;
		AutoPtr<dng_memory_block> uncompressedBuffer;
		AutoPtr<dng_memory_block> subTileBlockBuffer;
		AutoPtr<dng_memory_block> tempBuffer;

		if (fCompressedSize)
			{
			compressedBuffer.Reset (fHost.Allocate (fCompressedSize));
			}

		if (fUncompressedSize)
			{
			uncompressedBuffer.Reset (fHost.Allocate (fUncompressedSize));
			}

		if (fIFD.fSubTileBlockRows > 1 && fUncompressedSize)
			{
			subTileBlockBuffer.Reset (fHost.Allocate (fUncompressedSize));
			}

		while (true)
			{

			

			

			uint32 tileIndex = fNextTileIndex++;

			if (tileIndex >= fTileCount)
				{
				return;
				}

			

			uint32 tileByteCount = 0;

			dng_memory_stream tileStream (fHost.Allocator ());

			ProcessTask (tileIndex,
						 compressedBuffer,
						 uncompressedBuffer,
						 subTileBlockBuffer,
						 tempBuffer,
						 tileByteCount,
						 tileStream,
						 sniffer);

			

			dng_fingerprint tileDigest;

			if (fNeedDigest)
				{
				
				tileStream.SetReadPosition (0);
				
				dng_md5_printer_le_stream md5stream;
				
				tileStream.CopyToStream (md5stream, tileByteCount);
				
				tileDigest = md5stream.Result ();
				
				tileStream.SetReadPosition (0);

				}

			

				{

				dng_lock_mutex lock (&fMutex);

				while (!fTaskFailed &&
					   fWriteTileIndex != tileIndex)
					{

					fCondition.Wait (fMutex);

					}

				
				

				if (fTaskFailed)
					return;

				}

			
			
			

			WriteTask (tileIndex,
					   tileByteCount,
					   tileStream,
					   sniffer);

			

			if (fNeedDigest)
				{
				
				fOverallPrinter.Process (tileDigest);

				}

			

				{

				dng_lock_mutex lock (&fMutex);

				
				

				if (fTaskFailed)
					return;

				fWriteTileIndex++;

				fCondition.Broadcast ();

				}

			}

		}

	catch (...)
		{

		

		bool needBroadcast = false;

			{

			dng_lock_mutex lock (&fMutex);

			needBroadcast = !fTaskFailed;
			fTaskFailed = true;

			}

		if (needBroadcast)
			fCondition.Broadcast ();

		throw;

		}

	}

void dng_write_tiles_task::ProcessTask 
	(uint32 tileIndex,
	 AutoPtr<dng_memory_block> &compressedBuffer,
	 AutoPtr<dng_memory_block> &uncompressedBuffer,
	 AutoPtr<dng_memory_block> &subTileBlockBuffer,
	 AutoPtr<dng_memory_block> &tempBuffer,
	 uint32 &tileByteCount, 
	 dng_memory_stream &tileStream, 
	 dng_abort_sniffer *sniffer)
	{

	

	dng_abort_sniffer::SniffForAbort (sniffer);

	
					
	uint32 rowIndex = tileIndex / fTilesAcross;
					
	uint32 colIndex = tileIndex - rowIndex * fTilesAcross;
					
	dng_rect tileArea = fIFD.TileArea (rowIndex, colIndex);
										   
	tileStream.SetLittleEndian (fStream.LittleEndian ());
					
	dng_host host (&fHost.Allocator (),
				   sniffer);

	if (fHost.JXLEncodeSettings ())
		{
		host.SetJXLEncodeSettings (*fHost.JXLEncodeSettings ());
		}

	if (fHost.JXLColorSpaceInfo ())
		{
		host.SetJXLColorSpaceInfo (fHost.ShareJXLColorSpaceInfo ());
		}

	fImageWriter.WriteTile (host,
							fIFD,
							tileStream,
							fImage,
							tileArea,
							fFakeChannels,
							compressedBuffer,
							uncompressedBuffer,
							subTileBlockBuffer,
							tempBuffer,
							true);
											
	tileStream.Flush ();
											
	tileByteCount = (uint32) tileStream.Length ();
					
	tileStream.SetReadPosition (0);
					
	}

void dng_write_tiles_task::WriteTask (uint32 tileIndex,
									  uint32 tileByteCount,
									  dng_memory_stream &tileStream,
									  dng_abort_sniffer *sniffer)
	{

	
	

	dng_abort_sniffer::SniffForAbort (sniffer);

	

	uint64 tileOffset = fStream.Position ();

	fBasic.SetTileOffset (tileIndex, tileOffset);

	

	tileStream.CopyToStream (fStream, tileByteCount);

	

	fBasic.SetTileByteCount (tileIndex, tileByteCount);

	

	if (tileByteCount & 1)
		{
		fStream.Put_uint8 (0);
		}

	}

const dng_fingerprint & dng_write_tiles_task::ResultDigest () const
	{
	
	return fOverallPrinter.Result ();
	
	}

void dng_image_writer::DoWriteTiles (dng_host &host,
									 const dng_ifd &ifd,
									 dng_basic_tag_set &basic,
									 dng_stream &stream,
									 const dng_image &image,
									 uint32 fakeChannels,
									 uint32 tilesDown,
									 uint32 tilesAcross,
									 uint32 compressedSize,
									 const dng_safe_uint32 &uncompressedSize,
									 dng_fingerprint *outDigest)
	{
	
	const uint32 tileCount = SafeUint32Mult (tilesDown, tilesAcross);

	uint32 threadCount = Min_uint32 (tileCount,
									 host.PerformAreaTaskThreads ());
										 
	dng_write_tiles_task task (*this,
							   host,
							   ifd,
							   basic,
							   stream,
							   image,
							   fakeChannels,
							   tilesDown,
							   tilesAcross,
							   compressedSize,
							   uncompressedSize.Get (),
							   (outDigest != nullptr));
								  
	host.PerformAreaTask (task,
						  dng_rect (0, 0, 16, 16 * threadCount));

	if (outDigest)
		{
		
		*outDigest = task.ResultDigest ();

		#if qDNGValidate && 0
		printf ("write digest: %s\n",
				outDigest->ToUtf8HexString ().Get ());
		#endif

		}
	
	}

void dng_image_writer::WriteImage (dng_host &host,
								   const dng_ifd &ifd,
								   dng_basic_tag_set &basic,
								   dng_stream &stream,
								   const dng_image &image,
								   uint32 fakeChannels,
								   dng_fingerprint *outDigest)
	{

	

	if (ifd.fRowInterleaveFactor	> 1 ||
		ifd.fColumnInterleaveFactor > 1)
		{

		

		AutoPtr<dng_image> tempImage (host.Make_dng_image (image.Bounds (),
														   image.Planes (),
														   image.PixelType ()));
		
		Interleave2D (host,
					  image,
					  *tempImage,
					  ifd.fRowInterleaveFactor,
					  ifd.fColumnInterleaveFactor,
					  true);
	
		

		dng_ifd tempIFD (ifd);

		tempIFD.fRowInterleaveFactor    = 1;
		tempIFD.fColumnInterleaveFactor = 1;

		WriteImage (host,
					tempIFD,
					basic,
					stream,
					*tempImage,
					fakeChannels,
					outDigest);

		return;
		
		}

	#if qDNGValidate
	
	char message [256];
	
	AutoPtr<dng_timer> timer;
	
	if (ifd.fCompression == ccJXL && ifd.fJXLEncodeSettings.get ())
		{
		
		snprintf (message,
				  sizeof (message),
				  "JXL write %u by %u pixels, distance = %.2f, effort = %d",
				  image.Width (),
				  image.Height (),
				  ifd.fJXLEncodeSettings->Distance (),
				  ifd.fJXLEncodeSettings->Effort ());
				  
		timer.Reset (new dng_timer (message));
		
		}
	
	#endif

	

	dng_safe_uint32 bytesPerSample (TagTypeSize (image.PixelType ()));

	
	
	
	
	
	
	

	if (bytesPerSample.Get () == 0)
		{
		ThrowBadFormat ("zero bytesPerSample in WriteImage");
		}

	if (ifd.fSamplesPerPixel == 0)
		{
		ThrowBadFormat ("zero fSamplesPerPixel in WriteImage");
		}

	if (ifd.fTileWidth == 0)
		{
		ThrowBadFormat ("zero fTileWidth in WriteImage");
		}

	if (ifd.fTileLength == 0)
		{
		ThrowBadFormat ("zero fTileLength in WriteImage");
		}

	if (ifd.fSubTileBlockRows == 0)
		{
		ThrowBadFormat ("zero fSubTileBlockRows in WriteImage");
		}

	dng_safe_uint32 bytesPerPixel = bytesPerSample * ifd.fSamplesPerPixel;

	dng_safe_uint32 tileRowBytes = bytesPerPixel * ifd.fTileWidth;

	
	
	
	uint32 subTileLength = ifd.fTileLength;
	
	if (ifd.TileByteCount (ifd.TileArea (0, 0)) != 0)
		{
		
		subTileLength = Pin_uint32 (ifd.fSubTileBlockRows,
									kImageBufferSize / tileRowBytes.Get (), 
									ifd.fTileLength);
					
		
		
		subTileLength = subTileLength / ifd.fSubTileBlockRows
									  * ifd.fSubTileBlockRows;
									
		}
		
	
	
	dng_safe_uint32 uncompressedSize = tileRowBytes * subTileLength;
	
	
	
	uint32 compressedSize = CompressedBufferSize (ifd, uncompressedSize.Get ());
			
	
	
	uint32 tilesAcross = ifd.TilesAcross ();
	uint32 tilesDown   = ifd.TilesDown	 ();
	uint32 tileCount   = SafeUint32Mult (tilesDown, tilesAcross);
							   
	bool useMultipleThreads = (tileCount >= 2) &&
							  (host.PerformAreaTaskThreads () > 1) &&
							  (subTileLength == ifd.fTileLength) &&
							  (ifd.fCompression != ccUncompressed);
	
	if (useMultipleThreads)
		{

		DoWriteTiles (host,
					  ifd,
					  basic,
					  stream,
					  image,
					  fakeChannels,
					  tilesDown,
					  tilesAcross,
					  compressedSize,
					  uncompressedSize.Get (),
					  outDigest);

		}
		
	else
		{
									
		AutoPtr<dng_memory_block> compressedBuffer;
		AutoPtr<dng_memory_block> uncompressedBuffer;
		AutoPtr<dng_memory_block> subTileBlockBuffer;
		AutoPtr<dng_memory_block> tempBuffer;
		
		if (compressedSize)
			{
			compressedBuffer.Reset (host.Allocate (compressedSize));
			}
		
		if (uncompressedSize.Get ())
			{
			uncompressedBuffer.Reset (host.Allocate (uncompressedSize.Get ()));
			}
		
		if (ifd.fSubTileBlockRows > 1 && uncompressedSize.Get ())
			{
			subTileBlockBuffer.Reset (host.Allocate (uncompressedSize.Get ()));
			}

		dng_md5_direct_printer overallPrinter;

		const bool needDigest = (outDigest != nullptr);
				
		
		
		uint32 tileIndex = 0;
		
		for (uint32 rowIndex = 0; rowIndex < tilesDown; rowIndex++)
			{
			
			for (uint32 colIndex = 0; colIndex < tilesAcross; colIndex++)
				{
				
				
				
				uint64 tileOffset = stream.Position ();
			
				basic.SetTileOffset (tileIndex, tileOffset);
				
				
				
				dng_rect tileArea = ifd.TileArea (rowIndex, colIndex);
				
				uint32 subTileCount = (tileArea.H () + subTileLength - 1) /
									  subTileLength;

				
				
				

				dng_memory_stream tileStream (host.Allocator ());

				auto &subStream = needDigest ? tileStream : stream;

				for (uint32 subIndex = 0; subIndex < subTileCount; subIndex++)
					{
					
					host.SniffForAbort ();
				
					dng_rect subArea (tileArea);
					
					subArea.t = tileArea.t + subIndex * subTileLength;
					
					subArea.b = Min_int32 (subArea.t + subTileLength,
										   tileArea.b);
										   
					
					
					WriteTile (host,
							   ifd,
							   subStream,
							   image,
							   subArea,
							   fakeChannels,
							   compressedBuffer,
							   uncompressedBuffer,
							   subTileBlockBuffer,
							   tempBuffer,
							   useMultipleThreads);
							   
					}

				uint64 tileByteCount;

				if (needDigest)
					{

					

					tileStream.Flush ();

					tileByteCount = tileStream.Length ();

					

					tileStream.SetReadPosition (0);
					
					dng_md5_printer_le_stream md5stream;
					
					tileStream.CopyToStream (md5stream, tileByteCount);
					
					dng_fingerprint tileDigest = md5stream.Result ();

					

					overallPrinter.Process (tileDigest);

					
					
					tileStream.SetReadPosition (0);
					
					tileStream.CopyToStream (stream, tileByteCount);

					}

				else
					{
					
					
					
					tileByteCount = stream.Position () - tileOffset;
					
					}
					
				basic.SetTileByteCount (tileIndex, tileByteCount);
				
				tileIndex++;
				
				
													 
				if (tileByteCount & 1)
					{
					stream.Put_uint8 (0);
					}
					
				} 

			} 

		

		if (outDigest)
			{

			*outDigest = overallPrinter.Result ();

			#if qDNGValidate && 0
			printf ("single-thread write digest: %s\n",
					outDigest->ToUtf8HexString ().Get ());
			#endif

			}
			
		} 
		
	}

#if qDNGUseXMP

static void CopyString (const dng_xmp &oldXMP,
						dng_xmp &newXMP,
						const char *ns,
						const char *path,
						dng_string *exif = NULL)
	{
	
	dng_string s;
	
	if (oldXMP.GetString (ns, path, s))
		{
		
		if (s.NotEmpty ())
			{
			
			newXMP.SetString (ns, path, s);
			
			if (exif)
				{
				
				*exif = s;
				
				}
			
			}
			
		}
	
	}
								 

static void CopyStringList (const dng_xmp &oldXMP,
							dng_xmp &newXMP,
							const char *ns,
							const char *path,
							bool isBag)
	{
	
	dng_string_list list;
	
	if (oldXMP.GetStringList (ns, path, list))
		{
		
		if (list.Count ())
			{
			
			newXMP.SetStringList (ns, path, list, isBag);
						
			}
			
		}
	
	}
								 

static void CopyAltLangDefault (const dng_xmp &oldXMP,
								dng_xmp &newXMP,
								const char *ns,
								const char *path,
								dng_string *exif = NULL)
	{
	
	dng_string s;
	
	if (oldXMP.GetAltLangDefault (ns, path, s))
		{
		
		if (s.NotEmpty ())
			{
			
			newXMP.SetAltLangDefault (ns, path, s);
			
			if (exif)
				{
				
				*exif = s;
				
				}
			
			}
			
		}
	
	}
								 

static void CopyStructField (const dng_xmp &oldXMP,
							 dng_xmp &newXMP,
							 const char *ns,
							 const char *path,
							 const char *field)
	{
	
	dng_string s;
	
	if (oldXMP.GetStructField (ns, path, ns, field, s))
		{
		
		if (s.NotEmpty ())
			{
			
			newXMP.SetStructField (ns, path, ns, field, s);
			
			}
			
		}
	
	}

static void CopyBoolean (const dng_xmp &oldXMP,
						 dng_xmp &newXMP,
						 const char *ns,
						 const char *path)
	{
	
	bool b;
	
	if (oldXMP.GetBoolean (ns, path, b))
		{
		
		newXMP.SetBoolean (ns, path, b);
						
		}
	
	}
	

#endif	
								 

void dng_image_writer::CleanUpMetadata (dng_host &host,
										dng_metadata &metadata,
										dng_metadata_subset metadataSubset,
										const char *dstMIME,
										const char *software)
	{
	
	#if qDNGUseXMP
	
	if (metadata.GetXMP () && metadata.GetExif ())
		{
		
		dng_xmp	 &newXMP  (*metadata.GetXMP	 ());
		dng_exif &newEXIF (*metadata.GetExif ());
		
		
		
		if (software)
			{
	
			newEXIF.fSoftware.Set (software);
			
			newXMP.Set (XMP_NS_XAP,
						"CreatorTool",
						software);
			
			}
		
		#if qDNGXMPDocOps
		
		newXMP.DocOpsPrepareForSave (metadata.SourceMIME ().Get (),
									 dstMIME);
												  
		#else
		
		metadata.UpdateDateTimeToNow ();
		
		#endif
		
		
		
		
		if (!newEXIF.AtLeastVersion0231 ())
			{
		
			newEXIF.SetVersion0231 ();
			
			newXMP.Set (XMP_NS_EXIF, "ExifVersion", "0231");
			
			}
			
		
		
			{
		
			dng_exif blankExif;
		
			newEXIF.CopyGPSFrom (blankExif);
			
			}
		
		
	
		newXMP.SyncExif (newEXIF,
						 metadata.GetOriginalExif (),
						 false,
						 true);
									  
		
		
		
		
		newXMP.RemoveEmptyStringsAndArrays (XMP_NS_DC);
		newXMP.RemoveEmptyStringsAndArrays (XMP_NS_XAP);
		newXMP.RemoveEmptyStringsAndArrays (XMP_NS_PHOTOSHOP);
		newXMP.RemoveEmptyStringsAndArrays (XMP_NS_IPTC);
		newXMP.RemoveEmptyStringsAndArrays (XMP_NS_XAP_RIGHTS);
		newXMP.RemoveEmptyStringsAndArrays ("http://ns.iview-multimedia.com/mediapro/1.0/");

		
  
		if (metadataSubset != dng_metadata_subset::kMask_All)
			{
			
			
			
			dng_xmp	 oldXMP	 (newXMP );
			dng_exif oldEXIF (newEXIF);
			
			if (metadataSubset.Excludes (dng_metadata_subset::kMask_Other))
				{
				
				
				
					
				newXMP.RemoveProperties (NULL);
				
				newEXIF.SetEmpty ();
				
				metadata.ClearMakerNote ();

				

				newEXIF.fExifVersion = oldEXIF.fExifVersion;
				
				#if qDNGXMPDocOps
				
				
				
				CopyString (oldXMP,
							newXMP,
							XMP_NS_MM,
							"OriginalDocumentID");
							
				CopyString (oldXMP,
							newXMP,
							XMP_NS_MM,
							"DocumentID");
				
				CopyString (oldXMP,
							newXMP,
							XMP_NS_MM,
							"InstanceID");
				
				CopyString (oldXMP,
							newXMP,
							XMP_NS_XAP,
							"MetadataDate");
				
				#endif

				

				newXMP.DuplicateNameSpace (oldXMP, XMP_NS_HDR_META);
				newXMP.DuplicateNameSpace (oldXMP, XMP_NS_HDRGM);
				newXMP.DuplicateNameSpace (oldXMP, XMP_NS_APPLE_HDRGM);
					
				
				
				if (metadataSubset.Includes (dng_metadata_subset::kMask_Exif))
					{

					newEXIF = oldEXIF;
					
					
					
					dng_exif blankExif;
					
					newEXIF.CopyGPSFrom (blankExif);
					
					
					
					newXMP.DuplicateNameSpace (oldXMP, XMP_NS_EXIF);
					newXMP.DuplicateNameSpace (oldXMP, XMP_NS_EXIFEX);
					newXMP.DuplicateNameSpace (oldXMP, XMP_NS_AUX);
					
					}

				
				
				if (metadataSubset.Includes (dng_metadata_subset::kMask_ExifDate))
					{
					
					newEXIF.CopyDateFrom (oldEXIF);
					
					}

				
					
				if (metadataSubset.Includes (dng_metadata_subset::kMask_Location))
					{
					
					
					
					newEXIF.CopyGPSFrom (oldEXIF);
					
					
					
					CopyString (oldXMP,
								newXMP,
								XMP_NS_PHOTOSHOP,
								"City");
					
					CopyString (oldXMP,
								newXMP,
								XMP_NS_PHOTOSHOP,
								"State");
					
					CopyString (oldXMP,
								newXMP,
								XMP_NS_PHOTOSHOP,
								"Country");
					
					CopyString (oldXMP,
								newXMP,
								XMP_NS_IPTC,
								"Location");
					
					CopyString (oldXMP,
								newXMP,
								XMP_NS_IPTC,
								"CountryCode");
								
					newXMP.DuplicateSubtree (oldXMP,
											 XMP_NS_IPTC_EXT,
											 "LocationCreated");
					
					newXMP.DuplicateSubtree (oldXMP,
											 XMP_NS_IPTC_EXT,
											 "LocationShown");
											
					}

				
				
				if (metadataSubset.Includes (dng_metadata_subset::kMask_CameraRaw))
					{
					
					newXMP.DuplicateNameSpace (oldXMP, XMP_NS_CRS);
					
					}

				
				
				if (metadataSubset.Includes (dng_metadata_subset::kMask_Copyright))
					{
					
					CopyAltLangDefault (oldXMP,
										newXMP,
										XMP_NS_DC,
										"rights",
										&newEXIF.fCopyright);
														
					CopyAltLangDefault (oldXMP,
										newXMP,
										XMP_NS_XAP_RIGHTS,
										"UsageTerms");
										
					CopyString (oldXMP,
								newXMP,
								XMP_NS_XAP_RIGHTS,
								"WebStatement");
								
					CopyBoolean (oldXMP,
								 newXMP,
								 XMP_NS_XAP_RIGHTS,
								 "Marked");
									 
					}

				
				
				if (metadataSubset.Includes (dng_metadata_subset::kMask_Contact))
					{
					
					
					
					
					
					CopyStringList (oldXMP,
									newXMP,
									XMP_NS_DC,
									"creator",
									false);
									
					
					
									
					newEXIF.fArtist = oldEXIF.fArtist;
					
					
					
					CopyString (oldXMP,
								newXMP,
								XMP_NS_PHOTOSHOP,
								"AuthorsPosition");
								
					CopyStructField (oldXMP,
									 newXMP,
									 XMP_NS_IPTC,
									 "CreatorContactInfo",
									 "CiEmailWork");
								
					CopyStructField (oldXMP,
									 newXMP,
									 XMP_NS_IPTC,
									 "CreatorContactInfo",
									 "CiAdrExtadr");
								
					CopyStructField (oldXMP,
									 newXMP,
									 XMP_NS_IPTC,
									 "CreatorContactInfo",
									 "CiAdrCity");
								
					CopyStructField (oldXMP,
									 newXMP,
									 XMP_NS_IPTC,
									 "CreatorContactInfo",
									 "CiAdrRegion");
								
					CopyStructField (oldXMP,
									 newXMP,
									 XMP_NS_IPTC,
									 "CreatorContactInfo",
									 "CiAdrPcode");
								
					CopyStructField (oldXMP,
									 newXMP,
									 XMP_NS_IPTC,
									 "CreatorContactInfo",
									 "CiAdrCtry");
								
					CopyStructField (oldXMP,
									 newXMP,
									 XMP_NS_IPTC,
									 "CreatorContactInfo",
									 "CiTelWork");
								
					CopyStructField (oldXMP,
									 newXMP,
									 XMP_NS_IPTC,
									 "CreatorContactInfo",
									 "CiUrlWork");
									 
					}
					
				
				
				if (metadataSubset.Includes (dng_metadata_subset::kMask_Keywords))
					{
					
					CopyStringList (oldXMP,
									newXMP,
									XMP_NS_DC,
									"subject",
									true);
							
					CopyStringList (oldXMP,
									newXMP,
									XMP_NS_LR,
									"hierarchicalSubject",
									true);
							
					}
					
				
				
				if (metadataSubset.Includes (dng_metadata_subset::kMask_Rating))
					{
					
					CopyString (oldXMP,
								newXMP,
								XMP_NS_XAP,
								"Rating");
								
					}
							
				
					
				if (metadataSubset.Includes (dng_metadata_subset::kMask_Label))
					{
					
					CopyString (oldXMP,
								newXMP,
								XMP_NS_XAP,
								"Label");
								
					}
					
				
				
				if (metadataSubset.Includes (dng_metadata_subset::kMask_Caption))
					{
					
					CopyAltLangDefault (oldXMP,
										newXMP,
										XMP_NS_DC,
										"description");

					newEXIF.fImageDescription = oldEXIF.fImageDescription;
					
					}
									
				
				
				if (metadataSubset.Includes (dng_metadata_subset::kMask_Title))
					{
					
					CopyAltLangDefault (oldXMP,
										newXMP,
										XMP_NS_DC,
										"title");

					}
								
				
				
				if (metadataSubset.Includes (dng_metadata_subset::kMask_Regions))
					{
					
					newXMP.DuplicateNameSpace (oldXMP, XMP_NS_REGIONS);

					}
												
				}
				
			else
				{
				
				
				
				
				
				
				if (metadataSubset.Excludes (dng_metadata_subset::kMask_CameraRaw))
					{
					
					newXMP.RemoveProperties (XMP_NS_CRS);
					newXMP.RemoveProperties (XMP_NS_CRSS);
					newXMP.RemoveProperties (XMP_NS_CRX);

					

					newXMP.Remove (XMP_NS_MM, "History");

					}

				
				
				if (metadataSubset.Excludes (dng_metadata_subset::kMask_Exif))
					{

					
					
					
					newEXIF.SetEmpty ();
					
					newEXIF.fImageDescription  = oldEXIF.fImageDescription;		
					newEXIF.fSoftware		   = oldEXIF.fSoftware;
					newEXIF.fArtist			   = oldEXIF.fArtist;
					newEXIF.fCopyright		   = oldEXIF.fCopyright;
					newEXIF.fCopyright2		   = oldEXIF.fCopyright2;
					newEXIF.fDateTime		   = oldEXIF.fDateTime;
					newEXIF.fDateTimeOriginal  = oldEXIF.fDateTimeOriginal;
					newEXIF.fDateTimeDigitized = oldEXIF.fDateTimeDigitized;
					newEXIF.fExifVersion	   = oldEXIF.fExifVersion;
					newEXIF.fImageUniqueID	   = oldEXIF.fImageUniqueID;
					
					newEXIF.CopyGPSFrom (oldEXIF);
					
					
					
					newXMP.RemoveProperties (XMP_NS_EXIF);
					newXMP.RemoveProperties (XMP_NS_EXIFEX);
					newXMP.RemoveProperties (XMP_NS_AUX);
					
					

					newXMP.RemoveProperties (XMP_NS_PANO);
					newXMP.RemoveProperties (XMP_NS_GPANO);

					
					
					metadata.ClearMakerNote ();
				
					
					

					newXMP.Remove (XMP_NS_MM, "History");

					}
					
				}
				
			
			
			if (metadataSubset.Excludes (dng_metadata_subset::kMask_Keywords))
				{
				
				newXMP.Remove (XMP_NS_DC, "subject");
				
				newXMP.Remove (XMP_NS_LR, "hierarchicalSubject");
							
				}

			
			
			if (metadataSubset.Excludes (dng_metadata_subset::kMask_Location))
				{
				
				
				
				dng_exif blankExif;
				
				newEXIF.CopyGPSFrom (blankExif);
				
				
				
				
				metadata.ClearMakerNote ();
				
				
				
				newXMP.Remove (XMP_NS_PHOTOSHOP, "City");
				newXMP.Remove (XMP_NS_PHOTOSHOP, "State");
				newXMP.Remove (XMP_NS_PHOTOSHOP, "Country");
				newXMP.Remove (XMP_NS_IPTC, "Location");
				newXMP.Remove (XMP_NS_IPTC, "CountryCode");
				newXMP.Remove (XMP_NS_IPTC_EXT, "LocationCreated");
				newXMP.Remove (XMP_NS_IPTC_EXT, "LocationShown");
				
				}
				
			
			
			if (metadataSubset.Excludes (dng_metadata_subset::kMask_Contact))
				{
				
				newXMP.Remove (XMP_NS_DC, "creator");
				
				newEXIF.fArtist.Clear ();
				
				newXMP.Remove (XMP_NS_PHOTOSHOP, "AuthorsPosition");
							
				newXMP.Remove (XMP_NS_IPTC, "CreatorContactInfo");
				
				
												
				}
				
			
			
			if (metadataSubset.Excludes (dng_metadata_subset::kMask_Copyright))
				{
				
				newXMP.Remove (XMP_NS_DC, "rights");
				
				newXMP.Remove (XMP_NS_XAP_RIGHTS, "UsageTerms");
				newXMP.Remove (XMP_NS_XAP_RIGHTS, "WebStatement");
				newXMP.Remove (XMP_NS_XAP_RIGHTS, "Marked");
				
				newEXIF.fCopyright .Clear ();
				newEXIF.fCopyright2.Clear ();

				}
					
			
				
			if (metadataSubset.Excludes (dng_metadata_subset::kMask_Rating))
				{
				
				newXMP.Remove (XMP_NS_XAP, "Rating");
							
				}
						
			
				
			if (metadataSubset.Excludes (dng_metadata_subset::kMask_Label))
				{
				
				newXMP.Remove (XMP_NS_XAP, "Label");
							
				}
				
			
			
			if (metadataSubset.Excludes (dng_metadata_subset::kMask_Caption))
				{
				
				newXMP.Remove (XMP_NS_DC, "description");
				
				newEXIF.fImageDescription.Clear ();
				
				}
								
			
			
			if (metadataSubset.Excludes (dng_metadata_subset::kMask_Title))
				{
				
				newXMP.Remove (XMP_NS_DC, "title");
				
				}
									
			
			
			if (metadataSubset.Excludes (dng_metadata_subset::kMask_Regions))
				{
				
				newXMP.RemoveProperties (XMP_NS_REGIONS);

				}
										
			}
									  
		
		
		bool isTIFF = (strcmp (dstMIME, "image/tiff") == 0);
		bool isDNG	= (strcmp (dstMIME, "image/dng" ) == 0);

		if (!isDNG)
			{
		
			metadata.RebuildIPTC (host.Allocator (),
								  isTIFF);
								  
			}
			
		else
			{
			
			metadata.ClearIPTC ();
			
			}
	
		
									  
		newXMP.ClearOrientation ();
		
		newXMP.ClearImageInfo ();
		
		newXMP.RemoveProperties (XMP_NS_DNG);
		
		
		
		if (!isDNG)
			{
			
			newXMP.RemoveProperties (XMP_NS_CRD);
			newXMP.RemoveProperties (XMP_NS_CRLCP);
			
			}
		
		
		
		
		newXMP.ClearIPTCDigest ();
		
		
		
		newXMP.Remove (XMP_NS_PHOTOSHOP, "SidecarForExtension");
		newXMP.Remove (XMP_NS_PHOTOSHOP, "EmbeddedXMPDigest");
		
		}
	
	#endif	

	}

void dng_image_writer::UpdateExifColorSpaceTag (dng_metadata &metadata,
												const void *profileData,
												const uint32 profileSize)
	{
	
	if (!metadata.GetExif ())
		{
		return;
		}

	dng_exif &exif = *metadata.GetExif ();

	if (profileData && profileSize)
		{

		uint32 tagValue = 0xFFFF;

		

		uint32 sRGB_size	   = 0;
		const uint8 *sRGB_data = 0;

		if (dng_space_sRGB::Get ().ICCProfile (sRGB_size,
											   sRGB_data))
			{

			if ((sRGB_size == profileSize) &&
				!memcmp (profileData, 
						 (const void *) sRGB_data, 
						 (size_t) sRGB_size))
				{

				
					
				tagValue = 1;

				}
				
			}

		exif.fColorSpace = tagValue;
		
		}

	}

void dng_image_writer::WriteTIFF (dng_host &host,
								  dng_stream &stream,
								  const dng_image &image,
								  uint32 photometricInterpretation,
								  uint32 compression,
								  const dng_metadata *metadata,
								  const dng_color_space *space,
								  const dng_resolution *resolution,
								  const dng_jpeg_preview *thumbnail,
								  const dng_memory_block *imageResources,
								  dng_metadata_subset metadataSubset,
								  bool hasTransparency,
								  bool allowBigTIFF,
								  const dng_image *gainMapImage,
								  const const_dng_memory_block_sptr gainMapMetadataBlock,
								  const bool useHalfFloat,
								  const void *gainMapAltProfileData,
								  const uint32 gainMapAltProfileSize)
	{
	
	const void *profileData = NULL;
	uint32 profileSize = 0;
	
	const uint8 *data = NULL;
	uint32 size = 0;
	
	if (space && space->ICCProfile (size, data))
		{
		
		profileData = data;
		profileSize = size;
		
		}
		
	WriteTIFFWithProfile (host,
						  stream,
						  image,
						  photometricInterpretation,
						  compression,
						  metadata,
						  profileData,
						  profileSize,
						  resolution,
						  thumbnail,
						  imageResources,
						  metadataSubset,
						  hasTransparency,
						  allowBigTIFF,
						  gainMapImage,
						  gainMapMetadataBlock,
						  useHalfFloat,
						  gainMapAltProfileData,
						  gainMapAltProfileSize);
	
	}

static void SetupIFDfromCompressedImage (dng_ifd &info,
										 const dng_lossy_compressed_image &compressedImage)
	{
	
	info.fCompression = compressedImage.fCompressionCode;
	
	info.fTileWidth  = compressedImage.fTileSize.h;
	info.fTileLength = compressedImage.fTileSize.v;
	
	info.fUsesStrips =  compressedImage.fUsesStrips;
	info.fUsesTiles  = !compressedImage.fUsesStrips;
	
	for (uint32 j = 1; j < info.fSamplesPerPixel; j++)
		{
		info.fBitsPerSample [j] = compressedImage.fBitsPerSample;
		}
	
	info.fRowInterleaveFactor    = compressedImage.fRowInterleaveFactor;
	info.fColumnInterleaveFactor = compressedImage.fColumnInterleaveFactor;
	
	info.fJXLDistance    = compressedImage.JXLDistance    ();
	info.fJXLEffort      = compressedImage.JXLEffort      ();
	info.fJXLDecodeSpeed = compressedImage.JXLDecodeSpeed ();
	
	}

static dng_ifd * MakeGainMapIFD (const dng_image &image,
								 const dng_lossy_compressed_image *lossyCompressed,
								 const uint32 compression)
	{
	
	DNG_REQUIRE (image.Planes () == 1 ||
				 image.Planes () == 3,
				 "Invalid image plane count for MakeGainMapIFD");

	AutoPtr<dng_ifd> result (new dng_ifd);

	auto &ifd = *result;

	ifd.fNewSubFileType = sfGainMap;
	
	ifd.fImageWidth	 = image.Bounds ().W ();
	ifd.fImageLength = image.Bounds ().H ();
	
	ifd.fSamplesPerPixel = image.Planes ();
	
	ifd.fBitsPerSample [0] = TagTypeSize (image.PixelType ()) * 8;
	
	for (uint32 j = 1; j < ifd.fSamplesPerPixel; j++)
		{
		ifd.fBitsPerSample [j] = ifd.fBitsPerSample [0];
		}
		
	ifd.fPhotometricInterpretation = piGainMap;
	
	ifd.fExtraSamplesCount = 0;
	
	if (lossyCompressed)
		{
		
		SetupIFDfromCompressedImage (ifd,
									 *lossyCompressed);
		
		}
		
	else
		{

		ifd.fCompression = compression;
		
		if (ifd.fCompression == ccUncompressed)
			{
			
			ifd.SetSingleStrip ();
			
			}
			
		else
			{

			ifd.FindStripSize (128 * 1024);

			
			
			if (ifd.fCompression == ccDeflate)
				{
				
				if (image.PixelType () == ttFloat)
					{
					ifd.fPredictor = cpFloatingPoint;
					}
					
				else
					{
					ifd.fPredictor = cpHorizontalDifference;
					}
				
				}
			
			}
			
		}

	return result.Release ();

	}

void dng_image_writer::WriteTIFFWithProfile (dng_host &host,
											 dng_stream &stream,
											 const dng_image &image,
											 uint32 photometricInterpretation,
											 uint32 compression,
											 const dng_metadata *constMetadata,
											 const void *profileData,
											 uint32 profileSize,
											 const dng_resolution *resolution,
											 const dng_jpeg_preview *thumbnail,
											 const dng_memory_block *imageResources,
											 dng_metadata_subset metadataSubset,
											 bool hasTransparency,
											 bool allowBigTIFF,
											 const dng_image *gainMapImage,
											 const const_dng_memory_block_sptr gainMapMetadataBlock,
											 const bool useHalfFloat,
											 const void *gainMapAltProfileData,
											 const uint32 gainMapAltProfileSize)
	{
	
	
	
	#define qForceWriteBigTIFF 0
	
	
	
	#define qStressTestBigTIFF 0

	#if qStressTestBigTIFF
	const uint64 kStressTestBytes = 0x100000000ULL;
	#endif
	
	AutoPtr<dng_metadata> metadata;
	
	if (constMetadata)
		{
		
		metadata.Reset (constMetadata->Clone (host.Allocator ()));
		
		CleanUpMetadata (host, 
						 *metadata,
						 metadataSubset,
						 "image/tiff");

		UpdateExifColorSpaceTag (*metadata, 
								 profileData, 
								 profileSize);
		
		}
	
	dng_ifd ifd;
	
	ifd.fNewSubFileType = sfMainImage;
	
	ifd.fImageWidth	 = image.Bounds ().W ();
	ifd.fImageLength = image.Bounds ().H ();
	
	ifd.fSamplesPerPixel = image.Planes ();

	if ((image.PixelType () == ttFloat) && useHalfFloat)
		ifd.fBitsPerSample [0] = 16;

	else
		ifd.fBitsPerSample [0] = TagTypeSize (image.PixelType ()) * 8;
	
	for (uint32 j = 1; j < ifd.fSamplesPerPixel; j++)
		{
		ifd.fBitsPerSample [j] = ifd.fBitsPerSample [0];
		}
		
	ifd.fPhotometricInterpretation = photometricInterpretation;
	
	ifd.fCompression = compression;
	
	if (ifd.fCompression == ccUncompressed)
		{
		
		ifd.SetSingleStrip ();
		
		}
		
	else
		{

		if (ifd.fCompression == ccJXL)
			{
			
			ifd.FindTileSize (2048 * 2048);
			
			}

		else
			{
		
			ifd.FindStripSize (128 * 1024);

			}
		
		if (ifd.fCompression != ccJPEG &&
			ifd.fCompression != ccJXL)
			{
			
			if (image.PixelType () == ttFloat)
				{
				ifd.fPredictor = cpFloatingPoint;
				}
				
			else
				{
				ifd.fPredictor = cpHorizontalDifference;
				}
			
			}
		
		}

	uint32 extraSamples = 0;
	
	switch (photometricInterpretation)
		{
		
		case piBlackIsZero:
			{
			extraSamples = image.Planes () - 1;
			break;
			}
			
		case piRGB:
		case piCIELab:
		case piICCLab:
			{
			extraSamples = image.Planes () - 3;
			break;
			}
			
		case piCMYK:
			{
			extraSamples = image.Planes () - 4;
			break;
			}
			
		default:
			break;
			
		}
		
	ifd.fExtraSamplesCount = extraSamples;
	
	if (hasTransparency && extraSamples)
		{
		ifd.fExtraSamples [0] = esAssociatedAlpha;
		}
	
	if (image.PixelType () == ttFloat)
		{
		
		for (uint32 j = 0; j < ifd.fSamplesPerPixel; j++)
			{
			ifd.fSampleFormat [j] = sfFloatingPoint;
			}
			
		}

	dng_tiff_directory mainIFD;
	
	dng_basic_tag_set basic (mainIFD, ifd);

	
	
	dng_tiff_directory gainMapIFD;

	AutoPtr<dng_basic_tag_set> gainMapTagSet;
	
	AutoPtr<dng_ifd> gainMapImageIFD;

	AutoPtr<tag_owned_data_ptr> tagGainMapMetadata;

	AutoPtr<tag_icc_profile> tagGainMapAlternateProfile;

	const bool hasGainMap = ((gainMapImage != nullptr) &&
							 (gainMapMetadataBlock != nullptr));

	if (hasGainMap)
		{
		
		uint32 gainMapCompression = compression;
		
		if (gainMapImage->PixelType () == ttFloat &&
			gainMapCompression != ccDeflate)
			{
			gainMapCompression = ccUncompressed;
			}
		
		gainMapImageIFD.Reset (MakeGainMapIFD (*gainMapImage,
											   nullptr,
											   gainMapCompression));

		gainMapTagSet.Reset (new dng_basic_tag_set (gainMapIFD,
													*gainMapImageIFD));
		
		tagGainMapMetadata.Reset
			(new tag_owned_data_ptr (tcGainMapMetadata_ISO_21496_1,
									 ttUndefined,
									 gainMapMetadataBlock->LogicalSize (),
									 gainMapMetadataBlock));
		
		gainMapIFD.Add (tagGainMapMetadata.Get ());

		if (gainMapAltProfileData &&
			(gainMapAltProfileSize > 0))
			{
			
			tagGainMapAlternateProfile.Reset
				(new tag_icc_profile (gainMapAltProfileData,
									  gainMapAltProfileSize));

			gainMapIFD.Add (tagGainMapAlternateProfile.Get ());
			
			}
		
		}

	
	
	dng_resolution res;
	
	if (resolution)
		{
		res = *resolution;
		}
	
	tag_urational tagXResolution (tcXResolution, res.fXResolution);
	tag_urational tagYResolution (tcYResolution, res.fYResolution);
	
	tag_uint16 tagResolutionUnit (tcResolutionUnit, res.fResolutionUnit);
	
	if (resolution)
		{
		mainIFD.Add (&tagXResolution   );
		mainIFD.Add (&tagYResolution   );
		mainIFD.Add (&tagResolutionUnit);
		}

	
	
	tag_icc_profile iccProfileTag (profileData, profileSize);
	
	if (iccProfileTag.Count ())
		{
		mainIFD.Add (&iccProfileTag);
		}
		
	
	
	#if qDNGUseXMP
	
	tag_xmp tagXMP (metadata.Get () ? metadata->GetXMP () : NULL);
	
	if (tagXMP.Count ())
		{
		mainIFD.Add (&tagXMP);
		}
		
	#endif
	
	
	
	tag_iptc tagIPTC (metadata.Get () ? metadata->IPTCData	 () : NULL,
					  metadata.Get () ? metadata->IPTCLength () : 0);
		
	if (tagIPTC.Count ())
		{
		mainIFD.Add (&tagIPTC);
		}
		
	
	
	AutoPtr<dng_memory_block> adobeData (BuildAdobeData (host,
														 metadata.Get (),
														 thumbnail,
														 imageResources));
														 
	tag_uint8_ptr tagAdobe (tcAdobeData,
							adobeData->Buffer_uint8 (),
							adobeData->LogicalSize ());
								 
	if (tagAdobe.Count ())
		{
		mainIFD.Add (&tagAdobe);
		}
		
	
	
	const dng_exif emptyExif;

	const dng_exif *metaExifPtr = metadata.Get () ? metadata->GetExif () : nullptr;

	const dng_exif &exifRef = metaExifPtr ? *metaExifPtr : emptyExif;
	
	exif_tag_set exifSet (mainIFD,
						  exifRef,
						  metadata.Get () ? metadata->IsMakerNoteSafe () : false,
						  metadata.Get () ? metadata->MakerNoteData	  () : NULL,
						  metadata.Get () ? metadata->MakerNoteLength () : 0,
						  false);

	

	uint32 subFileCount = 0;

	if (hasGainMap)
		subFileCount++;

	

	tag_big_uints tagSubFile (tcSubIFDs,
							  subFileCount);
							   
	if (subFileCount)
		mainIFD.Add (&tagSubFile);

	
	
	uint64 headerSize32 = 8 +
						  mainIFD.Size () +
						  exifSet.Size () +
						  (hasGainMap ? gainMapIFD.Size () : 0);
						  
	
	
	mainIFD.SetBigTIFF (true);
	exifSet.SetBigTIFF (true);

	if (hasGainMap)
		gainMapIFD.SetBigTIFF (true);

	uint64 headerSize64 = 16 +
						  mainIFD.Size () +
						  exifSet.Size () +
						  (hasGainMap ? gainMapIFD.Size () : 0);
						  
	
	
	uint64 maxNonHeaderSize =
		(ifd.MaxImageDataByteCount () +
		 (hasGainMap ? gainMapImageIFD->MaxImageDataByteCount () : 0));
	
	
	
	
	const uint64 kMaxTargetNonBigTIFFSize = 0xFFFFFFFF - 16 * 1024 * 1024;
	
	
	
	if (headerSize32 + maxNonHeaderSize < kMaxTargetNonBigTIFFSize)
		{
		allowBigTIFF = false;
		}
						  
	#if qForceWriteBigTIFF || qStressTestBigTIFF

	allowBigTIFF = true;
	
	#if qCRStressTestBigTIFF
	headerSize64 += kStressTestBytes;
	#endif
	
	#endif
	
	
	
	uint64 headerSize = allowBigTIFF ? headerSize64
									 : headerSize32;
	
	stream.SetWritePosition (headerSize);
						  
	
	
	WriteImage (host,
				ifd,
				basic,
				stream,
				image);

	

	if (hasGainMap)
		{
		
		WriteImage (host,
					*gainMapImageIFD,
					*gainMapTagSet,
					stream,
					*gainMapImage);
		
		}
				
	
	
	
	if (!allowBigTIFF)
		{
		
		if (stream.Position () > 0xFFFFFFFF)
			{
			ThrowImageTooBigTIFF ();
			}
		
		}
				
	
	
	stream.SetLength (stream.Position ());
	
	
	
	bool isBigTIFF = allowBigTIFF && (stream.Length () > kMaxTargetNonBigTIFFSize);
	
	#if qForceWriteBigTIFF || qStressTestBigTIFF
	isBigTIFF = allowBigTIFF;
	#endif
		
	mainIFD.SetBigTIFF (isBigTIFF);
	exifSet.SetBigTIFF (isBigTIFF);

	if (hasGainMap)
		gainMapIFD.SetBigTIFF (isBigTIFF);
	
	
	
	stream.SetWritePosition (0);
	
	stream.Put_uint16 (stream.BigEndian () ? byteOrderMM : byteOrderII);
	
	if (isBigTIFF)
		{
		
		stream.Put_uint16 (magicBigTIFF);
		
		stream.Put_uint16 (8);
		stream.Put_uint16 (0);
		
		#if qStressTestBigTIFF
		stream.Put_uint64 (16 + kStressTestBytes);
		stream.PutZeros (kStressTestBytes);
		#else
		stream.Put_uint64 (16);
		#endif
			
		}
		
	else
		{
	
		stream.Put_uint16 (magicTIFF);
		
		stream.Put_uint32 (8);
		
		}

	
					
	uint64 offsetMainIFD = stream.Position ();
	
	uint64 offsetExifData = offsetMainIFD + mainIFD.Size ();
	
	uint64 offsetGainMapIFD = offsetExifData + exifSet.Size ();
	
	exifSet.Locate (offsetExifData);

	if (hasGainMap)
		tagSubFile.Set (0, offsetGainMapIFD);
	
	mainIFD.Put (stream);
	
	exifSet.Put (stream);

	if (hasGainMap)
		gainMapIFD.Put (stream);
	
	if (headerSize > stream.Position ())
		{
		
		stream.PutZeros (headerSize - stream.Position ());
		
		}
	
	stream.Flush ();
	
	}
							   

void dng_image_writer::WriteDNG (dng_host &host,
								 dng_stream &stream,
								 dng_negative &negative,
								 const dng_preview_list *previewList,
								 uint32 maxBackwardVersion,
								 bool uncompressed,
								 bool allowBigTIFF,
								 const dng_image *gainMapImage,
								 const dng_lossy_compressed_image *gainMapLossyCompressed,
								 const const_dng_memory_block_sptr gainMapMetadataBlock,
								 const void *gainMapAltProfileData,
								 const uint32 gainMapAltProfileSize)
	{
	
	WriteDNGWithMetadata (host,
						  stream,
						  negative,
						  negative.Metadata (),
						  previewList,
						  maxBackwardVersion,
						  uncompressed,
						  allowBigTIFF,
						  gainMapImage,
						  gainMapLossyCompressed,
						  gainMapMetadataBlock,
						  gainMapAltProfileData,
						  gainMapAltProfileSize);
	
	}
	

static bool HasDNGOpcode_1_6 (const dng_negative &negative,
							  bool includeOptional)
	{

	const uint32 version = dngVersion_1_6_0_0;

	return (negative.OpcodeList1 ().MinVersion (includeOptional) >= version ||
			negative.OpcodeList2 ().MinVersion (includeOptional) >= version ||
			negative.OpcodeList3 ().MinVersion (includeOptional) >= version);
	
	}

#if qDNGDebug

class dng_verify_size_estimate
	{
	
	private:
	
		dng_stream &fStream;
	
		uint64 fStart;
		
		uint64 fEstimate;
		
		const char *fMessage;
		
	public:
	
		dng_verify_size_estimate (dng_stream &stream,
								  uint64 estimate,
								  const char *message)
								  
			:	fStream	  (stream)
			,	fStart	  (stream.Position ())
			,	fEstimate (estimate)
			,	fMessage  (message)
			
			{
			
			}
			
		~dng_verify_size_estimate ()
			{
			
			uint64 actual = fStream.Position () - fStart;
			
			char message [2048];
			
			snprintf (message,
					  2048,
					  "%s: actual size = %llu, estimated size = %llu",
					  fMessage,
					  (unsigned long long) actual,
					  (unsigned long long) fEstimate);
					 
			DNG_ASSERT (actual <= fEstimate, message);
			
			#if 0
			fprintf (stderr, "%s\n", message);
			#endif
			
			}
		
	};
	
#define DNG_VERIFY_SIZE_ESTIMATE(stream,estimate,message) \
	dng_verify_size_estimate verify (stream, estimate, message);

#else

#define DNG_VERIFY_SIZE_ESTIMATE(stream,estimate,message) \
	(void) estimate;

#endif

static void SetEncodeSettingsForIFD (const dng_image &image,
									 const bool uncompressed,
									 const uint32 maxBackwardVersion,
									 dng_ifd &info,
									 const dng_lossy_compressed_image *compressedImage = nullptr)
	{
	
	const bool isFloat = (image.PixelType () == ttFloat);
	
	
			
	if (isFloat)
		{

		for (uint32 i = 0; i < info.fSamplesPerPixel; i++)
			info.fSampleFormat [i] = sfFloatingPoint;

		}
			
	
	
	if (compressedImage)
		{
		
		SetupIFDfromCompressedImage (info, *compressedImage);
				
		return;
		
		}

	

	if (uncompressed)
		{
		
		info.fCompression = ccUncompressed;

		}

	else if (isFloat)
		{
		
		info.fCompression = ccDeflate;
		
		}

	else if (image.PixelType () == ttLong)
			 
		{

		if (maxBackwardVersion >= dngVersion_1_4_0_0)
			info.fCompression = ccDeflate;

		else
			info.fCompression = ccUncompressed;
		
		}

	else
		{

		
		
		info.fCompression = ccJPEG;
		
		}

	

	if (info.fCompression == ccUncompressed)
		{
		
		info.fPredictor = cpNullPredictor;
			
		info.SetSingleStrip ();
		
		}

	
		
	else if (info.fCompression == ccDeflate)
		{
		
		info.fPredictor = isFloat ? cpFloatingPoint
								  : cpHorizontalDifference;
				
		info.FindTileSize (512 * 1024);

		}

	
		
	else
		{
		
		info.FindTileSize (128 * 1024);

		}

	}

void dng_image_writer::WriteDNGWithMetadata (dng_host &host,
											 dng_stream &stream,
											 const dng_negative &negative,
											 const dng_metadata &constMetadata,
											 const dng_preview_list *previewList,
											 const uint32 maxBackwardVersion,
											 const bool uncompressed,
											 bool allowBigTIFF,
											 const dng_image *gainMapImage,
											 const dng_lossy_compressed_image *gainMapLossyCompressed,
											 const const_dng_memory_block_sptr gainMapMetadataBlock,
											 const void *gainMapAltProfileData,
											 const uint32 gainMapAltProfileSize)
	{
	
	
	
	#define qForceWriteBigDNG 0
	
	
	
	#define qStressTestBigDNG 0

	#if qStressTestBigDNG
	const uint64 kStressTestBytes = 0x100000000ULL;
	#endif
	
	
	
	AutoPtr<dng_metadata> metadata (constMetadata.Clone (host.Allocator ()));

	CleanUpMetadata (host, 
					 *metadata,
					 kMetadataSubset_All,
					 "image/dng");

	
	
	const dng_lossy_compressed_image *rawLossyCompressedImage =
		negative.RawLossyCompressedImage ();
		
	
	
	
	if (!rawLossyCompressedImage && negative.RawLossyCompressedImageDigest ().IsValid ())
		{
		
		negative.ClearRawLossyCompressedImageDigest ();
			
		negative.ClearRawImageDigest ();
		
		}
		
	
	
	if (rawLossyCompressedImage)
		{
		
		if (uncompressed ||
			maxBackwardVersion < MinBackwardVersionForCompression
								 (rawLossyCompressedImage->fCompressionCode) ||
			(rawLossyCompressedImage->fColumnInterleaveFactor != 1 &&
			 maxBackwardVersion < dngVersion_1_7_1_0))
			{
			
			rawLossyCompressedImage = nullptr;
			
			negative.ClearRawLossyCompressedImageDigest ();
			
			negative.ClearRawImageDigest ();

			}
		
		}
		
	
	
	const bool isFloatingPoint =
		((negative.RawImage ().PixelType () == ttFloat) ||
		 (negative.RawImage ().PixelType () == ttHalfFloat));
	
	
	
	const bool isCompressed32BitInteger = (negative.RawImage ().PixelType () == ttLong) &&
										  (maxBackwardVersion >= dngVersion_1_4_0_0) &&
										  (!uncompressed);
	
	
	
	dng_mosaic_info mosaicInfo;
	
	if (negative.GetMosaicInfo ())
		{
		mosaicInfo = *(negative.GetMosaicInfo ());
		}
		
	
	

	uint32 compression = uncompressed ? ccUncompressed : ccJPEG;

	if (!uncompressed)
		{

		
		
		if (rawLossyCompressedImage)
			compression = rawLossyCompressedImage->fCompressionCode;

		
	
		else if (isFloatingPoint)
			compression = ccDeflate;

		

		else if (negative.RawImage ().PixelType () == ttLong)
			compression = isCompressed32BitInteger ? ccDeflate : ccUncompressed;

		
		
		else
			compression = ccJPEG;
		
		}

	
	
	bool saveOriginalDefaultFinalSize	  = false;
	bool saveOriginalBestQualityFinalSize = false;
	bool saveOriginalDefaultCropSize	  = false;
	
		{
		
		
		
		dng_point defaultFinalSize (negative.DefaultFinalHeight (),
									negative.DefaultFinalWidth	());
									
		saveOriginalDefaultFinalSize = (negative.OriginalDefaultFinalSize () !=
										defaultFinalSize);
		
		if (saveOriginalDefaultFinalSize)
			{
			
			
			
			
			saveOriginalBestQualityFinalSize = (negative.OriginalBestQualityFinalSize () != 
												negative.OriginalDefaultFinalSize ());
												
			saveOriginalDefaultCropSize = (negative.OriginalDefaultCropSizeV () !=
										   dng_urational (negative.OriginalDefaultFinalSize ().v, 1)) ||
										  (negative.OriginalDefaultCropSizeH () !=
										   dng_urational (negative.OriginalDefaultFinalSize ().h, 1));

			}
			
		else
			{
			
			
			
			dng_point bestQualityFinalSize (negative.BestQualityFinalHeight (),
											negative.BestQualityFinalWidth	());
											
			saveOriginalBestQualityFinalSize = (negative.OriginalBestQualityFinalSize () != 
												bestQualityFinalSize);
												
			saveOriginalDefaultCropSize = (negative.OriginalDefaultCropSizeV () !=
										   negative.DefaultCropSizeV ()) ||
										  (negative.OriginalDefaultCropSizeH () !=
										   negative.DefaultCropSizeH ());
			
			}
		
		}
		
	
	
	bool hasTransparencyMask = (negative.RawTransparencyMask () != NULL);
	
	const dng_lossy_compressed_image *rawLossyCompressedTransparencyMask =
		negative.RawLossyCompressedTransparencyMask ();
		
	if (rawLossyCompressedTransparencyMask &&
		(maxBackwardVersion <
		 MinBackwardVersionForCompression (rawLossyCompressedTransparencyMask->fCompressionCode)))
		{
		rawLossyCompressedTransparencyMask = nullptr;
		}
	
	
	
	bool hasDepthMap = (negative.RawDepthMap () != NULL);
	
	
	
	bool hasEnhancedImage = (&negative.RawImage () != negative.Stage3Image ()) &&
							negative.EnhanceParams ().NotEmpty ();
	
	
	
	
	
	AutoPtr<profile_tag_set> profileSet;
	
	AutoPtr<color_tag_set> colorSet;
	
	std::vector<uint32> extraProfileIndex;
	
	dng_camera_profile mainProfile;

	bool hasProfileWith_1_6_Features = false;
	bool hasProfileWith_1_7_Features = false;

	bool mainProfileDidWritePGTMtoMainIFD = false;
		
	
										 
	dng_tiff_directory mainIFD;
	
	if (!negative.IsMonochrome ())
		{
		
		(void) negative.GetProfileToEmbed (constMetadata, mainProfile);
		
		if (mainProfile.Uses_1_6_Features ())
			{
			hasProfileWith_1_6_Features = true;
			}
		
		if (mainProfile.Uses_1_7_Features ())
			{
			hasProfileWith_1_7_Features = true;
			}
		
		profileSet.Reset (new profile_tag_set (host,
											   mainIFD,
											   mainProfile));

		mainProfileDidWritePGTMtoMainIFD =
			profileSet->ProfileDidWritePGTMToMainIFD ();
		
		colorSet.Reset (new color_tag_set (mainIFD,
										   negative));
										   
		
										   
		uint32 profileCount = negative.ProfileCount ();
		
		for (uint32 index = 0; index < profileCount; index++)
			{
			
			const dng_camera_profile &profile (negative.ProfileByIndex (index));
			
			if (profile.WasReadFromDNG () &&
			   !profile.Name ().Matches (mainProfile.Name ().Get ()))
				{
				
				extraProfileIndex.push_back (index);

				if (profile.Uses_1_6_Features ())
					{
					hasProfileWith_1_6_Features = true;
					}

				if (profile.Uses_1_7_Features ())
					{
					hasProfileWith_1_7_Features = true;
					}

				}
				
			}
										   
		}

	

	const bool hasImageSequenceInfo = metadata->ImageSequenceInfo ().IsValid ();
		
	

	const bool hasImageStats =
		((metadata->ImageStats ().TagCount () > 0) &&
		 metadata->ImageStats ().IsValidForPlaneCount (negative.ColorChannels ()));

	
	
	const bool hasProfileGainTableMap2 =
		(negative.HasProfileGainTableMap () &&
		 negative.ProfileGainTableMap ().RequiresVersion2 ());

	

	const bool hasHDRColorimetricRef =
		(negative.ColorimetricReference () == crOutputReferredHDR);
	
	
	
	big_table_tag_set bigTableTagSet (host,
									  mainIFD,
									  metadata->BigTableDictionary (),
									  metadata->BigTableGroupIndex ());

	
	
	dng_tiff_directory gainMapIFD;

	AutoPtr<dng_basic_tag_set> gainMapTagSet;
	
	AutoPtr<dng_ifd> gainMapImageIFD;

	AutoPtr<tag_owned_data_ptr> tagGainMapMetadata;

	AutoPtr<tag_icc_profile> tagGainMapAlternateProfile;

	const bool hasGainMap = (gainMapImage != nullptr);

	if (hasGainMap)
		{

		uint32 gainMapCompression = (gainMapImage->PixelType () == ttFloat)
									? ccDeflate
									: ccJPEG;
											
		gainMapImageIFD.Reset (MakeGainMapIFD (*gainMapImage,
											   gainMapLossyCompressed,
											   gainMapCompression));

		gainMapTagSet.Reset (new dng_basic_tag_set (gainMapIFD,
													*gainMapImageIFD));

		if (gainMapMetadataBlock)
			{
		
			tagGainMapMetadata.Reset
				(new tag_owned_data_ptr (tcGainMapMetadata_ISO_21496_1,
										 ttUndefined,
										 gainMapMetadataBlock->LogicalSize (),
										 gainMapMetadataBlock));

			gainMapIFD.Add (tagGainMapMetadata.Get ());

			if (gainMapAltProfileData &&
				(gainMapAltProfileSize > 0))
				{

				tagGainMapAlternateProfile.Reset
					(new tag_icc_profile (gainMapAltProfileData,
										  gainMapAltProfileSize));

				gainMapIFD.Add (tagGainMapAlternateProfile.Get ());

				}

			}
		
		}
		
	
	
	uint32 dngVersion 		  = dngVersion_1_4_0_0;
	uint32 dngBackwardVersion = dngVersion_1_1_0_0;

	
	
	bool has_1_7_Features = false;

	if (compression == ccJXL)
		{
		
		has_1_7_Features = true;
		
		}
		
	else if (hasTransparencyMask &&
			 rawLossyCompressedTransparencyMask &&
			 rawLossyCompressedTransparencyMask->fCompressionCode == ccJXL)
		{
		
		has_1_7_Features = true;
		
		}
		
	else if (hasEnhancedImage &&
			 negative.EnhancedLossyCompressedImage () &&
			 negative.EnhancedLossyCompressedImage ()->fCompressionCode == ccJXL)
		{
		
		has_1_7_Features = true;
		
		}
	
	else if (hasDepthMap &&
			 negative.RawLossyCompressedDepthMap () &&
			 negative.RawLossyCompressedDepthMap ()->fCompressionCode == ccJXL)
		{
		
		has_1_7_Features = true;
		
		}
		
	else if (negative.HasSemanticMask ())
		{
		
		for (uint32 j = 0; j < negative.NumSemanticMasks (); j++)
			{

			const auto &rawMask = negative.RawSemanticMask (j);
			
			if (rawMask.fLossyCompressed.get () &&
				rawMask.fLossyCompressed->fCompressionCode == ccJXL)
				{
				has_1_7_Features = true;
				}
				
			}
			
		}
		
	if (previewList && !has_1_7_Features)
		{
		
		for (uint32 j = 0; j < previewList->Count (); j++)
			{
			
			dng_tiff_directory tempDirectory;
			
			AutoPtr<dng_basic_tag_set> tempBasic (previewList->Preview (j).AddTagSet (host,
																					  tempDirectory));
			
			if (tempBasic.Get () && tempBasic->Compression () == ccJXL)
				{
				has_1_7_Features = true;
				break;
				}
				
			}

		}
	
	

	if (has_1_7_Features            ||
		hasImageStats			    ||
		hasImageSequenceInfo	    ||
		hasHDRColorimetricRef       ||
		hasProfileGainTableMap2     ||
		hasProfileWith_1_7_Features ||
		hasGainMap                  )
		{
		
		dngVersion = Max_uint32 (dngVersion, dngVersion_1_7_0_0);
		
		}
		
	
		
	const bool hasRGBTables =
		(negative.ProfileCount () > 0 &&
		 negative.ProfileByIndex (0).HasMaskedRGBTables ());
	
	if (HasDNGOpcode_1_6 (negative, false) || 
		hasProfileWith_1_6_Features ||
		negative.HasSemanticMask () ||
		negative.HasProfileGainTableMap () ||
		hasRGBTables)
		{
		
		dngVersion = Max_uint32 (dngVersion, dngVersion_1_6_0_0);
		
		}

	

	if (hasDepthMap || hasEnhancedImage)
		{
		
		dngVersion = Max_uint32 (dngVersion, dngVersion_1_5_0_0);
		
		}
		
	#if defined(qTestRowInterleave) || defined(qTestSubTileBlockRows) || defined(qTestSubTileBlockCols)
	dngBackwardVersion = Max_uint32 (dngBackwardVersion, dngVersion_1_2_0_0);
	#endif
	
	#if defined(qTestColumnInterleave)
	dngBackwardVersion = Max_uint32 (dngBackwardVersion, dngVersion_1_7_1_0);
	#endif

	dngBackwardVersion = Max_uint32 (dngBackwardVersion,
									 negative.OpcodeList1 ().MinVersion (false));

	dngBackwardVersion = Max_uint32 (dngBackwardVersion,
									 negative.OpcodeList2 ().MinVersion (false));

	dngBackwardVersion = Max_uint32 (dngBackwardVersion,
									 negative.OpcodeList3 ().MinVersion (false));
									 
	if (negative.GetMosaicInfo () &&
		negative.GetMosaicInfo ()->fCFALayout >= 6)
		{
		dngBackwardVersion = Max_uint32 (dngBackwardVersion, dngVersion_1_3_0_0);
		}
		
	if (isFloatingPoint || hasTransparencyMask || isCompressed32BitInteger)
		{
		dngBackwardVersion = Max_uint32 (dngBackwardVersion, dngVersion_1_4_0_0);
		}

	if (rawLossyCompressedImage)
		{
		
		dngBackwardVersion =
			Max_uint32 (dngBackwardVersion,
						MinBackwardVersionForCompression
							(rawLossyCompressedImage->fCompressionCode));
							
		if (rawLossyCompressedImage->fColumnInterleaveFactor != 1)
			{
			dngBackwardVersion = Max_uint32 (dngBackwardVersion, dngVersion_1_7_1_0);
			}

		}

	if (rawLossyCompressedTransparencyMask)
		{
		dngBackwardVersion =
			Max_uint32 (dngBackwardVersion,
						MinBackwardVersionForCompression
							(rawLossyCompressedTransparencyMask->fCompressionCode));
		}

	
	
	dngVersion = Max_uint32 (dngVersion, dngBackwardVersion);
	
	
	
	DNG_REQUIRE (dngBackwardVersion <= maxBackwardVersion,
				 "dngBackwardVersion too high");
		
	

	const dng_preview *thumbnail = NULL;
	
	if (previewList)
		{
		
		uint32 thumbArea = 0;
		
		for (uint32 j = 0; j < previewList->Count (); j++)
			{
			
			const dng_preview &preview (previewList->Preview (j));
			
			if ((preview.NewSubFileType () == sfPreviewImage) &&
				(preview.PhotometricInterpretation () == piBlackIsZero ||
				 preview.PhotometricInterpretation () == piRGB         ||
				 preview.PhotometricInterpretation () == piYCbCr))
				{
				
				uint32 thisArea = preview.ImageWidth  () *
								  preview.ImageLength ();
								  
				if (!thumbnail || thisArea < thumbArea)
					{
					
					thumbnail = &preview;
					
					thumbArea = thisArea;
					
					}
			
				}
								
			}
		
		}
		
	
	
	
	AutoPtr<dng_tiff_directory> rawIFD_IfNotMain;
	
	if (thumbnail)
		{
		rawIFD_IfNotMain.Reset (new dng_tiff_directory);
		}

	dng_tiff_directory &rawIFD (thumbnail ? *rawIFD_IfNotMain : mainIFD);
	
	
	
	uint8 dngVersionData [4];
	
	dngVersionData [0] = (uint8) (dngVersion >> 24);
	dngVersionData [1] = (uint8) (dngVersion >> 16);
	dngVersionData [2] = (uint8) (dngVersion >>	 8);
	dngVersionData [3] = (uint8) (dngVersion	  );
	
	tag_uint8_ptr tagDNGVersion (tcDNGVersion, dngVersionData, 4);
	
	mainIFD.Add (&tagDNGVersion);
	
	uint8 dngBackwardVersionData [4];

	dngBackwardVersionData [0] = (uint8) (dngBackwardVersion >> 24);
	dngBackwardVersionData [1] = (uint8) (dngBackwardVersion >> 16);
	dngBackwardVersionData [2] = (uint8) (dngBackwardVersion >>	 8);
	dngBackwardVersionData [3] = (uint8) (dngBackwardVersion	  );
	
	tag_uint8_ptr tagDNGBackwardVersion (tcDNGBackwardVersion, dngBackwardVersionData, 4);
	
	mainIFD.Add (&tagDNGBackwardVersion);
	
	
								
	AutoPtr<dng_basic_tag_set> thmBasic;
	
	if (thumbnail)
		{
		thmBasic.Reset (thumbnail->AddTagSet (host, mainIFD));
		}
						  
	

	const dng_image &rawImage (negative.RawImage ());

	if (rawImage.Planes () > kMaxColorPlanes)
		{
		ThrowBadFormat ();
		}

	
	
	dng_ifd info;
	
	info.fImageWidth  = rawImage.Width	();
	info.fImageLength = rawImage.Height ();
	
	info.fSamplesPerPixel = rawImage.Planes ();
	
	info.fPhotometricInterpretation = mosaicInfo.IsColorFilterArray () ? piCFA
																	   : piLinearRaw;
			
	info.fCompression = compression;

	if (isFloatingPoint && (compression == ccDeflate))
		{
		
		info.fPredictor = cpFloatingPoint;
		
		if (mosaicInfo.IsColorFilterArray ())
			{
			
			if (mosaicInfo.fCFAPatternSize.h == 2)
				{
				info.fPredictor = cpFloatingPointX2;
				}
				
			else if (mosaicInfo.fCFAPatternSize.h == 4)
				{
				info.fPredictor = cpFloatingPointX4;
				}
				
			}
			
		} 
		
	if (isCompressed32BitInteger)
		{
		
		info.fPredictor = cpHorizontalDifference;
		
		if (mosaicInfo.IsColorFilterArray ())
			{
			
			if (mosaicInfo.fCFAPatternSize.h == 2)
				{
				info.fPredictor = cpHorizontalDifferenceX2;
				}
				
			else if (mosaicInfo.fCFAPatternSize.h == 4)
				{
				info.fPredictor = cpHorizontalDifferenceX4;
				}
				
			}
			
		}
	
	uint32 rawPixelType = rawImage.PixelType ();
	
	if (rawPixelType == ttShort)
		{
		
		
		
		
		const dng_linearization_info *rangeInfo = negative.GetLinearizationInfo ();
	
		if (rangeInfo)
			{

			if (rangeInfo->fLinearizationTable.Get ())
				{
				
				uint32 entries = rangeInfo->fLinearizationTable->LogicalSize () >> 1;
				
				if (entries <= 256)
					{
					
					rawPixelType = ttByte;
					
					}
												
				}
				
			}

		}
	
	switch (rawPixelType)
		{
		
		case ttByte:
			{
			info.fBitsPerSample [0] = 8;
			break;
			}
		
		case ttShort:
			{
			info.fBitsPerSample [0] = 16;
			break;
			}
			
		case ttLong:
			{
			info.fBitsPerSample [0] = 32;
			break;
			}
			
		case ttFloat:
			{
			
			if (negative.RawFloatBitDepth () == 16)
				{
				info.fBitsPerSample [0] = 16;
				}
				
			else if (negative.RawFloatBitDepth () == 24)
				{
				info.fBitsPerSample [0] = 24;
				}
				
			else
				{
				info.fBitsPerSample [0] = 32;
				}

			for (uint32 j = 0; j < info.fSamplesPerPixel; j++)
				{
				info.fSampleFormat [j] = sfFloatingPoint;
				}

			break;
			
			}
			
		default:
			{
			ThrowProgramError ();
			}
			
		}
	
	
	
	
	
	uint32 fakeChannels = 1;
	
	if (info.fCompression == ccJPEG)
		{
		
		if (mosaicInfo.IsColorFilterArray ())
			{
			
			if (mosaicInfo.fCFAPatternSize.h == 4)
				{
				fakeChannels = 4;
				}
				
			else if (mosaicInfo.fCFAPatternSize.h == 2)
				{
				fakeChannels = 2;
				}
			
			
			
			
			while (fakeChannels * info.fSamplesPerPixel > 4 &&
				   fakeChannels > 1)
				{
				
				fakeChannels >>= 1;
				
				}
			
			}
	
		}
		
	
	
	if (rawLossyCompressedImage)
		{

		SetupIFDfromCompressedImage (info, *rawLossyCompressedImage);
				
		}
	
	else if (info.fCompression == ccJPEG)
		{
		
		info.FindTileSize (128 * 1024);
		
		}
		
	else if (info.fCompression == ccDeflate)
		{
		
		info.FindTileSize (512 * 1024);
		
		}
		
	else if (info.fCompression == ccJXL)
		{
		
		ThrowProgramError ("No JXL compressed image");
		
		}

	else if (info.fCompression == ccLossyJPEG)
		{
		
		ThrowProgramError ("No JPEG compressed image");
				
		}
		
	
		
	else
		{
		
		info.SetSingleStrip ();
		
		}
		
	#ifdef qTestRowInterleave
	info.fRowInterleaveFactor = qTestRowInterleave;
	fakeChannels = 1;
	#endif
			
	#ifdef qTestColumnInterleave
	info.fColumnInterleaveFactor = qTestColumnInterleave;
	fakeChannels = 1;
	#endif
			
	#if defined(qTestSubTileBlockRows) && defined(qTestSubTileBlockCols)
	
	info.fSubTileBlockRows = qTestSubTileBlockRows;
	info.fSubTileBlockCols = qTestSubTileBlockCols;
	
	if (fakeChannels == 2)
		fakeChannels = 4;
	
	#endif
	
	
	
	dng_basic_tag_set rawBasic (rawIFD, info);
	
	
	

	
	
	tag_data_ptr tagJPEGTables (tcJPEGTables,
								ttUndefined,
								0,
								NULL);
								
	if (rawLossyCompressedImage && rawLossyCompressedImage->JPEGTables ())
		{
		
		tagJPEGTables.SetData (rawLossyCompressedImage->JPEGTables ()->Buffer ());
		
		tagJPEGTables.SetCount (rawLossyCompressedImage->JPEGTables ()->LogicalSize ());
		
		rawIFD.Add (&tagJPEGTables);
		
		}
						  
	

	dng_urational defaultScaleData [2];
	
	defaultScaleData [0] = negative.RawDefaultScaleH ().IsValid () ?
						   negative.RawDefaultScaleH () :
						   negative.DefaultScaleH ();
						   
	defaultScaleData [1] = negative.RawDefaultScaleV ().IsValid () ?
						   negative.RawDefaultScaleV () :
						   negative.DefaultScaleV ();
													
	tag_urational_ptr tagDefaultScale (tcDefaultScale,
									   defaultScaleData,
									   2);

	rawIFD.Add (&tagDefaultScale);
	
	
	
	tag_urational tagBestQualityScale (tcBestQualityScale,
									   negative.RawBestQualityScale ().IsValid () ?
									   negative.RawBestQualityScale () :
									   negative.BestQualityScale ());
									  
	rawIFD.Add (&tagBestQualityScale);
	
	

	dng_urational defaultCropOriginData [2];

	defaultCropOriginData [0] = negative.RawDefaultCropOriginH ().IsValid () ?
								negative.RawDefaultCropOriginH () :
								negative.DefaultCropOriginH ();
								
	defaultCropOriginData [1] = negative.RawDefaultCropOriginV ().IsValid () ?
								negative.RawDefaultCropOriginV () :
								negative.DefaultCropOriginV ();

	tag_urational_ptr tagDefaultCropOrigin (tcDefaultCropOrigin,
											defaultCropOriginData,
											2);

	rawIFD.Add (&tagDefaultCropOrigin);

	

	dng_urational defaultCropSizeData [2];

	defaultCropSizeData [0] = negative.RawDefaultCropSizeH ().IsValid () ?
							  negative.RawDefaultCropSizeH () :
							  negative.DefaultCropSizeH ();
							  
	defaultCropSizeData [1] = negative.RawDefaultCropSizeV ().IsValid () ?
							  negative.RawDefaultCropSizeV () :
							  negative.DefaultCropSizeV ();

	tag_urational_ptr tagDefaultCropSize (tcDefaultCropSize,
										  defaultCropSizeData,
										  2);

	rawIFD.Add (&tagDefaultCropSize);

	

	dng_urational defaultUserCropData [4];

	defaultUserCropData [0] = negative.DefaultUserCropT ();
	defaultUserCropData [1] = negative.DefaultUserCropL ();
	defaultUserCropData [2] = negative.DefaultUserCropB ();
	defaultUserCropData [3] = negative.DefaultUserCropR ();

	tag_urational_ptr tagDefaultUserCrop (tcDefaultUserCrop,
										  defaultUserCropData,
										  4);

	if (negative.HasDefaultUserCrop ())
		{

		rawIFD.Add (&tagDefaultUserCrop);

		}
	
	
	
	range_tag_set rangeSet (rawIFD, negative);
						  
	
	
	mosaic_tag_set mosaicSet (rawIFD, mosaicInfo);
			
	
	
	tag_urational tagChromaBlurRadius (tcChromaBlurRadius,
									   negative.ChromaBlurRadius ());
									  
	if (negative.ChromaBlurRadius ().IsValid ())
		{
		
		rawIFD.Add (&tagChromaBlurRadius);
		
		}
	
	
	
	tag_urational tagAntiAliasStrength (tcAntiAliasStrength,
										negative.AntiAliasStrength ());
									  
	if (negative.AntiAliasStrength ().IsValid ())
		{
		
		rawIFD.Add (&tagAntiAliasStrength);
		
		}
		
	
	
	uint32 extraProfileCount = (uint32) extraProfileIndex.size ();
	
	tag_big_uints extraProfileTag (tcExtraCameraProfiles,
								   extraProfileCount);
									
	if (extraProfileCount)
		{
		
		mainIFD.Add (&extraProfileTag);

		}
			
	
	
	tag_uint16 tagOrientation (tcOrientation,
							   (uint16) negative.ComputeOrientation (constMetadata).GetTIFF ());
							   
	mainIFD.Add (&tagOrientation);

	tag_srational tagBaselineExposure (tcBaselineExposure,
									   negative.BaselineExposureR ());
										  
	mainIFD.Add (&tagBaselineExposure);

	tag_urational tagBaselineNoise (tcBaselineNoise,
									negative.BaselineNoiseR ());
										  
	mainIFD.Add (&tagBaselineNoise);
	
	dng_urational rawNoiseReductionApplied = hasEnhancedImage ?
											 negative.RawNoiseReductionApplied () :
											 negative.NoiseReductionApplied ();

	tag_urational tagNoiseReductionAppliedMainIFD (tcNoiseReductionApplied,
												   rawNoiseReductionApplied);
											
	tag_urational tagNoiseReductionAppliedRawIFD (tcNoiseReductionApplied,
												  rawNoiseReductionApplied);
		
	if (rawNoiseReductionApplied.IsValid ())
		{
		
		rawIFD.Add (&tagNoiseReductionAppliedRawIFD);

		
		
		
		

		if ((&rawIFD) != (&mainIFD))
			{
			
			mainIFD.Add (&tagNoiseReductionAppliedMainIFD);
	  
			}
	
		}
  
	dng_noise_profile rawNoiseProfile = hasEnhancedImage ?
										negative.RawNoiseProfile () :
										negative.NoiseProfile	 ();

	tag_dng_noise_profile tagNoiseProfileMainIFD (rawNoiseProfile);
	tag_dng_noise_profile tagNoiseProfileRawIFD	 (rawNoiseProfile);
		
	if (rawNoiseProfile.IsValidForNegative (negative))
		{

		rawIFD.Add (&tagNoiseProfileRawIFD);

		
		
		
		

		if ((&rawIFD) != (&mainIFD))
			{

			mainIFD.Add (&tagNoiseProfileMainIFD);
			
			}
		
		}

	tag_urational tagBaselineSharpness (tcBaselineSharpness,
										negative.RawBaselineSharpness ().IsValid () && hasEnhancedImage ?
										negative.RawBaselineSharpness () :
										negative.BaselineSharpnessR ());
										  
	mainIFD.Add (&tagBaselineSharpness);

	tag_string tagUniqueName (tcUniqueCameraModel,
							  negative.ModelName (),
							  true);
								
	mainIFD.Add (&tagUniqueName);
	
	tag_string tagLocalName (tcLocalizedCameraModel,
							 negative.LocalName (),
							 false);
						   
	if (negative.LocalName ().NotEmpty ())
		{
		
		mainIFD.Add (&tagLocalName);
	
		}
	
	tag_urational tagShadowScale (tcShadowScale,
								  negative.ShadowScaleR ());
										  
	mainIFD.Add (&tagShadowScale);
	
	tag_uint16 tagColorimetricReference (tcColorimetricReference,
										 (uint16) negative.ColorimetricReference ());
										 
	if (negative.IsOutputReferred ())
		{
		
		mainIFD.Add (&tagColorimetricReference);
		
		}
		
	const bool useNewDigest = (maxBackwardVersion >= dngVersion_1_4_0_0);

	dng_fingerprint mainImageRawImageDigest;
	
	if (rawLossyCompressedImage)
		{
		
		negative.FindRawLossyCompressedImageDigest (host);
		
		mainImageRawImageDigest = negative.RawLossyCompressedImageDigest ();
		
		}
		
	else if (rawLossyCompressedTransparencyMask)
		{
		
		
		
		
		}
		
	else
		{
		
		if (useNewDigest)
			{
			negative.FindNewRawImageDigest (host);
			mainImageRawImageDigest = negative.NewRawImageDigest ();
			}
		
		else
			{
			negative.FindRawImageDigest (host);
			mainImageRawImageDigest = negative.RawImageDigest ();
			}
		
		}
	
	tag_uint8_ptr tagRawImageDigest (useNewDigest ? tcNewRawImageDigest : tcRawImageDigest,
									 mainImageRawImageDigest.Data (),
									 16);
									 
	if (mainImageRawImageDigest.IsValid ())
		{

		mainIFD.Add (&tagRawImageDigest);
		
		}
	
	negative.FindRawDataUniqueID (host);

	

	const auto rawDataUniqueID = negative.RawDataUniqueID ();
	
	tag_uint8_ptr tagRawDataUniqueID (tcRawDataUniqueID,
									  rawDataUniqueID.Data (),
									  16);
									  
	if (rawDataUniqueID.IsValid ())
		{
							   
		mainIFD.Add (&tagRawDataUniqueID);
		
		}
	
	tag_string tagOriginalRawFileName (tcOriginalRawFileName,
									   negative.OriginalRawFileName (),
									   false);
						   
	if (negative.HasOriginalRawFileName ())
		{
		
		mainIFD.Add (&tagOriginalRawFileName);
	
		}
		
	negative.FindOriginalRawFileDigest ();
		
	tag_data_ptr tagOriginalRawFileData (tcOriginalRawFileData,
										 ttUndefined,
										 negative.OriginalRawFileDataLength (),
										 negative.OriginalRawFileData		());
										 
	tag_uint8_ptr tagOriginalRawFileDigest (tcOriginalRawFileDigest,
											negative.OriginalRawFileDigest ().Data (),
											16);
										 
	if (negative.OriginalRawFileData ())
		{
		
		mainIFD.Add (&tagOriginalRawFileData);
		
		mainIFD.Add (&tagOriginalRawFileDigest);
	
		}

	

	#if qDNGUseXMP
		
	tag_xmp tagXMP (metadata->GetXMP ());
	
	if (tagXMP.Count ())
		{
		
		mainIFD.Add (&tagXMP);
		
		}
		
	#endif
	
	
	
	exif_tag_set exifSet (mainIFD,
						  *metadata->GetExif (),
						  metadata->IsMakerNoteSafe (),
						  metadata->MakerNoteData	(),
						  metadata->MakerNoteLength (),
						  true);
						
	
	
	tag_uint8_ptr tagPrivateData (tcDNGPrivateData,
								  negative.PrivateData (),
								  negative.PrivateLength ());
						   
	if (negative.PrivateLength ())
		{
		
		mainIFD.Add (&tagPrivateData);
		
		}
		
	
	
	uint32 originalDefaultFinalSizeData [2];
	
	originalDefaultFinalSizeData [0] = negative.OriginalDefaultFinalSize ().h;
	originalDefaultFinalSizeData [1] = negative.OriginalDefaultFinalSize ().v;
	
	tag_uint32_ptr tagOriginalDefaultFinalSize (tcOriginalDefaultFinalSize,
												originalDefaultFinalSizeData,
												2);
	
	if (saveOriginalDefaultFinalSize)
		{
		
		mainIFD.Add (&tagOriginalDefaultFinalSize);
		
		}
		
	uint32 originalBestQualityFinalSizeData [2];
	
	originalBestQualityFinalSizeData [0] = negative.OriginalBestQualityFinalSize ().h;
	originalBestQualityFinalSizeData [1] = negative.OriginalBestQualityFinalSize ().v;
	
	tag_uint32_ptr tagOriginalBestQualityFinalSize (tcOriginalBestQualityFinalSize,
													originalBestQualityFinalSizeData,
													2);
	
	if (saveOriginalBestQualityFinalSize)
		{
		
		mainIFD.Add (&tagOriginalBestQualityFinalSize);
		
		}
		
	dng_urational originalDefaultCropSizeData [2];
	
	originalDefaultCropSizeData [0] = negative.OriginalDefaultCropSizeH ();
	originalDefaultCropSizeData [1] = negative.OriginalDefaultCropSizeV ();
	
	tag_urational_ptr tagOriginalDefaultCropSize (tcOriginalDefaultCropSize,
												  originalDefaultCropSizeData,
												  2);
	
	if (saveOriginalDefaultCropSize)
		{
		
		mainIFD.Add (&tagOriginalDefaultCropSize);
		
		}
		
	
	
	AutoPtr<dng_memory_block> opcodeList1Data (negative.OpcodeList1 ().Spool (host));
	
	tag_data_ptr tagOpcodeList1 (tcOpcodeList1,
								 ttUndefined,
								 opcodeList1Data.Get () ? opcodeList1Data->LogicalSize () : 0,
								 opcodeList1Data.Get () ? opcodeList1Data->Buffer	   () : NULL);
								 
	if (opcodeList1Data.Get ())
		{
		
		rawIFD.Add (&tagOpcodeList1);
		
		}
		
	
	
	AutoPtr<dng_memory_block> opcodeList2Data (negative.OpcodeList2 ().Spool (host));
	
	tag_data_ptr tagOpcodeList2 (tcOpcodeList2,
								 ttUndefined,
								 opcodeList2Data.Get () ? opcodeList2Data->LogicalSize () : 0,
								 opcodeList2Data.Get () ? opcodeList2Data->Buffer	   () : NULL);
								 
	if (opcodeList2Data.Get ())
		{
		
		rawIFD.Add (&tagOpcodeList2);
		
		}
		
	
	
	AutoPtr<dng_memory_block> opcodeList3Data (negative.OpcodeList3 ().Spool (host));
	
	tag_data_ptr tagOpcodeList3 (tcOpcodeList3,
								 ttUndefined,
								 opcodeList3Data.Get () ? opcodeList3Data->LogicalSize () : 0,
								 opcodeList3Data.Get () ? opcodeList3Data->Buffer	   () : NULL);
								 
	if (opcodeList3Data.Get ())
		{
		
		rawIFD.Add (&tagOpcodeList3);
		
		}

	

	AutoPtr<dng_memory_block> profileGainTableMapBlock;

	uint16 tagCodeForPGTM = tcProfileGainTableMap;
	
	if (negative.HasProfileGainTableMap () &&

		
		

		!mainProfileDidWritePGTMtoMainIFD)
		{

		dng_memory_stream tempStream (host.Allocator (),
									  host.Sniffer ());

		const auto &gainTableMap = negative.ProfileGainTableMap ();

		gainTableMap.PutStream (tempStream);

		profileGainTableMapBlock.Reset
			(tempStream.AsMemoryBlock (host.Allocator ()));

		if (gainTableMap.RequiresVersion2 ())
			tagCodeForPGTM = tcProfileGainTableMap2;

		}
		
	tag_data_ptr tagProfileGainTableMapBlock
		(tagCodeForPGTM,
		 ttUndefined,
		 profileGainTableMapBlock.Get () ? profileGainTableMapBlock->LogicalSize () : 0,
		 profileGainTableMapBlock.Get () ? profileGainTableMapBlock->Buffer		 () : NULL);
								 
	if (profileGainTableMapBlock.Get ())
		{

		if (tagCodeForPGTM == tcProfileGainTableMap2)
			mainIFD.Add (&tagProfileGainTableMapBlock);

		else
			rawIFD.Add (&tagProfileGainTableMapBlock);

		}

	

	AutoPtr<tiff_tag> tagImageSequenceInfo;

	if (metadata->ImageSequenceInfo ().IsValid ())
		{
		
		tagImageSequenceInfo.Reset
			(metadata->ImageSequenceInfo ().MakeTag (host.Allocator ()));

		mainIFD.Add (tagImageSequenceInfo.Get ());
		
		}

	

	AutoPtr<tiff_tag> tagImageStats;

		{

		const auto &stats = metadata->ImageStats ();

		if (stats.IsValidForPlaneCount (negative.ColorChannels ()) &&
			(stats.TagCount () > 0))
			{
		
			tagImageStats.Reset (stats.MakeTag (host.Allocator ()));

			rawIFD.Add (tagImageStats.Get ());
		
			}

		}

	
	
	AutoPtr<dng_ifd> maskInfo;
	
	AutoPtr<dng_tiff_directory> maskIFD;
	
	AutoPtr<dng_basic_tag_set> maskBasic;
	
	if (hasTransparencyMask)
		{
		
		
		
		maskInfo.Reset (new dng_ifd);
		
		maskInfo->fNewSubFileType = sfTransparencyMask;
		
		maskInfo->fImageWidth  = negative.RawTransparencyMask ()->Bounds ().W ();
		maskInfo->fImageLength = negative.RawTransparencyMask ()->Bounds ().H ();
		
		maskInfo->fSamplesPerPixel = 1;
		
		maskInfo->fBitsPerSample [0] = negative.RawTransparencyMaskBitDepth ();
		
		maskInfo->fPhotometricInterpretation = piTransparencyMask;

		SetEncodeSettingsForIFD (*negative.RawTransparencyMask (),
								 uncompressed,
								 maxBackwardVersion,
								 *maskInfo,
								 rawLossyCompressedTransparencyMask);
			
		
			
		maskIFD.Reset (new dng_tiff_directory);
		
		
		
		maskBasic.Reset (new dng_basic_tag_set (*maskIFD, *maskInfo));
				
		}
  
	AutoPtr<dng_ifd> depthInfo;
	
	AutoPtr<dng_tiff_directory> depthIFD;
	
	AutoPtr<dng_basic_tag_set> depthBasic;
	
	tag_uint16 tagDepthFormat (tcDepthFormat,
							   (uint16) negative.DepthFormat ());
		
	tag_urational tagDepthNear (tcDepthNear,
								negative.DepthNear ());
		
	tag_urational tagDepthFar (tcDepthFar,
							   negative.DepthFar ());
		
	tag_uint16 tagDepthUnits (tcDepthUnits,
							  (uint16) negative.DepthUnits ());
		
	tag_uint16 tagDepthMeasureType (tcDepthMeasureType,
									(uint16) negative.DepthMeasureType ());
		
	if (hasDepthMap)
		{

		
	
		depthInfo.Reset (new dng_ifd);
	
		depthInfo->fNewSubFileType = sfDepthMap;
	
		depthInfo->fImageWidth	= negative.RawDepthMap ()->Bounds ().W ();
		depthInfo->fImageLength = negative.RawDepthMap ()->Bounds ().H ();
	
		depthInfo->fSamplesPerPixel = 1;
	
		depthInfo->fBitsPerSample [0] = negative.RawDepthMap ()->PixelSize () * 8;
	
		depthInfo->fPhotometricInterpretation = piDepth;

		SetEncodeSettingsForIFD (*negative.RawDepthMap (),
								 uncompressed,
								 maxBackwardVersion,
								 *depthInfo,
								 negative.RawLossyCompressedDepthMap ());

		
	
		depthIFD.Reset (new dng_tiff_directory);
	
		
	
		depthBasic.Reset (new dng_basic_tag_set (*depthIFD, *depthInfo));
		
		
		
		mainIFD.Add (&tagDepthFormat);
		mainIFD.Add (&tagDepthNear);
		mainIFD.Add (&tagDepthFar);
		mainIFD.Add (&tagDepthUnits);
		mainIFD.Add (&tagDepthMeasureType);
	
		}
		
	
	
	AutoPtr<dng_ifd> enhancedInfo;
	
	AutoPtr<dng_tiff_directory> enhancedIFD;
	
	AutoPtr<dng_basic_tag_set> enhancedBasic;
	
	tag_string enhanceParams (tcEnhanceParams,
							  negative.EnhanceParams (),
							  false);
	
	tag_urational enhanceBaselineSharpness (tcBaselineSharpness,
											negative.BaselineSharpnessR ());
		
	tag_urational enhanceNoiseReductionApplied (tcNoiseReductionApplied,
												negative.NoiseReductionApplied ());
		
	tag_dng_noise_profile enhanceNoiseProfile (negative.NoiseProfile ());
		
	uint16 enhanceBlackLevelData [kMaxColorPlanes];
	
	tag_uint16_ptr enhanceBlackLevel (tcBlackLevel,
									  enhanceBlackLevelData);

	dng_urational enhanceDefaultScaleData [2];
	
	enhanceDefaultScaleData [0] = negative.DefaultScaleH ();
	enhanceDefaultScaleData [1] = negative.DefaultScaleV ();
												
	tag_urational_ptr enhanceDefaultScale (tcDefaultScale,
										   enhanceDefaultScaleData,
										   2);

	tag_urational enhanceBestQualityScale (tcBestQualityScale,
										   negative.BestQualityScale ());
									  
	dng_urational enhanceDefaultCropOriginData [2];

	enhanceDefaultCropOriginData [0] = negative.DefaultCropOriginH ();
	enhanceDefaultCropOriginData [1] = negative.DefaultCropOriginV ();

	tag_urational_ptr enhanceDefaultCropOrigin (tcDefaultCropOrigin,
												enhanceDefaultCropOriginData,
												2);

	dng_urational enhanceDefaultCropSizeData [2];

	enhanceDefaultCropSizeData [0] = negative.DefaultCropSizeH ();
	enhanceDefaultCropSizeData [1] = negative.DefaultCropSizeV ();

	tag_urational_ptr enhanceDefaultCropSize (tcDefaultCropSize,
											  enhanceDefaultCropSizeData,
											  2);

	if (hasEnhancedImage)
		{
		
		
	
		enhancedInfo.Reset (new dng_ifd);
	
		enhancedInfo->fNewSubFileType = sfEnhancedImage;
	
		enhancedInfo->fImageWidth  = negative.Stage3Image ()->Bounds ().W ();
		enhancedInfo->fImageLength = negative.Stage3Image ()->Bounds ().H ();
	
		enhancedInfo->fSamplesPerPixel = negative.Stage3Image ()->Planes ();

		if (enhancedInfo->fSamplesPerPixel > kMaxColorPlanes)
			{
			ThrowBadFormat ();
			}

		const bool isEnhancedFloat =
			(negative.Stage3Image ()->PixelType () == ttFloat);
		
		for (uint32 plane = 0; plane < enhancedInfo->fSamplesPerPixel; plane++)
			{

			if (isEnhancedFloat)
				enhancedInfo->fBitsPerSample [plane] = 16;

			else
				enhancedInfo->fBitsPerSample [plane] =
					negative.Stage3Image ()->PixelSize () * 8;

			}
		
		enhancedInfo->fPhotometricInterpretation = piLinearRaw;

		SetEncodeSettingsForIFD (*negative.Stage3Image (),
								 uncompressed,
								 maxBackwardVersion,
								 *enhancedInfo,
								 negative.EnhancedLossyCompressedImage ());
			
		
	
		enhancedIFD.Reset (new dng_tiff_directory);
	
		
	
		enhancedBasic.Reset (new dng_basic_tag_set (*enhancedIFD, *enhancedInfo));
		
		
		
		enhancedIFD->Add (&enhanceParams);
		
		
		
		if (negative.RawBaselineSharpness () != negative.BaselineSharpnessR ())
			{
			
			enhancedIFD->Add (&enhanceBaselineSharpness);
		
			}
			
		
		
		if (negative.RawNoiseReductionApplied () != negative.NoiseReductionApplied ())
			{
			
			enhancedIFD->Add (&enhanceNoiseReductionApplied);
		
			}
			
		
		
		if (negative.RawNoiseProfile () != negative.NoiseProfile ())
			{
			
			enhancedIFD->Add (&enhanceNoiseProfile);
		
			}
			
		
		
		if (negative.Stage3BlackLevel ())
			{
			
			for (uint32 plane = 0; plane < enhancedInfo->fSamplesPerPixel; plane++)
				{
				
				enhanceBlackLevelData [plane] = negative.Stage3BlackLevel ();

				}
				
			enhanceBlackLevel.SetCount (enhancedInfo->fSamplesPerPixel);
			
			enhancedIFD->Add (&enhanceBlackLevel);

			}
			
		
		
		if (negative.RawDefaultScaleH () != negative.DefaultScaleH () ||
			negative.RawDefaultScaleV () != negative.DefaultScaleV ())
			{
			
			enhancedIFD->Add (&enhanceDefaultScale);
			
			}
		
		
		
		if (negative.RawBestQualityScale () != negative.BestQualityScale ())
			{
			
			enhancedIFD->Add (&enhanceBestQualityScale);

			}
			
		
		
		if (negative.RawDefaultCropSizeH   () != negative.DefaultCropSizeH	 () ||
			negative.RawDefaultCropSizeV   () != negative.DefaultCropSizeV	 () ||
			negative.RawDefaultCropOriginH () != negative.DefaultCropOriginH () ||
			negative.RawDefaultCropOriginV () != negative.DefaultCropOriginV ())
			{
			
			enhancedIFD->Add (&enhanceDefaultCropSize);

			enhancedIFD->Add (&enhanceDefaultCropOrigin);

			}
			
		}

	

	std::vector<std::unique_ptr<dng_ifd> >			  semanticMaskInfo;
	std::vector<std::unique_ptr<dng_tiff_directory> > semanticMaskIFD;
	std::vector<std::unique_ptr<dng_basic_tag_set> >  semanticMaskBasic;
	std::vector<std::unique_ptr<tag_string> >		  semanticNameTags;
	std::vector<std::unique_ptr<tag_string> >		  semanticInstanceIDTags;
	std::vector<std::unique_ptr<tag_data_ptr> >		  semanticXMPTags;
	std::vector<std::unique_ptr<tag_uint32_ptr> >	  semanticMaskSubAreaTags;

	uint32 semanticMaskCount = 0;
	
	if (negative.HasSemanticMask ())
		{
		
		semanticMaskCount = negative.NumSemanticMasks ();

		semanticMaskInfo	   .resize (semanticMaskCount);
		semanticMaskIFD		   .resize (semanticMaskCount);
		semanticMaskBasic	   .resize (semanticMaskCount);
		semanticNameTags	   .resize (semanticMaskCount);
		semanticInstanceIDTags .resize (semanticMaskCount);
		semanticXMPTags		   .resize (semanticMaskCount);
		semanticMaskSubAreaTags.resize (semanticMaskCount);
		
		for (uint32 j = 0; j < semanticMaskCount; j++)
			{

			semanticMaskInfo [j].reset (new dng_ifd);

			auto &smInfo = *semanticMaskInfo [j];

			smInfo.fNewSubFileType = sfSemanticMask;

			const auto &rawMask = negative.RawSemanticMask (j);
			
			smInfo.fImageWidth	= rawMask.fMask->Width	();
			smInfo.fImageLength = rawMask.fMask->Height ();
	
			smInfo.fSamplesPerPixel = 1;

			smInfo.fBitsPerSample [0] = rawMask.fMask->PixelSize () * 8;

			smInfo.fPhotometricInterpretation = piPhotometricMask;

			SetEncodeSettingsForIFD (*rawMask.fMask,
									 uncompressed,
									 maxBackwardVersion,
									 smInfo,
									 rawMask.fLossyCompressed.get ());

			
	
			semanticMaskIFD [j].reset (new dng_tiff_directory);
	
			
	
			semanticMaskBasic [j].reset
				(new dng_basic_tag_set (*semanticMaskIFD [j],
										smInfo));

			

			if (rawMask.fName.NotEmpty ())
				{
				
				semanticNameTags [j].reset (new tag_string (tcSemanticName,
															rawMask.fName,
															false));

				

				semanticMaskIFD [j]->Add (semanticNameTags [j].get ());
				
				}
			
			

			if (rawMask.fInstanceID.NotEmpty ())
				{
				
				semanticInstanceIDTags [j].reset (new tag_string (tcSemanticInstanceID,
																  rawMask.fInstanceID,
																  false));

				

				semanticMaskIFD [j]->Add (semanticInstanceIDTags [j].get ());
				
				}

			

			if (rawMask.fXMP)
				{
				
				semanticXMPTags [j].reset (new tag_data_ptr (tcXMP,
															 ttUndefined,
															 rawMask.fXMP->LogicalSize (),
															 rawMask.fXMP->Buffer	   ()));
				
				semanticMaskIFD [j]->Add (semanticXMPTags [j].get ());
				
				}
			
			

			if (rawMask.IsMaskSubAreaValid ())
				{
				
				semanticMaskSubAreaTags [j].reset
					(new tag_uint32_ptr (tcMaskSubArea,
										 &rawMask.fMaskSubArea [0],
										 4));

				

				semanticMaskIFD [j]->Add (semanticMaskSubAreaTags [j].get ());
				
				}
			
			}
		
		}

	
		
	uint32 subFileCount = thumbnail ? 1 : 0;
	
	if (hasTransparencyMask)
		{
		subFileCount++;
		}
	
	if (hasDepthMap)
		{
		subFileCount++;
		}
		
	if (hasEnhancedImage)
		{
		subFileCount++;
		}

	if (hasGainMap)
		{
		subFileCount++;
		}

	subFileCount += semanticMaskCount;
	
	
	
	uint32 previewCount = previewList ? previewList->Count () : 0;
	
	std::vector<std::unique_ptr<dng_tiff_directory>> previewIFD (previewCount);
	
	std::vector<std::unique_ptr<dng_basic_tag_set>> previewBasic (previewCount);
	
	for (uint32 j = 0; j < previewCount; j++)
		{
		
		if (thumbnail != &previewList->Preview (j))
			{
		
			previewIFD [j] . reset (new dng_tiff_directory);
			
			previewBasic [j] . reset (previewList->Preview (j).AddTagSet (host,
																		  *previewIFD [j]));
				
			subFileCount++;
			
			}
		
		}
		
	
	
	tag_big_uints tagSubFile (tcSubIFDs,
							  subFileCount);
							   
	if (subFileCount)
		{
	
		mainIFD.Add (&tagSubFile);
		
		}
		
	
	
	
	const uint64 kMaxTarget32BitDNGSize = 0xFFFFFFFF - 256 * 1024 * 1024;
	
	
	
	bool isBigTIFF = false;
	
	uint64 headerSize32 = 0;
	
	uint64 maxNonHeaderSize = 0;
	
	#if qForceWriteBigDNG || qStressTestBigDNG
	allowBigTIFF = true;
	#endif
	
	for (uint32 pass = 1; pass <= 3; pass++)
		{
		
		
		
		if (pass == 1)
			{
			isBigTIFF = false;
			}
			
		
		
		else if (pass == 2)
			{
			
			#if qForceWriteBigDNG
			
			isBigTIFF = true;
			
			#else

			if (allowBigTIFF)
				{
				isBigTIFF = (headerSize32 + maxNonHeaderSize > kMaxTarget32BitDNGSize);
				}
				
			#endif

			}
			
		
		
		else
			{
			
			#if !qForceWriteBigDNG
			
			if (stream.Length () <= kMaxTarget32BitDNGSize)
				{
				isBigTIFF = false;
				}
				
			else if (stream.Length () > 0xFFFFFFFF)
				{
				DNG_REQUIRE (isBigTIFF, "Bad maxNonHeaderSize estimate");
				}
				
			#endif

			}
			
		
		
		uint64 currentOffset = isBigTIFF ? 16 : 8;
		
		#if qStressTestBigDNG
		currentOffset += kStressTestBytes;
		#endif

		uint64 mainIFDOffset = currentOffset;
		
		mainIFD.SetBigTIFF (isBigTIFF);
		
		currentOffset += mainIFD.Size ();
		
		exifSet.SetBigTIFF (isBigTIFF);

		exifSet.Locate (currentOffset);
		
		currentOffset += exifSet.Size ();

		uint32 subFileIndex = 0;
		
		if (thumbnail)
			{
			
			rawIFD.SetBigTIFF (isBigTIFF);
		
			tagSubFile.Set (subFileIndex++, currentOffset);
		
			currentOffset += rawIFD.Size ();
			
			}
			
		if (hasTransparencyMask)
			{
			
			maskIFD->SetBigTIFF (isBigTIFF);
			
			tagSubFile.Set (subFileIndex++, currentOffset);
			
			currentOffset += maskIFD->Size ();
			
			}
  
		if (hasDepthMap)
			{
			
			depthIFD->SetBigTIFF (isBigTIFF);
			
			tagSubFile.Set (subFileIndex++, currentOffset);
			
			currentOffset += depthIFD->Size ();
			
			}
		
		if (hasEnhancedImage)
			{
			
			enhancedIFD->SetBigTIFF (isBigTIFF);
			
			tagSubFile.Set (subFileIndex++, currentOffset);
			
			currentOffset += enhancedIFD->Size ();
			
			}

		if (hasGainMap)
			{

			gainMapIFD.SetBigTIFF (isBigTIFF);
			
			tagSubFile.Set (subFileIndex++, currentOffset);
			
			currentOffset += gainMapIFD.Size ();
			
			}

		for (uint32 j = 0; j < semanticMaskCount; j++)
			{
			
			semanticMaskIFD [j]->SetBigTIFF (isBigTIFF);

			tagSubFile.Set (subFileIndex++, currentOffset);
			
			currentOffset += semanticMaskIFD [j]->Size ();
			
			}

		for (uint32 j = 0; j < previewCount; j++)
			{
			
			if (thumbnail != &previewList->Preview (j))
				{
				
				previewIFD [j]->SetBigTIFF (isBigTIFF);
		
				tagSubFile.Set (subFileIndex++, currentOffset);

				currentOffset += previewIFD [j]->Size ();
				
				}
			
			}
			
		
		
		if (pass == 3)
			{
			
			stream.SetWritePosition (0);
			
			stream.Put_uint16 (stream.BigEndian () ? byteOrderMM : byteOrderII);
			
			stream.Put_uint16 (isBigTIFF ? magicBigTIFF : magicTIFF);
			
			if (isBigTIFF)
				{
				stream.Put_uint16 (8);
				stream.Put_uint16 (0);
				stream.Put_uint64 (mainIFDOffset);
				}
			else
				{
				stream.Put_uint32 ((uint32) mainIFDOffset);
				}
			
			#if qStressTestBigDNG
			stream.PutZeros (kStressTestBytes);
			#endif
		
			
			
			mainIFD.Put (stream);
			
			exifSet.Put (stream);
	
			if (thumbnail)
				{
			
				rawIFD.Put (stream);
				
				}
			
			if (hasTransparencyMask)
				{
				
				maskIFD->Put (stream);
				
				}
				
			if (hasDepthMap)
				{
				
				depthIFD->Put (stream);
				
				}
				
			if (hasEnhancedImage)
				{
				
				enhancedIFD->Put (stream);
				
				}

			if (hasGainMap)
				{

				gainMapIFD.Put (stream);
				
				}

			for (uint32 j = 0; j < semanticMaskCount; j++)
				{
					
				semanticMaskIFD [j]->Put (stream);
					
				}

			for (uint32 j = 0; j < previewCount; j++)
				{
				
				if (thumbnail != &previewList->Preview (j))
					{
			
					previewIFD [j]->Put (stream);
					
					}
				
				}
				
			DNG_REQUIRE (currentOffset == stream.Position (),
						 "Header size logic error");
				
			break;		
			
			} 
			
		if (pass == 1)
			{
			
			
		
			headerSize32 = currentOffset;
			
			}
			
		else
			{
			
			
			
			stream.PutZeros (currentOffset);
			
			}
		
		
		
		if (extraProfileCount)
			{
			
			for (uint32 j = 0; j < extraProfileCount; j++)
				{
				
				uint32 index = extraProfileIndex [j];
				
				const dng_camera_profile &profile (negative.ProfileByIndex (index));

				tiff_dng_extended_color_profile extraWriter (profile, false);
					
				uint64 estimatedSize = extraWriter.DataSize ();
								
				if (pass == 1)
					{
					
					maxNonHeaderSize += estimatedSize;
					
					}
				
				else
					{
					
					DNG_VERIFY_SIZE_ESTIMATE (stream, estimatedSize, "Extra profile data");
					
					extraProfileTag.Set (j, stream.Position ());

					extraWriter.Put (host, stream);
					
					}
					
				}
			
			}
		
		
		
		if (thumbnail)
			{
			
			uint64 estimatedSize = thumbnail->MaxImageDataByteCount ();
								
			if (pass == 1)
				{
				
				maxNonHeaderSize += estimatedSize;
				
				}
				
			else
				{
		
				DNG_VERIFY_SIZE_ESTIMATE (stream, estimatedSize, "Thumbnail data");
					
				thumbnail->WriteData (host,
									  *this,
									  *thmBasic,
									  stream);
									  
				}
								 
			}
	
		
		
		for (uint32 j = 0; j < previewCount; j++)
			{
			
			if (thumbnail != &previewList->Preview (j))
				{
		
				uint64 estimatedSize = previewList->Preview (j).MaxImageDataByteCount ();
								
				if (pass == 1)
					{
					
					maxNonHeaderSize += estimatedSize;
					
					}
					
				else
					{
					
					DNG_VERIFY_SIZE_ESTIMATE (stream, estimatedSize, "Preview data");
					
					previewList->Preview (j).WriteData (host,
														*this,
														*previewBasic [j],
														stream);
														
					}
													
				}
				
			}
		
		
		
		if (rawLossyCompressedImage)
			{

			const auto &img = *rawLossyCompressedImage;
			
			if (pass == 1)
				{
				
				maxNonHeaderSize += img.NonHeaderSize ();
				
				}
				
			else
				{
				
				img.WriteData (stream, rawBasic);

				}
			
			}

		else
			{
			
			uint64 estimatedSize = info.MaxImageDataByteCount ();
			
			if (pass == 1)
				{
				
				maxNonHeaderSize += estimatedSize;
					
				}
				
			else
				{
					
				DNG_VERIFY_SIZE_ESTIMATE (stream, estimatedSize, "Raw image data");
					
				#if qDNGValidate
				dng_timer timer ("Write raw image time");
				#endif

				WriteImage (host,
							info,
							rawBasic,
							stream,
							rawImage,
							fakeChannels,
							nullptr);

				}
						
			}
		
		
		
		if (hasTransparencyMask)
			{
			
			
			
			if (rawLossyCompressedTransparencyMask)
				{

				const auto &img = *rawLossyCompressedTransparencyMask;
				
				if (pass == 1)
					{
					
					maxNonHeaderSize += img.NonHeaderSize ();
					
					}
					
				else
					{
					
					img.WriteData (stream, *maskBasic);

					}
					
				}
				
			else
				{
				
				uint64 estimatedSize = maskInfo->MaxImageDataByteCount ();
				
				if (pass == 1)
					{
					
					maxNonHeaderSize += estimatedSize;
						
					}
					
				else
					{
					
					DNG_VERIFY_SIZE_ESTIMATE (stream, estimatedSize, "Transparency mask data");
						
					#if qDNGValidate
					dng_timer timer ("Write transparency mask time");
					#endif
				
					WriteImage (host,
								*maskInfo,
								*maskBasic,
								stream,
								*negative.RawTransparencyMask ());
								
					}
					
				}
						
			}
	  
		
		
		if (hasDepthMap)
			{
			
			
			
			if (negative.RawLossyCompressedDepthMap ())
				{

				const auto &img = *(negative.RawLossyCompressedDepthMap ());
				
				if (pass == 1)
					{
					
					maxNonHeaderSize += img.NonHeaderSize ();
					
					}
					
				else
					{
					
					img.WriteData (stream, *depthBasic);
					
					}
					
				}
				
			else
				{
				
				uint64 estimatedSize = depthInfo->MaxImageDataByteCount ();
				
				if (pass == 1)
					{
					
					maxNonHeaderSize += estimatedSize;
						
					}
					
				else
					{
					
					DNG_VERIFY_SIZE_ESTIMATE (stream, estimatedSize, "Depth map data");
						
					#if qDNGValidate
					dng_timer timer ("Write depth map time");
					#endif
				
					WriteImage (host,
								*depthInfo,
								*depthBasic,
								stream,
								*negative.RawDepthMap ());
						
					}
					
				}
				
			}

		
		
		if (hasEnhancedImage)
			{
			
			
			
			if (negative.EnhancedLossyCompressedImage ())
				{

				const auto &img = *(negative.EnhancedLossyCompressedImage ());
				
				if (pass == 1)
					{
					
					maxNonHeaderSize += img.NonHeaderSize ();
					
					}
					
				else
					{
					
					img.WriteData (stream, *enhancedBasic);
					
					}
					
				}
				
			else
				{

				uint64 estimatedSize = enhancedInfo->MaxImageDataByteCount ();
				
				if (pass == 1)
					{
					
					maxNonHeaderSize += estimatedSize;
						
					}
					
				else
					{
					
					DNG_VERIFY_SIZE_ESTIMATE (stream, estimatedSize, "Enhanced image data");
						
					#if qDNGValidate
					dng_timer timer ("Write enhanced image time");
					#endif
				
					WriteImage (host,
								*enhancedInfo,
								*enhancedBasic,
								stream,
								*negative.Stage3Image ());
						
					}
					
				}
				
			}

		

		if (hasGainMap)
			{

			
			
			if (gainMapLossyCompressed)
				{

				const auto &img = *gainMapLossyCompressed;
				
				if (pass == 1)
					{
					
					maxNonHeaderSize += img.NonHeaderSize ();
					
					}
					
				else
					{
					
					img.WriteData (stream, *gainMapTagSet);
					
					}
					
				}
				
			else
				{
				
				uint64 estimatedSize = gainMapImageIFD->MaxImageDataByteCount ();
			
				if (pass == 1)
					{
					
					maxNonHeaderSize += estimatedSize;
						
					}
					
				else
					{
					
					DNG_VERIFY_SIZE_ESTIMATE (stream,
											  estimatedSize,
											  "Gain map image data");
						
					#if qDNGValidate
					dng_timer timer ("Write gain map image time");
					#endif
				
					WriteImage (host,
								*gainMapImageIFD,
								*gainMapTagSet,
								stream,
								*gainMapImage);
						
					}

				}
				
			}

		

		for (uint32 j = 0; j < semanticMaskCount; j++)
			{

			const auto &semanticMask = negative.RawSemanticMask (j);

			
			
			if (semanticMask.fLossyCompressed.get ())
				{

				const auto &img = *(semanticMask.fLossyCompressed);
				
				if (pass == 1)
					{
					
					maxNonHeaderSize += img.NonHeaderSize ();
					
					}
					
				else
					{
					
					img.WriteData (stream, *semanticMaskBasic [j]);
					
					}
					
				}
				
			else
				{
				
				uint64 estimatedSize = semanticMaskInfo [j]->MaxImageDataByteCount ();
				
				if (pass == 1)
					{
					
					maxNonHeaderSize += estimatedSize;
						
					}
					
				else
					{
					
					DNG_VERIFY_SIZE_ESTIMATE (stream, estimatedSize, "Semantic mask data");
						
					WriteImage (host,
								*semanticMaskInfo  [j],
								*semanticMaskBasic [j],
								stream,
								*semanticMask.fMask);
								
					}
					
				}
				
			}

		
		
			{
			
			uint64 estimatedSize = bigTableTagSet.DataSize ();
			
			if (pass == 1)
				{
				
				maxNonHeaderSize += estimatedSize;
			
				}
				
			else
				{
			
				DNG_VERIFY_SIZE_ESTIMATE (stream, estimatedSize, "Big table data");
					
				bigTableTagSet.WriteData (stream);
				
				}
				
			}
								
		
		
		if (pass == 2)
			{
		
			stream.SetLength (stream.Position ());
			
			}
			
		
		
		if (pass == 2 && !isBigTIFF)
			{
			
			if (stream.Length () > 0xFFFFFFFF)
				{
				
				if (allowBigTIFF)
					{
					
					
					
					
					DNG_REPORT ("Really bad maxNonHeaderSize estimate");
					
					maxNonHeaderSize = stream.Length () - headerSize32;
					
					stream.SetWritePosition (0);
					
					stream.SetLength (0);
					
					pass--;		
					
					}
					
				else
					{
				
					ThrowImageTooBigDNG ();
					
					}
				
				}

			}
			
		}
	
	stream.Flush ();
	
	}
							   

