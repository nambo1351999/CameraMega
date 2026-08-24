

#ifndef __dng_jxl__
#define __dng_jxl__

#include "dng_flags.h"

#include "dng_auto_ptr.h"
#include "dng_bmff.h"
#include "dng_classes.h"
#include "dng_orientation.h"
#include "dng_point.h"
#include "dng_tag_values.h"
#include "dng_types.h"
#include "dng_utils.h"

#include <vector>
#include <unordered_set>

#if PHOTON_ENABLE_DNG_JXL
#include "jxl/color_encoding.h"
#else
#include "photon_jxl_stub.h"
#endif

#define qLogJXL (qDNGValidate && 0)

static const uint32 kNumLeadingZeroBytesForEXIF = 4;

class dng_jxl_encode_settings
	{
	
	private:

		real32 fDistance = 1.0f;

		
		
		

		
		

		uint32 fEffort = 7;

		

		uint32 fDecodeSpeed = 4;

		

		bool fUseOriginalColorEncoding = false;

		bool fUseSingleThread = false;
	
	public:

		
	
		void SetDistance (real32 distance)
			{
			fDistance = distance;
			}

		real32 Distance () const
			{
			return fDistance;
			}

		void SetEffort (uint32 effort)
			{
			fEffort = Pin_uint32 (1, effort, 9);
			}

		uint32 Effort () const
			{
			return fEffort;
			}

		void SetDecodeSpeed (uint32 speed04)
			{
			fDecodeSpeed = Pin_uint32 (0, speed04, 4);
			}

		uint32 DecodeSpeed () const
			{
			return fDecodeSpeed;
			}

		
		

		void SetSmallest ()
			{
			fDecodeSpeed = 0;
			fEffort		 = 5;
			}

		
		

		void SetFastest ()
			{
			fDecodeSpeed = 4;
			fEffort		 = 3;
			}

		bool UseSingleThread () const
			{
			return fUseSingleThread;
			}

		void SetSingleThread (bool flag)
			{
			fUseSingleThread = flag;
			}

		bool UseOriginalColorEncoding () const
			{
			return fUseOriginalColorEncoding;
			}

		void SetUseOriginalColorEncoding (bool flag)
			{
			fUseOriginalColorEncoding = flag;
			}

		

		
		
	};

bool ParseJXL (dng_host &host,
			   dng_stream &stream,
			   dng_info &info,
			   bool supportBasicCodeStream,
			   bool supportContainer);

class dng_jxl_color_space_info
	{
		
	public:

		AutoPtr<JxlColorEncoding> fJxlColorEncoding;

		AutoPtr<dng_memory_block> fICCProfile;

		real32 fIntensityTargetNits = 0.0f;
		
	};

class dng_jxl_decoder
	{

	friend class dng_jxl_box_reader;
		
	public:

		

		bool fNeedBoxMeta = true;

		bool fNeedImage = true;

		bool fUseSingleThread = false;

		bool fUsePixelBuffer = false;

		

		bool fUsesOriginalProfile = false;

		dng_point fMainImageSize;

		uint32 fMainImagePlanes = 3;

		uint32 fBitsPerSample = 8;

		uint32 fExponentBitsPerSample = 0;

		uint32 fNumExtraChannels = 0;

		bool fHasPreview = false;

		dng_info *fInfo = nullptr;

		dng_orientation fOrientation;

		dng_jxl_color_space_info fColorSpaceInfo;

		

		AutoPtr<dng_image> fMainImage;

		AutoPtr<dng_memory_block> fMainBlock;

		AutoPtr<dng_pixel_buffer> fMainPixelBuffer;

		

		AutoPtr<dng_image> fAlphaMask;

		bool fAlphaPremultiplied = false;

		

		
		
		

		
		

		uint64 fCurrentBoxStreamOffset = 0;

		uint64 fCurrentBoxRawSize = 0;

		
		
		
		

		uint64 fC2PAManifestOffset = 0;

		uint32 fC2PAManifestRawSize = 0;

	public:

		virtual ~dng_jxl_decoder ();

		virtual void Decode (dng_host &host,
							 dng_stream &stream);

	protected:

		virtual void ProcessExifBox (dng_host &host,
									 const std::vector<uint8> &data);

		virtual void ProcessXMPBox (dng_host &host,
									const std::vector<uint8> &data);

		virtual void ProcessBox (dng_host &host,
								 const dng_string &name,
								 const std::vector<uint8> &data);

	};

void EncodeJXL_Tile (dng_host &host,
					 dng_stream &stream,
					 const dng_pixel_buffer &buffer,
					 const dng_jxl_color_space_info &colorSpaceInfo,
					 const dng_jxl_encode_settings &settings);

void EncodeJXL_Tile (dng_host &host,
					 dng_stream &stream,
					 const dng_image &image,
					 const dng_jxl_color_space_info &colorSpaceInfo,
					 const dng_jxl_encode_settings &settings);

void EncodeJXL_Container (dng_host &host,
						  dng_stream &stream,
						  const dng_image &image,
						  const dng_jxl_encode_settings &settings,
						  const dng_jxl_color_space_info &colorSpaceInfo,
						  const dng_metadata *metadata,
						  const bool includeExif,
						  const bool includeXMP,
						  const bool includeIPTC,
						  const dng_bmff_box_list *additionalBoxes);

void EncodeJXL_Container (dng_host &host,
						  dng_stream &stream,
						  const dng_pixel_buffer &buffer,
						  const dng_jxl_encode_settings &settings,
						  const dng_jxl_color_space_info &colorSpaceInfo,
						  const dng_metadata *metadata,
						  const bool includeExif,
						  const bool includeXMP,
						  const bool includeIPTC,
						  const dng_bmff_box_list *additionalBoxes);

real32 JXLQualityToDistance (uint32 quality);

dng_jxl_encode_settings * JXLQualityToSettings (uint32 quality);

static constexpr uint32 kMinJXLCompressionQuality =  1;
static constexpr uint32 kMaxJXLCompressionQuality = 13;

static constexpr uint32 kDefaultJXLCompressionQuality = 9;

void PreviewColorSpaceToJXLEncoding (const PreviewColorSpaceEnum colorSpace,
									 const uint32 planes,
									 dng_jxl_color_space_info &info);

bool SupportsJXL (const dng_image &image);

#endif	
	

