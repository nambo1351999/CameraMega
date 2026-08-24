

#ifndef __dng_flags__
#define __dng_flags__

#define kDNGSDK_MajorVersion 1
#define kDNGSDK_MinorVersion 7
#define kDNGSDK_DotVersion   1

#define kDNGSDK_VersionString "1.7.1"

#define kDNGSDK_BuildVersion 2611
#define kDNGSDK_BuildString  "2611"

#define kDNGSDK_GetInfoVersion	"1.7.1 (" kDNGSDK_BuildString ")"

#if !(defined(qMacOS) || defined(qWinOS) || defined(qAndroid) || defined(qiPhone) || defined(qLinux) || defined(qWeb))
#include "RawEnvironment.h"
#endif

#if !(defined(qMacOS) || defined(qWinOS) || defined(qAndroid) || defined(qiPhone) || defined(qLinux) || defined(qWeb))
#error Unable to figure out platform
#endif

#ifndef qMacOS
#define qMacOS 0
#endif

#ifndef qiPhone
#define qiPhone 0
#endif

#ifndef qiPhoneSimulator
#define qiPhoneSimulator 0
#endif

#ifndef qAndroid
#define qAndroid 0
#endif

#ifndef qWinOS
#define qWinOS 0
#endif

#ifndef qWinRT
#define qWinRT 0
#endif

#ifndef qLinux
#define qLinux 0
#endif

#ifndef qWeb
#define qWeb 0
#endif

#ifndef qIsFauxPlatformBuild
#define qIsFauxPlatformBuild 0
#endif

#ifndef qIsFauxWebPlatformBuild
#define qIsFauxWebPlatformBuild 0
#endif

#ifndef qIsFauxLinuxPlatformBuild
#define qIsFauxLinuxPlatformBuild 0
#endif

#ifndef qMacOSNonFaux
#define qMacOSNonFaux (qMacOS && !qIsFauxPlatformBuild)
#endif

#if qiPhoneSimulator
#if !qiPhone
#error "qiPhoneSimulator set and not qiPhone"
#endif
#endif

#if qWinRT
#if !qWinOS
#error "qWinRT set but not qWinOS"
#endif
#endif

#if defined(__arm__) || defined(__arm64__) || defined(_M_ARM) || defined(_M_ARM64) || defined(__aarch64__)
#define qARM 1
#endif

#if defined(__arm64__) || defined(_M_ARM64) || defined(__aarch64__)
#define qARM64 1
#endif

#ifndef qARM 
#define qARM 0
#endif

#ifndef qARM64 
#define qARM64 0
#endif

#if defined(__x86_64__) || defined(_M_AMD64) || defined(_M_X64)
#define qX86_64 1
#endif

#ifndef qX86_64
#define qX86_64 0
#endif

#if defined(_WIN32) && !defined(WIN32)
#define WIN32 1
#endif

#if defined(_WIN64) && !defined(WIN64)
#define WIN64 1
#endif

#ifndef qDNGDebug

#if defined(Debug)
#define qDNGDebug Debug

#elif defined(_DEBUG)
#define qDNGDebug _DEBUG

#else
#define qDNGDebug 0

#endif
#endif

#ifndef qDNGIntelCompiler
#if defined(__INTEL_COMPILER)
#define qDNGIntelCompiler (__INTEL_COMPILER >= 1700)
#elif defined(__INTEL_LLVM_COMPILER)
#define qDNGIntelCompiler __INTEL_LLVM_COMPILER
#else
#define qDNGIntelCompiler 0
#endif
#endif

#ifndef qDNGBigEndian

#if defined(qDNGLittleEndian)
#define qDNGBigEndian (!qDNGLittleEndian)

#elif defined(__POWERPC__)
#define qDNGBigEndian 1

#elif defined(__INTEL__)
#define qDNGBigEndian 0

#elif defined(_M_IX86)
#define qDNGBigEndian 0

#elif defined(_M_X64) || defined(__amd64__)
#define qDNGBigEndian 0

#elif defined(__LITTLE_ENDIAN__)
#define qDNGBigEndian 0

#elif defined(__BIG_ENDIAN__)
#define qDNGBigEndian 1

#elif defined(_ARM_) || defined(__ARM_NEON) || defined(__mips__)
#define qDNGBigEndian 0

#elif defined(_M_ARM64)

#define qDNGBigEndian 0

#else

#ifndef qXCodeRez
#error Unable to figure out byte order.
#endif

#endif
#endif

#ifndef qXCodeRez

#ifndef qDNGLittleEndian
#define qDNGLittleEndian (!qDNGBigEndian)
#endif

#endif

#ifndef qDNG64Bit

#if qMacOS

#ifdef __LP64__
#if	   __LP64__
#define qDNG64Bit 1
#endif
#endif

#elif qWinOS

#ifdef WIN64
#if	   WIN64
#define qDNG64Bit 1
#endif
#endif

#elif qLinux

#ifdef __LP64__
#if	   __LP64__
#define qDNG64Bit 1
#endif
#endif

#elif qAndroid

