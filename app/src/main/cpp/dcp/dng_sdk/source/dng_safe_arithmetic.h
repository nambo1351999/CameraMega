

#ifndef __dng_safe_arithmetic__
#define __dng_safe_arithmetic__

#include <cstddef>

#include <limits>

#include "dng_exceptions.h"
#include "dng_flags.h"
#include "dng_types.h"

#ifndef __has_builtin
#define __has_builtin(x) 0	
#endif

#if !defined(DNG_HAS_INT128) && defined(__SIZEOF_INT128__)
#define DNG_HAS_INT128
#endif

bool SafeInt32Add(int32 arg1, int32 arg2, int32 *result);

int32 SafeInt32Add(int32 arg1, int32 arg2);
int64 SafeInt64Add(int64 arg1, int64 arg2);

int32 SafeInt32Add(int32 arg1, int32 arg2, int32 arg3);
int64 SafeInt64Add(int64 arg1, int64 arg2, int64 arg3);

bool SafeUint32Add(uint32 arg1, uint32 arg2,
				   uint32 *result);

uint32 SafeUint32Add(uint32 arg1, uint32 arg2);
uint64 SafeUint64Add(uint64 arg1, uint64 arg2);

uint32 SafeUint32Add(uint32 arg1, uint32 arg2, uint32 arg3);
uint64 SafeUint64Add(uint64 arg1, uint64 arg2, uint64 arg3);

bool SafeInt32Sub(int32 arg1, int32 arg2, int32 *result);

int32 SafeInt32Sub(int32 arg1, int32 arg2);

uint32 SafeUint32Sub(uint32 arg1, uint32 arg2);

int32 SafeInt32Mult(int32 arg1, int32 arg2);
bool SafeInt32Mult(int32 arg1, int32 arg2, int32 *result);

bool SafeUint32Mult(uint32 arg1, uint32 arg2,
					uint32 *result);
bool SafeUint32Mult(uint32 arg1, uint32 arg2, uint32 arg3,
					uint32 *result);
bool SafeUint32Mult(uint32 arg1, uint32 arg2, uint32 arg3,
					uint32 arg4, uint32 *result);

uint32 SafeUint32Mult(uint32 arg1, uint32 arg2);
uint32 SafeUint32Mult(uint32 arg1, uint32 arg2,
							 uint32 arg3);
uint32 SafeUint32Mult(uint32 arg1, uint32 arg2,
							 uint32 arg3, uint32 arg4);

std::size_t SafeSizetMult(std::size_t arg1, std::size_t arg2);

namespace dng_internal {

int64 SafeInt64MultSlow(int64 arg1, int64 arg2);

#if !qWinOS
#ifdef __clang__
#define __USE_BUILTIN_SMULL_OVERFLOW __has_builtin(__builtin_smull_overflow)
#endif 
#endif 

#ifndef __USE_BUILTIN_SMULL_OVERFLOW
#define __USE_BUILTIN_SMULL_OVERFLOW 0
#endif

#if __USE_BUILTIN_SMULL_OVERFLOW
inline int64 SafeInt64MultByClang(int64 arg1, int64 arg2) {
	int64 result;
	bool failed;
	
	if (sizeof(long) >= 8) {
	  long temp_result;
	  failed = __builtin_smull_overflow((long)arg1, (long)arg2, &temp_result);
	  if (sizeof(long) > 8 && !failed) {
		  failed = (temp_result > std::numeric_limits<int64>::max() ||
				temp_result < std::numeric_limits<int64>::min());
	  }
	  result = (int64)temp_result;
	} else if (sizeof(long long) >= 8) {
	  long long temp_result;
	  failed = __builtin_smulll_overflow((long long)arg1, (long long)arg2, &temp_result);
	  if (sizeof(long long) > 8 && !failed) {
		  failed = (temp_result > std::numeric_limits<int64>::max() ||
				temp_result < std::numeric_limits<int64>::min());
	  }
	  result = (int64)temp_result;
	} else {
	  ThrowNotYetImplemented("No 64-bit capable multiply with overflow builtin.");
	}
	if (failed) {
	  ThrowProgramError("Arithmetic overflow");
	  abort();	
	}
	return result;
}
#endif

#ifdef DNG_HAS_INT128
inline int64 SafeInt64MultByInt128(int64 arg1,
									  int64 arg2) {
	const __int128 kInt64Max =
		static_cast<__int128>(std::numeric_limits<int64>::max());
	const __int128 kInt64Min =
		static_cast<__int128>(std::numeric_limits<int64>::min());
	__int128 result = static_cast<__int128>(arg1) * static_cast<__int128>(arg2);
	if (result > kInt64Max || result < kInt64Min) {
		ThrowProgramError("Arithmetic overflow");
	}
	return static_cast<int64>(result);
}
#endif

}  

inline int64 SafeInt64Mult(int64 arg1, int64 arg2) {
#if __USE_BUILTIN_SMULL_OVERFLOW
	return dng_internal::SafeInt64MultByClang(arg1, arg2);
#elif defined(DNG_HAS_INT128)
	return dng_internal::SafeInt64MultByInt128(arg1, arg2);
#else
	return dng_internal::SafeInt64MultSlow(arg1, arg2);
#endif
}

int64 SafeInt64Mult(int64 arg1, int64 arg2, int64 arg3);

uint32 SafeUint32DivideUp(uint32 arg1, uint32 arg2);

