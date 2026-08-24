

#ifndef __dng_types__
#define __dng_types__

#include "dng_flags.h"

#ifdef _MSC_VER
#include <stddef.h>
#endif

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#if qDNGUseCustomIntegralTypes

#include "dng_custom_integral_types.h"

#else

#ifdef __cplusplus

typedef std::int8_t  int8;
typedef std::int16_t int16;
typedef std::int32_t int32;
typedef std::int64_t int64;

typedef std::uint8_t  uint8;
typedef std::uint16_t uint16;
typedef std::uint32_t uint32;
typedef std::uint64_t uint64;

#else

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#endif	

#endif	

typedef uintptr_t uintptr;

typedef float  real32;
typedef double real64;

#define DNG_CHAR4(a,b,c,d)	((((uint32) a) << 24) |\
							 (((uint32) b) << 16) |\
							 (((uint32) c) <<  8) |\
							 (((uint32) d)		))

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#if defined(_MSC_VER) && _MSC_VER < 1600

#ifdef hypot
#undef hypot
#endif

#define hypot _hypot

#endif

#endif
	

