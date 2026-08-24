

#include "dng_utils.h"

#include "dng_area_task.h"
#include "dng_assertions.h"
#include "dng_bottlenecks.h"
#include "dng_flags.h"
#include "dng_globals.h"
#include "dng_host.h"
#include "dng_image.h"
#include "dng_image_writer.h"
#include "dng_memory_stream.h"
#include "dng_mutex.h"
#include "dng_point.h"
#include "dng_rect.h"
#include "dng_simd_type.h"
#include "dng_tag_codes.h"
#include "dng_tag_values.h"
#include "dng_tile_iterator.h"

#if qMacOS
#include <CoreServices/CoreServices.h>
#endif

#if qiPhone || qMacOS

#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#if qiPhone || qLinux
#include <signal.h> 
#endif

#if qWinOS
#include <windows.h>
#else
#include <sys/time.h>
#include <stdarg.h> 
#endif

#if defined(__EMSCRIPTEN__)
#include "emscripten.h"
#endif

#include <atomic>

#if qDNGDebug

#if qMacOS
	#if qARM
		#define DNG_DEBUG_BREAK do { } while (0)
	#else
		#define DNG_DEBUG_BREAK __asm__ volatile ("int3")
	#endif
#elif qiPhone
	#if qiPhoneSimulator
		
		#if qARM
			#define DNG_DEBUG_BREAK do { } while (0)
		#else
			#define DNG_DEBUG_BREAK __asm__ volatile ("int3")
		#endif
	#else
		
		#define DNG_DEBUG_BREAK raise(SIGTRAP)
	#endif
#elif qWinOS
	
	#define DNG_DEBUG_BREAK DebugBreak()  
#elif qAndroid
	#define DNG_DEBUG_BREAK raise(SIGTRAP)
#elif qLinux
	#define DNG_DEBUG_BREAK raise(SIGTRAP)
#else
	#define DNG_DEBUG_BREAK
#endif

#endif	

#if qWinOS

void dng_outputdebugstring (const char *s,
							const char *nl)
	{

	static const bool sDebuggerPresent (IsDebuggerPresent () != 0);

	if (sDebuggerPresent)
		{
		OutputDebugStringA (s);
		if (nl && nl [0])
			{
			OutputDebugStringA (nl);
			}
		}

	}

#endif

#if defined(__EMSCRIPTEN__)

void dng_emscripten_log (int emLogType,
						 const char *s)
	{
	
	#if qDNGDebug
	emLogType |= EM_LOG_CONSOLE;
	#endif

	emscripten_log (emLogType,"%s", s);
	
	}

#endif

void dng_show_message (const char *s)
	{
	
	const char* nl = "\n";
	if (s[0] && (s[strlen(s)-1] == '\n'))
		nl = "";
		
	#if qDNGPrintMessages
	
	
	if (gPrintAsserts)
		fprintf (stderr, "%s%s", s, nl);
		
	#elif qiPhone || qAndroid || qLinux || qWeb
	
	if (gPrintAsserts)
		fprintf (stderr, "%s%s", s, nl);

	#if qDNGDebug
	
	
	
	if (gBreakOnAsserts)
		DNG_DEBUG_BREAK;
	
	#endif	
	
	#elif qMacOS
	
	if (gBreakOnAsserts)
		{
		
		char ss [256];
	
		uint32 len = (uint32) strlen (s);
		if (len > 255)
			len = 255;
		strncpy (&(ss [1]), s, len );
		ss [0] = (unsigned char) len;
	
		DebugStr ((unsigned char *) ss);
		}
	 else if (gPrintAsserts)
		{
		
		
		
		
		fprintf (stdout, "%s%s", s, nl);
		}
	
	#elif qWinOS
	
	
	
	
	if (gBreakOnAsserts)
		{
		MessageBoxA (NULL, (LPSTR) s, NULL, MB_OK);
		}
	else if (gPrintAsserts)
		{
		fprintf (stderr, "%s%s", s, nl);

		
		dng_outputdebugstring (s, nl);
		}

	#endif

	}

