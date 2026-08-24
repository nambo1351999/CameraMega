

#ifndef __dng_tag_values__
#define __dng_tag_values__

#include "dng_flags.h"

enum
	{
	
	
	
	sfMainImage					= 0,
	
	
	
	sfPreviewImage				= 1,
	
	
	
	sfTransparencyMask			= 4,
		
	
	
	sfPreviewMask				= sfPreviewImage + sfTransparencyMask,
	
	
	
	sfDepthMap					= 8,
		
	
		
	sfPreviewDepthMap			= sfPreviewImage + sfDepthMap,
	
	
	
	sfEnhancedImage				= 16,

	

	sfGainMap					= 32,
		
	

	sfPreviewGainMap			= sfPreviewImage + sfGainMap,
		
	
	
	sfAltPreviewImage			= 0x10001,

	

	sfSemanticMask				= 0x10004,	 
	sfPreviewSemanticMask		= sfPreviewImage + sfSemanticMask,
	
	};

enum
	{

	piWhiteIsZero				= 0,
	piBlackIsZero				= 1,
	piRGB						= 2,
	piRGBPalette				= 3,
	piTransparencyMask			= 4,
	piCMYK						= 5,
	piYCbCr						= 6,
	piCIELab					= 8,
	piICCLab					= 9,

	piCFA						= 32803,		

	piLinearRaw					= 34892,

	piDepth						= 51177,

	piPhotometricMask			= 52527,		
	
	piGainMap					= 52553,		
	
	};

enum
	{
	
	pcInterleaved				= 1,
	pcPlanar					= 2,
	
	
	
	
	
	
	
	
	
	
	
	
	
	pcRowInterleaved			= 100000,		
	pcRowInterleavedAlignSIMD	= 100001		
	
	};

enum
	{
	
	esUnspecified				= 0,
	esAssociatedAlpha			= 1,
	esUnassociatedAlpha			= 2
	
	};

enum
	{
	
	sfUnsignedInteger			= 1,
	sfSignedInteger				= 2,
	sfFloatingPoint				= 3,
	sfUndefined					= 4
	
	};

enum
	{
	
	ccUncompressed				= 1,
	ccLZW						= 5,
	ccOldJPEG					= 6,
	ccJPEG						= 7,
	ccDeflate					= 8,

	#if qDNGSupportVC5
	ccVc5						= 9,
	#endif	

	ccPackBits					= 32773,
	ccOldDeflate				= 32946,
	
	
	
	ccLossyJPEG					= 34892,

	ccJXL						= 52546,
	
	};

enum
	{
	
	cpNullPredictor				= 1,
	cpHorizontalDifference		= 2,
	cpFloatingPoint				= 3,
	
	cpHorizontalDifferenceX2	= 34892,
	cpHorizontalDifferenceX4	= 34893,
	cpFloatingPointX2			= 34894,
	cpFloatingPointX4			= 34895
	
	};

enum
	{
	
	ruNone						= 1,
	ruInch						= 2,
	ruCM						= 3,
	ruMM						= 4,
	ruMicroM					= 5
	
	};		

enum
	{
	
	lsUnknown					=  0,
	
	lsDaylight					=  1,
	lsFluorescent				=  2,
	lsTungsten					=  3,
	lsFlash						=  4,
	lsFineWeather				=  9,
	lsCloudyWeather				= 10,
	lsShade						= 11,
	lsDaylightFluorescent		= 12,		
	lsDayWhiteFluorescent		= 13,		
	lsCoolWhiteFluorescent		= 14,		
	lsWhiteFluorescent			= 15,		
	lsWarmWhiteFluorescent		= 16,		
	lsStandardLightA			= 17,
	lsStandardLightB			= 18,
	lsStandardLightC			= 19,
	lsD55						= 20,
	lsD65						= 21,
	lsD75						= 22,
	lsD50						= 23,
	lsISOStudioTungsten			= 24,
	
	lsOther						= 255
	
	};

enum
	{
	
	epUnidentified				= 0,
	epManual					= 1,
	epProgramNormal				= 2,
	epAperturePriority			= 3,
	epShutterPriority			= 4,
	epProgramCreative			= 5,
	epProgramAction				= 6,
	epPortraitMode				= 7,
	epLandscapeMode				= 8
	
	};		

enum
	{
	
	mmUnidentified				= 0,
	mmAverage					= 1,
	mmCenterWeightedAverage		= 2,
	mmSpot						= 3,
	mmMultiSpot					= 4,
	mmPattern					= 5,
	mmPartial					= 6,
	
	mmOther						= 255
	
	};		