bool RoundUpUint32ToMultiple(uint32 val, uint32 multiple_of,
							 uint32 *result);

uint32 RoundUpUint32ToMultiple(uint32 val,
									  uint32 multiple_of);

bool ConvertUint32ToInt32(uint32 val, int32 *result);

int32 ConvertUint32ToInt32(uint32 val);

template <class TSrc, class TDest>
static void ConvertUnsigned(TSrc src, TDest *dest) {
#if 0
	
	if (!(std::numeric_limits<TSrc>::is_integer &&
		  !std::numeric_limits<TSrc>::is_signed &&
		  std::numeric_limits<TDest>::is_integer &&
		  !std::numeric_limits<TDest>::is_signed))
	{
	ThrowProgramError ("TSrc and TDest must be unsigned integer types");
	}
#else
	
	static_assert(std::numeric_limits<TSrc>::is_integer &&
				  !std::numeric_limits<TSrc>::is_signed &&
				  std::numeric_limits<TDest>::is_integer &&
				  !std::numeric_limits<TDest>::is_signed,
				  "TSrc and TDest must be unsigned integer types");
#endif

	const TDest converted = static_cast<TDest>(src);

	
	
	if (static_cast<TSrc>(converted) != src) {
		ThrowProgramError("Overflow in unsigned integer conversion");
	}

	*dest = converted;
}

int32 ConvertDoubleToInt32(double val);
uint32 ConvertDoubleToUint32(double val);

float ConvertDoubleToFloat(double val);

class dng_safe_int32;
class dng_safe_uint32;

#define CHECK_SAFE_UINT32									\
	static_assert (std::numeric_limits<T>::is_integer &&	\
				   !std::numeric_limits<T>::is_signed &&	\
				   (sizeof (T) == 4),						\
				   "src must be unsigned 32-bit integer")

class dng_safe_uint32
	{

	private:

		uint32 fValue;
		
	public:

		template<typename T>
		dng_safe_uint32 (T x)
			{
			CHECK_SAFE_UINT32;
			fValue = x;
			}

		explicit dng_safe_uint32 (const dng_safe_int32 &x);

		inline uint32 Get () const
			{
			return fValue;
			}

		

		dng_safe_uint32 & operator+= (const dng_safe_uint32 &x)
			{
			fValue = SafeUint32Add (fValue, x.fValue);
			return *this;
			}
		
		template<typename T>
		dng_safe_uint32 & operator+= (T x)
			{
			CHECK_SAFE_UINT32;
			fValue = SafeUint32Add (fValue, x);
			return *this;
			}
		
		dng_safe_uint32 & operator*= (const dng_safe_uint32 &x)
			{
			fValue = SafeUint32Mult (fValue, x.fValue);
			return *this;
			}
		
		template<typename T>
		dng_safe_uint32 & operator*= (T x)
			{
			CHECK_SAFE_UINT32;
			fValue = SafeUint32Mult (fValue, x);
			return *this;
			}

		
		
		const dng_safe_uint32 operator+ (const dng_safe_uint32 &x) const
			{
			return dng_safe_uint32 (*this) += x;
			}
		
		template<typename T>
		const dng_safe_uint32 operator+ (T x) const
			{
			CHECK_SAFE_UINT32;
			return dng_safe_uint32 (*this) += x;
			}
		
		const dng_safe_uint32 operator* (const dng_safe_uint32 &x) const
			{
			return dng_safe_uint32 (*this) *= x;
			}
		
		template<typename T>
		const dng_safe_uint32 operator* (T x) const
			{
			CHECK_SAFE_UINT32;
			return dng_safe_uint32 (*this) *= x;
			}
		
	};

#undef CHECK_SAFE_UINT32

#define CHECK_SAFE_INT32									\
	static_assert (std::numeric_limits<T>::is_integer &&	\
				   std::numeric_limits<T>::is_signed &&		\
				   (sizeof (T) == 4),						\
				   "src must be signed 32-bit integer")

class dng_safe_int32
	{

	private:

		int32 fValue;
		
	public:

		template<typename T>
		dng_safe_int32 (T x)
			{
			CHECK_SAFE_INT32;
			fValue = x;
			}

		
		

		explicit dng_safe_int32 (const dng_safe_uint32 &x);

		inline int32 Get () const
			{
			return fValue;
			}

		
		

		void Set_uint32 (uint32 x)
			{
			if (!ConvertUint32ToInt32 (x, &fValue))
				{
				ThrowProgramError ("Overflow in Set_uint32");
				}
			}

		

		dng_safe_int32 & operator+= (const dng_safe_int32 &x)
			{
			fValue = SafeInt32Add (fValue, x.fValue);
			return *this;
			}
		
		template<typename T>
		dng_safe_int32 & operator+= (T x)
			{
			CHECK_SAFE_INT32;
			fValue = SafeInt32Add (fValue, x);
			return *this;
			}
		
		dng_safe_int32 & operator-= (const dng_safe_int32 &x)
			{
			fValue = SafeInt32Sub (fValue, x.fValue);
			return *this;
			}
		
		template<typename T>
		dng_safe_int32 & operator-= (T x)
			{
			CHECK_SAFE_INT32;
			fValue = SafeInt32Sub (fValue, x);
			return *this;
			}
		
	};

#undef CHECK_SAFE_INT32

#endif	
