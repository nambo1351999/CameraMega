

#ifndef __dng_simd_type__
#define __dng_simd_type__

#include "dng_flags.h"

#if qDNGIntelCompiler
#include <immintrin.h>
#endif	

enum SIMDType
	{

	Scalar,

	SSE2,		
	arm64_neon = SSE2,
	AVX,		
	AVX2,		
	F16C = AVX2, 
	AVX512_SKX, 

	SIMD_Sentinel

	};

template <int SIMDType>
class SIMDTraits
{
public:
	static const int kVecSizeFloat = 1;
	static const int kVecSizeInt32 = 1;
};

template <>
class SIMDTraits<SSE2>
{
public:
	static const int kVecSizeFloat = 4;
	static const int kVecSizeInt32 = 4;
};

template <>
class SIMDTraits<AVX>
{
public:
	static const int kVecSizeFloat = 8;
	static const int kVecSizeInt32 = 4;
};

template <>
class SIMDTraits<AVX2>
{
public:
	static const int kVecSizeFloat = 8;
	static const int kVecSizeInt32 = 8;
};

template <>
class SIMDTraits<AVX512_SKX>
{
public:
	static const int kVecSizeFloat = 16;
	static const int kVecSizeInt32 = 16;
};

const SIMDType SIMDTypeMaxValue = SIMDType(SIMD_Sentinel - 1);

extern SIMDType	gDNGMaxSIMD;

#if qDNGIntelCompiler

#define CR_SIMD_MIN_FEATURE	  (_FEATURE_SSE2)
#define CR_AVX_FEATURE		  (_FEATURE_AVX)
#define CR_AVX2_FEATURE		  (_FEATURE_AVX|_FEATURE_FMA|_FEATURE_AVX2)
#define CR_F16C_FEATURE			CR_AVX2_FEATURE
#define CR_AVX512_SKX_FEATURE (_FEATURE_AVX512F|_FEATURE_AVX512CD|_FEATURE_AVX512BW|_FEATURE_AVX512DQ|_FEATURE_AVX512VL)
#define CR_COMPILER_USING_AVX512_SKX (__AVX512F__ && __AVX512VL__ && __AVX512BW__ && __AVX512DQ__ && __AVX512CD__)

#define __SIMDTYPE_TFY(x) #x
#define _SIMDTYPE_TFY(x) __SIMDTYPE_TFY(x)

#define INTEL_OMP_SIMD_SIMDLEN_ASSERT(s) omp simd simdlen(s) if(simd: s>1) assert

#if qDNGDebug

#define INTEL_PRAGMA_SIMD_ASSERT _Pragma("omp simd")
#define INTEL_PRAGMA_SIMD_ASSERT_VECLEN_FLOAT(s) _Pragma(_SIMDTYPE_TFY(omp simd simdlen( SIMDTraits<s>::kVecSizeFloat ) ))
#define INTEL_PRAGMA_SIMD_ASSERT_VECLEN_INT32(s) _Pragma(_SIMDTYPE_TFY(omp simd simdlen( SIMDTraits<s>::kVecSizeInt32 ) ))
#define INTEL_PRAGMA_SIMD_ASSERT_VECLEN_INT16(s) _Pragma(_SIMDTYPE_TFY(omp simd simdlen( SIMDTraits<s>::kVecSizeInt32*2 ) ))
#define INTEL_PRAGMA_SIMD_ASSERT_VECLEN_INT8(s) _Pragma(_SIMDTYPE_TFY(omp simd simdlen( SIMDTraits<s>::kVecSizeInt32*4 ) ))

#else  

