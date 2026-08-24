

#ifndef __dng_misc_opcodes__
#define __dng_misc_opcodes__

#include "dng_classes.h"

#include "dng_opcodes.h"
#include "dng_rational.h"

class dng_opcode_TrimBounds: public dng_opcode
	{
	
	private:
	
		dng_rect fBounds;
	
	public:

		
	
		dng_opcode_TrimBounds (const dng_rect &bounds);
		
		dng_opcode_TrimBounds (dng_stream &stream);
	
		virtual void PutData (dng_stream &stream) const;

		virtual void Apply (dng_host &host,
							dng_negative &negative,
							AutoPtr<dng_image> &image);
		
	};

class dng_area_spec
	{
	
	public:
	
		enum
			{
			kDataSize = 32
			};
	
	private:
	
		dng_rect fArea;
		
		uint32 fPlane;
		uint32 fPlanes;
		
		uint32 fRowPitch;
		uint32 fColPitch;

		dng_urational fAreaScale;
		
	public:

		
	
		dng_area_spec (const dng_rect &area = dng_rect (),
					   uint32 plane = 0,
					   uint32 planes = 1,
					   uint32 rowPitch = 1,
					   uint32 colPitch = 1)
					   
			:	fArea	   (area)
			,	fPlane	   (plane)
			,	fPlanes	   (planes)
			,	fRowPitch  (rowPitch)
			,	fColPitch  (colPitch)
			,	fAreaScale (1, 1)
			
			{
			}

		
			
		const dng_rect & Area () const
			{
			return fArea;
			}

		dng_rect ScaledArea () const;

		
		
		uint32 Plane () const
			{
			return fPlane;
			}

		
		
		uint32 Planes () const
			{
			return fPlanes;
			}

		
		
		uint32 RowPitch () const
			{
			return fRowPitch;
			}
		
		
		
		uint32 ColPitch () const
			{
			return fColPitch;
			}

		
			
		void GetData (dng_stream &stream);
		
		
			
		void PutData (dng_stream &stream) const;

		
		
		
		dng_rect Overlap (const dng_rect &tile) const;

		
		
		
		dng_rect ScaledOverlap (const dng_rect &tile) const;

		
		
		
		
		
		

		void ApplyAreaScale (const dng_urational & );

	};

class dng_opcode_MapTable: public dng_inplace_opcode
	{
	
	private:
	
		dng_area_spec fAreaSpec;
		
		AutoPtr<dng_memory_block> fTable;
		
		uint32 fCount;
  
		AutoPtr<dng_memory_block> fBlackAdjustedTable;
		
	public:

		
		
	
		dng_opcode_MapTable (dng_host &host,
							 const dng_area_spec &areaSpec,
							 const uint16 *table,
							 uint32 count = 0x10000);

		dng_opcode_MapTable (dng_host &host,
							 dng_stream &stream);
	
		void PutData (dng_stream &stream) const override;

		uint32 BufferPixelType (uint32 imagePixelType) override;
			
		dng_rect ModifiedBounds (const dng_rect &imageBounds) override;
	
		void Prepare (dng_negative &negative,
					  uint32 threadCount,
					  const dng_point &tileSize,
					  const dng_rect &imageBounds,
					  uint32 imagePlanes,
					  uint32 bufferPixelType,
					  dng_memory_allocator &allocator) override;

		void ProcessArea (dng_negative &negative,
						  uint32 threadIndex,
						  dng_pixel_buffer &buffer,
						  const dng_rect &dstArea,
						  const dng_rect &imageBounds) override;
								  
		void ApplyAreaScale (const dng_urational &scale) override
			{
			fAreaSpec.ApplyAreaScale (scale);
			}
		
	private:
	
		void ReplicateLastEntry ();

	};

