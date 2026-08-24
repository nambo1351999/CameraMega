

#ifndef __dng_utils__
#define __dng_utils__

#include <cmath>
#include <limits>
#include <utility>

#include "dng_classes.h"
#include "dng_flags.h"
#include "dng_memory.h"
#include "dng_safe_arithmetic.h"
#include "dng_string.h"
#include "dng_types.h"
#include "dng_uncopyable.h"

#if qWinOS
#undef min
#undef max
#endif

DNG_ATTRIB_NO_SANITIZE("unsigned-integer-overflow")
inline uint32 Abs_int32 (int32 x)
	{
	
	#if 0
	
	
	
	return (uint32) (x < 0 ? -x : x);
	
	#else
	
	
	
	uint32 mask = (uint32) (x >> 31);
	
	return (uint32) (((uint32) x + mask) ^ mask);
	
	#endif
	
	}

inline int32 Min_int32 (int32 x, int32 y)
	{
	
	return (x <= y ? x : y);
	
	}

inline int32 Max_int32 (int32 x, int32 y)
	{
	
	return (x >= y ? x : y);
	
	}

inline int32 Pin_int32 (int32 min, int32 x, int32 max)
	{
	
	return Max_int32 (min, Min_int32 (x, max));
	
	}

inline int32 Pin_int32_between (int32 a, int32 x, int32 b)
	{
	
	int32 min, max;
	if (a < b) { min = a; max = b; }
	else { min = b; max = a; }
	
	return Pin_int32 (min, x, max);
	
	}

inline uint16 Min_uint16 (uint16 x, uint16 y)
	{
	
	return (x <= y ? x : y);
	
	}

inline uint16 Max_uint16 (uint16 x, uint16 y)
	{
	
	return (x >= y ? x : y);
	
	}

inline int16 Pin_int16 (int32 x)
	{
	
	x = Pin_int32 (-32768, x, 32767);
	
	return (int16) x;
	
	}
	

inline uint32 Min_uint32 (uint32 x, uint32 y)
	{
	
	return (x <= y ? x : y);
	
	}

inline uint32 Min_uint32 (uint32 x, uint32 y, uint32 z)
	{
	
	return Min_uint32 (x, Min_uint32 (y, z));
	
	}
	
inline uint32 Max_uint32 (uint32 x, uint32 y)
	{
	
	return (x >= y ? x : y);
	
	}
	
inline uint32 Max_uint32 (uint32 x, uint32 y, uint32 z)
	{
	
	return Max_uint32 (x, Max_uint32 (y, z));
	
	}
	
inline uint32 Pin_uint32 (uint32 min, uint32 x, uint32 max)
	{
	
	return Max_uint32 (min, Min_uint32 (x, max));
	
	}

inline uint16 Pin_uint16 (int32 x)
	{
	
	#if 0
	
	
	
	x = Pin_int32 (0, x, 0x0FFFF);
	
	#else
	
	
	
	if (x & ~65535)
		{
		
		x = ~x >> 31;
		
		}
		
	#endif
		
	return (uint16) x;
	
	}

inline uint32 RoundUp2 (uint32 x)
	{
	
	return (x + 1) & (uint32) ~1;
	
	}

inline uint32 RoundUp4 (uint32 x)
	{
	
	return (x + 3) & (uint32) ~3;
	
	}

inline uint32 RoundUp8 (uint32 x)
	{
	
	return (x + 7) & (uint32) ~7;
	
	}

inline uint32 RoundUp16 (uint32 x)
	{
	
	return (x + 15) & (uint32) ~15;
	
	}

inline uint32 RoundUp32 (uint32 x)
	{
	
	return (x + 31) & (uint32) ~31;
	
	}

inline uint32 RoundUpSIMD (uint32 x)
	{

	#if qDNGAVXSupport
	return RoundUp32 (x);
	#else
	return RoundUp16 (x);
	#endif
	
	}

inline uint32 RoundUp4096 (uint32 x)
	{
	
	return (x + 4095) & (uint32) ~4095;
	
	}