#if (qMacOS || qiPhone)
__attribute__((__format__ (__printf__, 1, 2)))
#endif

void dng_show_message_f (const char *fmt, ... )
	{
	
	char buffer [2048];
	
	va_list ap;
	va_start (ap, fmt);

	vsnprintf (buffer, sizeof (buffer), fmt, ap);
	
	va_end (ap);
	
	dng_show_message (buffer);
	
	}

uint32 ComputeBufferSize (uint32 pixelType, 
						  const dng_point &tileSize,
						  uint32 numPlanes, 
						  PaddingType paddingType)
	{

	

	if (tileSize.h < 0 || tileSize.v < 0)
		{
		ThrowMemoryFull ("Negative tile size");
		}

	const uint32 tileSizeH = static_cast<uint32> (tileSize.h);
	const uint32 tileSizeV = static_cast<uint32> (tileSize.v);
	
	const uint32 pixelSize = TagTypeSize (pixelType);
	
	

	uint32 paddedWidth = tileSizeH;

	if (paddingType == padSIMDBytes)
		{

		if (!RoundUpForPixelSize (paddedWidth, 
								  pixelSize, 
								  &paddedWidth))
			{
			ThrowOverflow ("Arithmetic overflow computing buffer size");
			}

		}
	
	

	uint32 bufferSize;

	if (!SafeUint32Mult (paddedWidth, tileSizeV, &bufferSize) ||
		!SafeUint32Mult (bufferSize,  pixelSize, &bufferSize) ||
		!SafeUint32Mult (bufferSize,  numPlanes, &bufferSize))
		{
		ThrowOverflow ("Arithmetic overflow computing buffer size");
		}
	
	return bufferSize;

	}

real64 TickTimeInSeconds ()
	{
	
	#if qWinOS
	
	
	
	
	
	
	
	
		
	
	
	
	
		
	static real64 freqMultiplier = 0.0;

	if (freqMultiplier == 0.0)
		{

		LARGE_INTEGER freq;

		QueryPerformanceFrequency (&freq);

		freqMultiplier = 1.0 / (real64) freq.QuadPart;

		}

	LARGE_INTEGER cycles;

	QueryPerformanceCounter (&cycles);

	return (real64) cycles.QuadPart * freqMultiplier;

	#elif qiPhone || qMacOS
	
	
	static real64 freqMultiplier = 0.0;
	if (freqMultiplier == 0.0)
		{
			
		mach_timebase_info_data_t freq; 
		mach_timebase_info(&freq);
		
		
		
		freqMultiplier = ((real64)freq.numer / (real64)freq.denom) * 1.0e-9;
		
		}
	
	return mach_absolute_time() * freqMultiplier;
		
	#elif qAndroid || qLinux || qWeb

	
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return now.tv_sec + (real64)now.tv_nsec * 1.0e-9;

	#else

	

	struct timeval tv;
	
	gettimeofday (&tv, NULL);

	return tv.tv_sec + (real64)tv.tv_usec * 1.0e-6;
	
	#endif

	}

real64 TickCountInSeconds ()
	{
		
	return TickTimeInSeconds ();
	
	}

static std::atomic_int sTimerLevel (0);

void DNGIncrementTimerLevel ()
	{
	
	
	
	
	if (!gImagecore)
		{
		
		sTimerLevel++;
		
		}
		
	}

int32 DNGDecrementTimerLevel ()
	{

	if (gImagecore)
		{
		
		return 0;
		
		}
		
	else
		{
		
		return (int32) (--sTimerLevel);
		
		}
		
	}

dng_timer::dng_timer (const char *message)

	:	fMessage   (message				)
	,	fStartTime (TickTimeInSeconds ())
	
	{

	DNGIncrementTimerLevel ();
	
	}

