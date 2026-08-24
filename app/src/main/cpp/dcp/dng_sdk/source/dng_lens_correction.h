

#ifndef __dng_lens_correction__
#define __dng_lens_correction__

#include "dng_1d_function.h"
#include "dng_matrix.h"
#include "dng_memory.h"
#include "dng_opcodes.h"
#include "dng_pixel_buffer.h"
#include "dng_point.h"
#include "dng_resample.h"
#include "dng_sdk_limits.h"

class dng_warp_params
	{

	public:

		
		
		
		
		

		uint32 fPlanes;

		
		
		
		
		
		

		dng_point_real64 fCenter;
		
	public:

		

		dng_warp_params ();

		
		
		
		
		
		
		
		

		dng_warp_params (uint32 planes,
						 const dng_point_real64 &fCenter);

		virtual ~dng_warp_params ();

		

		virtual bool IsNOPAll () const;

		

		virtual bool IsNOP (uint32 plane) const;

		

		virtual bool IsRadNOPAll () const;

		

		virtual bool IsRadNOP (uint32 plane) const;

		

		virtual bool IsTanNOPAll () const;

		

		virtual bool IsTanNOP (uint32 plane) const;

		

		virtual bool IsValid () const;

		

		virtual bool IsValidForNegative (const dng_negative &negative) const;

		

		virtual void PropagateToAllPlanes (uint32 totalPlanes) = 0;

		
		
		
		
		

		virtual real64 Evaluate (uint32 plane,
								 real64 r) const = 0;

		
		
		
		
		
		

		virtual real64 EvaluateInverse (uint32 plane,
										real64 r) const;

		
		
		
		
		
		
		
		
		

		virtual real64 EvaluateRatio (uint32 plane,
									  real64 r2) const = 0;

		
		
		
		
		
		
		
		
		
		

		virtual dng_point_real64 EvaluateTangential (uint32 plane,
													 real64 r2,
													 const dng_point_real64 &diff,
													 const dng_point_real64 &diff2) const = 0;
		
		
		
		
		
		

		dng_point_real64 EvaluateTangential2 (uint32 plane,
											  const dng_point_real64 &diff) const;
		
		
		
		
		
		
		
		
		

		dng_point_real64 EvaluateTangential3 (uint32 plane,
											  real64 r2,
											  const dng_point_real64 &diff) const;

		
		
		
		
		
		
		
		
		

		virtual real64 MaxSrcRadiusGap (real64 maxDstGap) const = 0;

		
		
		
		
		
		

		virtual dng_point_real64 MaxSrcTanGap (dng_point_real64 minDst,
											   dng_point_real64 maxDst) const = 0;

		
		

		virtual real64 SafeMinRatio () const = 0;

		
		

		virtual real64 SafeMaxRatio () const = 0;

		

		virtual void Dump () const;

	};

class dng_warp_params_radial
	{

	public:

		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		

		static const uint32 kMaxTerms = 15;
		
		real64 fData [kMaxColorPlanes] [kMaxTerms];

		
		

		real64 fValidRange [kMaxColorPlanes] [2];

		bool fUseReciprocal = false;

	public:

		dng_warp_params_radial ();

		bool IsValid (uint32 plane) const;

		bool IsNOP (uint32 plane) const;

		void SetNOP ();

		void SetNOP (uint32 plane);

		
		

		void SetWarpRectilinear_1_3 (uint32 plane,
									 const dng_vector &params);

		
		

		bool CompatibleWithWarpRectilinear_1_3 (uint32 plane) const;

		
		
		
		
		
		
		real64 Evaluate (uint32 plane,
						 real64 r) const;

		real64 EvaluateRatio (uint32 plane,
							  real64 r2) const;

	};

class dng_warp_params_rectilinear: public dng_warp_params
	{

	public:

		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		

		dng_warp_params_radial fRadParams;
		
		dng_vector fTanParams [kMaxColorPlanes];

	public:

		

		dng_warp_params_rectilinear ();

		
		
		

		dng_warp_params_rectilinear (uint32 planes,
									 const dng_warp_params_radial &radParams,
									 const dng_vector tanParams [],
									 const dng_point_real64 &fCenter);

		virtual ~dng_warp_params_rectilinear ();

		

		virtual bool IsRadNOP (uint32 plane) const;

		virtual bool IsTanNOP (uint32 plane) const;

		virtual bool IsValid () const;

		virtual void PropagateToAllPlanes (uint32 totalPlanes);

		virtual real64 Evaluate (uint32 plane,
								 real64 r) const;

		virtual real64 EvaluateRatio (uint32 plane,
									  real64 r2) const;

		virtual dng_point_real64 EvaluateTangential (uint32 plane,
													 real64 r2,
													 const dng_point_real64 &diff,
													 const dng_point_real64 &diff2) const;

		virtual real64 MaxSrcRadiusGap (real64 maxDstGap) const;

		virtual dng_point_real64 MaxSrcTanGap (dng_point_real64 minDst,
											   dng_point_real64 maxDst) const;

		virtual real64 SafeMinRatio () const;

		virtual real64 SafeMaxRatio () const;

		virtual void Dump () const;

	};

