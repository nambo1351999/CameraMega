

#ifndef __dng_mosaic_info__
#define __dng_mosaic_info__

#include "dng_classes.h"
#include "dng_rect.h"
#include "dng_sdk_limits.h"
#include "dng_types.h"

class dng_mosaic_info
	{
	
	public:
	
		

		dng_point fCFAPatternSize;

		

		uint8 fCFAPattern [kMaxCFAPattern] [kMaxCFAPattern];
		
		

		uint32 fColorPlanes;
		
		uint8 fCFAPlaneColor [kMaxColorPlanes];
		
		
		
		
		
		
		
		
		
		
		
		

		uint32 fCFALayout;
		
		
		
		
		
		
		
		
		

		uint32 fBayerGreenSplit;
		
	protected:
		
		dng_point fSrcSize;
		
		dng_point fCroppedSize;
		
		real64 fAspectRatio;
		
	public:

		dng_mosaic_info ();
		
		virtual ~dng_mosaic_info ();
		
		virtual void Parse (dng_host &host,
							dng_stream &stream,
							dng_info &info);
							
		virtual void PostParse (dng_host &host,
								dng_negative &negative);
								
		
		

		bool IsColorFilterArray () const
			{
			return fCFAPatternSize != dng_point (0, 0);
			}
			
		
		
		
		
		

		virtual bool SetFourColorBayer ();

		
		
		
		

		virtual dng_point FullScale () const;

		
		
		
		
		
		

		virtual dng_point DownScale (uint32 minSize,
									 uint32 prefSize,
									 real64 cropFactor) const;

		
		
		

		virtual dng_point DstSize (const dng_point &downScale) const;
								   
		
		
		
		
		
		

		virtual void InterpolateGeneric (dng_host &host,
										 dng_negative &negative,
										 const dng_image &srcImage,
										 dng_image &dstImage,
										 uint32 srcPlane = 0) const;
										 
		
		
		
		
		
		
		

		virtual void InterpolateFast (dng_host &host,
									  dng_negative &negative,
									  const dng_image &srcImage,
									  dng_image &dstImage,
									  const dng_point &downScale,
									  uint32 srcPlane = 0) const;

		
		
		
		
		
		
		

		virtual void Interpolate (dng_host &host,
								  dng_negative &negative,
								  const dng_image &srcImage,
								  dng_image &dstImage,
								  const dng_point &downScale,
								  uint32 srcPlane = 0,
								  dng_matrix *scaleTransforms = NULL) const;
								  
		virtual bool SupportsPreservedBlackLevels () const;
								  
	protected:
	
		virtual bool IsSafeDownScale (const dng_point &downScale) const;

		uint32 SizeForDownScale (const dng_point &downScale) const;
		
		virtual bool ValidSizeDownScale (const dng_point &downScale,
										 uint32 minSize) const;

	};

#endif
	