inline uint32 RoundDown2 (uint32 x)
	{
	
	return x & (uint32) ~1;
	
	}

inline uint32 RoundDown4 (uint32 x)
	{
	
	return x & (uint32) ~3;
	
	}

inline uint32 RoundDown8 (uint32 x)
	{
	
	return x & (uint32) ~7;
	
	}

inline uint32 RoundDown16 (uint32 x)
	{
	
	return x & (uint32) ~15;
	
	}

inline uint32 RoundDown32 (uint32 x)
	{
	
	return x & (uint32) ~31;
	
	}

inline uint32 RoundDownSIMD (uint32 x)
	{

	#if qDNGAVXSupport
	return RoundDown32 (x);
	#else
	return RoundDown16 (x);
	#endif
	
	}

inline bool RoundUpForPixelSize (uint32 x, 
								 uint32 pixelSize, 
								 uint32 *result)
	{

	#if qDNGAVXSupport
	static const uint32 kTargetMultiple = 32;
	#else
	static const uint32 kTargetMultiple = 16;
	#endif

	uint32 multiple;

	switch (pixelSize)
		{
		
		case 1:
		case 2:
		case 4:
		case 8:
			{
			multiple = kTargetMultiple / pixelSize;
			break;
			}
			
		default:
			{
			multiple = kTargetMultiple;
			break;
			}
					
		}
	
	return RoundUpUint32ToMultiple (x, multiple, result);

	}

inline uint32 RoundUpForPixelSize (uint32 x,
								   uint32 pixelSize)
	{
	
	uint32 result = 0;

	if (!RoundUpForPixelSize (x, pixelSize, &result))
		{
		ThrowOverflow ("RoundUpForPixelSize");
		}

	return result;
	
	}

inline int32 RoundUpForPixelSizeAsInt32 (uint32 x,
										 uint32 pixelSize)
	{
	
	uint32 result = 0;

	if (!RoundUpForPixelSize (x, pixelSize, &result))
		{
		ThrowOverflow ("RoundUpForPixelSize");
		}

	dng_safe_uint32 safeResult (result);

	return dng_safe_int32 (safeResult).Get ();
	
	}

enum PaddingType
	{

	

	padNone,

	
	

	padSIMDBytes

	};

uint32 ComputeBufferSize (uint32 pixelType, 
						  const dng_point &tileSize,
						  uint32 numPlanes, 
						  PaddingType paddingType);

inline uint64 Abs_int64 (int64 x)
	{
	
	return (uint64) (x < 0 ? -x : x);

	}

inline int64 Min_int64 (int64 x, int64 y)
	{
	
	return (x <= y ? x : y);
	
	}

inline int64 Max_int64 (int64 x, int64 y)
	{
	
	return (x >= y ? x : y);
	
	}

inline int64 Pin_int64 (int64 min, int64 x, int64 max)
	{
	
	return Max_int64 (min, Min_int64 (x, max));
	
	}

inline uint64 Min_uint64 (uint64 x, uint64 y)
	{
	
	return (x <= y ? x : y);
	
	}

inline uint64 Max_uint64 (uint64 x, uint64 y)
	{
	
	return (x >= y ? x : y);
	
	}

inline uint64 Pin_uint64 (uint64 min, uint64 x, uint64 max)
	{
	
	return Max_uint64 (min, Min_uint64 (x, max));
	
	}

DNG_ALWAYS_INLINE
real32 Abs_real32 (real32 x)
	{
	
	return (x < 0.0f ? -x : x);
	
	}

DNG_ALWAYS_INLINE
real32 Min_real32 (real32 x, real32 y)
	{
	
	return (x < y ? x : y);
	
	}

DNG_ALWAYS_INLINE
real32 Max_real32 (real32 x, real32 y)
	{
	
	return (x > y ? x : y);
	
	}

DNG_ALWAYS_INLINE
real32 Pin_real32 (real32 min, real32 x, real32 max)
	{
	
	return Max_real32 (min, Min_real32 (x, max));
	
	}
	
DNG_ALWAYS_INLINE
real32 Pin_real32 (real32 x)
	{

	return Pin_real32 (0.0f, x, 1.0f);

	}

