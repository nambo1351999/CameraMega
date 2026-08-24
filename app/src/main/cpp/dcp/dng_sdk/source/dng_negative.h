

#ifndef __dng_negative__
#define __dng_negative__

#include "dng_1d_function.h"
#include "dng_auto_ptr.h"
#include "dng_big_table.h"
#include "dng_classes.h"
#include "dng_fingerprint.h"
#include "dng_image.h"
#include "dng_jpeg_image.h"
#include "dng_linearization_info.h"
#include "dng_matrix.h"
#include "dng_memory.h"
#include "dng_mosaic_info.h"
#include "dng_mutex.h"
#include "dng_opcode_list.h"
#include "dng_orientation.h"
#include "dng_rational.h"
#include "dng_sdk_limits.h"
#include "dng_semantic_mask.h"
#include "dng_string.h"
#include "dng_tag_types.h"
#include "dng_tag_values.h"
#include "dng_types.h"
#include "dng_utils.h"
#include "dng_xy_coord.h"

#include <memory>
#include <vector>

#if 1

#define qMetadataOnConst 0
#define METACONST

#else

#define qMetadataOnConst 1
#define METACONST const

#endif

typedef std::vector<dng_camera_profile_metadata> dng_profile_metadata_list;

class dng_noise_function: public dng_1d_function
	{
		
	protected:

		real64 fScale;
		real64 fOffset;

	public:

		

		dng_noise_function ()

			:	fScale	(0.0)
			,	fOffset (0.0)

			{

			}

		

		dng_noise_function (real64 scale,
							real64 offset)

			:	fScale	(scale)
			,	fOffset (offset)

			{

			}

		
		

		virtual real64 Evaluate (real64 x) const
			{
			return sqrt (fScale * x + fOffset);
			}

		

		real64 Scale () const 
			{ 
			return fScale; 
			}

		

		real64 Offset () const 
			{ 
			return fOffset; 
			}

		

		void SetScale (real64 scale)
			{
			fScale = scale;
			}

		

		void SetOffset (real64 offset)
			{
			fOffset = offset;
			}

		

		bool IsValid () const
			{
			return (fScale > 0.0 && fOffset >= 0.0);
			}
		
	};

class dng_noise_profile
	{
		
	protected:

		dng_std_vector<dng_noise_function> fNoiseFunctions;

	public:

		

		dng_noise_profile ();

		

		explicit dng_noise_profile (const dng_std_vector<dng_noise_function> &functions);

		

		bool IsValid () const;

		

		bool IsValidForNegative (const dng_negative &negative) const;

		

		const dng_noise_function & NoiseFunction (uint32 plane) const;

		

		uint32 NumFunctions () const;
  
		
		
		bool operator== (const dng_noise_profile &profile) const;

		bool operator!= (const dng_noise_profile &profile) const
			{
			return !(*this == profile);
			}

	};

class dng_metadata
	{
	
	private:
		
		
		
		
		bool fHasBaseOrientation;
	
		dng_orientation fBaseOrientation;
		
		
		
		
		bool fIsMakerNoteSafe;
		
		
		
		AutoPtr<dng_memory_block> fMakerNote;
		
		
		
		AutoPtr<dng_exif> fExif;
		
		
		
		AutoPtr<dng_exif> fOriginalExif;
		
		
		
		AutoPtr<dng_memory_block> fIPTCBlock;
		
		uint64 fIPTCOffset;
		
		
		
		#if qDNGUseXMP
		AutoPtr<dng_xmp> fXMP;
		#endif
		
		
		
		
		dng_fingerprint fEmbeddedXMPDigest;
		
		
		
		bool fXMPinSidecar;
		
		
		
		
		bool fXMPisNewer;
		
		
		
		dng_string fSourceMIME;
		
		
		
		dng_big_table_dictionary fBigTableDictionary;
		
		
		
		dng_big_table_index fBigTableIndex;

		
		

		dng_big_table_group_index fBigTableGroupIndex;

		

		dng_image_sequence_info fImageSequenceInfo;
		
		

		dng_image_stats fImageStats;
		
	public:

		dng_metadata (dng_host &host);
		
		dng_metadata (const dng_metadata &rhs,
					  dng_memory_allocator &allocator);
		
		virtual ~dng_metadata ();

		
		
		virtual dng_metadata * Clone (dng_memory_allocator &allocator) const;
		
		
			
		void SetBaseOrientation (const dng_orientation &orientation);
		
		

		bool HasBaseOrientation () const
			{
			return fHasBaseOrientation;
			}
		
		
			
		const dng_orientation & BaseOrientation () const
			{
			return fBaseOrientation;
			}
			
		
		
		
		void ApplyOrientation (const dng_orientation &orientation);

		
			
		void SetIPTC (AutoPtr<dng_memory_block> &block,
					  uint64 offset);
		
		void SetIPTC (AutoPtr<dng_memory_block> &block);
		
		void ClearIPTC ();
		
		const void * IPTCData () const;
		
		uint32 IPTCLength () const;
		
		const dng_memory_block & IPTCBlock () const
			{
			return *fIPTCBlock;
			}
		
		uint64 IPTCOffset () const;
		
		dng_fingerprint IPTCDigest (bool includePadding = true) const;
		
		void RebuildIPTC (dng_memory_allocator &allocator,
						  bool padForTIFF);
		
		
		
		void SetMakerNoteSafety (bool safe)
			{
			fIsMakerNoteSafe = safe;
			}
		
		bool IsMakerNoteSafe () const
			{
			return fIsMakerNoteSafe;
			}
		
		void SetMakerNote (AutoPtr<dng_memory_block> &block)
			{
			fMakerNote.Reset (block.Release ());
			}
		
		void ClearMakerNote ()
			{
			fIsMakerNoteSafe = false;
			fMakerNote.Reset ();
			}
		
		const void * MakerNoteData () const
			{
			return fMakerNote.Get () ? fMakerNote->Buffer ()
									 : NULL;
			}
		
		uint32 MakerNoteLength () const
			{
			return fMakerNote.Get () ? fMakerNote->LogicalSize ()
									 : 0;
			}
		
		
		
		dng_exif * GetExif ()
			{
			return fExif.Get ();
			}
			
		const dng_exif * GetExif () const
			{
			return fExif.Get ();
			}
			
		template< class E >
		E & Exif ();
			
		template< class E >
		const E & Exif () const;
					
		void ResetExif (dng_exif * newExif);
			
		dng_memory_block * BuildExifBlock (dng_memory_allocator &allocator,
										   const dng_resolution *resolution = NULL,
										   bool includeIPTC = false,
										   const dng_jpeg_preview *thumbnail = NULL,
										   uint32 numLeadingZeroBytes = 0) const;
												   
		
		
		dng_exif * GetOriginalExif ()
			{
			return fOriginalExif.Get ();
			}
			
		const dng_exif * GetOriginalExif () const
			{
			return fOriginalExif.Get ();
			}
			
		
		
		#if qDNGUseXMP
		
		bool SetXMP (dng_host &host,
					 const void *buffer,
					 uint32 count,
					 bool xmpInSidecar = false,
					 bool xmpIsNewer = false);
					 
		void SetEmbeddedXMP (dng_host &host,
							 const void *buffer,
							 uint32 count);
					 
		dng_xmp * GetXMP ()
			{
			return fXMP.Get ();
			}
			
		const dng_xmp * GetXMP () const
			{
			return fXMP.Get ();
			}

		template< class X >
		X & XMP ();

		template< class X >
		const X & XMP () const;

		bool XMPinSidecar () const
			{
			return fXMPinSidecar;
			}
			
		const dng_fingerprint & EmbeddedXMPDigest () const
			{
			return fEmbeddedXMPDigest;
			}
						
		bool HaveValidEmbeddedXMP () const
			{
			return fEmbeddedXMPDigest.IsValid ();
			}
		
		void ResetXMP (dng_xmp * newXMP);
	
		void ResetXMPSidecarNewer (dng_xmp * newXMP, bool inSidecar, bool isNewer );
			
		#endif	
		
		
		
		void SynchronizeMetadata ();
		
		
		
		
		void UpdateDateTime (const dng_date_time_info &dt);
							 
		void UpdateDateTimeToNow ();
		
		void UpdateMetadataDateTimeToNow ();
		
		
		
		void SetSourceMIME (const char *s)
			{
			fSourceMIME.Set (s);
			}
			
		const dng_string & SourceMIME () const
			{
			return fSourceMIME;
			}
			
		
		
		void SetBigTableDictionary (const dng_big_table_dictionary &dictionary)
			{
			fBigTableDictionary = dictionary;
			}
			
		const dng_big_table_dictionary & BigTableDictionary () const
			{
			return fBigTableDictionary;
			}
			
		
		
		void SetBigTableIndex (const dng_big_table_index &index)
			{
			fBigTableIndex = index;
			}
			
		const dng_big_table_index & BigTableIndex () const
			{
			return fBigTableIndex;
			}
			
		
		
		void SetBigTableGroupIndex (const dng_big_table_group_index &index)
			{
			fBigTableGroupIndex = index;
			}
			
		const dng_big_table_group_index & BigTableGroupIndex () const
			{
			return fBigTableGroupIndex;
			}
			
		

		void SetImageSequenceInfo (const dng_image_sequence_info &info)
			{
			fImageSequenceInfo = info;
			}

		const dng_image_sequence_info & ImageSequenceInfo () const
			{
			return fImageSequenceInfo;
			}
		
		

		void SetImageStats (const dng_image_stats &stats)
			{
			fImageStats = stats;
			}

		const dng_image_stats & ImageStats () const
			{
			return fImageStats;
			}
		
	};

