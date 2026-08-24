

#include "dng_bottlenecks.h"
#include "dng_flags.h"
#include "dng_lossless_jpeg.h"

#include "dng_reference.h"

dng_suite gDNGSuite =
	{
	RefZeroBytes,
	RefCopyBytes,
	RefSwapBytes16,
	RefSwapBytes32,
	RefSetArea8,
	RefSetArea<Scalar, uint16>,
	RefSetArea<Scalar, uint32>,
	RefCopyArea8,
	RefCopyArea16,
	RefCopyArea32,
	RefCopyArea8_16,
	RefCopyArea8_S16,
	RefCopyArea8_32,
	RefCopyArea16_S16<Scalar>,
	RefCopyArea16_32,
	RefCopyArea8_R32,
	RefCopyArea16_R32,
	RefCopyAreaS16_R32,
	RefCopyAreaR32_8,
	RefCopyAreaR32_16,
	RefCopyAreaR32_S16,
	RefRepeatArea8,
	RefRepeatArea16,
	RefRepeatArea32,
	RefShiftRight16,
	RefBilinearRow16,
	RefBilinearRow32,
	RefBaselineABCtoRGB,
	RefBaselineABCDtoRGB,
	RefBaselineHueSatMap,
	RefBaselineRGBtoGray,
	RefBaselineRGBtoRGB,
	RefBaseline1DTable,
	RefBaselineRGBTone,
	RefResampleDown16,
	RefResampleDown32,
	RefResampleAcross16,
	RefResampleAcross32,
	RefEqualBytes,
	RefEqualArea8,
	RefEqualArea16,
	RefEqualArea32,
	RefVignetteMask16,
	RefVignette16,
	RefVignette32,
	RefMapArea16,
	RefBaselineMapPoly32,
	DecodeLosslessJPEG<Scalar>,
	EncodeLosslessJPEG<Scalar>,
	RefBaselineProfileGainTableMap,
	RefRGBtoRGBTable3D,
	RefRGBtoRGBTable1D,
	};