DNG_ALWAYS_INLINE
real32 Pin_real32_Overrange (real32 min, 
							 real32 x, 
							 real32 max)
	{
	
	
	
	if (x > min && x < max)
		{
		return x;
		}
		
	
		
	else if (x > min)
		{
		return max;
		}
		
	
		
	return min;
	
	}

DNG_ALWAYS_INLINE
real32 Pin_Overrange (real32 x)
	{
	
	
	
	if (x > 0.0f && x <= 1.0f)
		{
		return x;
		}
		
	
		
	else if (x > 0.5f)
		{
		return 1.0f;
		}
		
	
		
	return 0.0f;
	
	}

DNG_ALWAYS_INLINE
real32 Lerp_real32 (real32 a, real32 b, real32 t)
	{
	
	return a + t * (b - a);
	
	}

inline real64 Abs_real64 (real64 x)
	{
	
	return (x < 0.0 ? -x : x);
	
	}

inline real64 Min_real64 (real64 x, real64 y)
	{
	
	return (x < y ? x : y);
	
	}

inline real64 Max_real64 (real64 x, real64 y)
	{
	
	return (x > y ? x : y);
	
	}

inline real64 Pin_real64 (real64 min, real64 x, real64 max)
	{
	
	return Max_real64 (min, Min_real64 (x, max));
	
	}

inline real64 Pin_real64 (real64 x)
	{
	
	return Pin_real64 (0.0, x, 1.0);
	
	}

inline real64 Pin_real64_Overrange (real64 min, 
									real64 x, 
									real64 max)
	{
	
	
	
	if (x > min && x < max)
		{
		return x;
		}
		
	
		
	else if (x > min)
		{
		return max;
		}
		
	
		
	return min;
	
	}

inline real64 Lerp_real64 (real64 a, real64 b, real64 t)
	{
	
	return a + t * (b - a);
	
	}

inline uint8 Floor_uint8 (real32 x)
	{

	return (uint8) Max_real32 (0.0f, x);

	}

inline uint8 Floor_uint8 (real64 x)
	{

	return (uint8) Max_real64 (0.0, x);

	}

inline uint8 Round_uint8 (real32 x)
	{

	return Floor_uint8 (x + 0.5f);

	}

inline uint8 Round_uint8 (real64 x)
	{

	return Floor_uint8 (x + 0.5);

	}

inline int32 Round_int32 (real32 x)
	{
	
	return (int32) (x > 0.0f ? x + 0.5f : x - 0.5f);
	
	}

inline int32 Round_int32 (real64 x)
	{
	
	const real64 temp = x > 0.0 ? x + 0.5 : x - 0.5;
	
	
	

	if (temp > real64 (std::numeric_limits<int32>::min ()) - 1.0 &&
		temp < real64 (std::numeric_limits<int32>::max ()) + 1.0)
		{
		return (int32) temp;
		}
	
	else
		{
		ThrowProgramError ("Overflow in Round_int32");
		
		return 0;
		}
	
	}

inline uint32 Floor_uint32 (real64 x)
	{

	const real64 temp = Max_real64 (0.0, x);

	
	
	
	if (temp < real64 (std::numeric_limits<uint32>::max ()) + 1.0)
		{
		return (uint32) temp;
		}
	
	else
		{
		ThrowProgramError ("Overflow in Floor_uint32");
		
		return 0;
		}
	
	}

inline uint32 Floor_uint32 (real32 x)
	{
	
	return Floor_uint32 (static_cast<real64> (x));
	
	}

inline uint32 Round_uint32 (real32 x)
	{
	
	return Floor_uint32 (x + 0.5f);
	
	}

inline uint32 Round_uint32 (real64 x)
	{
	
	return Floor_uint32 (x + 0.5);
	
	}

inline int64 Round_int64 (real64 x)
	{
	
	return (int64) (x >= 0.0 ? x + 0.5 : x - 0.5);
	
	}

const int64 kFixed64_One  = (((int64) 1) << 32);
const int64 kFixed64_Half = (((int64) 1) << 31);

