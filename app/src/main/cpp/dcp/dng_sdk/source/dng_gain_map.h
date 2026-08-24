

#ifndef __dng_gain_map__
#define __dng_gain_map__

#include "dng_classes.h"
#include "dng_fingerprint.h"
#include "dng_memory.h"
#include "dng_misc_opcodes.h"
#include "dng_safe_arithmetic.h"
#include "dng_tag_types.h"
#include "dng_uncopyable.h"

class dng_gain_map: private dng_uncopyable
	{
	
	private:
	
		dng_point fPoints;
		
		dng_point_real64 fSpacing;
		
		dng_point_real64 fOrigin;
		
		uint32 fPlanes;
		
		uint32 fRowStep;
		
		AutoPtr<dng_memory_block> fBuffer;
		
	public:
	
		
		

		dng_gain_map (dng_memory_allocator &allocator,
					  const dng_point &points,
					  const dng_point_real64 &spacing,
					  const dng_point_real64 &origin,
					  uint32 planes);

		

		const dng_point & Points () const
			{
			return fPoints;
			}
			
		
		

		const dng_point_real64 & Spacing () const
			{
			return fSpacing;
			}
			
		

		const dng_point_real64 & Origin () const
			{
			return fOrigin;
			}
			
		

		uint32 Planes () const
			{
			return fPlanes;
			}

		

		real32 & Entry (uint32 rowIndex,
						uint32 colIndex,
						uint32 plane)
			{
			
			return *(fBuffer->Buffer_real32 () +
					 SafeUint32Add (
						SafeUint32Add (
							SafeUint32Mult (rowIndex, fRowStep),
							SafeUint32Mult (colIndex, fPlanes)),
						plane));
			
			}
			
		
		

		const real32 & Entry (uint32 rowIndex,
							  uint32 colIndex,
							  uint32 plane) const
			{
			
			return *(fBuffer->Buffer_real32 () +
					 SafeUint32Add (
						SafeUint32Add (
							SafeUint32Mult (rowIndex, fRowStep),
							SafeUint32Mult (colIndex, fPlanes)),
						plane));
			
			}
			
		
		

		real32 Interpolate (int32 row,
							int32 col,
							uint32 plane,
							const dng_rect &bounds) const;
							
		

		uint32 PutStreamSize () const;
		
		

		void PutStream (dng_stream &stream) const;
		
		

		static dng_gain_map * GetStream (dng_host &host,
										 dng_stream &stream);

	};

