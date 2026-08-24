

#ifndef __dng_color_space__
#define __dng_color_space__

#include "dng_1d_function.h"
#include "dng_classes.h"
#include "dng_matrix.h"
#include "dng_types.h"

class dng_function_GammaEncode_sRGB: public dng_1d_function
	{
	
	public:
	
		virtual real64 Evaluate (real64 x) const;
			
		virtual real64 EvaluateInverse (real64 y) const;
	
		static const dng_1d_function & Get ();
	
	};

class dng_function_GammaEncode_1_8: public dng_1d_function
	{
	
	public:
	
		virtual real64 Evaluate (real64 x) const;
			
		virtual real64 EvaluateInverse (real64 y) const;
	
		static const dng_1d_function & Get ();
	
	};
			

class dng_function_GammaEncode_2_2: public dng_1d_function
	{
	
	public:
	
		virtual real64 Evaluate (real64 x) const;
			
		virtual real64 EvaluateInverse (real64 y) const;
	
		static const dng_1d_function & Get ();
	
	};

class dng_function_GammaEncode_TwoPart: public dng_1d_function
	{

	protected:

		real64 fAlpha = 1.0;
		real64 fBeta  = 0.0;
		real64 fSlope = 4.5;
		real64 fGamma = 1.0;
	
	public:
	
		real64 Evaluate (real64 x) const override
			{

			if (x <= fBeta)
				return fSlope * x;

			return fAlpha * pow (x, fGamma) - (fAlpha - 1.0);

			}
			
		real64 EvaluateInverse (real64 y) const override
			{

			if (y <= fSlope * fBeta)
				return y / fSlope;

			else
				return pow ((y + (fAlpha - 1.0)) / fAlpha, 1.0 / fGamma);

			}
	
	protected:

		dng_function_GammaEncode_TwoPart ()
			{
			}

	};

class dng_function_GammaEncode_Rec709: public dng_function_GammaEncode_TwoPart
	{

	public:

		dng_function_GammaEncode_Rec709 ()
			{
			fAlpha = 1.0992968268094429;
			fBeta  = 0.0180539685108078;
			fSlope = 4.5;
			fGamma = 0.45;
			}
	
		static const dng_1d_function & Get ()
			{

			static dng_function_GammaEncode_Rec709 static_function;

			return static_function;

			}
	
	};

typedef dng_function_GammaEncode_Rec709 dng_function_GammaEncode_Rec2020;
			

class dng_color_space
	{
	
	protected:
	
		dng_matrix fMatrixToPCS;
		
		dng_matrix fMatrixFromPCS;
		
	public:
	
		virtual ~dng_color_space ();
	
		
		

		const dng_matrix & MatrixToPCS () const
			{
			return fMatrixToPCS;
			}
		
		
		

		const dng_matrix & MatrixFromPCS () const
			{
			return fMatrixFromPCS;
			}

		
		

		bool IsMonochrome () const
			{
			return fMatrixToPCS.Cols () == 1;
			}
		
		

		virtual const dng_1d_function & GammaFunction () const;

		

		bool IsLinear () const
			{
			return GammaFunction ().IsIdentity ();
			}

		

		real64 GammaEncode (real64 x) const
			{
			return GammaFunction ().Evaluate (x);
			}

		
		

		real64 GammaDecode (real64 y) const
			{
			return GammaFunction ().EvaluateInverse (y);
			}
			
		
		
		
		

		virtual bool ICCProfile (uint32 &size,
								 const uint8 *&data) const;
								 
	protected:
	
		dng_color_space ();
			
		void SetMonochrome ();
		
		void SetMatrixToPCS (const dng_matrix_3by3 &M);
		
	};

class dng_space_sRGB: public dng_color_space
	{
	
	protected:
	
		dng_space_sRGB ();
		
	public:

		

		virtual const dng_1d_function & GammaFunction () const;

		

		virtual bool ICCProfile (uint32 &size,
								 const uint8 *&data) const;

		

		static const dng_color_space & Get ();
	
	};

class dng_space_AdobeRGB: public dng_color_space
	{
	
	protected:
	
		dng_space_AdobeRGB ();
		
	public:
	
		

		virtual const dng_1d_function & GammaFunction () const;

		

		virtual bool ICCProfile (uint32 &size,
								 const uint8 *&data) const;
		
		

		static const dng_color_space & Get ();
	
	};