class dng_opcode_MapPolynomial: public dng_inplace_opcode
	{
	
	public:
	
		enum
			{
			kMaxDegree = 8
			};
	
	protected:
	
		dng_area_spec fAreaSpec;
		
		uint32 fDegree;
		
		real64 fCoefficient [kMaxDegree + 1];
		
		real32 fCoefficient32 [kMaxDegree + 1];
		
	public:
	
		
		
		
		
		
		
		
		
	
		dng_opcode_MapPolynomial (const dng_area_spec &areaSpec,
								  uint32 degree,
								  const real64 *coefficient);
	
		dng_opcode_MapPolynomial (dng_stream &stream);
	
		virtual void PutData (dng_stream &stream) const override;

		virtual uint32 BufferPixelType (uint32 imagePixelType) override;
			
		virtual dng_rect ModifiedBounds (const dng_rect &imageBounds) override;
	
		virtual void ProcessArea (dng_negative &negative,
								  uint32 threadIndex,
								  dng_pixel_buffer &buffer,
								  const dng_rect &dstArea,
								  const dng_rect &imageBounds) override;

		uint32 Degree () const
			{
			return fDegree;
			}

		const real64 * Coefficients () const
			{
			return fCoefficient;
			}
								  
		void ApplyAreaScale (const dng_urational &scale) override
			{
			fAreaSpec.ApplyAreaScale (scale);
			}
		
	protected:

		virtual void DoProcess (dng_pixel_buffer &buffer,
								const dng_rect &area,
								const uint32 plane,
								const uint32 rowPitch,
								const uint32 colPitch,
								const real32 *coefficients,
								const uint32 degree,
								uint16 blackLevel) const;

	};

class dng_opcode_DeltaPerRow: public dng_inplace_opcode
	{
	
	private:
	
		dng_area_spec fAreaSpec;
		
		AutoPtr<dng_memory_block> fTable;
		
		real32 fScale;

	public:

		
		
	
		dng_opcode_DeltaPerRow (const dng_area_spec &areaSpec,
								AutoPtr<dng_memory_block> &table);
	
		dng_opcode_DeltaPerRow (dng_host &host,
								dng_stream &stream);
	
		virtual void PutData (dng_stream &stream) const;

		virtual uint32 BufferPixelType (uint32 imagePixelType);
			
		virtual dng_rect ModifiedBounds (const dng_rect &imageBounds);
	
		virtual void ProcessArea (dng_negative &negative,
								  uint32 threadIndex,
								  dng_pixel_buffer &buffer,
								  const dng_rect &dstArea,
								  const dng_rect &imageBounds);
								  
	};

class dng_opcode_DeltaPerColumn: public dng_inplace_opcode
	{
	
	private:
	
		dng_area_spec fAreaSpec;
		
		AutoPtr<dng_memory_block> fTable;
		
		real32 fScale;

	public:
	
		
		
	
		dng_opcode_DeltaPerColumn (const dng_area_spec &areaSpec,
								   AutoPtr<dng_memory_block> &table);
	
		dng_opcode_DeltaPerColumn (dng_host &host,
								   dng_stream &stream);
	
		virtual void PutData (dng_stream &stream) const;

		virtual uint32 BufferPixelType (uint32 imagePixelType);
			
		virtual dng_rect ModifiedBounds (const dng_rect &imageBounds);
	
		virtual void ProcessArea (dng_negative &negative,
								  uint32 threadIndex,
								  dng_pixel_buffer &buffer,
								  const dng_rect &dstArea,
								  const dng_rect &imageBounds);
								  
	};

class dng_opcode_ScalePerRow: public dng_inplace_opcode
	{
	
	private:
	
		dng_area_spec fAreaSpec;
		
		AutoPtr<dng_memory_block> fTable;

	public:
	
		
		
	
		dng_opcode_ScalePerRow (const dng_area_spec &areaSpec,
								AutoPtr<dng_memory_block> &table);
	
		dng_opcode_ScalePerRow (dng_host &host,
								dng_stream &stream);
	
		virtual void PutData (dng_stream &stream) const;

		virtual uint32 BufferPixelType (uint32 imagePixelType);
			
		virtual dng_rect ModifiedBounds (const dng_rect &imageBounds);
	
		virtual void ProcessArea (dng_negative &negative,
								  uint32 threadIndex,
								  dng_pixel_buffer &buffer,
								  const dng_rect &dstArea,
								  const dng_rect &imageBounds);
								  
	};

class dng_opcode_ScalePerColumn: public dng_inplace_opcode
	{
	
	private:
	
		dng_area_spec fAreaSpec;
		
		AutoPtr<dng_memory_block> fTable;

	public:
	
		
		
	
		dng_opcode_ScalePerColumn (const dng_area_spec &areaSpec,
								   AutoPtr<dng_memory_block> &table);
	
		dng_opcode_ScalePerColumn (dng_host &host,
								   dng_stream &stream);
	
		virtual void PutData (dng_stream &stream) const;

		virtual uint32 BufferPixelType (uint32 imagePixelType);
			
		virtual dng_rect ModifiedBounds (const dng_rect &imageBounds);
	
		virtual void ProcessArea (dng_negative &negative,
								  uint32 threadIndex,
								  dng_pixel_buffer &buffer,
								  const dng_rect &dstArea,
								  const dng_rect &imageBounds);
								  
	};

#endif
	

