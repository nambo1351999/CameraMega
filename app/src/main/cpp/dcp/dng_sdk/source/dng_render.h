

#ifndef __dng_render__
#define __dng_render__

#include "dng_1d_function.h"
#include "dng_auto_ptr.h"
#include "dng_big_table.h"
#include "dng_camera_profile.h"
#include "dng_classes.h"
#include "dng_matrix.h"
#include "dng_spline.h"
#include "dng_uncopyable.h"
#include "dng_xy_coord.h"

class dng_function_zero_offset: public dng_1d_function
	{
	
	public:
	
		real64 fZeroOffset;
		
		real64 fScale;
		
	public:
		
		dng_function_zero_offset (real64 zeroOffset);
		
		virtual real64 Evaluate (real64 x) const;

	};

class dng_function_exposure_ramp: public dng_1d_function
	{
	
	public:
	
		real64 fSlope;		
		
		real64 fBlack;		
		
		real64 fRadius;		
		
		real64 fQScale;		

		const bool fSupportOverrange = false;
		
	public:
		
		dng_function_exposure_ramp (real64 white,
									real64 black,
									real64 minBlack,
									bool supportOverrange);
			
		virtual real64 Evaluate (real64 x) const;

	};
			

class dng_function_exposure_tone: public dng_1d_function
	{
	
	protected:
	
		bool fIsNOP;		
		
		real64 fSlope;		
		
		real64 a;			
		real64 b;
		real64 c;
	
	public:
	
		dng_function_exposure_tone (real64 exposure);
				
		

		virtual real64 Evaluate (real64 x) const;
	
	};
	

class dng_tone_curve_acr3_default: public dng_1d_function
	{
	
	public:
		
		

		virtual real64 Evaluate (real64 x) const;
		
		

		virtual real64 EvaluateInverse (real64 x) const;
		
		static const dng_1d_function & Get ();

	};
			

class dng_function_gamma_encode: public dng_1d_function
	{
	
	protected:
	
		const dng_color_space &fSpace;
	
	public:
	
		dng_function_gamma_encode (const dng_color_space &space);
		
		virtual real64 Evaluate (real64 x) const;
		
	};

class dng_render: private dng_uncopyable
	{
	
	protected:
	
		dng_host &fHost;
	
		const dng_negative &fNegative;
	
		dng_xy_coord fWhiteXY;
		
		real64 fExposure;
		
		real64 fShadows;
		
		const dng_1d_function *fToneCurve;
		
		const dng_color_space *fFinalSpace;
		
		uint32 fFinalPixelType;
		
		uint32 fMaximumSize;

		

		dng_camera_profile_id fProfileID;
		
	private:
	
		AutoPtr<dng_spline_solver> fProfileToneCurve;
		
	public:
	
		
		
		

		dng_render (dng_host &host,
					const dng_negative &negative);
		
		virtual ~dng_render ()
			{
			}
		
		
		

		void SetWhiteXY (const dng_xy_coord &white)
			{
			fWhiteXY = white;
			}
			
		
		

		const dng_xy_coord WhiteXY () const
			{
			return fWhiteXY;
			}
			
		
		

		void SetExposure (real64 exposure)
			{
			fExposure = exposure;
			}
			
		
		

		real64 Exposure () const
			{
			return fExposure;
			}
			
		
		

		void SetShadows (real64 shadows)
			{
			fShadows = shadows;
			}
			
		
		

		real64 Shadows () const
			{
			return fShadows;
			}
			
		
		
	
		void SetToneCurve (const dng_1d_function &curve)
			{
			fToneCurve = &curve;
			}
			
		
		

		const dng_1d_function & ToneCurve () const
			{
			return *fToneCurve;
			}

		
		
		

		void SetFinalSpace (const dng_color_space &space)
			{
			fFinalSpace = &space;
			}
			
		
		

		const dng_color_space & FinalSpace (const dng_camera_profile *profile) const;
			
		
		
		

		void SetFinalPixelType (uint32 type)
			{
			fFinalPixelType = type;
			}
			
		
		
		

		uint32 FinalPixelType () const
			{
			return fFinalPixelType;
			}

		
		
		
		
		

		void SetMaximumSize (uint32 size)
			{
			fMaximumSize = size;
			}
			
		
		
		
		
		

		uint32 MaximumSize () const
			{
			return fMaximumSize;
			}

		

		void SetCameraProfileID (const dng_camera_profile_id &id)
			{
			fProfileID = id;
			}

		

		const dng_camera_profile_id & CameraProfileID  () const
			{
			return fProfileID;
			}

		
		
		

		virtual dng_image * Render ();
									
	};

#endif	
	

