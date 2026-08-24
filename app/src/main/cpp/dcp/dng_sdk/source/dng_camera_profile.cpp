

#include "dng_camera_profile.h"

#include "dng_1d_table.h"
#include "dng_assertions.h"
#include "dng_color_space.h"
#include "dng_gain_map.h"
#include "dng_host.h"
#include "dng_exceptions.h"
#include "dng_image_writer.h"
#include "dng_info.h"
#include "dng_parse_utils.h"
#include "dng_safe_arithmetic.h"
#include "dng_shared.h"
#include "dng_tag_codes.h"
#include "dng_tag_types.h"
#include "dng_temperature.h"
#include "dng_xy_coord.h"

const char * kProfileName_Embedded = "Embedded";

const char * kAdobeCalibrationSignature = "com.adobe";

const char * kProfileName_GroupPrefix = "Group: ";

bool HasProfileGroupPrefix (const dng_string &name)
	{
	
	return name.StartsWith (kProfileName_GroupPrefix, true) &&
		   name.Length () > strlen (kProfileName_GroupPrefix);
	
	}

dng_string StripProfileGroupPrefix (const dng_string &name)
	{
	
	if (HasProfileGroupPrefix (name))
		{
		
		dng_string result (name.Get () + strlen (kProfileName_GroupPrefix));
		
		return result;
		
		}
	
	return name;
	
	}

void dng_camera_profile_id::AddDigest (dng_md5_printer &printer) const
	{
	
	printer.ProcessPtr ("DCPI", 4);

	if (Name ().NotEmpty ())
		{
		
		printer.ProcessPtr (Name ().Get (),
							Name ().Length ());
		
		}

	if (Fingerprint ().IsValid ())
		{
		
		printer.Process (fFingerprint);
		
		}
	
	}

dng_camera_profile::dng_camera_profile ()

	:	fName ()
	,	fCalibrationIlluminant1 (lsUnknown)
	,	fCalibrationIlluminant2 (lsUnknown)
	,	fCalibrationIlluminant3 (lsUnknown)
	,	fColorMatrix1 ()
	,	fColorMatrix2 ()
	,	fColorMatrix3 ()
	,	fForwardMatrix1 ()
	,	fForwardMatrix2 ()
	,	fForwardMatrix3 ()
	,	fReductionMatrix1 ()
	,	fReductionMatrix2 ()
	,	fReductionMatrix3 ()
	,	fFingerprint ()
	,	fRenderDataFingerprint ()
	,	fCopyright ()
	,	fEmbedPolicy (pepAllowCopying)
	,	fHueSatDeltas1 ()
	,	fHueSatDeltas2 ()
	,	fHueSatDeltas3 ()
	,	fHueSatMapEncoding (encoding_Linear)
	,	fLookTable ()
	,	fLookTableEncoding (encoding_Linear)
	,	fBaselineExposureOffset (0, 100)
	,	fDefaultBlackRender (defaultBlackRender_Auto)
	,	fToneCurve ()
	,	fToneMethod (profileToneMethod_Unspecified)
	,	fProfileCalibrationSignature ()
	,	fUniqueCameraModelRestriction ()
	,	fWasReadFromDNG (false)
	,	fWasReadFromDisk (false)
	,	fWasStubbed (false)
	
	{

	fToneCurve.SetInvalid ();

	}

dng_camera_profile::~dng_camera_profile ()
	{
	
	}

uint32 dng_camera_profile::IlluminantModel () const
	{

	
	
	uint32 model = 1;

	
	

	if ((CalibrationIlluminant2 () != lsUnknown) && HasColorMatrix2 ())
		{
		
		model = 2;

		
		

		if ((CalibrationIlluminant3 () != lsUnknown) && HasColorMatrix3 ())
			{
			
			model = 3;
			
			}
		
		}

	return model;
	
	}

real64 dng_camera_profile::IlluminantToTemperature (uint32 light,
													const dng_illuminant_data &data)
	{

	switch (light)
		{
		
		case lsStandardLightA:
		case lsTungsten:
			{
			return 2850.0;
			}
			
		case lsISOStudioTungsten:
			{
			return 3200.0;
			}
			
		case lsD50:
			{
			return 5000.0;
			}
			
		case lsD55:
		case lsDaylight:
		case lsFineWeather:
		case lsFlash:
		case lsStandardLightB:
			{
			return 5500.0;
			}
			
		case lsD65:
		case lsStandardLightC:
		case lsCloudyWeather:
			{
			return 6500.0;
			}
			
		case lsD75:
		case lsShade:
			{
			return 7500.0;
			}
			
		case lsDaylightFluorescent:
			{
			return (5700.0 + 7100.0) * 0.5;
			}
			
		case lsDayWhiteFluorescent:
			{
			return (4600.0 + 5500.0) * 0.5;
			}
			
		case lsCoolWhiteFluorescent:
		case lsFluorescent:
			{
			return (3800.0 + 4500.0) * 0.5;
			}
			
		case lsWhiteFluorescent:
			{
			return (3250.0 + 3800.0) * 0.5;
			}
			
		case lsWarmWhiteFluorescent:
			{
			return (2600.0 + 3250.0) * 0.5;
			}

		case lsOther:
			{
			return dng_temperature (data.WhiteXY ()).Temperature ();
			}
			
		default:
			{
			return 0.0;
			}
			
		}
	
	}

void dng_camera_profile::NormalizeColorMatrix (dng_matrix &m)
	{
	
	if (m.NotEmpty ())
		{
	
		
		
		dng_vector coord = m * PCStoXYZ ();
		
		real64 maxCoord = coord.MaxEntry ();
		
		if (maxCoord > 0.0 && (maxCoord < 0.99 || maxCoord > 1.01))
			{
			
			m.Scale (1.0 / maxCoord);
			
			}
			
		
		
		m.Round (10000);
		
		}
			
	}

void dng_camera_profile::SetColorMatrix1 (const dng_matrix &m)
	{
	
	fColorMatrix1 = m;
	
	NormalizeColorMatrix (fColorMatrix1);

	ClearFingerprint ();
	
	}

void dng_camera_profile::SetColorMatrix2 (const dng_matrix &m)
	{
	
	fColorMatrix2 = m;
	
	NormalizeColorMatrix (fColorMatrix2);
	
	ClearFingerprint ();

	}
		