dng_timer::~dng_timer ()
	{
	
	uint32 level = Pin_int32 (0, DNGDecrementTimerLevel (), 10);
	
	if (!gDNGShowTimers)
		return;

	real64 totalTime = TickTimeInSeconds () - fStartTime;
	
	#if defined(qCRLogging) && qCRLogging && defined(cr_logi)
		
	if (gImagecore)
		{
		
		
		cr_logi("timer", "%s: %0.3f sec\n", fMessage, totalTime);
		return;
		}
		
	#endif
		
	fprintf (stderr, "%*s%s: %0.3f sec\n", level*2, "", fMessage, totalTime);
	
	}

real64 MaxSquaredDistancePointToRect (const dng_point_real64 &point,
									  const dng_rect_real64 &rect)
	{
	
	real64 distSqr = DistanceSquared (point, 
									  rect.TL ());

	distSqr = Max_real64 (distSqr,
						  DistanceSquared (point, 
										   rect.BL ()));

	distSqr = Max_real64 (distSqr,
						  DistanceSquared (point, 
										   rect.BR ()));

	distSqr = Max_real64 (distSqr,
						  DistanceSquared (point, 
										   rect.TR ()));

	return distSqr;
	
	}

real64 MaxDistancePointToRect (const dng_point_real64 &point,
							   const dng_rect_real64 &rect)
	{

	return sqrt (MaxSquaredDistancePointToRect (point, 
												rect));

	}

dng_dither::dng_dither ()

	:	fNoiseBuffer ()

	{
	
	const uint32 kSeed = 1;

	fNoiseBuffer.Allocate (kRNGSize2D * sizeof (uint16));

	uint16 *buffer = fNoiseBuffer.Buffer_uint16 ();
		
	uint32 seed = kSeed;

	for (uint32 i = 0; i < kRNGSize2D; i++)
		{
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		uint16 value;
		
		do
			{
			
			seed = DNG_Random (seed);
			
			value = (uint16) seed;
			
			}
		while (value < 255);

		buffer [i] = value;

		}
	
	}

const dng_dither & dng_dither::Get ()
	{
	
	static dng_dither dither;
	
	return dither;
	
	}

void HistogramArea (dng_host & ,
					const dng_image &image,
					const dng_rect &area,
					uint32 *hist,
					uint32 maxValue,
					uint32 plane)
	{
	
	DNG_ASSERT (image.PixelType () == ttShort, "Unsupported pixel type");
	
	DoZeroBytes (hist, (maxValue + 1) * (uint32) sizeof (uint32));
	
	dng_rect tile;
	
	dng_tile_iterator iter (image, area);
	
	while (iter.GetOneTile (tile))
		{
		
		dng_const_tile_buffer buffer (image, tile);
		
		const void *sPtr = buffer.ConstPixel (tile.t,
											  tile.l,
											  plane);
		
		uint32 count0 = 1;
		uint32 count1 = tile.H ();
		uint32 count2 = tile.W ();
		
		int32 step0 = 0;
		int32 step1 = buffer.fRowStep;
		int32 step2 = buffer.fColStep;
		
		OptimizeOrder (sPtr,
					   buffer.fPixelSize,
					   count0,
					   count1,
					   count2,
					   step0,
					   step1,
					   step2);
					   
		DNG_ASSERT (count0 == 1, "OptimizeOrder logic error");
		
		const uint16 *s1 = (const uint16 *) sPtr;
				
		for (uint32 row = 0; row < count1; row++)
			{
			
			if (maxValue == 0x0FFFF && step2 == 1)
				{
				
				for (uint32 col = 0; col < count2; col++)
					{
					
					uint32 x = s1 [col];
					
					hist [x] ++;
					
					}
					
				}
				
			else
				{
			
				const uint16 *s2 = s1;
			
				for (uint32 col = 0; col < count2; col++)
					{
					
					uint32 x = s2 [0];
					
					if (x <= maxValue)
						{
					
						hist [x] ++;
						
						}
					
					s2 += step2;
					
					}
					
				}
				
			s1 += step1;
			
			}
		
		}
		
	}
		

