

#ifndef __dng_pixel_buffer__
#define __dng_pixel_buffer__

#include "dng_assertions.h"
#include "dng_rect.h"
#include "dng_safe_arithmetic.h"
#include "dng_tag_types.h"

void OptimizeOrder (const void *&sPtr,
					void *&dPtr,
					uint32 sPixelSize,
					uint32 dPixelSize,
					uint32 &count0,
					uint32 &count1,
					uint32 &count2,
					int32 &sStep0,
					int32 &sStep1,
					int32 &sStep2,
					int32 &dStep0,
					int32 &dStep1,
					int32 &dStep2);

void OptimizeOrder (const void *&sPtr,
					uint32 sPixelSize,
					uint32 &count0,
					uint32 &count1,
					uint32 &count2,
					int32 &sStep0,
					int32 &sStep1,
					int32 &sStep2);

void OptimizeOrder (void *&dPtr,
					uint32 dPixelSize,
					uint32 &count0,
					uint32 &count1,
					uint32 &count2,
					int32 &dStep0,
					int32 &dStep1,
					int32 &dStep2);

#define qDebugPixelType 0

#if qDebugPixelType

#define ASSERT_PIXEL_TYPE(typeVal) CheckPixelType (typeVal)

#else

#define ASSERT_PIXEL_TYPE(typeVal) DNG_ASSERT (fPixelType == typeVal, "Pixel type access mismatch")

#endif