void dng_camera_profile::SetColorMatrix3 (const dng_matrix &m)
	{
	
	fColorMatrix3 = m;
	
	NormalizeColorMatrix (fColorMatrix3);
	
	ClearFingerprint ();

	}
		

void dng_camera_profile::NormalizeForwardMatrix (dng_matrix &m)
	{
	
	if (m.NotEmpty ())
		{
		
		dng_vector cameraOne;
		
		cameraOne.SetIdentity (m.Cols ());
		
		dng_vector xyz = m * cameraOne;
		
		m = PCStoXYZ ().AsDiagonal () *
			Invert (xyz.AsDiagonal ()) *
			m;
		
		}
	
	}

void dng_camera_profile::SetForwardMatrix1 (const dng_matrix &m)
	{
	
	fForwardMatrix1 = m;
	
	fForwardMatrix1.Round (10000);
	
	ClearFingerprint ();
	
	}

void dng_camera_profile::SetForwardMatrix2 (const dng_matrix &m)
	{
	
	fForwardMatrix2 = m;
	
	fForwardMatrix2.Round (10000);
	
	ClearFingerprint ();
	
	}

void dng_camera_profile::SetForwardMatrix3 (const dng_matrix &m)
	{
	
	fForwardMatrix3 = m;
	
	fForwardMatrix3.Round (10000);
	
	ClearFingerprint ();
	
	}

void dng_camera_profile::SetReductionMatrix1 (const dng_matrix &m)
	{
	
	fReductionMatrix1 = m;
	
	fReductionMatrix1.Round (10000);
	
	ClearFingerprint ();

	}

void dng_camera_profile::SetReductionMatrix2 (const dng_matrix &m)
	{
	
	fReductionMatrix2 = m;
	
	fReductionMatrix2.Round (10000);
	
	ClearFingerprint ();

	}

void dng_camera_profile::SetReductionMatrix3 (const dng_matrix &m)
	{
	
	fReductionMatrix3 = m;
	
	fReductionMatrix3.Round (10000);
	
	ClearFingerprint ();

	}

bool dng_camera_profile::HasColorMatrix1 () const
	{
	
	return fColorMatrix1.Cols () == 3 &&
		   fColorMatrix1.Rows ()  > 1;
	
	}
		

bool dng_camera_profile::HasColorMatrix2 () const
	{

	return fColorMatrix2.Cols () == 3 &&
		   fColorMatrix2.Rows () == fColorMatrix1.Rows ();
	
	}
		

bool dng_camera_profile::HasColorMatrix3 () const
	{

	return fColorMatrix3.Cols () == 3 &&
		   fColorMatrix3.Rows () == fColorMatrix2.Rows () &&
		   fColorMatrix3.Rows () == fColorMatrix1.Rows ();
	
	}
		

void dng_camera_profile::SetHueSatDeltas1 (const dng_hue_sat_map &deltas1)
	{

	fHueSatDeltas1 = deltas1;

	ClearFingerprint ();

	}

void dng_camera_profile::SetHueSatDeltas2 (const dng_hue_sat_map &deltas2)
	{

	fHueSatDeltas2 = deltas2;

	ClearFingerprint ();

	}

void dng_camera_profile::SetHueSatDeltas3 (const dng_hue_sat_map &deltas3)
	{

	fHueSatDeltas3 = deltas3;

	ClearFingerprint ();

	}

void dng_camera_profile::SetLookTable (const dng_hue_sat_map &table)
	{

	fLookTable = table;

	ClearFingerprint ();

	}

static void FingerprintMatrix (dng_md5_printer_stream &printer,
							   const dng_matrix &matrix)
	{

	tag_matrix tag (0, matrix);
	
	

	

	tag.Put (printer);

	}

static void FingerprintHueSatMap (dng_md5_printer_stream &printer,
								  const dng_hue_sat_map &map)
	{

	if (map.IsNull ())
		return;

	uint32 hues;
	uint32 sats;
	uint32 vals;

	map.GetDivisions (hues, sats, vals);

	printer.Put_uint32 (hues);
	printer.Put_uint32 (sats);
	printer.Put_uint32 (vals);

	for (uint32 val = 0; val < vals; val++)
		for (uint32 hue = 0; hue < hues; hue++)
			for (uint32 sat = 0; sat < sats; sat++)
				{

				dng_hue_sat_map::HSBModify modify;

				map.GetDelta (hue, sat, val, modify);

				printer.Put_real32 (modify.fHueShift);
				printer.Put_real32 (modify.fSatScale);
				printer.Put_real32 (modify.fValScale);

				}

	}