#ifdef __LP64__
#if	   __LP64__
#define qDNG64Bit 1
#endif
#endif

#endif

#ifndef qDNG64Bit
#ifdef qXCodeRez
#define qDNG64Bit qXCodeRez
#else
#define qDNG64Bit 0
#endif
#endif

#endif

#ifdef __cplusplus
#if defined(__clang__) && !defined(__INTEL_LLVM_COMPILER)
#define DNG_RESTRICT __restrict
#elif defined(qWinOS) && !defined(__INTEL_LLVM_COMPILER)
#define DNG_RESTRICT __restrict
#else
#define DNG_RESTRICT
#endif
#endif	

#ifdef __cplusplus
#if defined(__clang__) && !defined(__INTEL_LLVM_COMPILER)
#define DNG_ALWAYS_INLINE __attribute__((__always_inline__)) inline
#else
#define DNG_ALWAYS_INLINE inline
#endif
#endif	

#if defined(__cplusplus)
#if __cplusplus >= 201703L
#define DNG_FALLTHROUGH [[fallthrough]];
#else
#define DNG_FALLTHROUGH
#endif
#elif defined(__STDC_VERSION__)
#if __STDC_VERSION__ >= 202311L
#define DNG_FALLTHROUGH [[fallthrough]];
#else
#define DNG_FALLTHROUGH
#endif
#else
#define DNG_FALLTHROUGH
#endif

#ifndef qDNGThreadSafe
#define qDNGThreadSafe (qMacOS || qWinOS)
#endif

#ifndef qDNGValidateTarget
#define qDNGValidateTarget 0
#endif

#ifndef qDNGValidate
#define qDNGValidate qDNGValidateTarget
#endif

#ifndef qDNGPrintMessages
#define qDNGPrintMessages qDNGValidate
#endif

#ifndef qDNGExperimental
#define qDNGExperimental 1
#endif

#ifndef qDNGXMPFiles
#define qDNGXMPFiles 1
#endif

#ifndef qDNGXMPDocOps
#define qDNGXMPDocOps (!qDNGValidateTarget)
#endif

#ifndef qDNGUseLibJPEG
#define qDNGUseLibJPEG qDNGValidateTarget
#endif

#ifndef qDNGAVXSupport
#define qDNGAVXSupport ((qMacOS || qWinOS) && qDNG64Bit && !qARM && 1)
#endif

#if qDNGAVXSupport && !(qDNG64Bit && !qARM)
#error AVX support is enabled when 64-bit support is not or ARM is
#endif

#ifndef qDNGSupportVC5
#define qDNGSupportVC5 (1)
#endif

#ifndef qDNGUsingAddressSanitizer
#if defined(__clang__) && defined(__has_feature)
#if __has_feature(address_sanitizer)
#define qDNGUsingAddressSanitizer (1)
#endif
#endif
#endif

#ifndef qDNGUsingAddressSanitizer
#if defined(__SANITIZE_ADDRESS__)
#define qDNGUsingAddressSanitizer (1)
#endif
#endif

#ifndef qDNGUsingAddressSanitizer
#define qDNGUsingAddressSanitizer (0)
#endif

#ifndef qDNGUsingThreadSanitizer
#if defined(__clang__) && defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define qDNGUsingThreadSanitizer (1)
#endif
#endif
#endif

#ifndef qDNGUsingThreadSanitizer
#if defined(__SANITIZE_THREAD__)
#define qDNGUsingThreadSanitizer (1)
#endif
#endif

#ifndef qDNGUsingThreadSanitizer
#define qDNGUsingThreadSanitizer (0)
#endif

#ifndef qDNGUsingUndefinedBehaviorSanitizer
#if defined(__clang__) && defined(__has_feature)
#if __has_feature(undefined_behavior_sanitizer)
#define qDNGUsingUndefinedBehaviorSanitizer (1)
#endif
#endif
#endif

#ifndef qDNGUsingUndefinedBehaviorSanitizer
#define qDNGUsingUndefinedBehaviorSanitizer (0)
#endif

#ifndef qDNGUsingSanitizer
#define qDNGUsingSanitizer ((qDNGUsingAddressSanitizer || qDNGUsingThreadSanitizer || qDNGUsingUndefinedBehaviorSanitizer) || 0)
#endif

#ifndef DNG_ATTRIB_NO_SANITIZE

#ifndef RC_INVOKED
#if qDNGUsingSanitizer && defined(__clang__)
#define DNG_ATTRIB_NO_SANITIZE(type) __attribute__((no_sanitize(type)))
#else
#define DNG_ATTRIB_NO_SANITIZE(type)
#endif
#endif
#endif

#ifndef qDNGBigImage
#define qDNGBigImage (qDNGExperimental && 1)
#endif

#ifndef qDNGUseXMP
#define qDNGUseXMP 1
#endif

#ifndef qDNGUseCustomIntegralTypes
#define qDNGUseCustomIntegralTypes 0
#endif

#ifndef qDNGVerboseExceptions
#define qDNGVerboseExceptions 1
#endif

#include "dng_deprecated_flags.h"

#endif
	

