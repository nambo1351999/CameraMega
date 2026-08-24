

#ifndef __dng_sdk_limits__
#define __dng_sdk_limits__

#include "dng_flags.h"
#include "dng_types.h"

const uint32 kMaxDNGPreviews = 20;

const uint32 kMaxSemanticMasks = 100;

const uint32 kMaxSubIFDs = kMaxDNGPreviews + kMaxSemanticMasks + 5;

const uint32 kMaxChainedIFDs = 10;

const uint32 kMaxSamplesPerPixel = 5;

const uint32 kMaxColorPlanes = 4;

const uint32 kMaxCFAPattern = 8;

const uint32 kMaxBlackPattern = 8;

const uint32 kMaxMaskedAreas = 4;

#if qDNGBigImage
const uint32 kMaxImageSide = 300000;
#else
const uint32 kMaxImageSide = 65000;
#endif

const uint32 kMaxToneCurvePoints = 8192;

#if qDNG64Bit
const uint32 kMaxMPThreads = 128; 
#else
const uint32 kMaxMPThreads = 8;
#endif

const real64 kMaxStage3BlackLevelNormalized = 0.2;

const uint32 kMaxProfileGainTableMapPoints = 16777216;

const real32 kProfileGainTableMap_MinGainValue = 0.000244140625f; 
const real32 kProfileGainTableMap_MaxGainValue = 4096.0f;

const real32 kProfileGainTableMap_MinGamma = 0.125f;
const real32 kProfileGainTableMap_MaxGamma = 8.000f;

const uint32 kMinSpectrumSamples = 2;

const uint32 kMaxSpectrumSamples = 1000;

const uint32 kMaxParseDepth = 10;

#endif	
	