dng_fingerprint dng_camera_profile::CalculateFingerprint (bool renderDataOnly) const
	{
	
	DNG_ASSERT (!fWasStubbed, "CalculateFingerprint on stubbed profile");

	

	dng_md5_printer_le_stream printer;

	
	
	

	if (HasColorMatrix1 ())
		{

		uint32 colorChannels = ColorMatrix1 ().Rows ();
		
		printer.Put_uint16 ((uint16) fCalibrationIlluminant1);

		FingerprintMatrix (printer, fColorMatrix1);
		
		if (fForwardMatrix1.Rows () == fColorMatrix1.Cols () &&
			fForwardMatrix1.Cols () == fColorMatrix1.Rows ())
			{
			
			FingerprintMatrix (printer, fForwardMatrix1);
			
			}
		
		if (colorChannels > 3 && fReductionMatrix1.Rows () *
								 fReductionMatrix1.Cols () == colorChannels * 3)
			{
			
			FingerprintMatrix (printer, fReductionMatrix1);
			
			}

		
		
		if (HasColorMatrix2 ())
			{
			
			printer.Put_uint16 ((uint16) fCalibrationIlluminant2);
			
			FingerprintMatrix (printer, fColorMatrix2);
		
			if (fForwardMatrix2.Rows () == fColorMatrix2.Cols () &&
				fForwardMatrix2.Cols () == fColorMatrix2.Rows ())
				{
				
				FingerprintMatrix (printer, fForwardMatrix2);
				
				}
		
			if (colorChannels > 3 && fReductionMatrix2.Rows () *
									 fReductionMatrix2.Cols () == colorChannels * 3)
				{
				
				FingerprintMatrix (printer, fReductionMatrix2);
				
				}

			
			
		
			if (HasColorMatrix3 ())
				{

				printer.Put_uint16 ((uint16) fCalibrationIlluminant3);

				FingerprintMatrix (printer, fColorMatrix3);

				if (fForwardMatrix3.Rows () == fColorMatrix3.Cols () &&
					fForwardMatrix3.Cols () == fColorMatrix3.Rows ())
					{

					FingerprintMatrix (printer, fForwardMatrix3);

					}

				if (colorChannels > 3 && fReductionMatrix3.Rows () *
										 fReductionMatrix3.Cols () == colorChannels * 3)
					{

					FingerprintMatrix (printer, fReductionMatrix3);

					}

				} 

			} 
   
		if (!renderDataOnly)
			{
		
			printer.Put (fName.Get	  (),
						 fName.Length ());
				
			printer.Put (fGroupName.Get	   (),
						 fGroupName.Length ());
				
			}

		printer.Put (fProfileCalibrationSignature.Get	 (),
					 fProfileCalibrationSignature.Length ());

		if (!renderDataOnly)
			{
		
			printer.Put_uint32 (fEmbedPolicy);
			
			printer.Put (fCopyright.Get	   (),
						 fCopyright.Length ());
				
			}
					 
		bool haveHueSat1 = HueSatDeltas1 ().IsValid ();
		
		bool haveHueSat2 = HueSatDeltas2 ().IsValid () &&
						   HasColorMatrix2 ();

		bool haveHueSat3 = HueSatDeltas3 ().IsValid () &&
						   HasColorMatrix3 ();

		if (haveHueSat1)
			{
			
			FingerprintHueSatMap (printer, fHueSatDeltas1);
			
			}
			
		if (haveHueSat2)
			{
			
			FingerprintHueSatMap (printer, fHueSatDeltas2);
			
			}

		if (haveHueSat3)
			{
			
			FingerprintHueSatMap (printer, fHueSatDeltas3);
			
			}

		if (haveHueSat1 || haveHueSat2 || haveHueSat3)
			{

			if (fHueSatMapEncoding != 0)
				{

				printer.Put_uint32 (fHueSatMapEncoding);

				}
			
			}
			
		if (fLookTable.IsValid ())
			{
			
			FingerprintHueSatMap (printer, fLookTable);

			if (fLookTableEncoding != 0)
				{

				printer.Put_uint32 (fLookTableEncoding);

				}
			
			}

		if (fBaselineExposureOffset.IsValid ())
			{
			
			if (fBaselineExposureOffset.As_real64 () != 0.0)
				{

				printer.Put_real64 (fBaselineExposureOffset.As_real64 ());

				}
			
			}

		if (fDefaultBlackRender != 0)
			{
			
			printer.Put_int32 (fDefaultBlackRender);
			
			}
			
		if (fToneCurve.IsValid ())
			{
			
			for (uint32 i = 0; i < fToneCurve.fCoord.size (); i++)
				{
				
				printer.Put_real32 ((real32) fToneCurve.fCoord [i].h);
				printer.Put_real32 ((real32) fToneCurve.fCoord [i].v);
				
				}
				
			}
			
		if (fToneMethod != profileToneMethod_Unspecified)
			{
			
			printer.Put_int32 (fToneMethod);
			
			}
			
		}

	

		{

		auto pgtm = ShareProfileGainTableMap ();

		if (pgtm)
			{
			
			dng_fingerprint digest = pgtm->GetFingerprint ();

			printer.Put (digest);
			
			}

		}

	
		
		{
			
		const auto &range = DynamicRangeInfo ();

		if (range.IsHDR ())
			{
			
			printer.Put ("hdr", 3);

			if (range.fHintMaxOutputValue != 1.0f)
				printer.Put_real32 (range.fHintMaxOutputValue);
			
			}
			
		}

	

	if (HasMaskedRGBTables ())
		{
		
		dng_md5_printer_le_stream rgbTablesPrinter;
		
		MaskedRGBTables ().AddDigest (rgbTablesPrinter);
		
		auto rgbTableDigest = rgbTablesPrinter.Result ();
		
		printer.Put (rgbTableDigest);
		
		}

	return printer.Result ();

	}

dng_fingerprint dng_camera_profile::UniqueID () const
	{

	

	dng_md5_printer_le_stream printer;

	
 
	dng_fingerprint fingerprint = Fingerprint ();
	
	printer.Put (fingerprint);

	

	printer.Put (fUniqueCameraModelRestriction.Get	  (),
				 fUniqueCameraModelRestriction.Length ());

	

	

	return printer.Result ();
	
	}

bool dng_camera_profile::ValidForwardMatrix (const dng_matrix &m)
	{
	
	const real64 kThreshold = 0.01;
	
	if (m.NotEmpty ())
		{
		
		dng_vector cameraOne;
		
		cameraOne.SetIdentity (m.Cols ());
		
		dng_vector xyz = m * cameraOne;
		
		dng_vector pcs = PCStoXYZ ();
		
		if (Abs_real64 (xyz [0] - pcs [0]) > kThreshold ||
			Abs_real64 (xyz [1] - pcs [1]) > kThreshold ||
			Abs_real64 (xyz [2] - pcs [2]) > kThreshold)
			{
			
			return false;
			
			}
			
		}
		
	return true;
	
	}