template< class E >
E & dng_metadata::Exif ()
	{
	dng_exif * exif = GetExif ();
	if (!exif) ThrowProgramError ("EXIF object is NULL.");
	return dynamic_cast< E & > (*exif);
	}

template< class E >
const E & dng_metadata::Exif () const
	{
	const dng_exif * exif = GetExif ();
	if (!exif) ThrowProgramError ("EXIF object is NULL.");
	return dynamic_cast< const E & > (*exif);
	}

#if qDNGUseXMP

template< class X >
X & dng_metadata::XMP ()
	{
	dng_xmp * xmp = GetXMP ();
	if (!xmp) ThrowProgramError ("XMP object is NULL.");
	return dynamic_cast< X & > (*xmp);
	}

template< class X >
const X & dng_metadata::XMP () const
	{
	const dng_xmp * xmp = GetXMP ();
	if (!xmp) ThrowProgramError ("XMP object is NULL.");
	return dynamic_cast< const X & > (*xmp);
	}

#endif	

class dng_negative
	{
	
	public:
	
		enum RawImageStageEnum
			{
			rawImageStagePreOpcode1,
			rawImageStagePostOpcode1,
			rawImageStagePostOpcode2,
			rawImageStagePreOpcode3,
			rawImageStagePostOpcode3,
			rawImageStageNone
			};
			
	protected:
	
		
		
		
		
		
	
		dng_memory_allocator &fAllocator;
			
		
		
		dng_string fModelName;
		
		
		
		dng_string fLocalName;
		
		
		
		
		
		
		
		
		dng_urational fDefaultCropSizeH;
		dng_urational fDefaultCropSizeV;
		
		dng_urational fDefaultCropOriginH;
		dng_urational fDefaultCropOriginV;
		
		
		
		
		dng_urational fRawDefaultCropSizeH;
		dng_urational fRawDefaultCropSizeV;
		
		dng_urational fRawDefaultCropOriginH;
		dng_urational fRawDefaultCropOriginV;

		

		dng_urational fDefaultUserCropT;
		dng_urational fDefaultUserCropL;
		dng_urational fDefaultUserCropB;
		dng_urational fDefaultUserCropR;
		
		
		
		
		
		
		dng_urational fDefaultScaleH;
		dng_urational fDefaultScaleV;
		
		
		
		
		dng_urational fRawDefaultScaleH;
		dng_urational fRawDefaultScaleV;
		
		
		
		
		
		
		
		dng_urational fBestQualityScale;
		
		
		
		
		dng_urational fRawBestQualityScale;

		
		
				
		dng_point fOriginalDefaultFinalSize;
		dng_point fOriginalBestQualityFinalSize;
		
		dng_urational fOriginalDefaultCropSizeH;
		dng_urational fOriginalDefaultCropSizeV;
		
		
		
		
		
		
		
		
		real64 fRawToFullScaleH;
		real64 fRawToFullScaleV;
		
		
		
		
		dng_urational fBaselineNoise;
		
		
		
		
		
		dng_urational fNoiseReductionApplied;

		
		
		
		dng_urational fRawNoiseReductionApplied;

		

		dng_noise_profile fNoiseProfile;
		
		
		
		
		dng_noise_profile fRawNoiseProfile;
		
		
		
		
		dng_srational fBaselineExposure;
		
		
		
		
		
	
		dng_urational fBaselineSharpness;
  
		
		
		
		dng_urational fRawBaselineSharpness;
		
		
		
		
		dng_urational fChromaBlurRadius;
		
		
		
		
		dng_urational fAntiAliasStrength;
		
		
		
		
		
		dng_urational fLinearResponseLimit;
		
		
		
		
		dng_urational fShadowScale;
		
		
		
		uint32 fColorimetricReference;

		

		bool fFloatingPoint;
		
		
		
		uint32 fColorChannels;
		
		
		
		
		
		
		
		dng_vector fAnalogBalance;
		
		
		
		
		
		
		
		dng_vector fCameraNeutral;
		
		
		
		
		
		dng_xy_coord fCameraWhiteXY;
		
		
		
		
		
		
		
		
		
		dng_matrix fCameraCalibration1;
		dng_matrix fCameraCalibration2;
		dng_matrix fCameraCalibration3;
		
		
		

		dng_string fCameraCalibrationSignature;
		
		
		
		dng_std_vector<dng_camera_profile *> fCameraProfile;
		
		
		
		dng_string fAsShotProfileName;
		
		
		
		
		
		
		mutable dng_fingerprint fRawImageDigest;
		
		mutable dng_fingerprint fNewRawImageDigest;

		
		
		
		
		mutable dng_fingerprint fRawDataUniqueID;

		mutable dng_std_mutex fRawDataUniqueIDMutex;
		
		
		
		dng_string fOriginalRawFileName;
		
		
		
		bool fHasOriginalRawFileData;
		
		
		
		AutoPtr<dng_memory_block> fOriginalRawFileData;
		
		
		
		mutable dng_fingerprint fOriginalRawFileDigest;
		
		
		
		AutoPtr<dng_memory_block> fDNGPrivateData;
		
		
	
		dng_metadata fMetadata;
		
		
		
		AutoPtr<dng_linearization_info> fLinearizationInfo;
		
		
		
		AutoPtr<dng_mosaic_info> fMosaicInfo;
		
		
		
		dng_opcode_list fOpcodeList1;
		
		
		
		dng_opcode_list fOpcodeList2;
		
		
		
		dng_opcode_list fOpcodeList3;
		
		
		
		AutoPtr<dng_image> fStage1Image;
		
		
		
		
		AutoPtr<dng_image> fStage2Image;
		
		
		
		
		AutoPtr<dng_image> fStage3Image;
		
		
		
		real64 fStage3Gain;

		

		uint16 fStage3BlackLevel;

		
		
		
		bool fIsPreview;
		
		
		
		bool fIsDamaged;
		
		
		
		RawImageStageEnum fRawImageStage;
		
		
		
		AutoPtr<dng_image> fRawImage;
  
		
		
		uint16 fRawImageBlackLevel;
		
		
		
		uint32 fRawFloatBitDepth;
		
		
		
		AutoPtr<dng_lossy_compressed_image> fRawLossyCompressedImage;
		
		
		
		
		
		mutable dng_fingerprint fRawLossyCompressedImageDigest;
		
		
		
		AutoPtr<dng_image> fTransparencyMask;
		
		
		
		bool fTransparencyMaskWasLossyCompressed = false;
		
		
		
		AutoPtr<dng_image> fRawTransparencyMask;
		
		
		
		uint32 fRawTransparencyMaskBitDepth;
		
		
		
		AutoPtr<dng_lossy_compressed_image> fRawLossyCompressedTransparencyMask;
		
		
		
		
		AutoPtr<dng_image> fUnflattenedStage3Image;
		
		
		
		bool fHasDepthMap;
		
		AutoPtr<dng_image> fDepthMap;
		
		
		
		AutoPtr<dng_image> fRawDepthMap;
		
		
		
		AutoPtr<dng_lossy_compressed_image> fRawLossyCompressedDepthMap;
		
		
		
		uint32		  fDepthFormat;
		dng_urational fDepthNear;
		dng_urational fDepthFar;
		uint32		  fDepthUnits;
		uint32		  fDepthMeasureType;
		
		
		
		dng_string fEnhanceParams;

		
		
		AutoPtr<dng_lossy_compressed_image> fEnhancedLossyCompressedImage;
		
		

		std::vector<dng_semantic_mask> fSemanticMasks;

		

		std::shared_ptr<const dng_gain_table_map> fProfileGainTableMap;

	public:
	
		virtual ~dng_negative ();
		
		static dng_negative * Make (dng_host &host);
		
		
		
		dng_memory_allocator & Allocator () const
			{
			return fAllocator;
			}
			
		
		
		void SetModelName (const char *name)
			{
			fModelName.Set_ASCII (name);
			}
		
		

		const dng_string & ModelName () const
			{
			return fModelName;
			}

		
			
		void SetLocalName (const char *name)
			{
			fLocalName.Set (name);
			}
	
		

		const dng_string & LocalName () const
			{
			return fLocalName;
			}
			
		
			
		dng_metadata & Metadata ()
			{
			return fMetadata;
			}
			
		
		
		const dng_big_table_index & BigTableIndex () const
			{
			return fMetadata.BigTableIndex ();
			}
			
		
		
		const dng_big_table_group_index & BigTableGroupIndex () const
			{
			return fMetadata.BigTableGroupIndex ();
			}
			
		
		
		const dng_big_table_dictionary & BigTableDictionary () const
			{
			return fMetadata.BigTableDictionary ();
			}
			
		

		const dng_image_sequence_info & ImageSequenceInfo () const
			{
			return fMetadata.ImageSequenceInfo ();
			}
		
		

		const dng_image_stats & ImageStats () const
			{
			return fMetadata.ImageStats ();
			}
		
		#if qMetadataOnConst
			
		const dng_metadata & Metadata () const
			{
			return fMetadata;
			}
			
		#endif 

		
		

		dng_metadata * CloneInternalMetadata () const;
		
	protected:

		
		
		

		const dng_metadata &InternalMetadata () const
			{
			return fMetadata;
			}
			
	public:
				
		
			
		void SetBaseOrientation (const dng_orientation &orientation)
			{
			Metadata ().SetBaseOrientation (orientation);
			}
		
		

		bool HasBaseOrientation () METACONST
			{
			return Metadata ().HasBaseOrientation ();
			}
		
		
			
		const dng_orientation & BaseOrientation () METACONST
			{
			return Metadata ().BaseOrientation ();
			}
			
		
			
		virtual dng_orientation ComputeOrientation (const dng_metadata &metadata) const;
		
		
		
		dng_orientation Orientation ()
			{
			return ComputeOrientation (Metadata ());
			}
		
		
		
		
		void ApplyOrientation (const dng_orientation &orientation)
			{
			Metadata ().ApplyOrientation (orientation);
			}
		
		
		
		void SetDefaultCropSize (const dng_urational &sizeH,
								 const dng_urational &sizeV)
			{
			fDefaultCropSizeH = sizeH;
			fDefaultCropSizeV = sizeV;
			}
						  
		
		
		void SetDefaultCropSize (uint32 sizeH,
								 uint32 sizeV)
			{
			SetDefaultCropSize (dng_urational (sizeH, 1),
								dng_urational (sizeV, 1));
			}
						  
		
		
		const dng_urational & DefaultCropSizeH () const
			{
			return fDefaultCropSizeH;
			}
		
		
		
		const dng_urational & DefaultCropSizeV () const
			{
			return fDefaultCropSizeV;
			}

		
		
		void SetDefaultCropOrigin (const dng_urational &originH,
								   const dng_urational &originV)
			{
			fDefaultCropOriginH = originH;
			fDefaultCropOriginV = originV;
			}
		
		

		void SetDefaultCropOrigin (uint32 originH,
								   uint32 originV)
			{
			SetDefaultCropOrigin (dng_urational (originH, 1),
								  dng_urational (originV, 1));
			}
			
		

		void SetDefaultCropCentered (const dng_point &rawSize)
			{
			
			uint32 sizeH = Round_uint32 (fDefaultCropSizeH.As_real64 ());
			uint32 sizeV = Round_uint32 (fDefaultCropSizeV.As_real64 ());
			
			SetDefaultCropOrigin ((rawSize.h - sizeH) >> 1,
								  (rawSize.v - sizeV) >> 1);
			
			}
										
		

		const dng_urational & DefaultCropOriginH () const
			{
			return fDefaultCropOriginH;
			}
		
		

		const dng_urational & DefaultCropOriginV () const
			{
			return fDefaultCropOriginV;
			}

		
		
		
		void SetRawDefaultCrop ()
			{
			
			if (!fRawDefaultCropSizeH.IsValid ())
				{
				
				fRawDefaultCropSizeH = fDefaultCropSizeH;
				fRawDefaultCropSizeV = fDefaultCropSizeV;
				
				fRawDefaultCropOriginH = fDefaultCropOriginH;
				fRawDefaultCropOriginV = fDefaultCropOriginV;

				}
				
			}
		
		
		
		const dng_urational & RawDefaultCropSizeH () const
			{
			return fRawDefaultCropSizeH;
			}
		
		
		
		const dng_urational & RawDefaultCropSizeV () const
			{
			return fRawDefaultCropSizeV;
			}

		

		const dng_urational & RawDefaultCropOriginH () const
			{
			return fRawDefaultCropOriginH;
			}
		
		

		const dng_urational & RawDefaultCropOriginV () const
			{
			return fRawDefaultCropOriginV;
			}

		

		bool HasDefaultUserCrop () const
			{
			return (fDefaultUserCropT.As_real64 () != 0.0 ||
					fDefaultUserCropL.As_real64 () != 0.0 ||
					fDefaultUserCropB.As_real64 () != 1.0 ||
					fDefaultUserCropR.As_real64 () != 1.0);
			}
							  
		

		const dng_urational & DefaultUserCropT () const
			{
			return fDefaultUserCropT;
			}
							  
		

		const dng_urational & DefaultUserCropL () const
			{
			return fDefaultUserCropL;
			}
							  
		

		const dng_urational & DefaultUserCropB () const
			{
			return fDefaultUserCropB;
			}
							  
		

		const dng_urational & DefaultUserCropR () const
			{
			return fDefaultUserCropR;
			}

		

		void ResetDefaultUserCrop ()
			{
			fDefaultUserCropT = dng_urational (0, 1);
			fDefaultUserCropL = dng_urational (0, 1);
			fDefaultUserCropB = dng_urational (1, 1);
			fDefaultUserCropR = dng_urational (1, 1);
			}

		
							  
		void SetDefaultUserCrop (const dng_urational &t,
								 const dng_urational &l,
								 const dng_urational &b,
								 const dng_urational &r)
			{
			fDefaultUserCropT = t;
			fDefaultUserCropL = l;
			fDefaultUserCropB = b;
			fDefaultUserCropR = r;
			}

		
							  
		void SetDefaultUserCropT (const dng_urational &value)
			{
			fDefaultUserCropT = value;
			}

		
							  
		void SetDefaultUserCropL (const dng_urational &value)
			{
			fDefaultUserCropL = value;
			}

		
							  
		void SetDefaultUserCropB (const dng_urational &value)
			{
			fDefaultUserCropB = value;
			}

		
							  
		void SetDefaultUserCropR (const dng_urational &value)
			{
			fDefaultUserCropR = value;
			}

		
		
		void SetDefaultScale (const dng_urational &scaleH,
							  const dng_urational &scaleV)
			{
			fDefaultScaleH = scaleH;
			fDefaultScaleV = scaleV;
			}
							  
		

		const dng_urational & DefaultScaleH () const
			{
			return fDefaultScaleH;
			}
		
		

		const dng_urational & DefaultScaleV () const
			{
			return fDefaultScaleV;
			}
		
		
		
		
		void SetRawDefaultScale ()
			{
			if (!fRawDefaultScaleH.IsValid ())
				{
				fRawDefaultScaleH = fDefaultScaleH;
				fRawDefaultScaleV = fDefaultScaleV;
				}
			}
		
		

		const dng_urational & RawDefaultScaleH () const
			{
			return fRawDefaultScaleH;
			}
		
		

		const dng_urational & RawDefaultScaleV () const
			{
			return fRawDefaultScaleV;
			}
		
		
		
		void SetBestQualityScale (const dng_urational &scale)
			{
			fBestQualityScale = scale;
			}
							  
		
		
		const dng_urational & BestQualityScale () const
			{
			return fBestQualityScale;
			}
		
		
		
		bool HasBestQualityScale () const
			{
			return fBestQualityScale.As_real64 () != 1.0;
			}
			
		
		
		
		void SetRawBestQualityScale ()
			{
			if (!fRawBestQualityScale.IsValid ())
				{
				fRawBestQualityScale = fBestQualityScale;
				}
			}
			
		
		
		const dng_urational & RawBestQualityScale () const
			{
			return fRawBestQualityScale;
			}
		
		
		
		real64 RawToFullScaleH () const
			{
			return fRawToFullScaleH;
			}
			
		
		
		real64 RawToFullScaleV () const
			{
			return fRawToFullScaleV;
			}
			
		
		
		void SetRawToFullScale (real64 scaleH,
								real64 scaleV)
			{
			fRawToFullScaleH = scaleH;
			fRawToFullScaleV = scaleV;
			}
			
		
		
		
		
		
		real64 DefaultScale () const
			{
			return DefaultScaleH ().As_real64 ();
			}
		
		
		
		real64 SquareWidth () const
			{
			return DefaultCropSizeH ().As_real64 ();
			}
		
		
		
		real64 SquareHeight () const
			{
			return DefaultCropSizeV ().As_real64 () *
				   DefaultScaleV	().As_real64 () /
				   DefaultScaleH	().As_real64 ();
			}
		
		
		
		real64 BaseAspectRatio () const
			{
			return SquareWidth	() /
				   SquareHeight ();
			}
			
		
		
		real64 PixelAspectRatio () const
			{
			return (DefaultScaleH ().As_real64 () / RawToFullScaleH ()) /
				   (DefaultScaleV ().As_real64 () / RawToFullScaleV ());
			}
		
		
		
		uint32 FinalWidth (real64 scale) const
			{
			return Round_uint32 (SquareWidth () * scale);
			}
		
		
		
		uint32 FinalHeight (real64 scale) const
			{
			return Round_uint32 (SquareHeight () * scale);
			}
		
		
		
		uint32 DefaultFinalWidth () const
			{
			return FinalWidth (DefaultScale ());
			}
		
		
		
		uint32 DefaultFinalHeight () const
			{
			return FinalHeight (DefaultScale ());
			}
		
		
		
		
		
		uint32 BestQualityFinalWidth () const
			{
			return FinalWidth (DefaultScale () * BestQualityScale ().As_real64 ());
			}
		
		
		
		
		
		uint32 BestQualityFinalHeight () const
			{
			return FinalHeight (DefaultScale () * BestQualityScale ().As_real64 ());
			}
			
		
		
		
		
		
		const dng_point & OriginalDefaultFinalSize () const
			{
			return fOriginalDefaultFinalSize;
			}
			
		
		
		void SetOriginalDefaultFinalSize (const dng_point &size)
			{
			fOriginalDefaultFinalSize = size;
			}
		
		
		
		
		
		
		const dng_point & OriginalBestQualityFinalSize () const
			{
			return fOriginalBestQualityFinalSize;
			}
			
		
		
		void SetOriginalBestQualityFinalSize (const dng_point &size)
			{
			fOriginalBestQualityFinalSize = size;
			}
			
		
		
		
		
		const dng_urational & OriginalDefaultCropSizeH () const
			{
			return fOriginalDefaultCropSizeH;
			}
			
		const dng_urational & OriginalDefaultCropSizeV () const
			{
			return fOriginalDefaultCropSizeV;
			}
			
		
		
		void SetOriginalDefaultCropSize (const dng_urational &sizeH,
										 const dng_urational &sizeV)
			{
			fOriginalDefaultCropSizeH = sizeH;
			fOriginalDefaultCropSizeV = sizeV;
			}
			
		
		
		void ClearOriginalSizes ();
			
		
		
		
		void SetDefaultOriginalSizes ();

		
		
		void SetOriginalSizes (const dng_point &size);

		
							
		dng_rect DefaultCropArea () const;
								  
		
		
		void SetBaselineNoise (real64 noise)
			{
			fBaselineNoise.Set_real64 (noise, 100);
			}
					  
		

		const dng_urational & BaselineNoiseR () const
			{
			return fBaselineNoise;
			}
		
		

		real64 BaselineNoise () const
			{
			return fBaselineNoise.As_real64 ();
			}
			
		
		
		void SetNoiseReductionApplied (const dng_urational &value)
			{
			fNoiseReductionApplied = value;
			}
			
		
		
		const dng_urational & NoiseReductionApplied () const
			{
			return fNoiseReductionApplied;
			}

		
		
		
		void SetRawNoiseReductionApplied ()
			{
			if (fRawNoiseReductionApplied.NotValid ())
				{
				fRawNoiseReductionApplied = fNoiseReductionApplied;
				}
			}
		
		
		
		const dng_urational & RawNoiseReductionApplied () const
			{
			return fRawNoiseReductionApplied;
			}
		
		

		void SetNoiseProfile (const dng_noise_profile &noiseProfile)
			{
			fNoiseProfile = noiseProfile;
			}

		

		bool HasNoiseProfile () const
			{
			return fNoiseProfile.IsValidForNegative (*this);
			}

		

		const dng_noise_profile & NoiseProfile () const
			{
			return fNoiseProfile;
			}
			
		

		bool HasRawNoiseProfile () const
			{
			return fRawNoiseProfile.IsValidForNegative (*this);
			}

		
		
		
		void SetRawNoiseProfile ()
			{
			if (!HasRawNoiseProfile ())
				{
				fRawNoiseProfile = fNoiseProfile;
				}
			}
		
		

		const dng_noise_profile & RawNoiseProfile () const
			{
			return fRawNoiseProfile;
			}
		
		
		
		void SetBaselineExposure (real64 exposure)
			{
			fBaselineExposure.Set_real64 (exposure, 100);
			}
					  
		

		const dng_srational & BaselineExposureR () const
			{
			return fBaselineExposure;
			}
		
		

		real64 BaselineExposure () const
			{
			return BaselineExposureR ().As_real64 ();
			}

		
		

		real64 TotalBaselineExposure (const dng_camera_profile_id &profileID) const;

		
		
		void SetBaselineSharpness (real64 sharpness)
			{
			fBaselineSharpness.Set_real64 (sharpness, 100);
			}
		
		

		const dng_urational & BaselineSharpnessR () const
			{
			return fBaselineSharpness;
			}
		
		

		real64 BaselineSharpness () const
			{
			return BaselineSharpnessR ().As_real64 ();
			}
   
		
		
		
		void SetRawBaselineSharpness ()
			{
			if (fRawBaselineSharpness.d == 0)
				{
				fRawBaselineSharpness = fBaselineSharpness;
				}
			}
		
		
		
		const dng_urational & RawBaselineSharpness () const
			{
			if (fRawBaselineSharpness.d != 0)
				{
				return fRawBaselineSharpness;
				}
			return fBaselineSharpness;
			}
		
		
		
		void SetChromaBlurRadius (const dng_urational &radius)
			{
			fChromaBlurRadius = radius;
			}
		
		

		const dng_urational & ChromaBlurRadius () const
			{
			return fChromaBlurRadius;
			}
		
		
		
		void SetAntiAliasStrength (const dng_urational &strength)
			{
			fAntiAliasStrength = strength;
			}
					  
		

		const dng_urational & AntiAliasStrength () const
			{
			return fAntiAliasStrength;
			}
		
		
		
		void SetLinearResponseLimit (real64 limit)
			{
			fLinearResponseLimit.Set_real64 (limit, 100);
			}
		
		

		const dng_urational & LinearResponseLimitR () const
			{
			return fLinearResponseLimit;
			}
		
		

		real64 LinearResponseLimit () const
			{
			return LinearResponseLimitR ().As_real64 ();
			}
		
		
		
		void SetShadowScale (const dng_urational &scale);
		
		

		const dng_urational & ShadowScaleR () const
			{
			return fShadowScale;
			}
		
		

		real64 ShadowScale () const
			{
			return ShadowScaleR ().As_real64 ();
			}
			
		
		
		void SetColorimetricReference (uint32 ref)
			{
			fColorimetricReference = ref;
			}
			
		uint32 ColorimetricReference () const
			{
			return fColorimetricReference;
			}

		bool IsSceneReferred () const
			{
			return fColorimetricReference == crSceneReferred;
			}

		bool IsOutputReferred () const
			{
			return !IsSceneReferred ();
			}

		

		void SetFloatingPoint (bool isFloatingPoint)
			{
			fFloatingPoint = isFloatingPoint;
			}

		bool IsFloatingPoint () const
			{
			return fFloatingPoint;
			}

		
		

		bool IsHighDynamicRange () const
			{
			return IsFloatingPoint ();
			}

		bool IsNormalDynamicRange () const
			{
			return !IsHighDynamicRange ();
			}
		
		
			
		void SetColorChannels (uint32 channels)
			{
			fColorChannels = channels;
			}
		
		
			
		uint32 ColorChannels () const
			{
			return fColorChannels;
			}
		
		
			
		void SetMonochrome ()
			{
			SetColorChannels (1);
			}

		
			
		bool IsMonochrome () const
			{
			return ColorChannels () == 1;
			}
		
		

		void SetAnalogBalance (const dng_vector &b);
		
		

		dng_urational AnalogBalanceR (uint32 channel) const;
		
		

		real64 AnalogBalance (uint32 channel) const;
		
		

		void SetCameraNeutral (const dng_vector &n);
					  
		

		void ClearCameraNeutral ()
			{
			fCameraNeutral.Clear ();
			}
		
		

		bool HasCameraNeutral () const
			{
			return fCameraNeutral.NotEmpty ();
			}
			
		

		const dng_vector & CameraNeutral () const
			{
			return fCameraNeutral;
			}
		
		dng_urational CameraNeutralR (uint32 channel) const;
		
		

		void SetCameraWhiteXY (const dng_xy_coord &coord);
		
		bool HasCameraWhiteXY () const
			{
			return fCameraWhiteXY.IsValid ();
			}
		
		const dng_xy_coord & CameraWhiteXY () const;
		
		void GetCameraWhiteXY (dng_urational &x,
							   dng_urational &y) const;
							   
		
		
		
		
		
		
		
		
		
		

		void SetCameraCalibration1 (const dng_matrix &m);

		
		
		
		
		
		
		
		

		void SetCameraCalibration2 (const dng_matrix &m);
		
		
		
		
		
		
		
		
		

		void SetCameraCalibration3 (const dng_matrix &m);
		
		

		const dng_matrix & CameraCalibration1 () const
			{
			return fCameraCalibration1;
			}
	
		

		const dng_matrix & CameraCalibration2 () const
			{
			return fCameraCalibration2;
			}
		
		

		const dng_matrix & CameraCalibration3 () const
			{
			return fCameraCalibration3;
			}
		
		void SetCameraCalibrationSignature (const char *signature)
			{
			fCameraCalibrationSignature.Set (signature);
			}

		const dng_string & CameraCalibrationSignature () const
			{
			return fCameraCalibrationSignature;
			}
			
		
		
		virtual void AddProfile (AutoPtr<dng_camera_profile> &profile);
		
		virtual void ClearProfiles ();
			
		uint32 ProfileCount () const;
		
		const dng_camera_profile & ProfileByIndex (uint32 index) const;
		
		
		
  
		virtual void GetProfileMetadataList (dng_profile_metadata_list &list) const;
		
		
  
		bool GetProfileByID (const dng_camera_profile_id &id,
							 dng_camera_profile &foundProfile,
							 bool useDefaultIfNoMatch = true,
							 const dng_camera_profile_group_selector *groupSelector = nullptr) const;
		
		
		
		bool GetProfileToEmbed (const dng_metadata &metadata,
								dng_camera_profile &foundProfile,
								bool skipAdobeStandard = false) const;
		
		
			
		void SetAsShotProfileName (const char *name)
			{
			fAsShotProfileName.Set (name);
			}

		const dng_string & AsShotProfileName () const
			{
			return fAsShotProfileName;
			}
			
		
		
		virtual dng_color_spec * MakeColorSpec (const dng_camera_profile_id &id,
												bool allowStubbed = false) const;
		
		
		
		
			
		static dng_fingerprint FindImageDigest (dng_host &host,
												const dng_image &image);
												
		
		
			
		static dng_fingerprint FindFastImageDigest (dng_host &host,
													const dng_image &image,
													uint32 pixelType);
													
		
		
		void SetRawImageDigest (const dng_fingerprint &digest)
			{
			fRawImageDigest = digest;
			}
			
		void SetNewRawImageDigest (const dng_fingerprint &digest)
			{
			fNewRawImageDigest = digest;
			}
			
		void ClearRawImageDigest () const
			{
			fRawImageDigest	  .Clear ();
			fNewRawImageDigest.Clear ();
			}
			
		const dng_fingerprint & RawImageDigest () const
			{
			return fRawImageDigest;
			}
			
		const dng_fingerprint & NewRawImageDigest () const
			{
			return fNewRawImageDigest;
			}
			
		void FindRawImageDigest (dng_host &host) const;
		
		void FindNewRawImageDigest (dng_host &host) const;
		
		void ValidateRawImageDigest (dng_host &host);
		
		

		void SetRawDataUniqueID (const dng_fingerprint &id)
			{
			fRawDataUniqueID = id;
			}
			
		const dng_fingerprint & BaseRawDataUniqueID () const
			{
			return fRawDataUniqueID;
			}
		
		dng_fingerprint RawDataUniqueID () const;
		
		void FindRawDataUniqueID (dng_host &host) const;
		
		virtual void RecomputeRawDataUniqueID (dng_host &host);

		
		
		void SetOriginalRawFileName (const char *name)
			{
			fOriginalRawFileName.Set (name);
			}
			
		bool HasOriginalRawFileName () const
			{
			return fOriginalRawFileName.NotEmpty ();
			}
		
		const dng_string & OriginalRawFileName () const
			{
			return fOriginalRawFileName;
			}
		
		
		
		void SetHasOriginalRawFileData (bool hasData)
			{
			fHasOriginalRawFileData = hasData;
			}
		
		bool CanEmbedOriginalRaw () const
			{
			return fHasOriginalRawFileData && HasOriginalRawFileName ();
			}
		
		void SetOriginalRawFileData (AutoPtr<dng_memory_block> &data)
			{
			fOriginalRawFileData.Reset (data.Release ());
			}
		
		const void * OriginalRawFileData () const
			{
			return fOriginalRawFileData.Get () ? fOriginalRawFileData->Buffer ()
											   : NULL;
			}
		
		uint32 OriginalRawFileDataLength () const
			{
			return fOriginalRawFileData.Get () ? fOriginalRawFileData->LogicalSize ()
											   : 0;
			}
			
		
		
		void SetOriginalRawFileDigest (const dng_fingerprint &digest)
			{
			fOriginalRawFileDigest = digest;
			}
			
		const dng_fingerprint & OriginalRawFileDigest () const
			{
			return fOriginalRawFileDigest;
			}
			
		void FindOriginalRawFileDigest () const;
		
		void ValidateOriginalRawFileDigest ();
		
		
		
		void SetPrivateData (AutoPtr<dng_memory_block> &block)
			{
			fDNGPrivateData.Reset (block.Release ());
			}
		
		void ClearPrivateData ()
			{
			fDNGPrivateData.Reset ();
			}
		
		const uint8 * PrivateData () const
			{
			return fDNGPrivateData.Get () ? fDNGPrivateData->Buffer_uint8 ()
										  : NULL;
			}
		
		uint32 PrivateLength () const
			{
			return fDNGPrivateData.Get () ? fDNGPrivateData->LogicalSize ()
										  : 0;
			}
		
		
		
		void SetMakerNoteSafety (bool safe)
			{
			Metadata ().SetMakerNoteSafety (safe);
			}
		
		bool IsMakerNoteSafe () METACONST
			{
			return Metadata ().IsMakerNoteSafe ();
			}
		
		void SetMakerNote (AutoPtr<dng_memory_block> &block)
			{
			Metadata ().SetMakerNote (block);
			}
		
		void ClearMakerNote ()
			{
			Metadata ().ClearMakerNote ();
			}
		
		const void * MakerNoteData () METACONST
			{
			return Metadata ().MakerNoteData ();
			}
		
		uint32 MakerNoteLength () METACONST
			{
			return Metadata ().MakerNoteLength ();
			}
		
		
		
		dng_exif * GetExif ()
			{
			return Metadata ().GetExif ();
			}
			
		#if qMetadataOnConst
			
		const dng_exif * GetExif () const
			{
			return Metadata ().GetExif ();
			}
			
		#endif 
			
		void ResetExif (dng_exif * newExif)
			{
			Metadata ().ResetExif (newExif);
			}
			
		
		
		dng_exif * GetOriginalExif ()
			{
			return Metadata ().GetOriginalExif ();
			}
			
		#if qMetadataOnConst
			
		const dng_exif * GetOriginalExif () const
			{
			return Metadata ().GetOriginalExif ();
			}
			
		#endif 
			
		
			
		void SetIPTC (AutoPtr<dng_memory_block> &block,
					  uint64 offset)
			{
			Metadata ().SetIPTC (block, offset);
			}
		
		void SetIPTC (AutoPtr<dng_memory_block> &block)
			{
			Metadata ().SetIPTC (block);
			}
		
		void ClearIPTC ()
			{
			Metadata ().ClearIPTC ();
			}
		
		const void * IPTCData () METACONST
			{
			return Metadata ().IPTCData ();
			}
		
		uint32 IPTCLength () METACONST
			{
			return Metadata ().IPTCLength ();
			}
		
		uint64 IPTCOffset () METACONST
			{
			return Metadata ().IPTCOffset ();
			}
		
		dng_fingerprint IPTCDigest (bool includePadding = true) METACONST
			{
			return Metadata ().IPTCDigest (includePadding);
			}
		
		void RebuildIPTC (bool padForTIFF)
			{
			Metadata ().RebuildIPTC (Allocator (), padForTIFF);
			}
		
		
		
		#if qDNGUseXMP
		
		bool SetXMP (dng_host &host,
					 const void *buffer,
					 uint32 count,
					 bool xmpInSidecar = false,
					 bool xmpIsNewer = false)
			{
			return Metadata ().SetXMP (host,
									   buffer,
									   count,
									   xmpInSidecar,
									   xmpIsNewer);
			}
					 
		dng_xmp * GetXMP ()
			{
			return Metadata ().GetXMP ();
			}
			
		#if qMetadataOnConst
			
		const dng_xmp * GetXMP () const
			{
			return Metadata ().GetXMP ();
			}
			
		#endif 
			
		bool XMPinSidecar () METACONST
			{
			return Metadata ().XMPinSidecar ();
			}
			
		void ResetXMP (dng_xmp * newXMP)
			{
			Metadata ().ResetXMP (newXMP);
			}
	
		void ResetXMPSidecarNewer (dng_xmp * newXMP, bool inSidecar, bool isNewer )
			{
			Metadata ().ResetXMPSidecarNewer (newXMP, inSidecar, isNewer);
			}
		
		bool HaveValidEmbeddedXMP () METACONST
			{
			return Metadata ().HaveValidEmbeddedXMP ();
			}
			
		#endif	
		
		
		
		void SetSourceMIME (const char *s)
			{
			Metadata ().SetSourceMIME (s);
			}
		
		const dng_string & SourceMIME () const
			{
			return fMetadata.SourceMIME ();
			}

		
			
		const dng_linearization_info * GetLinearizationInfo () const
			{
			return fLinearizationInfo.Get ();
			}
			
		void ClearLinearizationInfo ()
			{
			fLinearizationInfo.Reset ();
			}
			
		
		
		
		
		void SetLinearization (AutoPtr<dng_memory_block> &curve);
		
		
		
		
		void SetActiveArea (const dng_rect &area);
		
		
		
		
		void SetMaskedAreas (uint32 count,
							 const dng_rect *area);
							 
		void SetMaskedArea (const dng_rect &area)
			{
			SetMaskedAreas (1, &area);
			}

		
		
		void SetBlackLevel (real64 black,
							int32 plane = -1);
							
		void SetQuadBlacks (real64 black0,
							real64 black1,
							real64 black2,
							real64 black3,
							int32 plane = -1);
	
		void Set6x6Blacks (real64 blacks6x6 [36],
						   int32 plane = -1);
							
		void SetRowBlacks (const real64 *blacks,
						   uint32 count);
						   
		void SetColumnBlacks (const real64 *blacks,
							  uint32 count);
							  
		
		
		uint32 WhiteLevel (uint32 plane = 0) const;
			
		void SetWhiteLevel (uint32 white,
							int32 plane = -1);

		
		
		const dng_mosaic_info * GetMosaicInfo () const
			{
			return fMosaicInfo.Get ();
			}
			
		void ClearMosaicInfo ()
			{
			fMosaicInfo.Reset ();
			}
		
		
							
		void SetColorKeys (ColorKeyCode color0,
						   ColorKeyCode color1,
						   ColorKeyCode color2,
						   ColorKeyCode color3 = colorKeyMaxEnum);

		void SetRGB ()
			{
			
			SetColorChannels (3);
			
			SetColorKeys (colorKeyRed,
						  colorKeyGreen,
						  colorKeyBlue);
						  
			}
			
		void SetCMY ()
			{
			
			SetColorChannels (3);
			
			SetColorKeys (colorKeyCyan,
						  colorKeyMagenta,
						  colorKeyYellow);
						  
			}
			
		void SetGMCY ()
			{
			
			SetColorChannels (4);
			
			SetColorKeys (colorKeyGreen,
						  colorKeyMagenta,
						  colorKeyCyan,
						  colorKeyYellow);
						  
			}
			
		
			
		void SetBayerMosaic (uint32 phase);

		void SetFujiMosaic (uint32 phase);

		void SetFujiMosaic6x6 (uint32 phase);

		void SetQuadMosaic (uint32 pattern);
			
		
							
		void SetGreenSplit (uint32 split);
		
		
		
		const dng_opcode_list & OpcodeList1 () const
			{
			return fOpcodeList1;
			}
			
		dng_opcode_list & OpcodeList1 ()
			{
			return fOpcodeList1;
			}
			
		const dng_opcode_list & OpcodeList2 () const
			{
			return fOpcodeList2;
			}
			
		dng_opcode_list & OpcodeList2 ()
			{
			return fOpcodeList2;
			}
			
		const dng_opcode_list & OpcodeList3 () const
			{
			return fOpcodeList3;
			}
			
		dng_opcode_list & OpcodeList3 ()
			{
			return fOpcodeList3;
			}
		
		
		
		virtual void Parse (dng_host &host,
							dng_stream &stream,
							dng_info &info);
							
		
		
		
							
		virtual void PostParse (dng_host &host,
								dng_stream &stream,
								dng_info &info);
								
		
		
		void SynchronizeMetadata ()
			{
			Metadata ().SynchronizeMetadata ();
			}
		
		
		
		
		void UpdateDateTime (const dng_date_time_info &dt)
			{
			Metadata ().UpdateDateTime (dt);
			}
							 
		void UpdateDateTimeToNow ()
			{
			Metadata ().UpdateDateTimeToNow ();
			}
		
		
		
		
			
		virtual bool SetFourColorBayer ();
		
		
							
		const dng_image * Stage1Image () const
			{
			return fStage1Image.Get ();
			}
			
		const dng_image * Stage2Image () const
			{
			return fStage2Image.Get ();
			}
			
		const dng_image * Stage3Image () const
			{
			return fStage3Image.Get ();
			}
			
		
		
		RawImageStageEnum RawImageStage () const
			{
			return fRawImageStage;
			}
			
		
		
		const dng_image & RawImage () const;
		
		
		
		void ClearRawImage ()
			{
			fRawImage.Reset ();
			}
  
		
		
		uint16 RawImageBlackLevel () const;
		
		
		
		uint32 RawFloatBitDepth () const
			{
			return fRawFloatBitDepth;
			}
			
		void SetRawFloatBitDepth (uint32 bitDepth)
			{
			fRawFloatBitDepth = bitDepth;
			}
		
		
		
		const dng_lossy_compressed_image * RawLossyCompressedImage () const;

		void SetRawLossyCompressedImage (AutoPtr<dng_lossy_compressed_image> &image);
			
		void ClearRawLossyCompressedImage ();
			
		
		
		void SetRawLossyCompressedImageDigest (const dng_fingerprint &digest)
			{
			fRawLossyCompressedImageDigest = digest;
			}
			
		void ClearRawLossyCompressedImageDigest () const
			{
			fRawLossyCompressedImageDigest.Clear ();
			}
			
		const dng_fingerprint & RawLossyCompressedImageDigest () const
			{
			return fRawLossyCompressedImageDigest;
			}
			
		void FindRawLossyCompressedImageDigest (dng_host &host) const;
		
		
		
		virtual void ReadOpcodeLists (dng_host &host,
									  dng_stream &stream,
									  dng_info &info);
		
		
		
		virtual void ReadStage1Image (dng_host &host,
									  dng_stream &stream,
									  dng_info &info);
		
		
		
		virtual void ReadEnhancedImage (dng_host &host,
										dng_stream &stream,
										dng_info &info);
		
		
		
		void SetStage1Image (AutoPtr<dng_image> &image);
		
		
		
		void ClearStage1Image ();
		
		
		
		void SetStage2Image (AutoPtr<dng_image> &image);
									  
		
		
		void SetStage3Image (AutoPtr<dng_image> &image);
		
		
		
		void BuildStage2Image (dng_host &host);
									   
		
									   
		void BuildStage3Image (dng_host &host,
							   int32 srcPlane = -1);
									   
		
		
		void SetStage3Gain (real64 gain)
			{
			fStage3Gain = gain;
			}
		
		real64 Stage3Gain () const
			{
			return fStage3Gain;
			}

		
  
		void SetStage3BlackLevel (uint16 level)
			{
			fStage3BlackLevel = level;
			}

		uint16 Stage3BlackLevel () const
			{
			return fStage3BlackLevel;
			}

		
			
		real64 Stage3BlackLevelNormalized () const
			{
			return fStage3BlackLevel * (1.0 / 65535.0);
			}

		
		
		
		
		
		
		
		

		virtual bool SupportsPreservedBlackLevels (dng_host &host);
			
		

		bool NeedLossyCompressMosaicJXL (dng_host &host) const;
		
		
		
		void LossyCompressMosaicJXL (dng_host &host,
									 dng_image_writer &writer);

		
		
		virtual void LosslessCompressJXL (dng_host &host,
										  dng_image_writer &writer,
										  bool nearLosslessOK = false);
			
		
		

		dng_image * EncodeRawProxy (dng_host &host,
									const dng_image &srcImage,
									dng_opcode_list &opcodeList,
									real64 *blackLevel) const;

		

		void ConvertToProxy (dng_host &host,
							 dng_image_writer &writer,
							 uint32 proxySize = 0,
							 uint64 proxyCount = 0);
		
		
		
		bool IsProxy () const;
	
		
			
		void SetIsPreview (bool preview)
			{
			fIsPreview = preview;
			}
		
		bool IsPreview () const
			{
			return fIsPreview;
			}
			
		
		
		void SetIsDamaged (bool damaged)
			{
			fIsDamaged = damaged;
			}
			
		bool IsDamaged () const
			{
			return fIsDamaged;
			}
			
		
		
		void SetTransparencyMask (AutoPtr<dng_image> &image,
								  uint32 bitDepth = 0);
		
		void ClearTransparencyMask ();
		
		const dng_image * TransparencyMask () const;
		
		const dng_image * RawTransparencyMask () const;
		
		uint32 RawTransparencyMaskBitDepth () const;
		
		const dng_lossy_compressed_image * RawLossyCompressedTransparencyMask () const
			{
			return fRawLossyCompressedTransparencyMask.Get ();
			}
		
		void ReadTransparencyMask (dng_host &host,
								   dng_stream &stream,
								   dng_info &info);
								   
		virtual void ResizeTransparencyToMatchStage3 (dng_host &host,
													  bool convertTo8Bit = false);
		
		virtual bool NeedFlattenTransparency (dng_host &host);
		
		virtual void FlattenTransparency (dng_host &host);
		
		const dng_image * UnflattenedStage3Image () const;
		
		
		
		bool HasDepthMap () const
			{
			return fHasDepthMap;
			}
		
		void SetHasDepthMap (bool hasDepthMap)
			{
			fHasDepthMap = hasDepthMap;
			}
		
		const dng_image * DepthMap () const
			{
			return fDepthMap.Get ();
			}
		
		void SetDepthMap (AutoPtr<dng_image> &depthMap);
		
		bool HasDepthMapImage () const
			{
			return (fDepthMap.Get () != NULL);
			}
		
		const dng_image * RawDepthMap () const
			{
			if (fRawDepthMap.Get ())
				{
				return fRawDepthMap.Get ();
				}
			return DepthMap ();
			}
		
		const dng_lossy_compressed_image * RawLossyCompressedDepthMap () const
			{
			return fRawLossyCompressedDepthMap.Get ();
			}
		
		void ResetDepthMap ()
			{
			fDepthMap.Reset ();
			fRawDepthMap.Reset ();
			fRawLossyCompressedDepthMap.Reset ();
			}

		void ReadDepthMap (dng_host &host,
						   dng_stream &stream,
						   dng_info &info);
		
		virtual void ResizeDepthToMatchStage3 (dng_host &host);
		
		uint32 DepthFormat () const
			{
			return fDepthFormat;
			}
		
		void SetDepthFormat (uint32 format)
			{
			fDepthFormat = format;
			}
		
		const dng_urational & DepthNear () const
			{
			return fDepthNear;
			}
		
		void SetDepthNear (const dng_urational &dist)
			{
			fDepthNear = dist;
			}
		
		const dng_urational & DepthFar () const
			{
			return fDepthFar;
			}
		
		void SetDepthFar (const dng_urational &dist)
			{
			fDepthFar = dist;
			}
		
		uint32 DepthUnits () const
			{
			return fDepthUnits;
			}
		
		void SetDepthUnits (uint32 units)
			{
			fDepthUnits = units;
			}

		uint32 DepthMeasureType () const
			{
			return fDepthMeasureType;
			}
		
		void SetDepthMeasureType (uint32 measure)
			{
			fDepthMeasureType = measure;
			}

		 
		
		const dng_string & EnhanceParams () const
			{
			return fEnhanceParams;
			}
		
		void SetEnhanceParams (const dng_string &s)
			{
			fEnhanceParams = s;
			}
		
		void SetEnhanceParams (const char *s)
			{
			fEnhanceParams.Set (s);
			}
		
		
		
		const dng_lossy_compressed_image * EnhancedLossyCompressedImage () const
			{
			return fEnhancedLossyCompressedImage.Get ();
			}
			
		void SetEnhancedLossyCompressedImage (dng_lossy_compressed_image *image)
			{
			fEnhancedLossyCompressedImage.Reset (image);
			}

		

		bool HasSemanticMask () const;
		
		bool HasSemanticMask (uint32 index) const;

		uint32 NumSemanticMasks () const;

		const dng_semantic_mask & SemanticMask (uint32 index) const;

		const dng_semantic_mask & RawSemanticMask (uint32 index) const;
		
		void SetSemanticMask (uint32 index,
							  const dng_semantic_mask &mask);

		void AppendSemanticMask (const dng_semantic_mask &mask);
		
		void ReadSemanticMasks (dng_host &host,
								dng_stream &stream,
								dng_info &info);

		virtual void ResizeSemanticMasksToMatchStage3 (dng_host &host);

		

		bool HasProfileGainTableMap () const;

		const dng_gain_table_map & ProfileGainTableMap () const;

		std::shared_ptr<const dng_gain_table_map> ShareProfileGainTableMap () const
			{
			return fProfileGainTableMap;
			}

		
		
		void SetProfileGainTableMap
			(const std::shared_ptr<const dng_gain_table_map> &gainTableMap);

		
		
		
		void SetProfileGainTableMap
			(AutoPtr<dng_gain_table_map> &gainTableMap);

	protected:
	
		dng_negative (dng_host &host);
		
		virtual void Initialize ();
		
		virtual dng_linearization_info * MakeLinearizationInfo ();
		
		void NeedLinearizationInfo ();
		
		virtual dng_mosaic_info * MakeMosaicInfo ();
		
		void NeedMosaicInfo ();
		
		virtual void DoBuildStage2 (dng_host &host);
		
		virtual void DoPostOpcodeList2 (dng_host &host);
									   
		virtual bool NeedDefloatStage2 (dng_host &host);
		
		virtual void DefloatStage2 (dng_host &host);
		
		virtual void DoInterpolateStage3 (dng_host &host,
										  int32 srcPlane,
										  dng_matrix *scaleTransforms);
									
		virtual void DoMergeStage3 (dng_host &host,
									dng_matrix *scaleTransforms);
									   
		virtual void DoBuildStage3 (dng_host &host,
									int32 srcPlane,
									dng_matrix *scaleTransforms);
									   
		virtual void AdjustGainMapForStage3 (dng_host &host);
									  
		virtual void AdjustProfileForStage3 ();
									  
		virtual bool GetProfileByMetadata (const dng_camera_profile_metadata &metadata,
										   dng_camera_profile &foundProfile) const;
		
		virtual bool GetProfileByIDFromList (const dng_profile_metadata_list &list,
											 const dng_camera_profile_id &id,
											 dng_camera_profile &foundProfile,
											 bool useDefaultIfNoMatch,
											 const dng_camera_profile_group_selector *groupSelector) const;
		
		virtual bool GetProfileToEmbedFromList (const dng_profile_metadata_list &list,
												const dng_metadata &metadata,
												dng_camera_profile &foundProfile,
												bool skipAdobeStandard = false) const;

		void CompressTransparencyMaskJXL (dng_host &host,
										  dng_image_writer &writer,
										  bool nearLosslessOK);
								  
		void CompressDepthMapJXL (dng_host &host,
								  dng_image_writer &writer,
								  bool nearLosslessOK);
								  
		void CompressSemanticMasksJXL (dng_host &host,
									   dng_image_writer &writer,
									   bool nearLosslessOK);

		void AdjustSemanticMasksForProxy (dng_host &host,
										  dng_image_writer &writer,
										  const dng_rect &originalStage3Bounds,
										  const dng_rect &defaultCropArea);

	};

dng_image * EncodeImageForCompression (dng_host &host,
									   const dng_image &srcImage,
									   const dng_rect &activeArea,
									   const bool isSceneReferred,
									   const bool use16bit,
									   const real64 srcBlackLevel,
									   real64 *dstBlackLevel,
									   dng_opcode_list &opcodeList);

#endif	
	

