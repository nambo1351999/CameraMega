

#ifndef __dng_opcodes__
#define __dng_opcodes__

#include "dng_auto_ptr.h"
#include "dng_classes.h"
#include "dng_rect.h"
#include "dng_types.h"

enum dng_opcode_id
	{
	
	
	
	dngOpcode_Private				= 0,
	
	
	
	
	dngOpcode_WarpRectilinear		= 1,
	
	
	
	
	dngOpcode_WarpFisheye			= 2,

	
	
	dngOpcode_FixVignetteRadial		= 3,
	
	
	
	dngOpcode_FixBadPixelsConstant	= 4,
	
	
	
	dngOpcode_FixBadPixelsList		= 5,
	
	
	
	dngOpcode_TrimBounds			= 6,
	
	
	
	dngOpcode_MapTable				= 7,
	
	
	
	dngOpcode_MapPolynomial			= 8,
	
	
	
	dngOpcode_GainMap				= 9,
	
	
	
	dngOpcode_DeltaPerRow			= 10,
	
	
	
	dngOpcode_DeltaPerColumn		= 11,
	
	
	
	dngOpcode_ScalePerRow			= 12,
	
	
	
	dngOpcode_ScalePerColumn		= 13,

	
	
	
	

	dngOpcode_WarpRectilinear2		= 14,
	
	};

class dng_opcode
	{
	
	public:

		
	
		enum
			{
			kFlag_None			= 0,	
			kFlag_Optional		= 1,	
			kFlag_SkipIfPreview = 2		
			};
	
	private:
	
		uint32 fOpcodeID;
		
		uint32 fMinVersion;
		
		uint32 fFlags;
		
		bool fWasReadFromStream;
		
		uint32 fStage;
	
	protected:
	
		dng_opcode (uint32 opcodeID,
					uint32 minVersion,
					uint32 flags);
					
		dng_opcode (uint32 opcodeID,
					dng_stream &stream,
					const char *name);

		
		
		
		

		virtual void DoAboutToApply (dng_host & ,
									 dng_negative & ,
									 const dng_rect & ,
									 uint32 )
			{
			}
					
	public:
		
		virtual ~dng_opcode ();

		
		
		uint32 OpcodeID () const
			{
			return fOpcodeID;
			}

		
			
		uint32 MinVersion () const
			{
			return fMinVersion;
			}

		
			
		uint32 Flags () const
			{
			return fFlags;
			}
			
		
			
		bool Optional () const
			{
			return (Flags () & kFlag_Optional) != 0;
			}
			
		
			
		bool SkipIfPreview () const
			{
			return (Flags () & kFlag_SkipIfPreview) != 0;
			}

		
			
		bool WasReadFromStream () const
			{
			return fWasReadFromStream;
			}

		
		
			
		uint32 Stage () const
			{
			return fStage;
			}

		
		
		
		
			
		void SetStage (uint32 stage)
			{
			fStage = stage;
			}

		
		

		virtual bool IsNOP () const
			{
			return false;
			}

		
	
		virtual bool IsValidForNegative (const dng_negative & ) const
			{
			return true;
			}

		
		
	
		virtual void PutData (dng_stream &stream) const;

		
		
		
		
		bool AboutToApply (dng_host &host,
						   dng_negative &negative,
						   const dng_rect &imageBounds,
						   uint32 imagePlanes);

		

		virtual void Apply (dng_host &host,
							dng_negative &negative,
							AutoPtr<dng_image> &image) = 0;

		
		
		
		

		virtual void ApplyAreaScale (const dng_urational & )
			{
			}
		
	};

class dng_opcode_Unknown: public dng_opcode
	{
	
	private:
	
		AutoPtr<dng_memory_block> fData;
	
	public:
	
		dng_opcode_Unknown (dng_host &host,
							uint32 opcodeID,
							dng_stream &stream);
	
		virtual void PutData (dng_stream &stream) const;

		virtual void Apply (dng_host &host,
							dng_negative &negative,
							AutoPtr<dng_image> &image);

	};

class dng_filter_opcode: public dng_opcode
	{
	
	protected:
	
		dng_filter_opcode (uint32 opcodeID,
						   uint32 minVersion,
						   uint32 flags);
					
		dng_filter_opcode (uint32 opcodeID,
						   dng_stream &stream,
						   const char *name);
					
	public:
	
		

		virtual uint32 BufferPixelType (uint32 imagePixelType)
			{
			return imagePixelType;
			}
	
		
		

		virtual dng_rect ModifiedBounds (const dng_rect &imageBounds)
			{
			return imageBounds;
			}
			
		

		virtual dng_point SrcRepeat ()
			{
			return dng_point (1, 1);
			}
	
		
		
		
		
		
		

		virtual dng_rect SrcArea (const dng_rect &dstArea,
								  const dng_rect &imageBounds)
			{
			(void) imageBounds;
			return dstArea;
			}

		
		
		
		
		
		
		
		
		
		

		virtual dng_point SrcTileSize (const dng_point &dstTileSize,
									   const dng_rect &imageBounds)
			{
			return SrcArea (dng_rect (dstTileSize),
							imageBounds).Size ();
			}

		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		

		virtual void Prepare (dng_negative &negative,
							  uint32 threadCount,
							  const dng_point &tileSize,
							  const dng_rect &imageBounds,
							  uint32 imagePlanes,
							  uint32 bufferPixelType,
							  dng_memory_allocator &allocator)
			{
			(void) negative;
			(void) threadCount;
			(void) tileSize;
			(void) imageBounds;
			(void) imagePlanes;
			(void) bufferPixelType;
			(void) allocator;
			}

		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		

		virtual void ProcessArea (dng_negative &negative,
								  uint32 threadIndex,
								  dng_pixel_buffer &srcBuffer,
								  dng_pixel_buffer &dstBuffer,
								  const dng_rect &dstArea,
								  const dng_rect &imageBounds) = 0;

		virtual void Apply (dng_host &host,
							dng_negative &negative,
							AutoPtr<dng_image> &image);
		
	};

class dng_inplace_opcode: public dng_opcode
	{
	
	protected:
	
		dng_inplace_opcode (uint32 opcodeID,
							uint32 minVersion,
							uint32 flags);
					
		dng_inplace_opcode (uint32 opcodeID,
							dng_stream &stream,
							const char *name);
					
	public:
	
		

		virtual uint32 BufferPixelType (uint32 imagePixelType)
			{
			return imagePixelType;
			}
			
		
		

		virtual dng_rect ModifiedBounds (const dng_rect &imageBounds)
			{
			return imageBounds;
			}
	
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		

		virtual void Prepare (dng_negative &negative,
							  uint32 threadCount,
							  const dng_point &tileSize,
							  const dng_rect &imageBounds,
							  uint32 imagePlanes,
							  uint32 bufferPixelType,
							  dng_memory_allocator &allocator)
			{
			(void) negative;
			(void) threadCount;
			(void) tileSize;
			(void) imageBounds;
			(void) imagePlanes;
			(void) bufferPixelType;
			(void) allocator;
			}

		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		

		virtual void ProcessArea (dng_negative &negative,
								  uint32 threadIndex,
								  dng_pixel_buffer &buffer,
								  const dng_rect &dstArea,
								  const dng_rect &imageBounds) = 0;

		virtual void Apply (dng_host &host,
							dng_negative &negative,
							AutoPtr<dng_image> &image);
		
	};

#endif
	