bool dng_camera_profile::IsValid (uint32 channels) const
	{

	bool hasFirstTwoColorMatrices = false;
	
	bool hasThreeColorMatrices = false;
	
	
		
	if (channels == 1)
		{
		
		return true;
		
		}
		
	
		
	if (fColorMatrix1.Cols () != 3 ||
		fColorMatrix1.Rows () != channels)
		{
		
		#if qDNGValidate
	
		ReportError ("ColorMatrix1 is wrong size");
					 
		#endif
					 
		return false;
		
		}

	
	
	if (fColorMatrix2.Cols () != 0 ||
		fColorMatrix2.Rows () != 0)
		{
		
		if (fColorMatrix2.Cols () != 3 ||
			fColorMatrix2.Rows () != channels)
			{
			
			#if qDNGValidate
		
			ReportError ("ColorMatrix2 is wrong size");
						 
			#endif
					 
			return false;
			
			}

		

		hasFirstTwoColorMatrices = true;
		
		}
		
	
	
	if (fColorMatrix3.Cols () != 0 ||
		fColorMatrix3.Rows () != 0)
		{
		
		if (fColorMatrix3.Cols () != 3 ||
			fColorMatrix3.Rows () != channels)
			{
			
			#if qDNGValidate
		
			ReportError ("ColorMatrix3 is wrong size");
						 
			#endif
					 
			return false;
			
			}

		

		if (!hasFirstTwoColorMatrices)
			{
			
			#if qDNGValidate
		
			ReportError ("ColorMatrix3 present without ColorMatrix1/ColorMatrix2");
						 
			#endif
					 
			return false;
			
			}

		

		hasThreeColorMatrices = true;
		
		}

	
	
	if (fForwardMatrix1.Cols () != 0 ||
		fForwardMatrix1.Rows () != 0)
		{
		
		if (fForwardMatrix1.Rows () != 3 ||
			fForwardMatrix1.Cols () != channels)
			{
			
			#if qDNGValidate
		
			ReportError ("ForwardMatrix1 is wrong size");
						 
			#endif
						 
			return false;
			
			}

		
		
		if (!ValidForwardMatrix (fForwardMatrix1))
			{
			
			#if qDNGValidate
		
			ReportError ("ForwardMatrix1 does not map equal camera values to XYZ D50");
						 
			#endif
						 
			return false;
		
			}
				
		}

	
	
	if (fForwardMatrix2.Cols () != 0 ||
		fForwardMatrix2.Rows () != 0)
		{
		
		if (fForwardMatrix2.Rows () != 3 ||
			fForwardMatrix2.Cols () != channels)
			{
			
			#if qDNGValidate
		
			ReportError ("ForwardMatrix2 is wrong size");
						 
			#endif
						 
			return false;
			
			}

		
		
		if (!ValidForwardMatrix (fForwardMatrix2))
			{
			
			#if qDNGValidate
		
			ReportError ("ForwardMatrix2 does not map equal camera values to XYZ D50");
						 
			#endif
						 
			return false;
		
			}
				
		}

	
	
	if (fForwardMatrix3.Cols () != 0 ||
		fForwardMatrix3.Rows () != 0)
		{
		
		if (fForwardMatrix3.Rows () != 3 ||
			fForwardMatrix3.Cols () != channels)
			{
			
			#if qDNGValidate
		
			ReportError ("ForwardMatrix3 is wrong size");
						 
			#endif
						 
			return false;
			
			}

		
		
		if (!ValidForwardMatrix (fForwardMatrix3))
			{
			
			#if qDNGValidate
		
			ReportError ("ForwardMatrix3 does not map equal camera values to XYZ D50");
						 
			#endif
						 
			return false;
		
			}

		

		if (!hasThreeColorMatrices)
			{
			
			#if qDNGValidate
		
			ReportError ("ForwardMatrix3 present without three color matrices");
						 
			#endif
					 
			return false;
			
			}
		
		}

	
	
	if (fReductionMatrix1.Cols () != 0 ||
		fReductionMatrix1.Rows () != 0)
		{
		
		if (fReductionMatrix1.Cols () != channels ||
			fReductionMatrix1.Rows () != 3)
			{
			
			#if qDNGValidate
		
			ReportError ("ReductionMatrix1 is wrong size");
						 
			#endif
					 
			return false;
			
			}
		
		}
	
	
	
	if (fReductionMatrix2.Cols () != 0 ||
		fReductionMatrix2.Rows () != 0)
		{
		
		if (fReductionMatrix2.Cols () != channels ||
			fReductionMatrix2.Rows () != 3)
			{
			
			#if qDNGValidate
		
			ReportError ("ReductionMatrix2 is wrong size");
						 
			#endif
					 
			return false;
			
			}
		
		}
		
	
	
	if (fReductionMatrix3.Cols () != 0 ||
		fReductionMatrix3.Rows () != 0)
		{
		
		if (fReductionMatrix3.Cols () != channels ||
			fReductionMatrix3.Rows () != 3)
			{
			
			#if qDNGValidate
		
			ReportError ("ReductionMatrix3 is wrong size");
						 
			#endif
					 
			return false;
			
			}
		
		

		if (!hasThreeColorMatrices)
			{
			
			#if qDNGValidate
		
			ReportError ("ReductionMatrix3 present without three color matrices");
						 
			#endif
					 
			return false;
			
			}
		
		}
		
	
	
	try
		{
		
		if (fReductionMatrix1.NotEmpty ())
			{
			
			(void) Invert (fColorMatrix1,
						   fReductionMatrix1);
			
			}
			
		else
			{
		
			(void) Invert (fColorMatrix1);
			
			}
		
		}
		
	catch (...)
		{
			
		#if qDNGValidate
	
		ReportError ("ColorMatrix1 is not invertible");
					 
		#endif
					 
		return false;
	
		}
		
	
	
	if (fColorMatrix2.NotEmpty ())
		{
						
		try
			{
			
			if (fReductionMatrix2.NotEmpty ())
				{
				
				(void) Invert (fColorMatrix2,
							   fReductionMatrix2);
				
				}
				
			else
				{
			
				(void) Invert (fColorMatrix2);
				
				}

			}
			
		catch (...)
			{
				
			#if qDNGValidate
	
			ReportError ("ColorMatrix2 is not invertible");
						 
			#endif
						 
			return false;
		
			}
			
		}

	
	
	if (fColorMatrix3.NotEmpty ())
		{
						
		try
			{
			
			if (fReductionMatrix3.NotEmpty ())
				{
				
				(void) Invert (fColorMatrix3,
							   fReductionMatrix3);
				
				}
				
			else
				{
			
				(void) Invert (fColorMatrix3);
				
				}

			}
			
		catch (...)
			{
				
			#if qDNGValidate
	
			ReportError ("ColorMatrix3 is not invertible");
						 
			#endif
						 
			return false;
		
			}
			
		}

	
	

	if (IlluminantModel () == 3)
		{
		
		

		if (CalibrationIlluminant1 () == lsUnknown ||
			CalibrationIlluminant2 () == lsUnknown ||
			CalibrationIlluminant3 () == lsUnknown)
			{
			
			#if qDNGValidate
		
			ReportError ("CalibrationIlluminant1/2/3 cannot be unknown for "
						 "a triple-illuminant profile");
						 
			#endif

			return false;
			
			}

		

		dng_illuminant_data light1 (CalibrationIlluminant1 (), &IlluminantData1 ());
		dng_illuminant_data light2 (CalibrationIlluminant2 (), &IlluminantData2 ());
		dng_illuminant_data light3 (CalibrationIlluminant3 (), &IlluminantData3 ());
		
		dng_xy_coord white1 = light1.WhiteXY ();
		dng_xy_coord white2 = light2.WhiteXY ();
		dng_xy_coord white3 = light3.WhiteXY ();

		if (white1 == white2 ||
			white1 == white3 ||
			white2 == white3)
			{
			
			#if qDNGValidate
		
			ReportError ("In a triple-illuminant profile all three illuminants "
						 "must be distinct");
						 
			#endif

			return false;
			
			}

		

		if (!HasColorMatrix1 () ||
			!HasColorMatrix2 () ||
			!HasColorMatrix3 ())
			{
			
			#if qDNGValidate
		
			ReportError ("ColorMatrix1/2/3 must all be present and valid for "
						 "a triple-illuminant profile");
						 
			#endif			
			
			return false;
			
			}

		

		if (ForwardMatrix1 ().NotEmpty () ||
			ForwardMatrix2 ().NotEmpty () ||
			ForwardMatrix3 ().NotEmpty ())
			{
			
			if (ForwardMatrix1 ().IsEmpty () ||
				ForwardMatrix2 ().IsEmpty () ||
				ForwardMatrix3 ().IsEmpty ())
				{
				
				#if qDNGValidate

				ReportError ("For a triple-illuminant profile, ForwardMatrix "
							 "must be absent for all three illuminants, or "
							 "present for all three illuminants");

				#endif			

				return false;
				
				}
			
			}

		
		
		if (ReductionMatrix1 ().NotEmpty () ||
			ReductionMatrix2 ().NotEmpty () ||
			ReductionMatrix3 ().NotEmpty ())
			{
			
			if (ReductionMatrix1 ().IsEmpty () ||
				ReductionMatrix2 ().IsEmpty () ||
				ReductionMatrix3 ().IsEmpty ())
				{
				
				#if qDNGValidate

				ReportError ("For a triple-illuminant profile, ReductionMatrix "
							 "must be absent for all three illuminants, or "
							 "present for all three illuminants");

				#endif			

				return false;
				
				}
			
			}

		
		
		if (HueSatDeltas1 ().IsValid () ||
			HueSatDeltas2 ().IsValid () ||
			HueSatDeltas3 ().IsValid ())
			{
			
			if (HueSatDeltas1 ().IsNull () ||
				HueSatDeltas2 ().IsNull () ||
				HueSatDeltas3 ().IsNull ())
				{
				
				#if qDNGValidate

				ReportError ("For a triple-illuminant profile, HueSatDeltas "
							 "must be absent for all three illuminants, or "
							 "present for all three illuminants");

				#endif			

				return false;
				
				}
			
			}

		}

	return true;
	
	}

