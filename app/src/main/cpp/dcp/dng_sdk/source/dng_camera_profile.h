

#ifndef __dng_camera_profile__
#define __dng_camera_profile__

#include "dng_auto_ptr.h"
#include "dng_assertions.h"
#include "dng_classes.h"
#include "dng_fingerprint.h"
#include "dng_hue_sat_map.h"
#include "dng_matrix.h"
#include "dng_string.h"
#include "dng_tag_values.h"
#include "dng_tone_curve.h"
#include "dng_xy_coord.h"

extern const char * kProfileName_Embedded;

extern const char * kAdobeCalibrationSignature;

class dng_camera_profile_id
	{
	
	private:
	
		dng_string fName;
		
		dng_fingerprint fFingerprint;
		
	public:
	
		

		dng_camera_profile_id ()
		
			:	fName		 ()
			,	fFingerprint ()
			
			{
			}
			
		
		

		dng_camera_profile_id (const char *name)
			
			:	fName		 ()
			,	fFingerprint ()
			
			{
			fName.Set (name);
			}

		
		

		dng_camera_profile_id (const dng_string &name)
			
			:	fName		 (name)
			,	fFingerprint ()
			
			{
			}

		
		
		

		dng_camera_profile_id (const char *name,
							   const dng_fingerprint &fingerprint)
			
			:	fName		 ()
			,	fFingerprint (fingerprint)
			
			{
			fName.Set (name);
			DNG_ASSERT (!fFingerprint.IsValid () || fName.NotEmpty (),
						"Cannot have profile fingerprint without name");
			}

		
		
		

		dng_camera_profile_id (const dng_string &name,
							   const dng_fingerprint &fingerprint)
			
			:	fName		 (name)
			,	fFingerprint (fingerprint)
			
			{
			DNG_ASSERT (!fFingerprint.IsValid () || fName.NotEmpty (),
						"Cannot have profile fingerprint without name");
			}

		
		

		const dng_string & Name () const
			{
			return fName;
			}
			
		
		

		const dng_fingerprint & Fingerprint () const
			{
			return fFingerprint;
			}
			
		
		

		bool operator== (const dng_camera_profile_id &id) const
			{
			return fName		== id.fName &&
				   fFingerprint == id.fFingerprint;
			}

		
		

		bool operator!= (const dng_camera_profile_id &id) const
			{
			return !(*this == id);
			}
			
		

		bool IsValid () const
			{
			return fName.NotEmpty ();		
			}
			
		
		

		void Clear ()
			{
			*this = dng_camera_profile_id ();
			}

		

		void AddDigest (dng_md5_printer &printer) const;

	};
	

extern const char * kProfileName_GroupPrefix;

bool HasProfileGroupPrefix (const dng_string &name);

dng_string StripProfileGroupPrefix (const dng_string &name);

class dng_camera_profile_group_selector
	{
	
	public:
	
		
	
		bool fHDR = false;
		
	};

