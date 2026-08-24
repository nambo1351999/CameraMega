

#ifndef __dng_1d_function__
#define __dng_1d_function__

#include "dng_classes.h"
#include "dng_types.h"

#include <vector>

class dng_1d_function
	{
	
	public:
	
		virtual ~dng_1d_function ();

		
		

		virtual bool IsIdentity () const;

		
		
		
		
		

		virtual real64 Evaluate (real64 x) const = 0;

		
		
		
		
		
		

		virtual real64 EvaluateInverse (real64 y) const;
	
	};

class dng_1d_identity: public dng_1d_function
	{
	
	public:
		

		virtual bool IsIdentity () const;
	
		

		virtual real64 Evaluate (real64 x) const;
		
		

		virtual real64 EvaluateInverse (real64 y) const;

		
		

		static const dng_1d_function & Get ();
	
	};

class dng_1d_concatenate: public dng_1d_function
	{
	
	protected:
	
		const dng_1d_function &fFunction1;
		
		const dng_1d_function &fFunction2;
	
	public:
	
		
		
		
		
		
		
		
		
		
		

		dng_1d_concatenate (const dng_1d_function &function1,
							const dng_1d_function &function2);
	
		

		virtual bool IsIdentity () const;
	
		
		
		

		virtual real64 Evaluate (real64 x) const;
	
		
		
		
		
		
		

		virtual real64 EvaluateInverse (real64 y) const;
	
	};

class dng_1d_inverse: public dng_1d_function
	{
	
	protected:
	
		const dng_1d_function &fFunction;
	
	public:
	
		dng_1d_inverse (const dng_1d_function &f);
	
		virtual bool IsIdentity () const;
	
		virtual real64 Evaluate (real64 x) const;
	
		virtual real64 EvaluateInverse (real64 y) const;
	
	};

class dng_piecewise_linear: public dng_1d_function
	{
		
	public:

		std::vector<real64> X;
		std::vector<real64> Y;
		
	public:

		dng_piecewise_linear ();

		virtual ~dng_piecewise_linear ();
	
		void Reset ();

		void Add (real64 x, real64 y);

		bool IsValid () const
			{
			return (X.size () >= 2) && (X.size () == Y.size ());
			}

		bool NotValid () const
			{
			return !IsValid ();
			}

		virtual bool IsIdentity () const;

		virtual real64 Evaluate (real64 x) const;
		
		virtual real64 EvaluateInverse (real64 y) const;

		void PutFingerprintData (dng_stream &stream) const;

		bool operator== (const dng_piecewise_linear &piecewise) const;
		
		bool operator!= (const dng_piecewise_linear &piecewise) const
			{
			return !(*this == piecewise);
			}

	};

#endif
	