#define INTEL_PRAGMA_SIMD_ASSERT _Pragma("omp simd assert")
#define INTEL_PRAGMA_SIMD_ASSERT_VECLEN_FLOAT(s) _Pragma(_SIMDTYPE_TFY(INTEL_OMP_SIMD_SIMDLEN_ASSERT( SIMDTraits<s>::kVecSizeFloat	 ) ))
#define INTEL_PRAGMA_SIMD_ASSERT_VECLEN_INT32(s) _Pragma(_SIMDTYPE_TFY(INTEL_OMP_SIMD_SIMDLEN_ASSERT( SIMDTraits<s>::kVecSizeInt32	 ) ))
#define INTEL_PRAGMA_SIMD_ASSERT_VECLEN_INT16(s) _Pragma(_SIMDTYPE_TFY(INTEL_OMP_SIMD_SIMDLEN_ASSERT( SIMDTraits<s>::kVecSizeInt32*2 ) ))
#define INTEL_PRAGMA_SIMD_ASSERT_VECLEN_INT8(s)	 _Pragma(_SIMDTYPE_TFY(INTEL_OMP_SIMD_SIMDLEN_ASSERT( SIMDTraits<s>::kVecSizeInt32*4 ) ))

#endif	

#ifdef __INTEL_COMPILER
#define SET_CPU_FEATURE(simd) _allow_cpu_features( (simd >= AVX512_SKX) ? CR_AVX512_SKX_FEATURE : (simd >= AVX2) ? CR_AVX2_FEATURE : ((simd >= AVX) ? CR_AVX_FEATURE : CR_SIMD_MIN_FEATURE) )
#elif __INTEL_LLVM_COMPILER
#define SET_CPU_FEATURE(simd) allow_cpu_features( (simd >= AVX512_SKX) ? CR_AVX512_SKX_FEATURE : (simd >= AVX2) ? CR_AVX2_FEATURE : ((simd >= AVX) ? CR_AVX_FEATURE : CR_SIMD_MIN_FEATURE) )
#endif

#ifdef __INTEL_COMPILER
#define SET_CPU_FEATURE_NOFMA(simd) _allow_cpu_features( (simd >= AVX) ? CR_AVX_FEATURE : CR_SIMD_MIN_FEATURE)
#elif __INTEL_LLVM_COMPILER
#define SET_CPU_FEATURE_NOFMA(simd) allow_cpu_features( (simd >= AVX) ? CR_AVX_FEATURE : CR_SIMD_MIN_FEATURE)
#endif

#define INTEL_PRAGMA_NOVECTOR _Pragma("novector")
#define INTEL_COMPILER_NEEDED_NOTE

#else  

#define Stringize( L )			#L
#define MakeString( M, L )		M(L)
#define _x_Line					MakeString( Stringize, __LINE__ )

#if qDNGValidateTarget || qMacOS || defined(__clang__)

#define INTEL_COMPILER_NEEDED_NOTE
#else
#if !(defined (IOS_ENV) || defined(ANDROID_ENV)) && (defined(__x86_64__) || defined(__i386__))
#ifndef _MSC_VER
#define INTEL_COMPILER_NEEDED_NOTE _Pragma("message(\"NOTE: Intel Compiler needed for optimizations in \" __FILE__ \":\" _x_Line )")
#else

#define INTEL_COMPILER_NEEDED_NOTE __pragma(message("NOTE: Intel Compiler needed for optimizations in " __FILE__ ":" _x_Line " in " __FUNCTION__))
#endif
#else
#define INTEL_COMPILER_NEEDED_NOTE
#endif
#endif

#define INTEL_PRAGMA_SIMD_ASSERT

#define SET_CPU_FEATURE(simd)
#define INTEL_PRAGMA_SIMD_ASSERT_VECLEN_FLOAT(simd)
#define INTEL_PRAGMA_SIMD_ASSERT_VECLEN_INT16(simd)
#define INTEL_PRAGMA_SIMD_ASSERT_VECLEN_INT32(simd)
#define INTEL_PRAGMA_SIMD_ASSERT_VECLEN_INT8(simd)
#define INTEL_PRAGMA_NOVECTOR

#endif	

#endif	

