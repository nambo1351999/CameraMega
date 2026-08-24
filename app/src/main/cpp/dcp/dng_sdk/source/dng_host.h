

#ifndef __dng_host__
#define __dng_host__

#include "dng_auto_ptr.h"
#include "dng_classes.h"
#include "dng_errors.h"
#include "dng_flags.h"
#include "dng_types.h"
#include "dng_uncopyable.h"

class dng_host: private dng_uncopyable
	{
	
	private:
	
		dng_memory_allocator *fAllocator;
		
		dng_abort_sniffer *fSniffer;
	
		
		
	
		bool fNeedsMeta;
		
		
		
		
		bool fNeedsImage;
		
		
		
		bool fForPreview;
		
		
		
		
		
		uint32 fMinimumSize;
		
		
		
		
		
		uint32 fPreferredSize;
		
		
		
		
		uint32 fMaximumSize;
		
		
		
		
		
		real64 fCropFactor;
		
		
		
		uint32 fSaveDNGVersion;
		
		
		
		bool fSaveLinearDNG;
		
		
		
		bool fLossyMosaicJXL = false;
		bool fLosslessJXL    = false;
		
		
		
		bool fKeepOriginalFile;
		
		
		
		bool fIgnoreEnhanced;

		
		

		bool fForFastSaveToDNG;

		uint32 fFastSaveToDNGSize;

		bool fPreserveStage2;
	
		std::shared_ptr<const dng_jxl_encode_settings> fJXLEncodeSettings;
		
		std::shared_ptr<const dng_jxl_color_space_info> fJXLColorSpaceInfo;
		
	public:
	
		
		
		
		
		
		
		

		dng_host (dng_memory_allocator *allocator = NULL,
				  dng_abort_sniffer *sniffer = NULL);
		
		
		
		

		virtual ~dng_host ();
		
		

		dng_memory_allocator & Allocator ();
		
		
		
		
		
		

		virtual dng_memory_block * Allocate (uint32 logicalSize);
		
		
		
		void SetSniffer (dng_abort_sniffer *sniffer)
			{
			fSniffer = sniffer;
			}
		
		

		dng_abort_sniffer * Sniffer ()
			{
			return fSniffer;
			}
		
		
		

		virtual void SniffForAbort ();
		
		
		
		
		

		void SetNeedsMeta (bool needs)
			{
			fNeedsMeta = needs;
			}

		

		bool NeedsMeta () const
			{
			return fNeedsMeta;
			}

		
		
		
		

		void SetNeedsImage (bool needs)
			{
			fNeedsImage = needs;
			}

		

		bool NeedsImage () const
			{
			return fNeedsImage;
			}

		
		
		

		void SetForPreview (bool preview)
			{
			fForPreview = preview;
			}

		
		
		
		
		
		

		bool ForPreview () const
			{
			return fForPreview;
			}

		
		
		
		void SetMinimumSize (uint32 size)
			{
			fMinimumSize = size;
			}

		

		uint32 MinimumSize () const
			{
			return fMinimumSize;
			}

		
		
		
		void SetPreferredSize (uint32 size)
			{
			fPreferredSize = size;
			}
		
		

		uint32 PreferredSize () const
			{
			return fPreferredSize;
			}
			
		
		
			
		void SetMaximumSize (uint32 size)
			{
			fMaximumSize = size;
			}
			
		
		
		uint32 MaximumSize () const
			{
			return fMaximumSize;
			}

		
		
		

		void SetForFastSaveToDNG (bool flag,
								  uint32 size)
			{
			fForFastSaveToDNG = flag;
			fFastSaveToDNGSize = size;
			}

		
		
		
		bool ForFastSaveToDNG () const
			{
			return fForFastSaveToDNG;
			}

		uint32 FastSaveToDNGSize () const
			{
			return fFastSaveToDNGSize;
			}
			
		
		
		
		void SetCropFactor (real64 cropFactor)
			{
			fCropFactor = cropFactor;
			}
			
		
		
		real64 CropFactor () const
			{
			return fCropFactor;
			}
			
		
			
		void ValidateSizes ();
						
		
		

		void SetSaveDNGVersion (uint32 version)
			{
			fSaveDNGVersion = version;
			}
			
		

		virtual uint32 SaveDNGVersion () const;

		
		

		void SetSaveLinearDNG (bool linear)
			{
			fSaveLinearDNG = linear;
			}
			
		

		virtual bool SaveLinearDNG (const dng_negative &negative) const;
			
		
		

		bool LossyMosaicJXL () const
			{
			return fLossyMosaicJXL;
			}
			
		
		
		

		bool SetLossyMosaicJXL (bool want)
			{
			return fLossyMosaicJXL = want;
			}
			
		
		

		bool LosslessJXL () const
			{
			return fLosslessJXL;
			}
			
		
		
		

		bool SetLosslessJXL (bool want)
			{
			return fLosslessJXL = want;
			}
			
		
		

		void SetKeepOriginalFile (bool keep)
			{
			fKeepOriginalFile = keep;
			}

		

		bool KeepOriginalFile ()
			{
			return fKeepOriginalFile;
			}
			
		

		bool IgnoreEnhanced () const
			{
			return fIgnoreEnhanced;
			}
			
		
		
		void SetIgnoreEnhanced (bool state)
			{
			fIgnoreEnhanced = state;
			}
		
		
		
		
		
		
		
		

		virtual bool IsTransientError (dng_error_code code);

		
		
		
		
		
		

		virtual void PerformAreaTask (dng_area_task &task,
									  const dng_rect &area,
									  dng_area_task_progress *progress = NULL);
									  
		
		
		
		virtual uint32 PerformAreaTaskThreads ();

		
		

		virtual dng_exif * Make_dng_exif ();

		
		

		#if qDNGUseXMP
		virtual dng_xmp * Make_dng_xmp ();
		#endif

		
		

		virtual dng_shared * Make_dng_shared ();

		
		

		virtual dng_ifd * Make_dng_ifd ();
		
		
		

		virtual dng_negative * Make_dng_negative ();
		
		
		
		
		virtual dng_image * Make_dng_image (const dng_rect &bounds,
											uint32 planes,
											uint32 pixelType);
								 
		
		
		
		virtual dng_opcode * Make_dng_opcode (uint32 opcodeID,
											  dng_stream &stream);
											  
		
		
		virtual dng_rgb_to_rgb_table_data *
			Make_dng_rgb_to_rgb_table_data (const dng_rgb_table &table);
											  
		
		
		
		virtual void ApplyOpcodeList (dng_opcode_list &list,
									  dng_negative &negative,
									  AutoPtr<dng_image> &image);
									  
		
		
		
		virtual void ResampleImage (const dng_image &srcImage,
									dng_image &dstImage);

		
		

		bool WantsPreserveStage2 () const
			{
			return fPreserveStage2;
			}

		
		

		void SetWantsPreserveStage2 (bool flag)
			{
			fPreserveStage2 = flag;
			}

		

		void SetJXLEncodeSettings (const dng_jxl_encode_settings &settings);
		
		const dng_jxl_encode_settings * JXLEncodeSettings () const
			{
			return fJXLEncodeSettings.get ();
			}
			
		void SetJXLColorSpaceInfo (std::shared_ptr<const dng_jxl_color_space_info> info)
			{
			fJXLColorSpaceInfo = info;
			}
		
		const dng_jxl_color_space_info * JXLColorSpaceInfo () const
			{
			return fJXLColorSpaceInfo.get ();
			}
			
		std::shared_ptr<const dng_jxl_color_space_info> ShareJXLColorSpaceInfo () const
			{
			return fJXLColorSpaceInfo;
			}
			
		enum use_case_enum
			{
			use_case_LossyMosaic,
			use_case_LosslessMosaic,
			use_case_MainImage,
			use_case_LosslessMainImage,
			use_case_EncodedMainImage,
			use_case_ProxyImage,
			use_case_EnhancedImage,
			use_case_LosslessEnhancedImage,
			use_case_MergeResults,
			use_case_Transparency,
			use_case_LosslessTransparency,
			use_case_Depth,
			use_case_LosslessDepth,
			use_case_SemanticMask,
			use_case_LosslessSemanticMask,
			use_case_RenderedPreview,
			use_case_GainMap,
			use_case_LosslessGainMap
			};
			
		virtual dng_jxl_encode_settings *
			MakeJXLEncodeSettings (use_case_enum useCase,
								   const dng_image &image,
								   const dng_negative *negative = nullptr) const;

	};
	

#endif	
	