void dng_camera_profile::ReadHueSatMap (dng_stream &stream,
										dng_hue_sat_map &hueSatMap,
										uint32 hues,
										uint32 sats,
										uint32 vals,
										bool skipSat0)
	{

	hueSatMap.SetDivisions (hues, sats, vals);

	for (uint32 val = 0; val < vals; val++)
		{

		for (uint32 hue = 0; hue < hues; hue++)
			{

			for (uint32 sat = skipSat0 ? 1 : 0; sat < sats; sat++)
				{

				dng_hue_sat_map::HSBModify modify;

				modify.fHueShift = stream.Get_real32 ();
				modify.fSatScale = stream.Get_real32 ();
				modify.fValScale = stream.Get_real32 ();

				hueSatMap.SetDelta (hue, sat, val, modify);
				
				}
				
			}
			
		}

	hueSatMap.AssignNewUniqueRuntimeFingerprint ();

	}

void dng_camera_profile::Parse (dng_stream &stream,
								dng_camera_profile_info &profileInfo)
	{
	
	SetUniqueCameraModelRestriction (profileInfo.fUniqueCameraModel.Get ());

	if (profileInfo.fProfileName.NotEmpty ())
		{
		
		SetName (profileInfo.fProfileName.Get ());
		
		}
	
	SetCopyright (profileInfo.fProfileCopyright.Get ());

	SetEmbedPolicy (profileInfo.fEmbedPolicy);

	SetCalibrationIlluminant1 (profileInfo.fCalibrationIlluminant1);
			
	SetColorMatrix1 (profileInfo.fColorMatrix1);
			
	if (profileInfo.fForwardMatrix1.NotEmpty ())
		{
		
		SetForwardMatrix1 (profileInfo.fForwardMatrix1);
		
		}
		
	if (profileInfo.fReductionMatrix1.NotEmpty ())
		{
		
		SetReductionMatrix1 (profileInfo.fReductionMatrix1);
		
		}

	if (CalibrationIlluminant1 () == lsOther)
		{
		
		SetIlluminantData1 (profileInfo.fIlluminantData1);
		
		}

	

	if (profileInfo.fColorMatrix2.NotEmpty ())
		{
		
		SetCalibrationIlluminant2 (profileInfo.fCalibrationIlluminant2);
		
		SetColorMatrix2 (profileInfo.fColorMatrix2);
					
		if (profileInfo.fForwardMatrix2.NotEmpty ())
			{
			
			SetForwardMatrix2 (profileInfo.fForwardMatrix2);
			
			}
		
		if (profileInfo.fReductionMatrix2.NotEmpty ())
			{
			
			SetReductionMatrix2 (profileInfo.fReductionMatrix2);
			
			}

		if (CalibrationIlluminant2 () == lsOther)
			{

			SetIlluminantData2 (profileInfo.fIlluminantData2);

			}

		}

	

	if (profileInfo.fColorMatrix3.NotEmpty ())
		{
		
		SetCalibrationIlluminant3 (profileInfo.fCalibrationIlluminant3);
		
		SetColorMatrix3 (profileInfo.fColorMatrix3);
					
		if (profileInfo.fForwardMatrix3.NotEmpty ())
			{
			
			SetForwardMatrix3 (profileInfo.fForwardMatrix3);
			
			}
		
		if (profileInfo.fReductionMatrix3.NotEmpty ())
			{
			
			SetReductionMatrix3 (profileInfo.fReductionMatrix3);
			
			}
		
		if (CalibrationIlluminant3 () == lsOther)
			{

			SetIlluminantData3 (profileInfo.fIlluminantData3);

			}

		}

	SetProfileCalibrationSignature (profileInfo.fProfileCalibrationSignature.Get ());

	if (profileInfo.fHueSatDeltas1Offset != 0 &&
		profileInfo.fHueSatDeltas1Count	 != 0)
		{

		TempBigEndian setEndianness (stream, profileInfo.fBigEndian);

		stream.SetReadPosition (profileInfo.fHueSatDeltas1Offset);
		
		bool skipSat0 = (profileInfo.fHueSatDeltas1Count ==
						 SafeUint32Mult (profileInfo.fProfileHues,
										 SafeUint32Sub (profileInfo.fProfileSats, 1),
										 profileInfo.fProfileVals, 3));

		ReadHueSatMap (stream,
					   fHueSatDeltas1,
					   profileInfo.fProfileHues,
					   profileInfo.fProfileSats,
					   profileInfo.fProfileVals,
					   skipSat0);

		}

	if (profileInfo.fHueSatDeltas2Offset != 0 &&
		profileInfo.fHueSatDeltas2Count	 != 0)
		{

		TempBigEndian setEndianness (stream, profileInfo.fBigEndian);

		stream.SetReadPosition (profileInfo.fHueSatDeltas2Offset);

		bool skipSat0 = (profileInfo.fHueSatDeltas2Count ==
						 SafeUint32Mult (profileInfo.fProfileHues,
										 SafeUint32Sub (profileInfo.fProfileSats, 1),
										 profileInfo.fProfileVals, 3));

		ReadHueSatMap (stream,
					   fHueSatDeltas2,
					   profileInfo.fProfileHues,
					   profileInfo.fProfileSats,
					   profileInfo.fProfileVals,
					   skipSat0);

		}

	if (profileInfo.fHueSatDeltas3Offset != 0 &&
		profileInfo.fHueSatDeltas3Count	 != 0)
		{

		TempBigEndian setEndianness (stream, profileInfo.fBigEndian);

		stream.SetReadPosition (profileInfo.fHueSatDeltas3Offset);

		bool skipSat0 = (profileInfo.fHueSatDeltas3Count ==
						 SafeUint32Mult (profileInfo.fProfileHues,
										 SafeUint32Sub (profileInfo.fProfileSats, 1),
										 profileInfo.fProfileVals, 3));

		ReadHueSatMap (stream,
					   fHueSatDeltas3,
					   profileInfo.fProfileHues,
					   profileInfo.fProfileSats,
					   profileInfo.fProfileVals,
					   skipSat0);

		}

	if (profileInfo.fLookTableOffset != 0 &&
		profileInfo.fLookTableCount	 != 0)
		{

		TempBigEndian setEndianness (stream, profileInfo.fBigEndian);

		stream.SetReadPosition (profileInfo.fLookTableOffset);

		bool skipSat0 = (profileInfo.fLookTableCount ==
						 SafeUint32Mult (profileInfo.fLookTableHues,
										 SafeUint32Sub (profileInfo.fLookTableSats, 1),
										 profileInfo.fLookTableVals, 3));

		ReadHueSatMap (stream,
					   fLookTable,
					   profileInfo.fLookTableHues,
					   profileInfo.fLookTableSats,
					   profileInfo.fLookTableVals,
					   skipSat0);

		}

	if ((profileInfo.fToneCurveCount & 1) == 0)
		{

		TempBigEndian setEndianness (stream, profileInfo.fBigEndian);

		stream.SetReadPosition (profileInfo.fToneCurveOffset);

		uint32 points = profileInfo.fToneCurveCount / 2;

		if (points > kMaxToneCurvePoints)
			{
			ThrowProgramError ("Too many tone curve points");
			}

		fToneCurve.fCoord.resize (points);

		for (size_t i = 0; i < points; i++)
			{

			dng_point_real64 point;

			point.h = stream.Get_real32 ();
			point.v = stream.Get_real32 ();

			fToneCurve.fCoord [i] = point;

			}
			
		}

	SetToneMethod (profileInfo.fToneMethod);
		
	SetHueSatMapEncoding (profileInfo.fHueSatMapEncoding);
		
	SetLookTableEncoding (profileInfo.fLookTableEncoding);

	SetBaselineExposureOffset (profileInfo.fBaselineExposureOffset.As_real64 ());

	SetDefaultBlackRender (profileInfo.fDefaultBlackRender);

	SetProfileGainTableMap (profileInfo.fProfileGainTableMap);

	SetGroupName (profileInfo.fProfileGroupName);

	SetDynamicRangeInfo (profileInfo.fProfileDynamicRange);

	SetMaskedRGBTables (profileInfo.fMaskedRGBTables);
		
	}
		

