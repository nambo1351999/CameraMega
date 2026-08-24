

#ifndef __dng_hue_sat_map__
#define __dng_hue_sat_map__

#include "dng_classes.h"
#include "dng_fingerprint.h"
#include "dng_ref_counted_block.h"
#include "dng_safe_arithmetic.h"
#include "dng_types.h"

#include <atomic>

class dng_hue_sat_map
	{

	public:

		
		
		
		
		
		
		

		struct HSBModify
			{
			real32 fHueShift;
			real32 fSatScale;
			real32 fValScale;
			};

	private:

		uint32 fHueDivisions;
		uint32 fSatDivisions;
		uint32 fValDivisions;
		
		uint32 fHueStep;
		uint32 fValStep;

		
		
		
		
		
		
		

		dng_fingerprint fRuntimeFingerprint;

		static std::atomic<uint64> sRuntimeFingerprintCounter;

		dng_ref_counted_block fDeltas;

		HSBModify *SafeGetDeltas ()
			{
			return (HSBModify *) fDeltas.Buffer_real32 ();
			}

	public:

		

		dng_hue_sat_map ();

		

		dng_hue_sat_map (const dng_hue_sat_map &src);

		

		dng_hue_sat_map & operator= (const dng_hue_sat_map &rhs);

		

		virtual ~dng_hue_sat_map ();

		

		bool IsNull () const
			{
			return !IsValid ();
			}

		

		bool IsValid () const
			{
			
			return fHueDivisions > 0 &&
				   fSatDivisions > 1 &&
				   fValDivisions > 0 &&
				   fDeltas.Buffer ();
				   
			}

		

		void SetInvalid ()
			{

			fHueDivisions = 0;
			fSatDivisions = 0;
			fValDivisions = 0;
			
			fHueStep = 0;
			fValStep = 0;

			fRuntimeFingerprint.Clear ();

			fDeltas.Clear ();

			}

		

		void GetDivisions (uint32 &hueDivisions,
						   uint32 &satDivisions,
						   uint32 &valDivisions) const
			{
			hueDivisions = fHueDivisions;
			satDivisions = fSatDivisions;
			valDivisions = fValDivisions;
			}
			
		
		

		void SetDivisions (uint32 hueDivisions,
						   uint32 satDivisions,
						   uint32 valDivisions = 1);	

		

		void GetDelta (uint32 hueDiv,
					   uint32 satDiv,
					   uint32 valDiv,
					   HSBModify &modify) const;

		

		void EnsureWriteable ()
			{
			fDeltas.EnsureWriteable ();
			}
		
		

		void SetDelta (uint32 hueDiv,
					   uint32 satDiv,
					   uint32 valDiv,
					   const HSBModify &modify)
			{
			
			EnsureWriteable ();
			
			SetDeltaKnownWriteable (hueDiv,
									satDiv,
									valDiv,
									modify);
			
			}
		
		

		void SetDeltaKnownWriteable (uint32 hueDiv,
									 uint32 satDiv,
									 uint32 valDiv,
									 const HSBModify &modify);
		
		

		uint32 DeltasCount () const
			{
			return (dng_safe_uint32 (fValDivisions) *
					dng_safe_uint32 (fHueDivisions) *
					dng_safe_uint32 (fSatDivisions)).Get ();
			}
		
		
		

		HSBModify *GetDeltas ()
			{
				
			EnsureWriteable ();

			return (HSBModify *) fDeltas.Buffer_real32 ();

			}

		
		

		const HSBModify *GetConstDeltas () const
			{
			return (const HSBModify *) fDeltas.Buffer_real32 ();
			}

		void AssignNewUniqueRuntimeFingerprint ();

		

		void SetRuntimeFingerprint (const dng_fingerprint fingerprint)
			{
			fRuntimeFingerprint = fingerprint;
			}

		

		const dng_fingerprint & RuntimeFingerprint () const
			{
			return fRuntimeFingerprint;
			}

		

		bool operator== (const dng_hue_sat_map &rhs) const;
		
		
		
		

		static dng_hue_sat_map * Interpolate (const dng_hue_sat_map &map1,
											  const dng_hue_sat_map &map2,
											  real64 weight1);

		
		
		

		static dng_hue_sat_map * Interpolate (const dng_hue_sat_map &map1,
											  const dng_hue_sat_map &map2,
											  const dng_hue_sat_map &map3,
											  real64 weight1,
											  real64 weight2);

	};

#endif