inline int64 Real64ToFixed64 (real64 x)
	{
	
	return Round_int64 (x * (real64) kFixed64_One);
	
	}

inline real64 Fixed64ToReal64 (int64 x)
	{
	
	return x * (1.0 / (real64) kFixed64_One);
	
	}

inline char ForceUppercase (char c)
	{
	
	if (c >= 'a' && c <= 'z')
		{
		
		c -= 'a' - 'A';
		
		}
		
	return c;
	
	}

inline uint16 SwapBytes16 (uint16 x)
	{
	
	return (uint16) ((x << 8) |
					 (x >> 8));
	
	}

inline uint32 SwapBytes32 (uint32 x)
	{
	
	return (x << 24) +
		   ((x << 8) & 0x00FF0000) +
		   ((x >> 8) & 0x0000FF00) +
		   (x >> 24);
	
	}

inline bool IsAligned16 (const void *p)
	{
	
	return (((uintptr) p) & 1) == 0;
	
	}

inline bool IsAligned32 (const void *p)
	{
	
	return (((uintptr) p) & 3) == 0;
	
	}

inline bool IsAligned64 (const void *p)
	{
	
	return (((uintptr) p) & 7) == 0;
	
	}

inline bool IsAligned128 (const void *p)
	{
	
	return (((uintptr) p) & 15) == 0;
	
	}

inline void DNG_RGBtoHSV (real32 r,
						  real32 g,
						  real32 b,
						  real32 &h,
						  real32 &s,
						  real32 &v)
	{

	v = Max_real32 (r, Max_real32 (g, b));

	real32 gap = v - Min_real32 (r, Min_real32 (g, b));
	
	if (gap > 0.0f)
		{

		if (r == v)
			{
			
			h = (g - b) / gap;
			
			if (h < 0.0f)
				{
				h += 6.0f;
				}
				
			}
			
		else if (g == v) 
			{
			h = 2.0f + (b - r) / gap;
			}
			
		else
			{
			h = 4.0f + (r - g) / gap;
			}
			
		s = gap / v;
		
		}
		
	else
		{
		h = 0.0f;
		s = 0.0f;
		}

	}

inline void DNG_PinnedNonnegativeRGBtoHSV (real32 r,
										   real32 g,
										   real32 b,
										   real32 &h,
										   real32 &s,
										   real32 &v)
	{

	

	r = Max_real32 (r, 0.0f);
	g = Max_real32 (g, 0.0f);
	b = Max_real32 (b, 0.0f);

	DNG_RGBtoHSV (r, g, b, h, s, v);

	}

inline void DNG_HSVtoRGB (real32 h,
						  real32 s,
						  real32 v,
						  real32 &r,
						  real32 &g,
						  real32 &b)
	{
	
	if (s > 0.0f)
		{
		
		if (!std::isfinite (h))
			ThrowProgramError ("Unexpected NaN or Inf");

		h = std::fmod (h, 6.0f);
		
		if (h < 0.0f)
			h += 6.0f;
			
		int32  i = (int32) h;
		real32 f = h - (real32) i;
		
		real32 p = v * (1.0f - s);
		
		#define q	(v * (1.0f - s * f))
		#define t	(v * (1.0f - s * (1.0f - f)))

		
		
		
		
		
		
		

		switch (i)
			{
			case 0: r = v; g = t; b = p; break;
			case 1: r = q; g = v; b = p; break;
			case 2: r = p; g = v; b = t; break;
			case 3: r = p; g = q; b = v; break;
			case 4: r = t; g = p; b = v; break;
			case 5: r = v; g = p; b = q; break;
			case 6: r = v; g = t; b = p; break; 
			}
			
		#undef q
		#undef t
		
		}
		
	else
		{
		r = v;
		g = v;
		b = v;
		}
	
	}

real64 TickTimeInSeconds ();

real64 TickCountInSeconds ();

void DNGIncrementTimerLevel ();

int32 DNGDecrementTimerLevel ();