template <SIMDType simd>
class dng_limit_float_depth_task: public dng_area_task
	{
	
	private:
	
		const dng_image &fSrcImage;
		
		dng_image &fDstImage;
		
		uint32 fBitDepth;
		
		real32 fScale;
			
	public:
	
		dng_limit_float_depth_task (const dng_image &srcImage,
									dng_image &dstImage,
									uint32 bitDepth,
									real32 scale);
							 
		virtual dng_rect RepeatingTile1 () const
			{
			return fSrcImage.RepeatingTile ();
			}
			
		virtual dng_rect RepeatingTile2 () const
			{
			return fDstImage.RepeatingTile ();
			}
							 
		virtual void Process (uint32 threadIndex,
							  const dng_rect &tile,
							  dng_abort_sniffer *sniffer);
								  
	};

template <SIMDType simd>
dng_limit_float_depth_task<simd>::dng_limit_float_depth_task
	(const dng_image &srcImage,
	 dng_image &dstImage,
	 uint32 bitDepth,
	 real32 scale)

	:	dng_area_task ("dng_limit_float_depth_task")
										
	,	fSrcImage (srcImage)
	,	fDstImage (dstImage)
	,	fBitDepth (bitDepth)
	,	fScale	  (scale)
	
	{
	
	}

template <SIMDType simd>

#ifdef __INTEL_LLVM_COMPILER
__attribute__((SET_CPU_FEATURE(simd)))
#endif 

void dng_limit_float_depth_task<simd>::Process (uint32 ,
												const dng_rect &tile,
												dng_abort_sniffer * )
	{

	INTEL_COMPILER_NEEDED_NOTE

#ifdef __INTEL_COMPILER
		SET_CPU_FEATURE(simd);
#endif 

	dng_const_tile_buffer srcBuffer (fSrcImage, tile);
	dng_dirty_tile_buffer dstBuffer (fDstImage, tile);
	
	uint32 count0 = tile.H ();
	uint32 count1 = tile.W ();
	uint32 count2 = fDstImage.Planes ();
	
	int32 sStep0 = srcBuffer.fRowStep;
	int32 sStep1 = srcBuffer.fColStep;
	int32 sStep2 = srcBuffer.fPlaneStep;
	
	int32 dStep0 = dstBuffer.fRowStep;
	int32 dStep1 = dstBuffer.fColStep;
	int32 dStep2 = dstBuffer.fPlaneStep;

	const void *sPtr = srcBuffer.ConstPixel (tile.t,
											 tile.l,
											 0);
											 
		  void *dPtr = dstBuffer.DirtyPixel (tile.t,
											 tile.l,
											 0);

	OptimizeOrder (sPtr,
				   dPtr,
				   srcBuffer.fPixelSize,
				   dstBuffer.fPixelSize,
				   count0,
				   count1,
				   count2,
				   sStep0,
				   sStep1,
				   sStep2,
				   dStep0,
				   dStep1,
				   dStep2);
				   
	const real32 *sPtr0 = (const real32 *) sPtr;
		  real32 *dPtr0 = (		 real32 *) dPtr;
		  
	real32 scale = fScale;
		  
	bool limit16 = (fBitDepth == 16);
	bool limit24 = (fBitDepth == 24);
				   
	for (uint32 index0 = 0; index0 < count0; index0++)
		{
		
		const real32 *sPtr1 = sPtr0;
			  real32 *dPtr1 = dPtr0;
			  
		for (uint32 index1 = 0; index1 < count1; index1++)
			{
			
			
			
			
			if (scale == 1.0f && sStep2 == 1 && dStep2 == 1)
				{
				
				if (dPtr1 != sPtr1)			
					{
				
					memcpy (dPtr1, sPtr1, count2 * (uint32) sizeof (real32));
					
					}
				
				}
				
			else
				{
			
				const real32 *sPtr2 = sPtr1;
					  real32 *dPtr2 = dPtr1;
				INTEL_PRAGMA_SIMD_ASSERT_VECLEN_FLOAT(simd)
				for (uint32 index2 = 0; index2 < count2; index2++)
					{
					
					real32 x = sPtr2 [0];
					
					x *= scale;
					
					dPtr2 [0] = x;
					
					sPtr2 += sStep2;
					dPtr2 += dStep2;
					
					}
					
				}
				
			
				
			if (limit16)
				{

				
				

				uint32 *dPtr2 = (uint32 *) dPtr1;

				INTEL_PRAGMA_SIMD_ASSERT_VECLEN_INT32(simd)

				for (uint32 index2 = 0; index2 < count2; index2++)
					{
					
					uint32 x = dPtr2 [0];
					
					uint16 y = DNG_FloatToHalf (x);
					
					x = DNG_HalfToFloat (y);
											
					dPtr2 [0] = x;
					
					dPtr2 += dStep2;
					
					}
					
				}
				
			else if (limit24)
				{
			
				uint32 *dPtr2 = (uint32 *) dPtr1;
					  
				for (uint32 index2 = 0; index2 < count2; index2++)
					{
					
					uint32 x = dPtr2 [0];
											
					uint8 temp [3];
					
					DNG_FloatToFP24 (x, temp);
					
					x = DNG_FP24ToFloat (temp);
					
					dPtr2 [0] = x;
					
					dPtr2 += dStep2;
					
					}
					
				}
			  
			sPtr1 += sStep1;
			dPtr1 += dStep1;
			
			}
				   
		sPtr0 += sStep0;
		dPtr0 += dStep0;
		
		}
					
	}