bool dng_camera_profile::ParseExtended (dng_stream &stream)
	{

	try
		{
		
		dng_camera_profile_info profileInfo;
		
		if (!profileInfo.ParseExtended (stream))
			{
			return false;
			}
			
		Parse (stream, profileInfo);

		return true;

		}
		
	catch (...)
		{
		
		
		
		}

	return false;

	}

void dng_camera_profile::SetFourColorBayer ()
	{
	
	uint32 j;
	
	if (!IsValid (3))
		{
		ThrowProgramError ();
		}
		
	if (fColorMatrix1.NotEmpty ())
		{
		
		dng_matrix m (4, 3);
		
		for (j = 0; j < 3; j++)
			{
			m [0] [j] = fColorMatrix1 [0] [j];
			m [1] [j] = fColorMatrix1 [1] [j];
			m [2] [j] = fColorMatrix1 [2] [j];
			m [3] [j] = fColorMatrix1 [1] [j];
			}
			
		fColorMatrix1 = m;
		
		}
	
	if (fColorMatrix2.NotEmpty ())
		{
		
		dng_matrix m (4, 3);
		
		for (j = 0; j < 3; j++)
			{
			m [0] [j] = fColorMatrix2 [0] [j];
			m [1] [j] = fColorMatrix2 [1] [j];
			m [2] [j] = fColorMatrix2 [2] [j];
			m [3] [j] = fColorMatrix2 [1] [j];
			}
			
		fColorMatrix2 = m;
		
		}
			
	if (fColorMatrix3.NotEmpty ())
		{
		
		dng_matrix m (4, 3);
		
		for (j = 0; j < 3; j++)
			{
			m [0] [j] = fColorMatrix3 [0] [j];
			m [1] [j] = fColorMatrix3 [1] [j];
			m [2] [j] = fColorMatrix3 [2] [j];
			m [3] [j] = fColorMatrix3 [1] [j];
			}
			
		fColorMatrix3 = m;
		
		}
			
	fReductionMatrix1.Clear ();
	fReductionMatrix2.Clear ();
	fReductionMatrix3.Clear ();
	
	fForwardMatrix1.Clear ();
	fForwardMatrix2.Clear ();
	fForwardMatrix3.Clear ();

	ClearFingerprint ();
	
	}