class dng_space_ColorMatch: public dng_color_space
	{
	
	protected:
	
		dng_space_ColorMatch ();
		
	public:
	
		

		virtual const dng_1d_function & GammaFunction () const;
		
		

		virtual bool ICCProfile (uint32 &size,
								 const uint8 *&data) const;
		
		

		static const dng_color_space & Get ();
	
	};

class dng_space_DisplayP3: public dng_color_space
	{
	
	protected:
	
		dng_space_DisplayP3 ();
		
	public:

		virtual bool ICCProfile (uint32 &size,
								 const uint8 *&data) const;
								 
		virtual const dng_1d_function & GammaFunction () const;
	
		static const dng_color_space & Get ();
	
	};

class dng_space_Rec2020: public dng_color_space
	{
	
	protected:
	
		dng_space_Rec2020 ();
		
	public:

		virtual bool ICCProfile (uint32 &size,
								 const uint8 *&data) const;
								 
		virtual const dng_1d_function & GammaFunction () const;
	
		static const dng_color_space & Get ();
	
	};

class dng_space_ProPhoto: public dng_color_space
	{
	
	protected:
	
		dng_space_ProPhoto ();
		
	public:
	
		

		virtual const dng_1d_function & GammaFunction () const;
		
		

		virtual bool ICCProfile (uint32 &size,
								 const uint8 *&data) const;
		
		

		static const dng_color_space & Get ();
	
	};

class dng_space_GrayGamma18: public dng_color_space
	{
	
	protected:
	
		dng_space_GrayGamma18 ();
		
	public:
	
		

		virtual const dng_1d_function & GammaFunction () const;
		
		

		virtual bool ICCProfile (uint32 &size,
								 const uint8 *&data) const;
		
		

		static const dng_color_space & Get ();
	
	};

class dng_space_GrayGamma22: public dng_color_space
	{
	
	protected:
	
		dng_space_GrayGamma22 ();
		
	public:

		

		virtual const dng_1d_function & GammaFunction () const;
		
		

		virtual bool ICCProfile (uint32 &size,
								 const uint8 *&data) const;
		
		

		static const dng_color_space & Get ();
	
	};

class dng_space_sRGB_Linear: public dng_color_space
	{
	
	protected:
	
		dng_space_sRGB_Linear ();
		
	public:
	
		virtual bool ICCProfile (uint32 &size,
								 const uint8 *&data) const;
								 
		static const dng_color_space & Get ();
	
	};

class dng_space_AdobeRGB_Linear: public dng_color_space
	{
	
	protected:
	
		dng_space_AdobeRGB_Linear ();
		
	public:
	
		virtual bool ICCProfile (uint32 &size,
								 const uint8 *&data) const;
								 
		static const dng_color_space & Get ();
	
	};

class dng_space_ProPhoto_Linear: public dng_color_space
	{
	
	protected:
	
		dng_space_ProPhoto_Linear ();
		
	public:
	
		virtual bool ICCProfile (uint32 &size,
								 const uint8 *&data) const;
								 
		static const dng_color_space & Get ();
	
	};

class dng_space_Gray_Linear: public dng_color_space
	{
	
	protected:
	
		dng_space_Gray_Linear ();
		
	public:
	
		virtual bool ICCProfile (uint32 &size,
								 const uint8 *&data) const;
								 
		static const dng_color_space & Get ();
	
	};

class dng_space_LinearP3: public dng_color_space
	{
	
	protected:
	
		dng_space_LinearP3 ();
		
	public:

		virtual bool ICCProfile (uint32 &size,
								 const uint8 *&data) const;
								 
		static const dng_color_space & Get ();
	
	};

class dng_space_Rec2020_Linear: public dng_color_space
	{
	
	protected:
	
		dng_space_Rec2020_Linear ();
		
	public:

		virtual bool ICCProfile (uint32 &size,
								 const uint8 *&data) const;
								 
		static const dng_color_space & Get ();
	
	};

class dng_space_fakeRGB: public dng_color_space
	{
	
	protected:
	
		dng_space_fakeRGB ();
		
	public:

		static const dng_color_space & Get ();
	
	};

#endif