template <SIMDType simd>
void LimitFloatBitDepth (dng_host &host,
						 const dng_image &srcImage,
						 dng_image &dstImage,
						 uint32 bitDepth,
						 real32 scale)
	{
	
	DNG_ASSERT (srcImage.PixelType () == ttFloat, "Floating point image expected");
	DNG_ASSERT (dstImage.PixelType () == ttFloat, "Floating point image expected");
	
	dng_limit_float_depth_task<simd> task (srcImage,
										   dstImage,
										   bitDepth,
										   scale);
									 
	host.PerformAreaTask (task, dstImage.Bounds ());
	
	}

template
void LimitFloatBitDepth<Scalar> (dng_host &host,
								 const dng_image &srcImage,
								 dng_image &dstImage,
								 uint32 bitDepth,
								 real32 scale);

#if qDNGIntelCompiler

template
void LimitFloatBitDepth<AVX2> (dng_host &host,
							   const dng_image &srcImage,
							   dng_image &dstImage,
							   uint32 bitDepth,
							   real32 scale);

#endif	

void LimitFloatBitDepth (dng_host &host,
						 const dng_image &srcImage,
						 dng_image &dstImage,
						 uint32 bitDepth,
						 real32 scale)
	{
	
	
	
	
	
	
	
	#if (qDNGIntelCompiler && qDNGExperimental && 0)

	if (gDNGMaxSIMD >= AVX2)
		{

		LimitFloatBitDepth<AVX2> (host,
								  srcImage,
								  dstImage,
								  bitDepth,
								  scale);

		}

	else

	#endif	

		{

		LimitFloatBitDepth<Scalar> (host,
									srcImage,
									dstImage,
									bitDepth,
									scale);

		}

	}

uint32 MinBackwardVersionForCompression (uint32 compression)
	{
	
	if (compression == ccLossyJPEG)
		return dngVersion_1_4_0_0;

	if (compression == ccJXL)
		return dngVersion_1_7_0_0;

	return dngVersion_1_1_0_0;
	
	}