class dng_pixel_buffer
	{
	
	public:
	
		
	
		dng_rect fArea;
		
		
		
		uint32 fPlane;
		uint32 fPlanes;
		
		
		
		int32 fRowStep;
		int32 fColStep;
		int32 fPlaneStep;
		
		
	
		uint32 fPixelType;
		
		
		
		uint32 fPixelSize;
		
		
		
		void *fData;
		
		
		
		bool fDirty;
		
	private:
	
		void * InternalPixel (int32 row,
							  int32 col,
							  uint32 plane = 0) const
			{

			
			
			
			
			
			
			
			
			
			
			
			

			#if 0

			
			if (row < fArea.t || row >= fArea.b ||
				col < fArea.l || col >= fArea.r ||
				plane < fPlane || (plane - fPlane) >= fPlanes)
				{
				ThrowProgramError ("Out-of-range pixel access");
				}

			
			const int64 rowOffset = SafeInt64Mult(fRowStep,
				static_cast<int64> (row) - static_cast<int64> (fArea.t));
			const int64 colOffset = SafeInt64Mult(fColStep,
				static_cast<int64> (col) - static_cast<int64> (fArea.l));
			const int64 planeOffset = SafeInt64Mult(fPlaneStep,
				static_cast<int64> (plane - fPlane));
			const int64 offset = SafeInt64Mult(static_cast<int64>(fPixelSize),
				SafeInt64Add(SafeInt64Add(rowOffset, colOffset), planeOffset));

			
			return static_cast<void *> (static_cast<uint8 *> (fData) + offset);

			#else

			return (void *)
				   (((uint8 *) fData) + (int64) fPixelSize *
					(fRowStep	* (row	 - (int64) fArea.t) +
					 fColStep	* (col	 - (int64) fArea.l) +
					 fPlaneStep * (plane - (int64) fPlane )));

			#endif

			}
			
		#if qDebugPixelType
			
		void CheckPixelType (uint32 pixelType) const;
		
		#endif
		
	public:
	
		dng_pixel_buffer ();
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		

		dng_pixel_buffer (const dng_rect &area, 
						  uint32 plane, 
						  uint32 planes,
						  uint32 pixelType, 
						  uint32 planarConfiguration,
						  void *data);
		
		dng_pixel_buffer (const dng_pixel_buffer &buffer);
		
		dng_pixel_buffer & operator= (const dng_pixel_buffer &buffer);

		virtual ~dng_pixel_buffer ();
		
		
		

		uint32 PixelRange () const;

		
		

		const dng_rect & Area () const
			{
			return fArea;
			}

		
		

		uint32 Planes () const
			{
			return fPlanes;
			}

		
		

		int32 RowStep () const
			{
			return fRowStep;
			}
			
		
		

		int32 PlaneStep () const
			{
			return fPlaneStep;
			}

		
		
		
		
		

		const void * ConstPixel (int32 row,
								 int32 col,
								 uint32 plane = 0) const
			{
			
			return InternalPixel (row, col, plane);
					 
			}
			
		
		
		
		
		

		void * DirtyPixel (int32 row,
						   int32 col,
						   uint32 plane = 0)
			{
			
			DNG_ASSERT (fDirty, "Dirty access to const pixel buffer");
			
			return InternalPixel (row, col, plane);
			
			}

		
		
		
		
		
			
		const uint8 * ConstPixel_uint8 (int32 row,
										int32 col,
										uint32 plane = 0) const
			{
			
			ASSERT_PIXEL_TYPE (ttByte);

			return (const uint8 *) ConstPixel (row, col, plane);
			
			}

		const uint8 * ConstPixel_uint8_overrideType (int32 row,
										int32 col,
										uint32 plane = 0) const
			{
			
			

			return (const uint8 *) ConstPixel (row, col, plane);
			
			}
			
		
		
		
		
		

		uint8 * DirtyPixel_uint8 (int32 row,
								  int32 col,
								  uint32 plane = 0)
			{
			
			ASSERT_PIXEL_TYPE (ttByte);

			return (uint8 *) DirtyPixel (row, col, plane);
			
			}

		uint8 * DirtyPixel_uint8_overrideType (int32 row,
								  int32 col,
								  uint32 plane = 0)
			{
			
			

			return (uint8 *) DirtyPixel (row, col, plane);
			
			}
			
		
		
		
		
		

		const int8 * ConstPixel_int8 (int32 row,
									  int32 col,
									  uint32 plane = 0) const
			{
			
			ASSERT_PIXEL_TYPE (ttSByte);

			return (const int8 *) ConstPixel (row, col, plane);
			
			}
			
		
		
		
		
		

		int8 * DirtyPixel_int8 (int32 row,
								int32 col,
								uint32 plane = 0)
			{
			
			ASSERT_PIXEL_TYPE (ttSByte);

			return (int8 *) DirtyPixel (row, col, plane);
			
			}
			
		
		
		
		
		

		const uint16 * ConstPixel_uint16 (int32 row,
										  int32 col,
										  uint32 plane = 0) const
			{
			
			ASSERT_PIXEL_TYPE (ttShort);

			return (const uint16 *) ConstPixel (row, col, plane);
			
			}
			
		
		
		
		
		

		uint16 * DirtyPixel_uint16 (int32 row,
									int32 col,
									uint32 plane = 0)
			{
			
			ASSERT_PIXEL_TYPE (ttShort);

			return (uint16 *) DirtyPixel (row, col, plane);
			
			}
			
		
		
		
		
		

		const int16 * ConstPixel_int16 (int32 row,
										int32 col,
										uint32 plane = 0) const
			{
			
			ASSERT_PIXEL_TYPE (ttSShort);

			return (const int16 *) ConstPixel (row, col, plane);
			
			}
			
		
		
		
		
		

		int16 * DirtyPixel_int16 (int32 row,
								  int32 col,
								  uint32 plane = 0)
			{
			
			ASSERT_PIXEL_TYPE (ttSShort);

			return (int16 *) DirtyPixel (row, col, plane);
			
			}

		
		
		
		
		

		const uint32 * ConstPixel_uint32 (int32 row,
										  int32 col,
										  uint32 plane = 0) const
			{
			
			ASSERT_PIXEL_TYPE (ttLong);

			return (const uint32 *) ConstPixel (row, col, plane);
			
			}
			
		
		
		
		
		

		uint32 * DirtyPixel_uint32 (int32 row,
									int32 col,
									uint32 plane = 0)
			{
			
			ASSERT_PIXEL_TYPE (ttLong);

			return (uint32 *) DirtyPixel (row, col, plane);
			
			}
			
		
		
		
		
		

		const int32 * ConstPixel_int32 (int32 row,
										int32 col,
										uint32 plane = 0) const
			{
			
			ASSERT_PIXEL_TYPE (ttSLong);

			return (const int32 *) ConstPixel (row, col, plane);
			
			}
			
		
		
		
		
		

		int32 * DirtyPixel_int32 (int32 row,
								  int32 col,
								  uint32 plane = 0)
			{
			
			ASSERT_PIXEL_TYPE (ttSLong);

			return (int32 *) DirtyPixel (row, col, plane);
			
			}

		
		
		
		
		

		const uint64 * ConstPixel_uint64 (int32 row,
										  int32 col,
										  uint32 plane = 0) const
			{
			
			ASSERT_PIXEL_TYPE (ttLong8);

			return (const uint64 *) ConstPixel (row, col, plane);
			
			}
			
		
		
		
		
		

		uint64 * DirtyPixel_uint64 (int32 row,
									int32 col,
									uint32 plane = 0)
			{
			
			ASSERT_PIXEL_TYPE (ttLong8);

			return (uint64 *) DirtyPixel (row, col, plane);
			
			}
			
		
		
		
		
		

		const int64 * ConstPixel_int64 (int32 row,
										int32 col,
										uint32 plane = 0) const
			{
			
			ASSERT_PIXEL_TYPE (ttSLong8);

			return (const int64 *) ConstPixel (row, col, plane);
			
			}
			
		
		
		
		
		

		int64 * DirtyPixel_int64 (int32 row,
								  int32 col,
								  uint32 plane = 0)
			{
			
			ASSERT_PIXEL_TYPE (ttSLong8);

			return (int64 *) DirtyPixel (row, col, plane);
			
			}

		
		
		
		
		

		const real32 * ConstPixel_real32 (int32 row,
										  int32 col,
										  uint32 plane = 0) const
			{
			
			ASSERT_PIXEL_TYPE (ttFloat);

			return (const real32 *) ConstPixel (row, col, plane);
			
			}
			
		
		
		
		
		

		real32 * DirtyPixel_real32 (int32 row,
									int32 col,
									uint32 plane = 0)
			{
			
			ASSERT_PIXEL_TYPE (ttFloat);

			return (real32 *) DirtyPixel (row, col, plane);
			
			}
		
		
		
		
		
		

		const real64 * ConstPixel_real64 (int32 row,
										  int32 col,
										  uint32 plane = 0) const
			{

			ASSERT_PIXEL_TYPE (ttDouble);

			return (const real64 *) ConstPixel (row, col, plane);

			}

		
		
		
		
		

		real64 * DirtyPixel_real64 (int32 row,
									int32 col,
									uint32 plane = 0)
			{

			ASSERT_PIXEL_TYPE (ttDouble);

			return (real64 *) DirtyPixel (row, col, plane);

			}

		
		
		
		
		

		void SetConstant (const dng_rect &area,
						  uint32 plane,
						  uint32 planes,
						  uint32 value);
		
		
		
		
		
		

		void SetConstant_uint8 (const dng_rect &area,
								uint32 plane,
								uint32 planes,
								uint8 value)
			{
			
			DNG_ASSERT (fPixelType == ttByte, "Mismatched pixel type");
			
			SetConstant (area, plane, planes, (uint32) value);
			
			}
		
		
		
		
		
		

		void SetConstant_uint16 (const dng_rect &area,
								 uint32 plane,
								 uint32 planes,
								 uint16 value)
			{
			
			DNG_ASSERT (fPixelType == ttShort, "Mismatched pixel type");
			
			SetConstant (area, plane, planes, (uint32) value);
			
			}
		
		
		
		
		
		

		void SetConstant_int16 (const dng_rect &area,
								uint32 plane,
								uint32 planes,
								int16 value)
			{
			
			DNG_ASSERT (fPixelType == ttSShort, "Mismatched pixel type");
			
			SetConstant (area, plane, planes, (uint32) (uint16) value);
			
			}
		
		
		
		
		
		

		void SetConstant_uint32 (const dng_rect &area,
								 uint32 plane,
								 uint32 planes,
								 uint32 value)
			{
			
			DNG_ASSERT (fPixelType == ttLong, "Mismatched pixel type");
			
			SetConstant (area, plane, planes, value);
			
			}
		
		
		
		
		
		

		void SetConstant_real32 (const dng_rect &area,
								 uint32 plane,
								 uint32 planes,
								 real32 value)
			{
			
			DNG_ASSERT (fPixelType == ttFloat, "Mismatched pixel type");
			
			union
				{
				uint32 i;
				real32 f;
				} x;
				
			x.f = value;
			
			SetConstant (area, plane, planes, x.i);
			
			}

		
		
		
		

		void SetZero (const dng_rect &area,
					  uint32 plane,
					  uint32 planes);
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		

		void CopyArea (const dng_pixel_buffer &src,
					   const dng_rect &area,
					   uint32 srcPlane,
					   uint32 dstPlane,
					   uint32 planes);
					   
		
		
		
		
		

		void CopyArea (const dng_pixel_buffer &src,
					   const dng_rect &area,
					   uint32 plane,
					   uint32 planes)
			{
			
			CopyArea (src, area, plane, plane, planes);
			
			}
					   
		
		
		
		

		static dng_point RepeatPhase (const dng_rect &srcArea,
									  const dng_rect &dstArea);

		
		
		
		

		void RepeatArea (const dng_rect &srcArea,
						 const dng_rect &dstArea);
						 
		
		
		void RepeatSubArea (const dng_rect subArea,
							uint32 repeatV = 1,
							uint32 repeatH = 1);

		
		

		void ShiftRight (uint32 shift);
		
		
		

		void FlipH ();
		
		
		

		void FlipV ();
		
		
		

		void FlipZ ();	
		
		
		
		
		
		
		

		bool EqualArea (const dng_pixel_buffer &rhs,
						const dng_rect &area,
						uint32 plane,
						uint32 planes) const;

		
		
		
		
		
		

		real64 MaximumDifference (const dng_pixel_buffer &rhs,
								  const dng_rect &area,
								  uint32 plane,
								  uint32 planes) const;

	};