class dng_timer: private dng_uncopyable
	{

	public:

		dng_timer (const char *message);

		~dng_timer ();
		
	private:

		const char *fMessage;
		
		real64 fStartTime;
		
	};

real64 MaxSquaredDistancePointToRect (const dng_point_real64 &point,
									  const dng_rect_real64 &rect);

real64 MaxDistancePointToRect (const dng_point_real64 &point,
							   const dng_rect_real64 &rect);

inline uint32 DNG_HalfToFloat (uint16 halfValue)
	{

	int32 sign	   = (halfValue >> 15) & 0x00000001;
	int32 exponent = (halfValue >> 10) & 0x0000001f;
	int32 mantissa =  halfValue		   & 0x000003ff;
	
	if (exponent == 0)
		{
		
		if (mantissa == 0)
			{
			
			

			return (uint32) (sign << 31);
			
			}
			
		else
			{
			
			

			while (!(mantissa & 0x00000400))
				{
				mantissa <<= 1;
				exponent -=	 1;
				}

			exponent += 1;
			mantissa &= ~0x00000400;
			
			}
			
		}
		
	else if (exponent == 31)
		{
		
		if (mantissa == 0)
			{
			
			
			
			return (uint32) ((sign << 31) | ((0x1eL + 127 - 15) << 23) |  (0x3ffL << 13));

			}
			
		else
			{
			
			

			return 0;
			
			}
			
		}

	

	exponent += (127 - 15);
	mantissa <<= 13;

	

	return (uint32) ((sign << 31) | (exponent << 23) | mantissa);
	
	}

inline uint16 DNG_FloatToHalf (uint32 i)
	{
	
	int32 sign	   =  (i >> 16) & 0x00008000;
	int32 exponent = ((i >> 23) & 0x000000ff) - (127 - 15);
	int32 mantissa =   i		& 0x007fffff;

	if (exponent <= 0)
		{
		
		if (exponent < -10)
			{
			
			
			
			return (uint16)sign;
			
			}

		

		mantissa = (mantissa | 0x00800000) >> (1 - exponent);

		
		
		
		
		
		

		if (mantissa &	0x00001000)
			mantissa += 0x00002000;

		

		return (uint16)(sign | (mantissa >> 13));
		
		}
	
	else if (exponent == 0xff - (127 - 15))
		{
		
		if (mantissa == 0)
			{
			
			
			

			return (uint16)(sign | 0x7c00);
			
			}
			
		else
			{
			
			
			
			

			return (uint16)(sign | 0x7c00 | (mantissa >> 13));
			
			}
	
		}

	
	

	

	if (mantissa & 0x00001000)
		{
		
		mantissa += 0x00002000;

		if (mantissa & 0x00800000)
			{
			mantissa =	0;		
			exponent += 1;		
			}

		}

	

	if (exponent > 30)
		{
		return (uint16)(sign | 0x7c00);	
		}

	

	return (uint16)(sign | (exponent << 10) | (mantissa >> 13));
		
	}

inline uint32 DNG_FP24ToFloat (const uint8 *input)
	{

	int32 sign	   = (input [0] >> 7) & 0x01;
	int32 exponent = (input [0]		) & 0x7F;
	int32 mantissa = (((int32) input [1]) << 8) | input[2];
	
	if (exponent == 0)
		{
		
		if (mantissa == 0)
			{
			
			
			
			return (uint32) (sign << 31);
			
			}

		else
			{
			
			

			while (!(mantissa & 0x00010000))
				{
				mantissa <<= 1;
				exponent -=	 1;
				}

			exponent += 1;
			mantissa &= ~0x00010000;
			
			}
	
		}
		
	else if (exponent == 127)
		{
		
		if (mantissa == 0)
			{
			
			
	
			return (uint32) ((sign << 31) | ((0x7eL + 128 - 64) << 23) |  (0xffffL << 7));
			
			}
			
		else
			{
			
			

			return 0;
			
			}
			
		}
	
	

	exponent += (128 - 64);
	mantissa <<= 7;

	

	return (uint32) ((sign << 31) | (exponent << 23) | mantissa);
	
	}