dng_hue_sat_map * dng_camera_profile::HueSatMapForWhite (const dng_xy_coord &white) const
	{
	
	if (fHueSatDeltas1.IsValid ())
		{

		
		
		if (!fHueSatDeltas2.IsValid ())
			{
			
			return new dng_hue_sat_map (fHueSatDeltas1);
			
			}

		

		if (IlluminantModel () == 3)
			{

			
			
			
			
			return HueSatMapForWhite_Triple (white);
			
			}

		else
			{

			
			
			return HueSatMapForWhite_Dual (white);
			
			}

		}

	return nullptr;

	}

dng_hue_sat_map *
	dng_camera_profile::HueSatMapForWhite_Dual (const dng_xy_coord &white) const
	{

	DNG_REQUIRE (fHueSatDeltas1.IsValid () &&
				 fHueSatDeltas2.IsValid (),
				 "Bad hue sat map deltas 1 or 2");
				 
	
		
	real64 temperature1 = CalibrationTemperature1 ();
	real64 temperature2 = CalibrationTemperature2 ();
		
	if (temperature1 <= 0.0 ||
		temperature2 <= 0.0 ||
		temperature1 == temperature2)
		{
			
		return new dng_hue_sat_map (fHueSatDeltas1);
			
		}
			
	bool reverseOrder = temperature1 > temperature2;
		
	if (reverseOrder)
		{
		real64 temp	 = temperature1;
		temperature1 = temperature2;
		temperature2 = temp;
		}

	
		
	dng_temperature td (white);
		
	
		
	real64 g;
		
	if (td.Temperature () <= temperature1)
		g = 1.0;
		
	else if (td.Temperature () >= temperature2)
		g = 0.0;
		
	else
		{
			
		real64 invT = 1.0 / td.Temperature ();
			
		g = (invT				  - (1.0 / temperature2)) /
			((1.0 / temperature1) - (1.0 / temperature2));
				
		}
			
	
		
	if (reverseOrder)
		{
		g = 1.0 - g;
		}
		
	
		
	return dng_hue_sat_map::Interpolate (HueSatDeltas1 (),
										 HueSatDeltas2 (),
										 g);
		
	}

dng_hue_sat_map *
	dng_camera_profile::HueSatMapForWhite_Triple (const dng_xy_coord &white) const
	{

	DNG_REQUIRE (fHueSatDeltas1.IsValid () &&
				 fHueSatDeltas2.IsValid () &&
				 fHueSatDeltas3.IsValid (),
				 "Bad hue sat map deltas 1 or 2 or 3");

	real64 w1, w2, w3;
	
	CalculateTripleIlluminantWeights
		(white,
		 dng_illuminant_data (CalibrationIlluminant1 (), &IlluminantData1 ()),
		 dng_illuminant_data (CalibrationIlluminant2 (), &IlluminantData2 ()),
		 dng_illuminant_data (CalibrationIlluminant3 (), &IlluminantData3 ()),
		 w1,
		 w2,
		 w3);

	return dng_hue_sat_map::Interpolate (HueSatDeltas1 (),
										 HueSatDeltas2 (),
										 HueSatDeltas3 (),
										 w1,
										 w2);
		 
	}

void dng_camera_profile::Stub ()
	{
	
	(void) Fingerprint ();
 
	(void) RenderDataFingerprint ();
	
	dng_hue_sat_map nullTable;
	
	fHueSatDeltas1 = nullTable;
	fHueSatDeltas2 = nullTable;
	fHueSatDeltas3 = nullTable;
	
	fLookTable = nullTable;
	
	fToneCurve.SetInvalid ();
	
	fWasStubbed = true;
	
	}

bool dng_camera_profile::HasProfileGainTableMap () const
	{
	
	return fProfileGainTableMap != nullptr;
	
	}

void dng_camera_profile::SetProfileGainTableMap
			(const std::shared_ptr<const dng_gain_table_map> &gainTableMap)
	{
	
	fProfileGainTableMap = gainTableMap;

	ClearFingerprint ();
	
	}

const dng_camera_profile_dynamic_range & dng_camera_profile::DynamicRangeInfo () const
	{
	
	if (fDynamicRangeInfo)
		return *fDynamicRangeInfo;
	
	static const dng_camera_profile_dynamic_range sNoInfo;

	return sNoInfo;
	
	}

bool dng_camera_profile::IsSDR () const
	{
	
	return !IsHDR ();
	
	}
		

bool dng_camera_profile::IsHDR () const
	{
	
	return DynamicRangeInfo ().IsHDR ();
	
	}

void dng_camera_profile::SetDynamicRangeInfo (const dng_camera_profile_dynamic_range &info)
	{
	
	fDynamicRangeInfo.reset (new dng_camera_profile_dynamic_range (info));

	ClearFingerprint ();
	
	}

bool dng_camera_profile::HasMaskedRGBTables () const
	{
	
	return fMaskedRGBTables != nullptr;
	
	}