class dng_gain_table_map: private dng_uncopyable
	{
	
	private:
	
		dng_point fPoints;					 
		
		dng_point_real64 fSpacing;			 
		
		dng_point_real64 fOrigin;			 

		uint32 fNumTablePoints = 0;			 
		
		uint32 fRowStep = 0;
		uint32 fColStep = 0;

		uint32 fNumSamples = 0;

		uint32 fSampleBytes = 0;

		real32 fMapInputWeights [5];		 
		
		AutoPtr<dng_memory_block> fBuffer;

		mutable dng_fingerprint fFingerprint;

		

		
		
		
		
		
		

		uint32 fDataType = 3;

		

		real32 fGamma = 1.0f;

		

		real32 fGainMin = 1.0f;
		real32 fGainMax = 1.0f;

		
		

		AutoPtr<dng_memory_block> fOriginalBuffer;

	public:
	
		
		
		

		dng_gain_table_map (dng_memory_allocator &allocator,
							const dng_point &points,
							const dng_point_real64 &spacing,
							const dng_point_real64 &origin,
							uint32 numTablePoints,
							const real32 weights [5],
							uint32 dataType = 2,
							real32 gamma = 1.0f,
							real32 gainMin = 1.0f,
							real32 gainMax = 1.0f);

		

		const dng_point & Points () const
			{
			return fPoints;
			}
			
		
		

		const dng_point_real64 & Spacing () const
			{
			return fSpacing;
			}
			
		

		const dng_point_real64 & Origin () const
			{
			return fOrigin;
			}

		

		uint32 NumTablePoints () const
			{
			return fNumTablePoints;
			}

		
		

		uint32 NumSamples () const
			{
			return fNumSamples;
			}
		
		

		uint32 SampleBytes () const
			{
			return fSampleBytes;
			}
		
		

		const real32 * MapInputWeights () const
			{
			return fMapInputWeights;
			}
			
		
		

		real32 & Entry (uint32 rowIndex,
						uint32 colIndex,
						uint32 tableIndex)
			{
			
			return *(fBuffer->Buffer_real32 () +
					 SafeUint32Add (
						SafeUint32Add (
							SafeUint32Mult (rowIndex, fRowStep),
							SafeUint32Mult (colIndex, fColStep)),
						tableIndex));
			
			}
			
		
		

		const real32 & Entry (uint32 rowIndex,
							  uint32 colIndex,
							  uint32 tableIndex) const
			{
			
			return *(fBuffer->Buffer_real32 () +
					 SafeUint32Add (
						SafeUint32Add (
							SafeUint32Mult (rowIndex, fRowStep),
							SafeUint32Mult (colIndex, fColStep)),
						tableIndex));
			
			}

		

		uint32 RowStep () const
			{
			return fRowStep;
			}
							
		uint32 ColStep () const
			{
			return fColStep;
			}

		

		dng_memory_block * Block ()
			{
			return fBuffer.Get ();
			}
							
		const dng_memory_block * Block () const
			{
			return fBuffer.Get ();
			}

		

		void * RawTablePtr () const;

		uint32 RawTableNumBytes () const;

		

		uint32 PutStreamSize () const;
		
		

		void PutStream (dng_stream &stream,
						bool forceVersion2 = false) const;

		

		void AddDigest (dng_md5_printer_stream &printer) const;

		

		dng_fingerprint GetFingerprint () const;

		

		static dng_gain_table_map * GetStream (dng_host &host,
											   dng_stream &stream,
											   bool useVersion2,
											   uint32 tagByteCount);

		

		uint32 DataType () const
			{
			return fDataType;
			}

		bool IsFloat16 () const
			{
			return fDataType == 2;
			}

		bool IsFloat32 () const
			{
			return fDataType >= 3;
			}

		bool IsUint8 () const
			{
			return fDataType == 0;
			}

		bool IsUint16 () const
			{
			return fDataType == 1;
			}

		real32 GainMin () const
			{
			return fGainMin;
			}

		real32 GainMax () const
			{
			return fGainMax;
			}

		real32 Gamma () const
			{
			return fGamma;
			}
		
		bool SupportsVersion1 () const;
		
		bool RequiresVersion2 () const
			{
			return !SupportsVersion1 ();
			}

		uint32 BytesPerEntry () const
			{
			if (fDataType == 0) return 1;	 
			if (fDataType <= 2) return 2;	 
			return 4;						 
			}

		
		

		uint32 DataStorageBytes () const;

		void ClearOriginalBuffer ();

		bool HasOriginalBuffer () const;

		const dng_memory_block * OriginalBuffer () const;

		void SetOriginalBuffer (AutoPtr<dng_memory_block> &blockToRelease);

	private:

		

		void EnsureFingerprint () const;

	};

class dng_opcode_GainMap: public dng_inplace_opcode,
						  private dng_uncopyable
	{
	
	private:
	
		dng_area_spec fAreaSpec;
	
		AutoPtr<dng_gain_map> fGainMap;
	
	public:
	
		
		

		dng_opcode_GainMap (const dng_area_spec &areaSpec,
							AutoPtr<dng_gain_map> &gainMap);
	
		

		dng_opcode_GainMap (dng_host &host,
							dng_stream &stream);

		

		const dng_area_spec& AreaSpec() const { return fAreaSpec; }
		const dng_gain_map& GainMap() const { return *fGainMap; }
	
		

		virtual void PutData (dng_stream &stream) const override;
		
		

		virtual uint32 BufferPixelType (uint32 ) override
			{
			return ttFloat;
			}
	
		
		

		virtual dng_rect ModifiedBounds (const dng_rect &imageBounds) override
			{
			return fAreaSpec.ScaledOverlap (imageBounds);
			}
	
		

		virtual void ProcessArea (dng_negative &negative,
								  uint32 threadIndex,
								  dng_pixel_buffer &buffer,
								  const dng_rect &dstArea,
								  const dng_rect &imageBounds) override;

		void ApplyAreaScale (const dng_urational &scale) override
			{
			fAreaSpec.ApplyAreaScale (scale);
			}
		
	};
	

#endif
	