tiff_tag * dng_image_sequence_info::MakeTag (dng_memory_allocator &allocator) const
	{

	dng_memory_stream stream (allocator);

	TempBigEndian tempEndian (stream);

	

	if (fSequenceID.NotEmpty ())
		stream.Put (fSequenceID.Get (),
					fSequenceID.Length ());

	stream.PutZeros (1);

	

	if (fSequenceType.NotEmpty ())
		stream.Put (fSequenceType.Get (),
					fSequenceType.Length ());

	stream.PutZeros (1);

	

	if (fFrameInfo.NotEmpty ())
		stream.Put (fFrameInfo.Get (),
					fFrameInfo.Length ());

	stream.PutZeros (1);

	

	stream.Put_uint32 (fIndex);
	stream.Put_uint32 (fCount);
	stream.Put_uint8 (fIsFinal);

	stream.SetReadPosition (0);

	const_dng_memory_block_sptr block (stream.AsMemoryBlock (allocator));

	AutoPtr<tag_owned_data_ptr> tag
		(new tag_owned_data_ptr (tcImageSequenceInfo,
								 ttUndefined,
								 block->LogicalSize (),
								 block));

	return tag.Release ();
	
	}

bool dng_image_stats::IsValidForPlaneCount (uint32 planeCount) const
	{
	
	DNG_REQUIRE (planeCount > 0, "Invalid plane count");

	if (fWeightedAverage.size () >= 2)
		return false;

	if (fWeights.size () != 0 &&
		fWeights.size () != size_t (planeCount))
		return false;
	
	if (fColorAverage.size () != 0 &&
		fColorAverage.size () != size_t (planeCount))
		return false;

	
		
	if (!fWeightedSamples.empty ())
		{

		

		if (fWeightedSamples.size () > kMaxSamples)
			return false;
		
		

		real32 fPrev = fWeightedSamples.front ().fFrac;

		for (size_t i = 1; i < fWeightedSamples.size (); i++)
			{

			real32 fCurrent = fWeightedSamples [i].fFrac;
			
			if (fCurrent <= fPrev)
				return false;

			fPrev = fCurrent;

			}
		
		}

	

	if (!fColorSamples.empty ())
		{

		

		if (fColorSamples.size () > kMaxSamples)
			return false;
		
		

		real32 fPrev = fColorSamples.front ().fFrac;

		for (size_t i = 1; i < fColorSamples.size (); i++)
			{

			const auto &sample = fColorSamples [i];
			
			if (sample.fFrac <= fPrev)
				return false;

			fPrev = sample.fFrac;

			

			if (sample.fValues.size () != size_t (planeCount))
				return false;

			}
		
		}

	

	return true;
	
	}

uint32 dng_image_stats::TagCount () const
	{

	uint32 count = 0;
	
	count += (!fWeightedAverage.empty () ? 1 : 0);
	count += (!fWeightedSamples.empty () ? 1 : 0);
	count += (!fWeights		   .empty () ? 1 : 0);
	count += (!fColorAverage   .empty () ? 1 : 0);
	count += (!fColorSamples   .empty () ? 1 : 0);

	return count;
	
	}

static void Put (dng_stream &stream,
				 uint32 tagCode,
				 const std::vector<real32> &values)
	{

	if (!values.empty ())
		{

		

		DNG_REQUIRE (values.size () <= 16,
					 "values vector too large");

		

		stream.Put_uint32 (tagCode);

		
		
		stream.Put_uint32 (uint32 (4 * values.size ()));

		

		for (const auto x : values)
			stream.Put_real32 (x);
		
		}
	
	}

static void Put (dng_stream &stream,
				 uint32 tagCode,
				 const std::vector<dng_image_stats::weighted_sample> &samples)
	{

	if (!samples.empty ())
		{

		

		DNG_REQUIRE (samples.size () <= dng_image_stats::kMaxSamples,
					 "samples vector too large");

		

		stream.Put_uint32 (tagCode);

		
		
		stream.Put_uint32 (uint32 (4 + 8 * samples.size ()));

		

		

		stream.Put_uint32 (uint32 (samples.size ()));

		

		for (const auto &sample : samples)
			{
			stream.Put_real32 (sample.fFrac);
			stream.Put_real32 (sample.fValue);
			}
		
		}
	
	}