template <typename T>
inline T * DirtyPixel (dng_pixel_buffer & pixBuf,
					   const int32 row,
					   const int32 col,
					   const uint32 plane = 0) = delete;

#define DEFINE_DIRTY_PIXEL_SPECIALIZATION_FOR_TYPE(T)		 \
	template <>												 \
	inline T * DirtyPixel (dng_pixel_buffer &pixBuf,		 \
						   const int32 row,					 \
						   const int32 col,					 \
						   const uint32 plane)				 \
		{													 \
															 \
		return pixBuf.DirtyPixel_ ## T (row, col, plane);	 \
															 \
		}

DEFINE_DIRTY_PIXEL_SPECIALIZATION_FOR_TYPE (uint8 )
DEFINE_DIRTY_PIXEL_SPECIALIZATION_FOR_TYPE ( int8 )
DEFINE_DIRTY_PIXEL_SPECIALIZATION_FOR_TYPE (uint16)
DEFINE_DIRTY_PIXEL_SPECIALIZATION_FOR_TYPE ( int16)
DEFINE_DIRTY_PIXEL_SPECIALIZATION_FOR_TYPE (uint32)
DEFINE_DIRTY_PIXEL_SPECIALIZATION_FOR_TYPE ( int32)
DEFINE_DIRTY_PIXEL_SPECIALIZATION_FOR_TYPE (uint64)
DEFINE_DIRTY_PIXEL_SPECIALIZATION_FOR_TYPE ( int64)
DEFINE_DIRTY_PIXEL_SPECIALIZATION_FOR_TYPE (real32)
DEFINE_DIRTY_PIXEL_SPECIALIZATION_FOR_TYPE (real64)

