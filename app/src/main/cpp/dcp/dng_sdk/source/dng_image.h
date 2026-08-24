

#ifndef __dng_image__
#define __dng_image__

#include "dng_assertions.h"
#include "dng_classes.h"
#include "dng_pixel_buffer.h"
#include "dng_point.h"
#include "dng_rect.h"
#include "dng_tag_types.h"
#include "dng_types.h"
#include "dng_uncopyable.h"

class dng_tile_buffer: public dng_pixel_buffer,
					   private dng_uncopyable
	{
	
	protected:
	
		const dng_image &fImage;
		
		void *fRefData;
	
	protected:
	
		
		
		
		

		dng_tile_buffer (const dng_image &image,
						 const dng_rect &tile,
						 bool dirty);
						 
		virtual ~dng_tile_buffer ();
		
	public:
	
		void SetRefData (void *refData)
			{
			fRefData = refData;
			}
			
		void * GetRefData () const
			{
			return fRefData;
			}
			
	};

class dng_const_tile_buffer: public dng_tile_buffer
	{
	
	public:
	
		
		
		

		dng_const_tile_buffer (const dng_image &image,
							   const dng_rect &tile);
							   
		virtual ~dng_const_tile_buffer ();
	
	};
	

class dng_dirty_tile_buffer: public dng_tile_buffer
	{
	
	public:
	
		
		
		

		dng_dirty_tile_buffer (dng_image &image,
							   const dng_rect &tile);
							   
		virtual ~dng_dirty_tile_buffer ();
	
	};
	

class dng_image
	{
	
	friend class dng_tile_buffer;
	
	protected:
	
		
		
		dng_rect fBounds;
		
		
		
		uint32 fPlanes;
		
		
	
		uint32 fPixelType;
		
	public:
	
		

		enum edge_option
			{
			
			

			edge_none,
			
			
			
			edge_zero,
			
			
			
			edge_repeat,
			
			
			
			edge_repeat_zero_last,

			

			edge_wrap_horizontal,
			
			

			edge_wrap_vertical,
			
			
			

			edge_wrap_all
			
			};
	
	protected:
	
		dng_image (const dng_rect &bounds,
				   uint32 planes,
				   uint32 pixelType);
		
	public:
	
		virtual ~dng_image ();
		
		virtual dng_image * Clone () const;

		
		
		const dng_rect & Bounds () const
			{
			return fBounds;
			}

		
			
		dng_point Size () const
			{
			return Bounds ().Size ();
			}
		
		

		uint32 Width () const
			{
			return Bounds ().W ();
			}

		
		
		uint32 Height () const
			{
			return Bounds ().H ();
			}
			
		

		uint32 Planes () const
			{
			return fPlanes;
			}
			
		
		
		

		uint32 PixelType () const
			{
			return fPixelType;
			}
			
		
		
		
		virtual void SetPixelType (uint32 pixelType);
		
		
		

		uint32 PixelSize () const;
		
		
		
		
		

		uint32 PixelRange () const;

		

		virtual dng_rect RepeatingTile () const;

		
		
		
		
		
		
		
		
		

		void Get (dng_pixel_buffer &buffer,
				  edge_option edgeOption = edge_none,
				  uint32 repeatV = 1,
				  uint32 repeatH = 1) const;

		
		

		void Put (const dng_pixel_buffer &buffer);

		
		

		virtual void Trim (const dng_rect &r);

		
		

		virtual void Rotate (const dng_orientation &orientation);
		
		
		
		
		virtual void Offset (const dng_point &offset);
		
		
		
		
		
		
		

		void CopyArea (const dng_image &src,
					   const dng_rect &area,
					   uint32 srcPlane,
					   uint32 dstPlane,
					   uint32 planes)
			{

			DoCopyArea (src, area, srcPlane, dstPlane, planes);

			}

		
		
		
		
		

		void CopyArea (const dng_image &src,
					   const dng_rect &area,
					   uint32 plane,
					   uint32 planes)
			{

			DoCopyArea (src, area, plane, plane, planes);

			}

		
		
		
		
		

		virtual bool EqualArea (const dng_image &rhs,
								const dng_rect &area,
								uint32 plane,
								uint32 planes) const;
						
		
		
		void SetConstant_uint8 (uint8 value,
								const dng_rect &area)
			{
			
			DNG_ASSERT (fPixelType == ttByte, "Mismatched pixel type");
			
			SetConstant ((uint32) value, area);
			
			}
		
		void SetConstant_uint8 (uint8 value)
			{
			SetConstant (value, Bounds ());
			}
		
		void SetConstant_uint16 (uint16 value,
								 const dng_rect &area)
			{
			
			DNG_ASSERT (fPixelType == ttShort, "Mismatched pixel type");
			
			SetConstant ((uint32) value, area);
			
			}
		
		void SetConstant_uint16 (uint16 value)
			{
			SetConstant_uint16 (value, Bounds ());
			}
		
		void SetConstant_int16 (int16 value,
								const dng_rect &area)
			{
			
			DNG_ASSERT (fPixelType == ttSShort, "Mismatched pixel type");
			
			SetConstant ((uint32) (uint16) value, area);
			
			}
		
		void SetConstant_int16 (int16 value)
			{
			SetConstant_int16 (value, Bounds ());
			}
		
		void SetConstant_uint32 (uint32 value,
								 const dng_rect &area)
			{
			
			DNG_ASSERT (fPixelType == ttLong, "Mismatched pixel type");
			
			SetConstant (value, area);
			
			}
		
		void SetConstant_uint32 (uint32 value)
			{
			SetConstant_uint32 (value, Bounds ());
			}
		
		void SetConstant_real32 (real32 value,
								 const dng_rect &area)
			{
			
			DNG_ASSERT (fPixelType == ttFloat, "Mismatched pixel type");
			
			union
				{
				uint32 i;
				real32 f;
				} x;
				
			x.f = value;
			
			SetConstant (x.i, area);
			
			}

		void SetConstant_real32 (real32 value)
			{
			SetConstant_real32 (value, Bounds ());
			}

		void SetZero (const dng_rect &area)
			{
			SetConstant (0, area);
			}
		
		virtual void GetRepeat (dng_pixel_buffer &buffer,
								const dng_rect &srcArea,
								const dng_rect &dstArea) const;
	
	protected:
	
		virtual void AcquireTileBuffer (dng_tile_buffer &buffer,
										const dng_rect &area,
										bool dirty) const;
	
		virtual void ReleaseTileBuffer (dng_tile_buffer &buffer) const;
	
		virtual void DoGet (dng_pixel_buffer &buffer) const;
		
		virtual void DoPut (const dng_pixel_buffer &buffer);

		virtual void DoCopyArea (const dng_image &src,
								 const dng_rect &area,
								 uint32 srcPlane,
								 uint32 dstPlane,
								 uint32 planes);

		void GetEdge (dng_pixel_buffer &buffer,
					  edge_option edgeOption,
					  const dng_rect &srcArea,
					  const dng_rect &dstArea) const;
					  
		virtual void SetConstant (uint32 value,
								  const dng_rect &area);

	};

#endif
	

