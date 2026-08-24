

#ifndef __dng_assertions__
#define __dng_assertions__

#include "dng_exceptions.h"
#include "dng_flags.h"

#if qWinOS

void dng_outputdebugstring (const char *s,
							const char *nl = NULL);

#endif

#if defined(__EMSCRIPTEN__)

void dng_emscripten_log (int emLogType,
						 const char *s);

#endif

void dng_show_message (const char *s);

void dng_show_message_f (const char *fmt, ...);

#ifndef DNG_ASSERT

#if qDNGDebug

#define DNG_ASSERT(x,y) { if (!(x)) dng_show_message (y); }

#else

#define DNG_ASSERT(x,y)

#endif
#endif

#ifndef DNG_REQUIRE

#if qDNGDebug

#define DNG_REQUIRE(condition,msg)				\
	do											\
		{										\
												\
		if (!(condition))						\
			{									\
												\
			DNG_ASSERT(condition, msg);			\
												\
			ThrowProgramError (msg);			\
												\
			}									\
												\
		}										\
	while (0)

#else

#define DNG_REQUIRE(condition,msg)				\
	do											\
		{										\
												\
		if (!(condition))						\
			{									\
												\
			ThrowProgramError (msg);			\
												\
			}									\
												\
		}										\
	while (0)

#endif
#endif

#ifndef DNG_REPORT

#define DNG_REPORT(x) DNG_ASSERT (false, x)

#endif

#endif