template <typename T>
inline const T * ConstPixel (const dng_pixel_buffer &pixBuf,
							 const int32 row,
							 const int32 col,
							 const uint32 plane = 0) = delete;

#define DEFINE_CONST_PIXEL_SPECIALIZATION_FOR_TYPE(T)			 \
	template <>													 \
	inline const T * ConstPixel (const dng_pixel_buffer &pixBuf, \
						  const int32 row,						 \
						  const int32 col,						 \
						  const uint32 plane)					 \
		{														 \
																 \
		return pixBuf.ConstPixel_ ## T (row, col, plane);		 \
																 \
		}

DEFINE_CONST_PIXEL_SPECIALIZATION_FOR_TYPE (uint8 )
DEFINE_CONST_PIXEL_SPECIALIZATION_FOR_TYPE ( int8 )
DEFINE_CONST_PIXEL_SPECIALIZATION_FOR_TYPE (uint16)
DEFINE_CONST_PIXEL_SPECIALIZATION_FOR_TYPE ( int16)
DEFINE_CONST_PIXEL_SPECIALIZATION_FOR_TYPE (uint32)
DEFINE_CONST_PIXEL_SPECIALIZATION_FOR_TYPE ( int32)
DEFINE_CONST_PIXEL_SPECIALIZATION_FOR_TYPE (uint64)
DEFINE_CONST_PIXEL_SPECIALIZATION_FOR_TYPE ( int64)
DEFINE_CONST_PIXEL_SPECIALIZATION_FOR_TYPE (real32)
DEFINE_CONST_PIXEL_SPECIALIZATION_FOR_TYPE (real64)

template <typename T>
inline constexpr T SignBit () = delete;

template <> inline constexpr   uint8 SignBit () { return  (uint8)0x80;       }
template <> inline constexpr    int8 SignBit () { return   (int8)0x80;       }
template <> inline constexpr  uint16 SignBit () { return (uint16)0x8000;     }
template <> inline constexpr   int16 SignBit () { return  (int16)0x8000;     }
template <> inline constexpr  uint32 SignBit () { return (uint32)0x80000000; }
template <> inline constexpr   int32 SignBit () { return  (int32)0x80000000; }
template <> inline constexpr  uint64 SignBit () { return (uint64)0x8000000000000000ULL; }
template <> inline constexpr   int64 SignBit () { return  (int64)0x8000000000000000ULL; }

#endif
	