static void Put (dng_stream &stream,
				 uint32 tagCode,
				 const std::vector<dng_image_stats::color_sample> &samples)
	{

	if (!samples.empty ())
		{

		

		DNG_REQUIRE (samples.size () <= dng_image_stats::kMaxSamples,
					 "samples vector too large");

		

		stream.Put_uint32 (tagCode);

		

		uint32 bytes = 4;

		for (const auto &sample : samples)
			bytes += (4 + 4 * uint32 (sample.fValues.size ()));
		
		stream.Put_uint32 (bytes);

		

		

		stream.Put_uint32 (uint32 (samples.size ()));

		

		for (const auto &sample : samples)
			{
			
			stream.Put_real32 (sample.fFrac);
			
			for (const auto &x : sample.fValues)
				stream.Put_real32 (x);
			
			}
		
		}
	
	}

tiff_tag * dng_image_stats::MakeTag (dng_memory_allocator &allocator) const
	{
	
	dng_memory_stream stream (allocator);

	

	TempBigEndian tempEndian (stream);

	

	uint32 count = TagCount ();

	stream.Put_uint32 (count);

	

	Put (stream, kTag_WeightedAverage , fWeightedAverage);
	Put (stream, kTag_WeightedSamples , fWeightedSamples);
	Put (stream, kTag_Weights		  , fWeights);
	Put (stream, kTag_ColorAverage	  , fColorAverage);
	Put (stream, kTag_ColorSamples	  , fColorSamples);
	
	

	stream.SetReadPosition (0);

	const_dng_memory_block_sptr block (stream.AsMemoryBlock (allocator));

	AutoPtr<tag_owned_data_ptr> tag
		(new tag_owned_data_ptr (tcImageStats,
								 ttUndefined,
								 block->LogicalSize (),
								 block));

	return tag.Release ();
	
	}

bool dng_image_stats::operator== (const dng_image_stats &src) const
	{
	
	return (fWeightedAverage   == src.fWeightedAverage &&
			fWeightedSamples   == src.fWeightedSamples &&
			fWeights		   == src.fWeights		   &&
			fColorAverage	   == src.fColorAverage	   &&
			fColorSamples	   == src.fColorSamples);
	
	}