inline void DNG_FloatToFP24 (uint32 input, uint8 *output)
	{
	
	int32 exponent = (int32) ((input >> 23) & 0xFF) - 128;
	int32 mantissa = input & 0x007FFFFF;

	if (exponent == 127)	
		{
		
		
		
		if (mantissa != 0x007FFFFF && ((mantissa >> 7) == 0xFFFF))
			{
			
			mantissa &= 0x003FFFFF;		
			
			}
			
		}
		
	else if (exponent > 63)		
		{
		
		exponent = 63;
		mantissa = 0x007FFFFF;
		
		}
		
	else if (exponent <= -64) 
		{
		
		if (exponent >= -79)		
			{
			mantissa = (mantissa | 0x00800000) >> (-63 - exponent);
			}
			
		else						
			{
			mantissa = 0;
			}

		exponent = -64;
		
		}

	output [0] = (uint8)(((input >> 24) & 0x80) | (uint32) (exponent + 64));
	
	output [1] = (mantissa >> 15) & 0x00FF;
	output [2] = (mantissa >>  7) & 0x00FF;
	
	}

#ifndef MULUH

#if defined(_X86_) && defined(_MSC_VER)

inline uint32 Muluh86 (uint32 x, uint32 y)
	{
	uint32 result;
	__asm
		{
		MOV		EAX, x
		MUL		y
		MOV		result, EDX
		}
	return (result);
	}

#define MULUH	Muluh86

#else

#define MULUH(x,y)	((uint32) (((x) * (uint64) (y)) >> 32))

#endif

#endif

#ifndef MULSH

#if defined(_X86_) && defined(_MSC_VER)

inline int32 Mulsh86 (int32 x, int32 y)
	{
	int32 result;
	__asm
		{
		MOV		EAX, x
		IMUL	y
		MOV		result, EDX
		}
	return (result);
	}

#define MULSH	Mulsh86

#else

#define MULSH(x,y)	((int32) (((x) * (int64) (y)) >> 32))

#endif

#endif

DNG_ATTRIB_NO_SANITIZE("unsigned-integer-overflow")
inline uint32 DNG_Random (uint32 seed)
	{
	
	
	
	uint32 temp = MULUH (0x069C16BD, seed);
	uint32 high = (temp + ((seed - temp) >> 1)) >> 16;
	
	
	
	uint32 low = seed - high * 127773;
	
	
	
	seed = 16807 * low - 2836 * high;
	
	if (seed & 0x80000000)
		seed += 2147483647;
		
	return seed;
	
	}

class dng_dither: private dng_uncopyable
	{
		
	public:

		static const uint32 kRNGBits = 7;

		static const uint32 kRNGSize = 1 << kRNGBits;

		static const uint32 kRNGMask = kRNGSize - 1;

		static const uint32 kRNGSize2D = kRNGSize * kRNGSize;

	private:

		dng_memory_data fNoiseBuffer;

	private:

		dng_dither ();

	public:

		static const dng_dither & Get ();

	public:

		const uint16 *NoiseBuffer16 () const
			{
			return fNoiseBuffer.Buffer_uint16 ();
			}
		
	};

void HistogramArea (dng_host &host,
					const dng_image &image,
					const dng_rect &area,
					uint32 *hist,
					uint32 histLimit,
					uint32 plane = 0);

void LimitFloatBitDepth (dng_host &host,
						 const dng_image &srcImage,
						 dng_image &dstImage,
						 uint32 bitDepth,
						 real32 scale = 1.0f);

#if qMacOS

template<typename T>
class CFReleaseHelper
	{

	private:

		T fRef;

	public:

		CFReleaseHelper (T ref)
			:	fRef (ref)
			{
			}

		~CFReleaseHelper ()
			{
			if (fRef)
				{
				CFRelease (fRef);
				}
			}

		T Get () const
			{
			return fRef;
			}

	};

#endif	

static inline real64 SmoothStep (real64 x)
	{
	
	return x * x * (3.0 - 2.0 * x);
	
	}