class dng_warp_params_fisheye: public dng_warp_params
	{

	public:

		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		

		dng_vector fRadParams [kMaxColorPlanes];

	public:

		

		dng_warp_params_fisheye ();

		
		
		

		dng_warp_params_fisheye (uint32 planes,
								 const dng_vector radParams [],
								 const dng_point_real64 &fCenter);

		virtual ~dng_warp_params_fisheye ();

		

		virtual bool IsRadNOP (uint32 plane) const;

		virtual bool IsTanNOP (uint32 plane) const;

		virtual bool IsValid () const;

		virtual void PropagateToAllPlanes (uint32 totalPlanes);

		virtual real64 Evaluate (uint32 plane,
								 real64 r) const;

		virtual real64 EvaluateRatio (uint32 plane,
									  real64 r2) const;

		virtual dng_point_real64 EvaluateTangential (uint32 plane,
													 real64 r2,
													 const dng_point_real64 &diff,
													 const dng_point_real64 &diff2) const;
		
		virtual real64 MaxSrcRadiusGap (real64 maxDstGap) const;

		virtual dng_point_real64 MaxSrcTanGap (dng_point_real64 minDst,
											   dng_point_real64 maxDst) const;

		virtual real64 SafeMinRatio () const;

		virtual real64 SafeMaxRatio () const;

		virtual void Dump () const;

	};

class dng_opcode_BaseWarpRectilinear: public dng_opcode
	{
		
	protected:

		dng_warp_params_rectilinear fWarpParams;

	public:
	
		

		bool IsNOP () const override;
		
		bool IsValidForNegative (const dng_negative &negative) const override;
	
		void Apply (dng_host &host,
					dng_negative &negative,
					AutoPtr<dng_image> &image) override;

		

		bool HasDistort () const;

		bool HasLateralCA () const;

		const dng_warp_params_rectilinear & Params () const
			{
			return fWarpParams;
			}

	protected:

		

		dng_opcode_BaseWarpRectilinear (uint32 opcodeTag,
										uint32 minVersion,
										const dng_warp_params_rectilinear &params,
										uint32 flags);
		
		dng_opcode_BaseWarpRectilinear (uint32 opcodeTag,
										const char *name,
										dng_stream &stream);
	
	};

class dng_opcode_WarpRectilinear: public dng_opcode_BaseWarpRectilinear
	{
		
	public:
	
		dng_opcode_WarpRectilinear (const dng_warp_params_rectilinear &params,
									uint32 flags);
		
		explicit dng_opcode_WarpRectilinear (dng_stream &stream);
	
		

		void PutData (dng_stream &stream) const override;

	protected:

		static uint32 ParamBytes (uint32 planes);

	};

class dng_opcode_WarpRectilinear2: public dng_opcode_BaseWarpRectilinear
	{
		
	public:
	
		dng_opcode_WarpRectilinear2 (const dng_warp_params_rectilinear &params,
									 uint32 flags);
		
		explicit dng_opcode_WarpRectilinear2 (dng_stream &stream);
	
		

		void PutData (dng_stream &stream) const override;

	protected:

		static uint32 ParamBytes (uint32 planes);

	};

class dng_opcode_WarpFisheye: public dng_opcode
	{
		
	protected:

		dng_warp_params_fisheye fWarpParams;

	public:
	
		dng_opcode_WarpFisheye (const dng_warp_params_fisheye &params,
								uint32 flags);
		
		explicit dng_opcode_WarpFisheye (dng_stream &stream);
	
		

		virtual bool IsNOP () const;
		
		virtual bool IsValidForNegative (const dng_negative &negative) const;
	
		virtual void PutData (dng_stream &stream) const;

		virtual void Apply (dng_host &host,
							dng_negative &negative,
							AutoPtr<dng_image> &image);

	protected:

		static uint32 ParamBytes (uint32 planes);

	};

class dng_vignette_radial_params
	{

	public:

		static const uint32 kNumTerms = 5;

	public:

		
		
		
		
		
		
		
		
		
		
		
		
		

		dng_std_vector<real64> fParams;

		dng_point_real64 fCenter;

	public:

		dng_vignette_radial_params ();

		dng_vignette_radial_params (const dng_std_vector<real64> &params,
									const dng_point_real64 &center);

		dng_vignette_radial_params (const dng_vignette_radial_params &params);

		bool IsNOP () const;

		bool IsValid () const;

		

		void Dump () const;

	};

class dng_opcode_FixVignetteRadial: public dng_inplace_opcode
	{
		
	protected:

		dng_vignette_radial_params fParams;

		uint32 fImagePlanes;

		int64 fSrcOriginH;
		int64 fSrcOriginV;
		
		int64 fSrcStepH;
		int64 fSrcStepV;
		
		uint32 fTableInputBits;
		uint32 fTableOutputBits;

		AutoPtr<dng_memory_block> fGainTable;

		AutoPtr<dng_memory_block> fMaskBuffers [kMaxMPThreads];

	public:
	
		dng_opcode_FixVignetteRadial (const dng_vignette_radial_params &params,
									  uint32 flags);
		
		explicit dng_opcode_FixVignetteRadial (dng_stream &stream);
	
		const dng_vignette_radial_params & Params () const
			{
			return fParams;
			}

		virtual bool IsNOP () const;
		
		virtual bool IsValidForNegative (const dng_negative &) const;
	
		virtual void PutData (dng_stream &stream) const;

		virtual uint32 BufferPixelType (uint32 )
			{
			return ttFloat;
			}
			
		virtual void Prepare (dng_negative &negative,
							  uint32 threadCount,
							  const dng_point &tileSize,
							  const dng_rect &imageBounds,
							  uint32 imagePlanes,
							  uint32 bufferPixelType,
							  dng_memory_allocator &allocator);

		virtual void ProcessArea (dng_negative &negative,
								  uint32 threadIndex,
								  dng_pixel_buffer &buffer,
								  const dng_rect &dstArea,
								  const dng_rect &imageBounds);

	protected:

		static uint32 ParamBytes ();

		virtual dng_vignette_radial_params MakeParamsForRender (const dng_negative &negative);

	};

#endif
	