void dng_image_stats::Parse (dng_stream &stream,
							 uint32 tagByteCount)
	{
	
	

	TempBigEndian tempEndian (stream);

	uint64 tagEnd = SafeUint64Add (stream.Position (),
								   (uint64) tagByteCount);

	const auto requireAvailableBytes =
		[&stream,
		 tagEnd] (uint64 byteCount)
		{

		if (stream.Position () > tagEnd ||
			byteCount > tagEnd - stream.Position ())
			{
			ThrowBadFormat ("ImageStats tag is truncated");
			}

		};

	

	requireAvailableBytes (4);

	uint32 count = stream.Get_uint32 ();

	
	

	if (count > 5)
		ThrowBadFormat ("too many tags in dng_image_stats");

	

	for (uint32 i = 0; i < count; i++)
		{
		
		requireAvailableBytes (8);

		

		uint32 childTagCode = stream.Get_uint32 ();

		

		uint32 length = stream.Get_uint32 ();

		

		if (length == 0)
			ThrowBadFormat ("child tag byte length must be > 0");
		
		

		if ((length & 3) != 0)
			ThrowBadFormat ("child tag byte length expected to be multiple of 4");

		

		constexpr uint32 kMaxBytes = 4 + kMaxSamples * 4 * (kMaxColorPlanes + 1);

		if (length > kMaxBytes)
			ThrowBadFormat ("child tag byte length too large");

		requireAvailableBytes (length);

		uint64 childEnd = SafeUint64Add (stream.Position (),
										 (uint64) length);

		

		std::vector<real32> *data = nullptr;

		switch (childTagCode)
			{
			
			case kTag_WeightedAverage:
				{
				data = &fWeightedAverage;
				break;
				}
				
			case kTag_Weights:
				{
				data = &fWeights;
				break;
				}
			
			case kTag_ColorAverage:
				{
				data = &fColorAverage;
				break;
				}
				
			default:
				break;
			
			}

		if (data)
			{

			const uint32 numFloats = (length >> 2);

			data->resize (numFloats);

			for (uint32 c = 0; c < numFloats; c++)
				(*data) [c] = stream.Get_real32 ();

			}

		else if (childTagCode == kTag_WeightedSamples)
			{

			const uint32 sampleCount = stream.Get_uint32 ();

			

			if (sampleCount == 0)
				ThrowBadFormat ("too few samples for weighted samples");

			if (sampleCount > kMaxSamples)
				ThrowBadFormat ("too many samples for weighted samples");

			

			if (4 + (8 * sampleCount) != length)
				ThrowBadFormat ("mismatch byte length for weighted samples");

			

			fWeightedSamples.resize (sampleCount);

			for (auto &sample : fWeightedSamples)
				{

				sample.fFrac  = stream.Get_real32 ();
				sample.fValue = stream.Get_real32 ();
				
				}
			
			}

		else if (childTagCode == kTag_ColorSamples)
			{
			
			const uint32 sampleCount = stream.Get_uint32 ();

			

			if (sampleCount == 0)
				ThrowBadFormat ("too few samples for color samples");

			if (sampleCount > kMaxSamples)
				ThrowBadFormat ("too many samples for color samples");

			

			const uint32 planes = ((length - 4) / sampleCount / 4) - 1;

			if (planes == 0)
				ThrowBadFormat ("unexpected 0 plane count for color samples");

			if (planes > kMaxColorPlanes)
				ThrowBadFormat ("too large plane count for color samples");

			if (4 + (sampleCount * 4 * (planes + 1)) != length)
				ThrowBadFormat ("mismatched plane count for color samples");

			

			fColorSamples.resize (sampleCount);

			for (auto &sample : fColorSamples)
				{

				sample.fFrac = stream.Get_real32 ();

				sample.fValues.resize (planes);

				for (auto &value : sample.fValues)
					value = stream.Get_real32 ();

				}
			
			}

		else
			{
			
			ThrowBadFormat ("unsupported child tag code");
			
			}

		if (stream.Position () != childEnd)
			{
			ThrowBadFormat ("ImageStats child tag length mismatch");
			}

		}

	if (stream.Position () != tagEnd)
		{
		ThrowBadFormat ("ImageStats tag length mismatch");
		}
	
	}

#if qDNGValidate

static void DumpTag (const char *name,
					 const std::vector<real32> &values)
	{

	if (!values.empty ())
		{
	
		printf ("  %s: %.4f",
				name,
				values.front ());

		for (size_t i = 1; i < values.size (); i++)
			printf (", %.4f", values [i]);

		printf ("\n");

		}
	
	}

void dng_image_stats::Dump () const
	{

	printf ("ImageStats: %u child tag(s)\n", TagCount ());

	DumpTag ("weights", fWeights);
	
	DumpTag ("weighted average", fWeightedAverage);
	
	DumpTag ("color average", fColorAverage);

	if (!fWeightedSamples.empty ())
		{

		printf ("  weighted samples:\n");
		
		for (const auto &s : fWeightedSamples)
			{
			
			printf ("    frac: %.6f, value: %.6lf\n",
					s.fFrac,
					s.fValue);
			
			}
		
		}
	
	if (!fColorSamples.empty ())
		{

		printf ("  color samples:\n");
		
		for (const auto &s : fColorSamples)
			{
			
			printf ("    frac: %.6f, values: ",
					s.fFrac);

			if (s.fValues.empty ())
				continue;

			printf ("%.6f", s.fValues.front ());

			for (size_t i = 1; i < s.fValues.size (); i++)
				printf (", %.6f", s.fValues [i]);

			printf ("\n");
			
			}
		
		}
	
	}

#endif	