uint32 MinBackwardVersionForCompression (uint32 compression);

class dng_line_real32
	{
		
	public:

		real32 fX;
		real32 fY;
		real32 fSlope;

	public:

		dng_line_real32 (real32 x0,
						 real32 y0,
						 real32 x1,
						 real32 y1)

			:	fX (x0)
			,	fY (y0)

			,	fSlope ((x0 == x1) ? 0.0f : ((y0 - y1) / (x0 - x1)))

			{

			}

		DNG_ALWAYS_INLINE real32 Evaluate (real32 x) const
			{

			return fY + fSlope * (x - fX);

			}

	};

class dng_image_sequence_info
	{

	public:

		dng_string fSequenceID;

		dng_string fSequenceType;

		dng_string fFrameInfo;

		uint32 fIndex = 0;

		uint32 fCount = 0;

		uint8 fIsFinal = 2;		 

	public:

		bool IsValid () const
			{
			return (fSequenceID  .Length () >= 8 &&
					fSequenceType.Length () >= 1);
			}

		uint32 TagCount () const
			{
			
			uint32 sum = SafeUint32Add (fSequenceID	 .Length (),
										fSequenceType.Length ());
			
			sum = SafeUint32Add (fFrameInfo.Length (), sum);

			sum = SafeUint32Add (sum,
								 1 +		 
								 1 +		 
								 1 +		 
								 4 +		 
								 4 +		 
								 1);		 
								 
			return sum;
			
			}

		tiff_tag * MakeTag (dng_memory_allocator &allocator) const;

		bool operator== (const dng_image_sequence_info &src) const
			{
			return (fSequenceID	  == src.fSequenceID   &&
					fSequenceType == src.fSequenceType &&
					fFrameInfo	  == src.fFrameInfo	   &&
					fIndex		  == src.fIndex		   &&
					fCount		  == src.fCount		   &&
					fIsFinal	  == src.fIsFinal);
			}
		
	};

class dng_image_stats
	{

	public:

		static constexpr uint32 kTag_WeightedAverage   = 1;
		static constexpr uint32 kTag_WeightedSamples   = 2;
		static constexpr uint32 kTag_Weights		   = 3;
		static constexpr uint32 kTag_ColorAverage	   = 4;
		static constexpr uint32 kTag_ColorSamples	   = 5;

		static constexpr size_t kMaxSamples = 1024;

	public:

		struct weighted_sample
			{
				
			real32 fFrac  = 0.0f;	
			real32 fValue = 0.0f;
				
			bool operator== (const weighted_sample &src) const
				{
				return (fFrac  == src.fFrac &&
						fValue == src.fValue);
				}
				
			};

		struct color_sample
			{
				
			real32 fFrac = 0.0f;
				
			std::vector<real32> fValues;

			bool operator== (const color_sample &src) const
				{
				return (fFrac	== src.fFrac &&
						fValues == src.fValues);
				}
				
			};

	public:

		

		std::vector<real32> fWeightedAverage;

		std::vector<weighted_sample> fWeightedSamples;

		std::vector<real32> fWeights;

		std::vector<real32> fColorAverage;
		
		std::vector<color_sample> fColorSamples;

	public:

		bool IsValidForPlaneCount (uint32 planeCount) const;

		uint32 TagCount () const;

		tiff_tag * MakeTag (dng_memory_allocator &allocator) const;

		bool operator== (const dng_image_stats &src) const;

		void Parse (dng_stream &stream,
					uint32 tagByteCount);

		#if qDNGValidate
		void Dump () const;
		#endif
		
	};

DNG_ALWAYS_INLINE real32 EncodeOverrange (real32 x)
	{

	x = Max_real32 (x, 0.0f);

	return x * (256.0f + x) / (256.0f * (1.0f + x));
	
	}

DNG_ALWAYS_INLINE real32 DecodeOverrange (real32 x)
	{

	x = Max_real32 (x, 0.0f);
	
	return 16.0f * ((8.0f * x) - 8.0f + sqrtf (64.0f * x * x - 127.0f * x + 64.0f));
	
	}

#endif	
	