const dng_masked_rgb_tables & dng_camera_profile::MaskedRGBTables () const
	{
	
	DNG_REQUIRE (HasMaskedRGBTables (), "Missing masked RGBTables");

	return *fMaskedRGBTables;
	
	}

void dng_camera_profile::SetMaskedRGBTables
	(const std::shared_ptr<const dng_masked_rgb_tables> &maskedRGBTables)
	{
	
	fMaskedRGBTables = maskedRGBTables;

	ClearFingerprint ();
	
	}

void dng_camera_profile::SetMaskedRGBTables
	(AutoPtr<dng_masked_rgb_tables> &maskedRGBTables)
	{
	
	fMaskedRGBTables.reset (maskedRGBTables.Release ());

	ClearFingerprint ();
	
	}

bool dng_camera_profile::Uses_1_6_Features () const
	{

	

	if (Requires_1_6_Reader ())
		{
		return true;
		}

	

	if (IlluminantModel () == 3)
		{
		return true;
		}

	
	
	

	return false;
	
	}

bool dng_camera_profile::Requires_1_6_Reader () const
	{
	
	
	
	
	if (CalibrationIlluminant1 () == lsOther &&
		IlluminantData1 ().WhiteXY ().IsValid ())
		{
		return true;
		}
	
	if (CalibrationIlluminant2 () == lsOther &&
		IlluminantData2 ().WhiteXY ().IsValid ())
		{
		return true;
		}

	return false;

	}

bool dng_camera_profile::Uses_1_7_Features () const
	{

	

	if (HasProfileGainTableMap ())
		return true;

	

	if (HasMaskedRGBTables ())
		return true;

	

	if (DynamicRangeInfo ().IsValid () &&
		DynamicRangeInfo ().IsHDR ())
		return true;

	return false;
	
	}

dng_camera_profile_metadata::dng_camera_profile_metadata
							 (const dng_camera_profile &profile,
							  int32 index)

	:	fProfileID (profile.ProfileID ())
	
	,	fGroupName (profile.GroupName ())
	
	,	fHDR (profile.DynamicRangeInfo ().IsHDR ())

	,	fRenderDataFingerprint (profile.RenderDataFingerprint ())
	
	,	fIsLegalToEmbed (profile.IsLegalToEmbed ())
	
	,	fWasReadFromDNG (profile.WasReadFromDNG ())
	
	,	fWasReadFromDisk (profile.WasReadFromDisk ())
	
	,	fUniqueID ()

	,	fFilePath ()

	,	fReadOnly (true)

	,	fIndex (index)
	
	{
	
	if (fWasReadFromDisk)
		{
		fUniqueID = profile.UniqueID ();
		}
	
	}

bool dng_camera_profile_metadata::operator==
		(const dng_camera_profile_metadata &metadata) const
	{
	
	return fProfileID			  == metadata.fProfileID			 &&
		   fGroupName			  == metadata.fGroupName			 &&
		   fHDR					  == metadata.fHDR					 &&
		   fRenderDataFingerprint == metadata.fRenderDataFingerprint &&
		   fIsLegalToEmbed		  == metadata.fIsLegalToEmbed		 &&
		   fWasReadFromDNG		  == metadata.fWasReadFromDNG		 &&
		   fWasReadFromDisk		  == metadata.fWasReadFromDisk		 &&
		   fUniqueID			  == metadata.fUniqueID				 &&
		   fFilePath			  == metadata.fFilePath				 &&
		   fReadOnly			  == metadata.fReadOnly				 &&
		   fIndex				  == metadata.fIndex;
	
	}

void SplitCameraProfileName (const dng_string &name,
							 dng_string &baseName,
							 int32 &version)
	{
	
	baseName = name;
	
	version = 0;
	
	uint32 len = baseName.Length ();

	if (len == 7 && baseName.StartsWith ("ACR ", true))
		{

		if (name.Get () [len - 3] >= '0' &&
			name.Get () [len - 3] <= '9' &&
			name.Get () [len - 2] == '.' &&
			name.Get () [len - 1] >= '0' &&
			name.Get () [len - 1] <= '9')
		
		baseName.Truncate (3);

		version = ((int32) (name.Get () [len - 3] - '0')) * 10 +
				  ((int32) (name.Get () [len - 1] - '0'));

		return;

		}
	
	if (len > 5 && baseName.EndsWith (" beta"))
		{
		
		baseName.Truncate (len - 5);
		
		version += -10;
		
		}
		
	else if (len > 7)
		{
		
		char lastChar = name.Get () [len - 1];
		
		if (lastChar >= '0' && lastChar <= '9')
			{
			
			dng_string temp = name;
			
			temp.Truncate (len - 1);
			
			if (temp.EndsWith (" beta "))
				{
				
				baseName.Truncate (len - 7);
				
				version += ((int32) (lastChar - '0')) - 10;
				
				}
				
			}
			
		}
		
	len = baseName.Length ();
	
	if (len > 3)
		{
		
		char lastChar = name.Get () [len - 1];
		
		if (lastChar >= '0' && lastChar <= '9')
			{
			
			dng_string temp = name;
			
			temp.Truncate (len - 1);
			
			if (temp.EndsWith (" v"))
				{
				
				baseName.Truncate (len - 3);
				
				version += ((int32) (lastChar - '0')) * 100;
				
				}
				
			}
			
		}

	}

void BuildHueSatMapEncodingTable (dng_memory_allocator &allocator,
								  uint32 encoding,
								  AutoPtr<dng_1d_table> &encodeTable,
								  AutoPtr<dng_1d_table> &decodeTable,
								  bool subSample)
	{

	encodeTable.Reset ();
	decodeTable.Reset ();
	
	switch (encoding)
		{
		
		case encoding_Linear:
			{

			break;

			}
		
		case encoding_sRGB:
			{

			encodeTable.Reset (new dng_1d_table);
			decodeTable.Reset (new dng_1d_table);

			const dng_1d_function & curve = dng_function_GammaEncode_sRGB::Get ();

			encodeTable->Initialize (allocator,
									 curve,
									 subSample);

			const dng_1d_inverse inverse (curve);

			decodeTable->Initialize (allocator,
									 inverse,
									 subSample);

			break;

			}

		default:
			{

			DNG_REPORT ("Unsupported hue sat map / look table encoding.");

			break;

			}
		
		}

	}
							