class dng_camera_profile
	{
	
	private:
	
		
		
		dng_string fName;
	
		
		
		dng_string fGroupName;

		

		std::shared_ptr<const dng_camera_profile_dynamic_range> fDynamicRangeInfo;
	
		
		
		
		
		uint32 fCalibrationIlluminant1;
		uint32 fCalibrationIlluminant2;
		uint32 fCalibrationIlluminant3;		 

		
		

		dng_illuminant_data fIlluminantData1; 
		dng_illuminant_data fIlluminantData2; 
		dng_illuminant_data fIlluminantData3; 
		
		
		
		
		
		
		
		
		
		dng_matrix fColorMatrix1;
		dng_matrix fColorMatrix2;
		dng_matrix fColorMatrix3;

		
		
		
		
		
		dng_matrix fForwardMatrix1;
		dng_matrix fForwardMatrix2;
		dng_matrix fForwardMatrix3;
	
		
		
		
		
		
		dng_matrix fReductionMatrix1;
		dng_matrix fReductionMatrix2;
		dng_matrix fReductionMatrix3;
		
		

		mutable dng_fingerprint fFingerprint;
		
		
		
		
		mutable dng_fingerprint fRenderDataFingerprint;

		

		dng_string fCopyright;
		
		

		uint32 fEmbedPolicy;
		
		

		dng_hue_sat_map fHueSatDeltas1;
		dng_hue_sat_map fHueSatDeltas2;
		dng_hue_sat_map fHueSatDeltas3;
		
		

		uint32 fHueSatMapEncoding;

		
		
		dng_hue_sat_map fLookTable;

		

		uint32 fLookTableEncoding;

		
		
		

		dng_srational fBaselineExposureOffset;

		

		uint32 fDefaultBlackRender;
		
		
		

		dng_tone_curve fToneCurve;
		
		
		
		uint32 fToneMethod;
		
		
		

		dng_string fProfileCalibrationSignature;
		
		
		

		dng_string fUniqueCameraModelRestriction;

		
		
		
		
		bool fWasReadFromDNG;
		
		
		
		
		
		bool fWasReadFromDisk;
		
		
		
		
		bool fWasStubbed;

		

		std::shared_ptr<const dng_gain_table_map> fProfileGainTableMap;

		

		std::shared_ptr<const dng_masked_rgb_tables> fMaskedRGBTables;

	public:
	
		dng_camera_profile ();
		
		virtual ~dng_camera_profile ();
		
		

		
		

		void SetName (const char *name)
			{
			fName.Set (name);
			ClearFingerprint ();
			}

		
		

		const dng_string & Name () const
			{
			return fName;
			}
		
		
		

		void SetGroupName (const dng_string &s)
			{
			fGroupName = s;
			ClearFingerprint ();
			}

		
		

		const dng_string & GroupName () const
			{
			return fGroupName;
			}
		
		
		

		bool NameIsEmbedded () const
			{
			return fName.Matches (kProfileName_Embedded, true);
			}
			
		

		
		
		
		

		uint32 IlluminantModel () const;
		
		
		
		
		

		void SetCalibrationIlluminant1 (uint32 light)
			{
			fCalibrationIlluminant1 = light;
			ClearFingerprint ();
			}
			
		
		
		
		

		void SetCalibrationIlluminant2 (uint32 light)
			{
			fCalibrationIlluminant2 = light;
			ClearFingerprint ();
			}
			
		
		
		
		

		void SetCalibrationIlluminant3 (uint32 light)
			{
			fCalibrationIlluminant3 = light;
			ClearFingerprint ();
			}
			
		
		
		
		

		uint32 CalibrationIlluminant1 () const
			{
			return fCalibrationIlluminant1;
			}
			
		
		
		
		

		uint32 CalibrationIlluminant2 () const
			{
			return fCalibrationIlluminant2;
			}
		
		
		
		
		

		uint32 CalibrationIlluminant3 () const
			{
			return fCalibrationIlluminant3;
			}

		void SetIlluminantData1 (const dng_illuminant_data &data)
			{
			fIlluminantData1 = data;
			ClearFingerprint ();
			}
		
		const dng_illuminant_data & IlluminantData1 () const
			{
			return fIlluminantData1;
			}
			
		void SetIlluminantData2 (const dng_illuminant_data &data)
			{
			fIlluminantData2 = data;
			ClearFingerprint ();
			}
		
		const dng_illuminant_data & IlluminantData2 () const
			{
			return fIlluminantData2;
			}
			
		void SetIlluminantData3 (const dng_illuminant_data &data)
			{
			fIlluminantData3 = data;
			ClearFingerprint ();
			}
		
		const dng_illuminant_data & IlluminantData3 () const
			{
			return fIlluminantData3;
			}
			
		
		

		real64 CalibrationTemperature1 () const
			{
			return IlluminantToTemperature (CalibrationIlluminant1 (),
											IlluminantData1 ());
			}

		
		

		real64 CalibrationTemperature2 () const
			{
			return IlluminantToTemperature (CalibrationIlluminant2 (),
											IlluminantData2 ());
			}
			
		
		

		real64 CalibrationTemperature3 () const
			{
			return IlluminantToTemperature (CalibrationIlluminant3 (),
											IlluminantData3 ());
			}

		
		
		
		
		static void NormalizeColorMatrix (dng_matrix &m);
		
		
		
		
		
		
		

		void SetColorMatrix1 (const dng_matrix &m);

		
		
		
		
		
		

		void SetColorMatrix2 (const dng_matrix &m);
										
		
		
		
		
		
		

		void SetColorMatrix3 (const dng_matrix &m);
										
		

		bool HasColorMatrix1 () const;

		

		bool HasColorMatrix2 () const;
		
		

		bool HasColorMatrix3 () const;
		
		

		const dng_matrix & ColorMatrix1 () const
			{
			return fColorMatrix1;
			}
			
		

		const dng_matrix & ColorMatrix2 () const
			{
			return fColorMatrix2;
			}
			
		

		const dng_matrix & ColorMatrix3 () const
			{
			return fColorMatrix3;
			}
			
		
		
		
		
		static void NormalizeForwardMatrix (dng_matrix &m);
		
		

		void SetForwardMatrix1 (const dng_matrix &m);

		

		void SetForwardMatrix2 (const dng_matrix &m);

		

		void SetForwardMatrix3 (const dng_matrix &m);

		

		const dng_matrix & ForwardMatrix1 () const
			{
			return fForwardMatrix1;
			}
			
		

		const dng_matrix & ForwardMatrix2 () const
			{
			return fForwardMatrix2;
			}
		
		

		const dng_matrix & ForwardMatrix3 () const
			{
			return fForwardMatrix3;
			}
		
		
		
		
		
		

		void SetReductionMatrix1 (const dng_matrix &m);

		
		
		

		void SetReductionMatrix2 (const dng_matrix &m);
		
		
		
		

		void SetReductionMatrix3 (const dng_matrix &m);
		
		

		const dng_matrix & ReductionMatrix1 () const
			{
			return fReductionMatrix1;
			}
			
		

		const dng_matrix & ReductionMatrix2 () const
			{
			return fReductionMatrix2;
			}
			
		

		const dng_matrix & ReductionMatrix3 () const
			{
			return fReductionMatrix3;
			}
			
		
			
		const dng_fingerprint & Fingerprint () const
			{

			if (!fFingerprint.IsValid ())
				{
				fFingerprint = CalculateFingerprint (false);
				}

			return fFingerprint;

			}

		

		const dng_fingerprint & RenderDataFingerprint () const
			{

			if (!fRenderDataFingerprint.IsValid ())
				{
				fRenderDataFingerprint = CalculateFingerprint (true);
				}

			return fRenderDataFingerprint;

			}
		
		
		

		dng_fingerprint UniqueID () const;

		
		

		dng_camera_profile_id ProfileID () const
			{
			return dng_camera_profile_id (Name (), Fingerprint ());
			}
		
		
		

		void SetCopyright (const char *copyright)
			{
			fCopyright.Set (copyright);
			ClearFingerprint ();
			}

		
		

		const dng_string & Copyright () const
			{
			return fCopyright;
			}
			
		

		
		

		void SetEmbedPolicy (uint32 policy)
			{
			fEmbedPolicy = policy;
			ClearFingerprint ();
			}

		
		

		uint32 EmbedPolicy () const
			{
			return fEmbedPolicy;
			}
			
		
		

		bool IsLegalToEmbed () const
			{
			return WasReadFromDNG () ||
				   EmbedPolicy () == pepAllowCopying ||
				   EmbedPolicy () == pepEmbedIfUsed	 ||
				   EmbedPolicy () == pepNoRestrictions;
			}
			
		

		

		bool HasHueSatDeltas () const
			{
			return fHueSatDeltas1.IsValid ();
			}

		

		const dng_hue_sat_map & HueSatDeltas1 () const
			{
			return fHueSatDeltas1;
			}

		

		void SetHueSatDeltas1 (const dng_hue_sat_map &deltas1);

		

		const dng_hue_sat_map & HueSatDeltas2 () const
			{
			return fHueSatDeltas2;
			}

		

		void SetHueSatDeltas2 (const dng_hue_sat_map &deltas2);

		

		const dng_hue_sat_map & HueSatDeltas3 () const
			{
			return fHueSatDeltas3;
			}

		

		void SetHueSatDeltas3 (const dng_hue_sat_map &deltas3);

		

		

		uint32 HueSatMapEncoding () const
			{
			return fHueSatMapEncoding;
			}

		
		

		void SetHueSatMapEncoding (uint32 encoding)
			{
			fHueSatMapEncoding = encoding;
			ClearFingerprint ();
			}
		
		

		
		
		bool HasLookTable () const
			{
			return fLookTable.IsValid ();
			}
			
		

		const dng_hue_sat_map & LookTable () const
			{
			return fLookTable;
			}
			
		

		void SetLookTable (const dng_hue_sat_map &table);

		

		

		uint32 LookTableEncoding () const
			{
			return fLookTableEncoding;
			}

		
		

		void SetLookTableEncoding (uint32 encoding)
			{
			fLookTableEncoding = encoding;
			ClearFingerprint ();
			}

		

		
		

		void SetBaselineExposureOffset (real64 exposureOffset)
			{
			fBaselineExposureOffset.Set_real64 (exposureOffset, 100);
			ClearFingerprint ();
			}
					  
		
		

		const dng_srational & BaselineExposureOffset () const
			{
			return fBaselineExposureOffset;
			}
		
		

		
		

		void SetDefaultBlackRender (uint32 defaultBlackRender)
			{
			fDefaultBlackRender = defaultBlackRender;
			ClearFingerprint ();
			}
					  
		
		

		uint32 DefaultBlackRender () const
			{
			return fDefaultBlackRender;
			}
		
		

		

		const dng_tone_curve & ToneCurve () const
			{
			return fToneCurve;
			}

		

		void SetToneCurve (const dng_tone_curve &curve)
			{
			fToneCurve = curve;
			ClearFingerprint ();
			}

		

		

		void SetToneMethod (uint32 toneMethod)
			{
			fToneMethod = toneMethod;
			ClearFingerprint ();
			}
					  
		

		uint32 ToneMethod () const
			{
			return fToneMethod;
			}
		
		

		
		

		void SetProfileCalibrationSignature (const char *signature)
			{
			fProfileCalibrationSignature.Set (signature);
			ClearFingerprint ();
			}

		
		

		const dng_string & ProfileCalibrationSignature () const
			{
			return fProfileCalibrationSignature;
			}

		
		
		

		void SetUniqueCameraModelRestriction (const char *camera)
			{
			fUniqueCameraModelRestriction.Set (camera);
			
			}

		
		
		

		const dng_string & UniqueCameraModelRestriction () const
			{
			return fUniqueCameraModelRestriction;
			}
			
		
		
		
		

		void SetWasReadFromDNG (bool state = true)
			{
			fWasReadFromDNG = state;
			}
			
		

		bool WasReadFromDNG () const
			{
			return fWasReadFromDNG;
			}

		
		
		
		

		void SetWasReadFromDisk (bool state = true)
			{
			fWasReadFromDisk = state;
			}
			
		

		bool WasReadFromDisk () const
			{
			return fWasReadFromDisk;
			}

		
		

		bool IsValid (uint32 channels) const;
		
		
		
		

		bool EqualData (const dng_camera_profile &profile) const
			{
			return RenderDataFingerprint () == profile.RenderDataFingerprint ();
			}
		
		

		void Parse (dng_stream &stream,
					dng_camera_profile_info &profileInfo);
					
		
		
					
		bool ParseExtended (dng_stream &stream);

		

		virtual void SetFourColorBayer ();
		
		
		
		
		dng_hue_sat_map * HueSatMapForWhite (const dng_xy_coord &white) const;
		
		
		
		void Stub ();
		
		
		
		bool WasStubbed () const
			{
			return fWasStubbed;
			}

		

		bool HasProfileGainTableMap () const;

		std::shared_ptr<const dng_gain_table_map> ShareProfileGainTableMap () const
			{
			return fProfileGainTableMap;
			}

		
		
		void SetProfileGainTableMap
			(const std::shared_ptr<const dng_gain_table_map> &gainTableMap);

		
		
		const dng_camera_profile_dynamic_range & DynamicRangeInfo () const;

		
		
		bool IsSDR () const;
		
		
		
		bool IsHDR () const;
		
		void SetDynamicRangeInfo (const dng_camera_profile_dynamic_range &info);

		

		bool HasMaskedRGBTables () const;

		const dng_masked_rgb_tables & MaskedRGBTables () const;

		std::shared_ptr<const dng_masked_rgb_tables> ShareMaskedRGBTables () const
			{
			return fMaskedRGBTables;
			}

		
		
		void SetMaskedRGBTables
			(const std::shared_ptr<const dng_masked_rgb_tables> &maskedRGBTables);

		
		
		
		void SetMaskedRGBTables
			(AutoPtr<dng_masked_rgb_tables> &maskedRGBTables);

		

		
		
		

		bool Uses_1_6_Features () const;
		
		
		
		

		
		
		

		bool Requires_1_6_Reader () const;

		
		
		

		bool Uses_1_7_Features () const;
		
	private:
	
		static real64 IlluminantToTemperature (uint32 light,
											   const dng_illuminant_data &data);
		
		void ClearFingerprint ()
			{
			fFingerprint.Clear ();
			fRenderDataFingerprint.Clear ();
			}

		dng_fingerprint CalculateFingerprint (bool renderDataOnly) const;

		static bool ValidForwardMatrix (const dng_matrix &m);

		static void ReadHueSatMap (dng_stream &stream,
								   dng_hue_sat_map &hueSatMap,
								   uint32 hues,
								   uint32 sats,
								   uint32 vals,
								   bool skipSat0);

		dng_hue_sat_map * HueSatMapForWhite_Dual (const dng_xy_coord &white) const;	
								   
		dng_hue_sat_map * HueSatMapForWhite_Triple (const dng_xy_coord &white) const;	
								   
	};

class dng_camera_profile_metadata
	{
	
	public:
	
		dng_camera_profile_id fProfileID;
		
		dng_string fGroupName;
		
		bool fHDR;
		
		dng_fingerprint fRenderDataFingerprint;
		
		bool fIsLegalToEmbed;
		
		bool fWasReadFromDNG;
		
		bool fWasReadFromDisk;
		
		dng_fingerprint fUniqueID;	
		
		dng_string fFilePath;		
		
		bool fReadOnly;				
		
		int32 fIndex;				
		
	public:
	
		dng_camera_profile_metadata (const dng_camera_profile &profile,
									 int32 index = -1);
		
		bool operator== (const dng_camera_profile_metadata &metadata) const;
		
		bool operator!= (const dng_camera_profile_metadata &metadata) const
			{
			return !(*this == metadata);
			}
	
	};

void SplitCameraProfileName (const dng_string &name,
							 dng_string &baseName,
							 int32 &version);

void BuildHueSatMapEncodingTable (dng_memory_allocator &allocator,
								  uint32 encoding,
								  AutoPtr<dng_1d_table> &encodeTable,
								  AutoPtr<dng_1d_table> &decodeTable,
								  bool subSample);
							

#endif