enum ColorKeyCode
	{
	
	colorKeyRed					= 0,
	colorKeyGreen				= 1,
	colorKeyBlue				= 2,
	colorKeyCyan				= 3,
	colorKeyMagenta				= 4,
	colorKeyYellow				= 5,
	colorKeyWhite				= 6,
	
	colorKeyMaxEnum				= 0xFF
	
	};

enum
	{
		
	stUnknown					= 0,

	stStandardOutputSensitivity = 1,
	stRecommendedExposureIndex	= 2,
	stISOSpeed					= 3,
	stSOSandREI					= 4,
	stSOSandISOSpeed			= 5,
	stREIandISOSpeed			= 6,
	stSOSandREIandISOSpeed		= 7
		
	};
	

enum
	{
	
	
	
	crSceneReferred				= 0,
	
	
	
	crICCProfilePCS				= 1,
	
	
	
	crOutputReferredHDR			= 2
	
	};

enum
	{
	
	
	
	
	pepAllowCopying				= 0,
	
	
	
	
	pepEmbedIfUsed				= 1,
	
	
	
	
	
	
	pepEmbedNever				= 2,
	
	
	
	pepNoRestrictions			= 3

	};

enum
	{
	
	
	
	
	
	
	encoding_Linear				= 0,
	
	
	
	
	
	
	

	encoding_sRGB				= 1
	
	};

enum
	{

	
	
	
	defaultBlackRender_Auto		= 0,
	
	
	
	
	defaultBlackRender_None		= 1
	
	};

enum
	{

	
	
	profileToneMethod_Unspecified	= 0,
	
	
	
	
	profileToneMethod_AdobePV5		= 1,
	
	
	
	
	profileToneMethod_AdobePV6		= 2
	
	};

enum PreviewColorSpaceEnum
	{
	
	previewColorSpace_Unknown		= 0,
	previewColorSpace_GrayGamma22	= 1,
	previewColorSpace_sRGB			= 2,
	previewColorSpace_AdobeRGB		= 3,
	previewColorSpace_ProPhotoRGB	= 4,
	
	previewColorSpace_LastValid		= previewColorSpace_ProPhotoRGB,

	previewColorSpace_MaxEnum		= 0xFFFFFFFF
	
	};

enum
	{
	
	
	
	cacheVersionMask				= 0x0FFFF,
	
	
	
	cacheVersionDefault				= 0x00100,
	
	
	
	cacheVersionDefloated			= 0x10000,
	
	
	
	cacheVersionFlattened			= 0x20000,
	
	
	

	cacheVersionFakeMerge			= 0x40000,

	
	
	
	
	
	
	
	

	cacheVersionGainMapApplied		= 0x80000

	};

enum
	{
	depthFormatUnknown				= 0,
	depthFormatLinear				= 1,
	depthFormatInverse				= 2
	};

enum
	{
	depthUnitsUnknown				= 0,
	depthUnitsMeters				= 1
	};

enum
	{
	depthMeasureUnknown				= 0,
	depthMeasureOpticalAxis			= 1,
	depthMeasureOpticalRay			= 2
	};

enum
	{
	
	byteOrderII					= 0x4949,		
	byteOrderMM					= 0x4D4D		
	
	};

enum
	{
	
	
	
	magicTIFF					= 42,			
	magicBigTIFF				= 43,			
	magicExtendedProfile		= 0x4352,		
	magicRawCache				= 1022,			
	
	
	
	magicPanasonic				= 85,
	magicOlympusA				= 0x4F52,
	magicOlympusB				= 0x5352
	
	};
	

enum
	{
	
	dngVersion_None				= 0,
	
	dngVersion_1_0_0_0			= 0x01000000,
	dngVersion_1_1_0_0			= 0x01010000,
	dngVersion_1_2_0_0			= 0x01020000,
	dngVersion_1_3_0_0			= 0x01030000,
	dngVersion_1_4_0_0			= 0x01040000,
	dngVersion_1_5_0_0			= 0x01050000,
	dngVersion_1_6_0_0			= 0x01060000,
	dngVersion_1_7_0_0			= 0x01070000,
	dngVersion_1_7_1_0			= 0x01070100,

	dngVersion_Current			= dngVersion_1_7_1_0,

	dngVersion_SaveDefault		= dngVersion_1_7_1_0
	
	};

#endif
	

