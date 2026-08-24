

#ifndef __dng_big_table__
#define __dng_big_table__

#include "dng_classes.h"
#include "dng_camera_profile.h"
#include "dng_rect.h"
#include "dng_safe_arithmetic.h"
#include "dng_semantic_mask.h"
#include "dng_tag_types.h"
#include "dng_tag_values.h"

#include <map>
#include <memory>
#include <vector>

#define qDebugMaskedRGBTableRender (qDNGValidate && 0)

void dng_big_table_cache_flush ();

void dng_big_table_cache_clear ();

class dng_big_table
	{

	protected:

		enum BigTableTypeEnum
			{
			btt_LookTable				  = 0,
			btt_RGBTable				  = 1,
			btt_ImageTable				  = 2,
			btt_CompressedSettingsTable	  = 3,
			btt_UncompressedSettingsTable = 4,
			btt_PackedImageTable		  = 5,
			};

	private:

		dng_fingerprint fFingerprint;

		dng_big_table_cache * fCache;
		
		bool fIsMissing;

	protected:

		dng_big_table (dng_big_table_cache *cache);

		dng_big_table (const dng_big_table &table);

		dng_big_table & operator= (const dng_big_table &table);

	public:

		virtual ~dng_big_table ();
		
		bool IsMissing () const
			{
			return fIsMissing;
			}
		
		void SetMissing ()
			{
			fIsMissing = true;
			}
		
		bool IsEmbedNever () const
			{
			return (GetFlags () & 1) != 0;
			}
		
		void SetEmbedNever ()
			{
			SetFlags (GetFlags () | 1);
			}

		virtual bool IsValid () const = 0;

		const dng_fingerprint & Fingerprint () const;

		bool DecodeFromBinary (dng_host &host,
							   const uint8 * compressedData,
							   uint32 compressedSize,
							   AutoPtr<dng_memory_block> *uncompressedCache = nullptr);
		
		static void ASCIItoBinary (dng_memory_allocator &allocator,
								   const char *sPtr,
								   uint32 sCount,
								   AutoPtr<dng_memory_block> &dBlock,
								   uint32 &dCount);
								   
		bool DecodeFromString (dng_host &host,
							   const dng_string &block1);
		
		dng_memory_block * EncodeAsBinary (dng_memory_allocator &allocator,
										   uint32 &compressedSize) const;
		
		dng_memory_block * EncodeAsString (dng_memory_allocator &allocator) const;

		virtual bool ExtractFromCache (const dng_fingerprint &fingerprint);
		
		#if qDNGUseXMP
		
		bool ReadTableFromXMP (const dng_xmp &xmp,
							   const char *ns,
							   const dng_fingerprint &fingerprint,
							   dng_big_table_storage *storage = nullptr,
							   dng_abort_sniffer *sniffer = nullptr);
		
		bool ReadFromXMP (const dng_xmp &xmp,
						  const char *ns,
						  const char *path,
						  dng_big_table_storage &storage,
						  dng_abort_sniffer *sniffer = NULL);

		void WriteToXMP (dng_xmp &xmp,
						 const char *ns,
						 const char *path,
						 dng_big_table_storage &storage) const;

		#endif	

		#if qDNGValidate

		

		void WriteUncompressedStream (dng_stream &stream) const;

		#endif	

	protected:
	
		virtual dng_fingerprint ComputeFingerprint () const;

		void RecomputeFingerprint ();

		
		

		void SetFingerprintDirect (const dng_fingerprint &digest)
			{
			fFingerprint = digest;
			}
		
		virtual bool UseCompression () const
			{
			return true;
			}

		virtual uint32 MaxCompressedDecodedStreamSize () const;

		virtual bool GetStream (dng_stream &stream) = 0;

		virtual void PutStream (dng_stream &stream,
								bool forFingerprint) const = 0;
		
		virtual uint32 GetFlags () const = 0;
		
		virtual void SetFlags (uint32 flags) = 0;

	};

class dng_big_table_accessor
	{
	
	public:
	
		dng_big_table_accessor ()
			{
			}
			
		virtual ~dng_big_table_accessor ()
			{
			}
									   
		virtual bool HasTable (const dng_fingerprint & ) const
			{
			return false;
			}
		
		virtual bool GetTable (const dng_fingerprint & ,
							   dng_ref_counted_block & ) const
			{
			return false;
			}
	
		virtual void AddTable (const dng_fingerprint & ,
							   const dng_ref_counted_block & )
			{
			ThrowProgramError ("AddTable not implemented");
			}
			
		virtual void CopyToDictionary (dng_big_table_dictionary & ) const
			{
			ThrowProgramError ("CopyToDictionary not implemented");
			}
	
		virtual bool GroupToInstance (const dng_fingerprint & ,
									  dng_fingerprint & ) const
			{
			return false;
			}

	};
	

class dng_big_table_dictionary : public dng_big_table_accessor
	{
	
	private:
	
		std::map<dng_fingerprint, dng_ref_counted_block> fMap;
		
	public:
	
		const std::map<dng_fingerprint, dng_ref_counted_block> & Map () const
			{
			return fMap;
			}
		
		bool IsEmpty () const
			{
			return fMap.empty ();
			}
			
		virtual bool HasTable (const dng_fingerprint &fingerprint) const;
		
		virtual bool GetTable (const dng_fingerprint &fingerprint,
							   dng_ref_counted_block &block) const;
	
		virtual void AddTable (const dng_fingerprint &fingerprint,
							   const dng_ref_counted_block &block);
	
		virtual void CopyToDictionary (dng_big_table_dictionary & ) const;
			
	};
	

class dng_big_table_index
	{
	
	public:
	
		struct IndexEntry
			{
			uint32 fTableSize;
			uint64 fTableOffset;
			};
			
	private:
	
		std::map<dng_fingerprint, IndexEntry> fMap;
		
	public:
	
		dng_big_table_index ();
			
		bool IsEmpty () const
			{
			return fMap.empty ();
			}
			
		const std::map<dng_fingerprint, IndexEntry> & Map () const
			{
			return fMap;
			}
			
		bool HasEntry (const dng_fingerprint &fingerprint) const;

		bool GetEntry (const dng_fingerprint &fingerprint,
					   uint32 &tableSize,
					   uint64 &tableOffset) const;

		void AddEntry (const dng_fingerprint &fingerprint,
					   uint32 tableSize,
					   uint64 tableOffset);
					   
	};

class dng_big_table_group_index
	{
	
	private:
	
		std::map<dng_fingerprint,			 
				 dng_fingerprint> fMap;		 
		
	public:
	
		bool IsEmpty () const
			{
			return fMap.empty ();
			}
			
		const std::map<dng_fingerprint,
					   dng_fingerprint> & Map () const
			{
			return fMap;
			}
			
		bool HasEntry (const dng_fingerprint &groupDigest) const
			{
			return fMap.find (groupDigest) != fMap.end ();
			}

		bool GetEntry (const dng_fingerprint &groupDigest,
					   dng_fingerprint &instanceDigest) const;

		void AddEntry (const dng_fingerprint &groupDigest,
					   const dng_fingerprint &instanceDigest)
			{
			if (fMap.find (groupDigest) == fMap.end ())
				fMap.insert (std::make_pair (groupDigest,
											 instanceDigest));
			}
					   
	};

class dng_big_table_storage
	{
	
	public:
		
		dng_big_table_storage ();
		
		virtual ~dng_big_table_storage ();
		
		virtual bool ReadTable (dng_big_table &table,
								const dng_fingerprint &fingerprint,
								dng_memory_allocator &allocator);
	
		virtual bool WriteTable (const dng_big_table &table,
								 const dng_fingerprint &fingerprint,
								 dng_memory_allocator &allocator);
		
		virtual void MissingTable (const dng_fingerprint &fingerprint);

		virtual bool GroupToInstance (const dng_fingerprint &groupDigest,
									  dng_fingerprint &instanceDigest) const;

	};

class dng_look_table : public dng_big_table
	{

	friend class dng_look_table_cache;

	public:

		enum
			{
			
			
			
			
			
			
			
			
			kMaxTotalSamples = 36 * 32 * 16,
			
			
			
			kMaxHueSamples = 360,
			kMaxSatSamples = 256,
			kMaxValSamples = 256

			};

	private:

		enum
			{
			kLookTableVersion1 = 1,
			kLookTableVersion2 = 2
			};

		

		struct table_data
			{

			
			
			dng_hue_sat_map fMap;

			

			uint32 fEncoding;
			
			

			real64 fMinAmount;
			real64 fMaxAmount;

			
			
			bool fMonochrome;
			
			
			
			uint32 fFlags;

			

			table_data ()

				:	fMap		()
				,	fEncoding	(encoding_Linear)
				,	fMinAmount	(1.0)
				,	fMaxAmount	(1.0)
				,	fMonochrome (false)
				,	fFlags		(0)

				{
				}
				
			
			
			void ComputeMonochrome ()
				{
				
				fMonochrome = true;
				
				uint32 count = fMap.DeltasCount ();

				dng_hue_sat_map::HSBModify * deltas = fMap.GetDeltas ();
				
				for (uint32 index = 0; index < count; index++)
					{

					if (deltas [index] . fSatScale != 0.0f)
						{
						
						fMonochrome = false;
						
						return;
						
						}
					
					}
				
				}

			};

		table_data fData;

		

		real64 fAmount;

	public:

		dng_look_table ();

		dng_look_table (const dng_look_table &table);

		dng_look_table & operator= (const dng_look_table &table);

		virtual ~dng_look_table ();

		bool operator== (const dng_look_table &table) const
			{
			return Fingerprint () == table.Fingerprint () &&
				   Amount	   () == table.Amount	   () &&
				   IsMissing   () == table.IsMissing   ();
			}

		bool operator!= (const dng_look_table &table) const
			{
			return !(*this == table);
			}

		void Set (const dng_hue_sat_map &map,
				  uint32 encoding);

		virtual bool IsValid () const;

		void SetInvalid ();

		real64 MinAmount () const
			{
			return fData.fMinAmount;
			}

		real64 MaxAmount () const
			{
			return fData.fMaxAmount;
			}

		void SetAmountRange (real64 minAmount,
							 real64 maxAmount)
			{

			fData.fMinAmount = Pin_real64 (0.0,
										   Round_int32 (minAmount * 100.0) * 0.01,
										   1.0);
				
			fData.fMaxAmount = Pin_real64 (1.0,
										   Round_int32 (maxAmount * 100.0) * 0.01,
										   2.0);
			
			fAmount = Pin_real64 (fData.fMinAmount, fAmount, fData.fMaxAmount);

			RecomputeFingerprint ();

			}

		real64 Amount () const
			{
			return fAmount;
			}

		void SetAmount (real64 amount)
			{

			fAmount = Pin_real64 (fData.fMinAmount,
								  Round_int32 (amount * 100.0) * 0.01,
								  fData.fMaxAmount);

			

			}

		const dng_hue_sat_map & Map () const
			{
			return fData.fMap;
			}

		uint32 Encoding () const
			{
			return fData.fEncoding;
			}
		
		bool Monochrome () const
			{
			return IsValid () && fAmount == 1.0 && fData.fMonochrome;
			}

	protected:

		virtual bool GetStream (dng_stream &stream);

		virtual void PutStream (dng_stream &stream,
								bool forFingerprint) const;

		virtual uint32 GetFlags () const
			{
			return fData.fFlags;
			}
		
		virtual void SetFlags (uint32 flags)
			{
			
			fData.fFlags = flags;
			
			RecomputeFingerprint ();

			}

	};

class dng_rgb_table : public dng_big_table
	{

	friend class dng_rgb_table_cache;

	public:

		enum
			{

			kMinDivisions1D = 2,
			kMaxDivisions1D = 4096,

			kMinDivisions3D = 2,
			kMaxDivisions3D = 32,

			kMaxDivisions3D_InMemory = 130

			};

		enum primaries_enum
			{

			primaries_sRGB = 0,
			primaries_Adobe,
			primaries_ProPhoto,
			primaries_P3,
			primaries_Rec2020,

			primaries_count

			};

		enum gamma_enum
			{

			gamma_Linear = 0,
			gamma_sRGB,
			gamma_1_8,
			gamma_2_2,
			gamma_Rec2020,

			gamma_count

			};

		enum gamut_enum
			{

			gamut_clip = 0,
			gamut_extend,

			gamut_count

			};

	private:

		enum
			{
			kRGBTableVersion = 1
			};

		

		struct table_data
			{

			

			uint32 fDimensions;

			
			
			uint32 fDivisions;

			
			
			
			dng_ref_counted_block fSamples;

			

			primaries_enum fPrimaries;

			

			gamma_enum fGamma;

			

			gamut_enum fGamut;

			

			real64 fMinAmount;
			real64 fMaxAmount;

			
			
			bool fMonochrome;
			
			
			
			uint32 fFlags;

			

			table_data ()

				:	fDimensions (0)
				,	fDivisions	(0)
				,	fSamples	()
				,	fPrimaries	(primaries_sRGB)
				,	fGamma		(gamma_sRGB)
				,	fGamut		(gamut_clip)
				,	fMinAmount	(0.0)
				,	fMaxAmount	(2.0)
				,	fMonochrome (false)
				,	fFlags		(0)

				{
				}

			
			
			void ComputeMonochrome ()
				{
				
				if (fPrimaries != primaries_ProPhoto &&
					fGamut	   != gamut_clip)
					{
					
					fMonochrome = false;
					
					return;
					
					}
					
				if (fDimensions != 3)
					{
					
					fMonochrome = false;
					
					return;
					
					}
				
				fMonochrome = true;
				
				uint32 count = SafeUint32Mult (fDivisions,
											   fDivisions,
											   fDivisions);

				const uint16 * sample = fSamples.Buffer_uint16 ();
				
				for (uint32 index = 0; index < count; index++)
					{
					
					if (sample [0] != sample [1] ||
						sample [0] != sample [2])
						{

						fMonochrome = false;
						
						return;
						
						}
						
					sample += 4;
					
					}
					
				}

			};

		table_data fData;

		

		real64 fAmount;

	public:

		dng_rgb_table ();

		dng_rgb_table (const dng_rgb_table &table);

		dng_rgb_table & operator= (const dng_rgb_table &table);

		virtual ~dng_rgb_table ();

		bool operator== (const dng_rgb_table &table) const
			{
			return Fingerprint () == table.Fingerprint () &&
				   Amount	   () == table.Amount	   () &&
				   IsMissing   () == table.IsMissing   ();
			}

		bool operator!= (const dng_rgb_table &table) const
			{
			return !(*this == table);
			}

		virtual bool IsValid () const;

		void SetInvalid ();

		primaries_enum Primaries () const
			{
			return fData.fPrimaries;
			}

		void SetPrimaries (primaries_enum primaries)
			{

			fData.fPrimaries = primaries;

			fData.ComputeMonochrome ();

			RecomputeFingerprint ();

			}

		gamma_enum Gamma () const
			{
			return fData.fGamma;
			}

		void SetGamma (gamma_enum gamma)
			{

			fData.fGamma = gamma;

			RecomputeFingerprint ();

			}

		gamut_enum Gamut () const
			{
			return fData.fGamut;
			}

		void SetGamut (gamut_enum gamut)
			{

			fData.fGamut = gamut;

			fData.ComputeMonochrome ();

			RecomputeFingerprint ();

			}

		real64 MinAmount () const
			{
			return fData.fMinAmount;
			}

		real64 MaxAmount () const
			{
			return fData.fMaxAmount;
			}

		void SetAmountRange (real64 minAmount,
							 real64 maxAmount)
			{

			fData.fMinAmount = Pin_real64 (0.0,
										   Round_int32 (minAmount * 100.0) * 0.01,
										   1.0);
				
			fData.fMaxAmount = Pin_real64 (1.0,
										   Round_int32 (maxAmount * 100.0) * 0.01,
										   2.0);
			
			fAmount = Pin_real64 (fData.fMinAmount, fAmount, fData.fMaxAmount);

			RecomputeFingerprint ();

			}

		real64 Amount () const
			{
			return fAmount;
			}

		void SetAmount (real64 amount)
			{

			fAmount = Pin_real64 (fData.fMinAmount,
								  Round_int32 (amount * 100.0) * 0.01,
								  fData.fMaxAmount);

			

			}

		uint32 Dimensions () const
			{
			return fData.fDimensions;
			}

		uint32 Divisions () const
			{
			return fData.fDivisions;
			}

		const uint16 * Samples () const
			{
			return fData.fSamples.Buffer_uint16 ();
			}

		bool Monochrome () const
			{
			return IsValid () && fAmount == 1.0 && fData.fMonochrome;
			}

		void Set (uint32 dimensions,
				  uint32 divisions,
				  dng_ref_counted_block samples);

	protected:

		virtual bool GetStream (dng_stream &stream);

		virtual void PutStream (dng_stream &stream,
								bool forFingerprint) const;

		virtual uint32 GetFlags () const
			{
			return fData.fFlags;
			}

		virtual void SetFlags (uint32 flags)
			{
			
			fData.fFlags = flags;
			
			RecomputeFingerprint ();

			}

	};

class dng_image_table_compression_info
	{

	public:

		virtual ~dng_image_table_compression_info ();
		
		virtual uint32 Type () const
			{
			return 0;
			}

		virtual void Compress (dng_host &host,
							   dng_stream &stream,
							   const dng_image &image) const;
		
	};

class dng_image_table_jxl_compression_info : public dng_image_table_compression_info
	{

	public:

		AutoPtr<dng_jxl_encode_settings> fEncodeSettings;

		std::shared_ptr<const dng_jxl_color_space_info> fColorSpaceInfo;

		
		
		
		bool fPreferHalfFloat = true;

	public:

		dng_image_table_jxl_compression_info ();

		uint32 Type () const override
			{
			return ccJXL;
			}
		
		void Compress (dng_host &host,
					   dng_stream &stream,
					   const dng_image &image) const override;
		
	};

class dng_image_table_data
	{

	public:
		
		std::shared_ptr<const dng_image> fImage;

		mutable std::shared_ptr<const dng_memory_block> fCompressedData;

		uint32 fCompressionType = 0;
		
	};

class dng_image_table : public dng_big_table
	{
	
	friend class dng_image_table_cache;
	friend class dng_packed_image_table_cache;

	private:

		enum
			{
			kImageTableVersion = 1
			};

	protected:
	
		std::shared_ptr<const dng_image> fImage;

		
		

		mutable std::shared_ptr<const dng_memory_block> fCompressedData;

		
		
		uint32 fCompressionType = 0;
		
	public:
	
		dng_image_table ();

		dng_image_table (const dng_image_table &table);

		dng_image_table & operator= (const dng_image_table &table);

		virtual ~dng_image_table ();
	
		virtual bool IsValid () const;
		
		void SetInvalid ();

		bool operator== (const dng_image_table &table) const
			{
			return Fingerprint () == table.Fingerprint () &&
				   IsMissing   () == table.IsMissing   ();
			}

		bool operator!= (const dng_image_table &table) const
			{
			return !(*this == table);
			}
			
		const dng_image & Image () const
			{
			return *fImage;
			}
			
		std::shared_ptr<const dng_image> ShareImage () const
			{
			return fImage;
			};
			
		void SetImage (const dng_image *image,
					   const dng_image_table_compression_info *compressionInfo = nullptr,
					   dng_abort_sniffer *sniffer = nullptr);

		void SetImage (const std::shared_ptr<const dng_image> &image,
					   const dng_image_table_compression_info *compressionInfo = nullptr,
					   dng_abort_sniffer *sniffer = nullptr);

		uint32 CompressionType () const
			{
			return fCompressionType;
			}

		std::shared_ptr<const dng_memory_block> CompressedData () const
			{
			return fCompressedData;
			}

	private:

		void SetData (const dng_image_table_data &data);

		void GetData (dng_image_table_data &data) const;

	protected:

		virtual dng_host * MakeHost (dng_abort_sniffer *sniffer) const;

		virtual dng_fingerprint ComputeFingerprint () const;

		
		
		
		
		
		virtual bool UseCompression () const
			{
			return false;
			}
			
		virtual bool GetStream (dng_stream &stream);

		virtual void PutStream (dng_stream &stream,
								bool forFingerprint) const;
		
		virtual uint32 GetFlags () const
			{
			return 0;
			}

		virtual void SetFlags (uint32 )
			{
			}

		virtual void CompressImage (const dng_image_table_compression_info &info,
									dng_abort_sniffer *sniffer);

	protected:
		
		virtual void
			PutCompressedStream (dng_stream &stream,
								 bool forFingerprint,
								 const dng_image_table_compression_info &info) const;

	};

class dng_packed_image_table : public dng_big_table
	{

	friend class dng_packed_image_table_cache;
	
	private:

		enum
			{
			kPackedImageTableVersion = 2
			};

	protected:

		

		dng_fingerprint fTableDigest;

		

		std::unique_ptr<dng_image_table> fTable;

		
		
		std::shared_ptr<const dng_memory_block> fBlock;

		

		dng_point fSize;

		uint32 fPlanes = 0;

		uint32 fPixelType = 0;

	public:

		dng_packed_image_table ();
		
		dng_packed_image_table (const dng_packed_image_table &table);

		dng_packed_image_table & operator= (const dng_packed_image_table &table);

	public:

		const dng_fingerprint & TableDigest () const
			{
			return fTableDigest;
			}

		const dng_image_table & Table () const;

		const dng_image & Image () const
			{
			return Table ().Image ();
			}

		const_dng_image_sptr ShareImage () const
			{
			return fTable ? fTable->ShareImage () : nullptr;
			}

		std::shared_ptr<const dng_memory_block> ShareBlock () const
			{
			return fBlock;
			}

		bool IsValid () const override;

		bool HasTable () const
			{
			return fTable && fTable->IsValid ();
			}

		bool IsValidUnpacked () const
			{
			return HasTable ();
			}

		bool IsPacked () const
			{
			return fBlock != nullptr;
			}

		
		

		bool UseCompression () const override
			{
			return false;
			}

		uint32 PackedBytes () const;

		

		const dng_point & Size () const
			{
			return fSize;
			}

		uint32 Planes () const
			{
			return fPlanes;
			}

		uint32 PixelType () const
			{
			return fPixelType;
			}

	public:

		bool operator== (const dng_packed_image_table &table) const
			{
			return (Fingerprint () == table.Fingerprint () &&
					IsMissing	() == table.IsMissing	());
			}

		bool operator!= (const dng_packed_image_table &table) const
			{
			return !(*this == table);
			}
			
	public:

		

		virtual dng_host * MakeHost (dng_abort_sniffer *sniffer) const;

		virtual dng_image_table * MakeTable () const;

		virtual dng_image_table * CloneTable () const;

	public:

		void Clear ();

		void ClearPackedData ();

		void SetImage (const dng_image *image,
					   const dng_image_table_compression_info *compressionInfo = nullptr,
					   dng_abort_sniffer *sniffer = nullptr);

		void SetImage (const std::shared_ptr<const dng_image> &image,
					   const dng_image_table_compression_info *compressionInfo = nullptr,
					   dng_abort_sniffer *sniffer = nullptr);

		void Unpack (dng_abort_sniffer *sniffer);

		void Pack (dng_abort_sniffer *sniffer);

	protected:

		bool GetStream (dng_stream &stream) override;

		void PutStream (dng_stream &stream,
						bool forFingerprint) const override;
		
		uint32 GetFlags () const override
			{
			return 0;
			}
		
		void SetFlags (uint32 ) override
			{
			}

		dng_fingerprint ComputeFingerprint () const override
			{
			return fTableDigest;
			}
		
	};
	

class dng_masked_rgb_table: private dng_uncopyable
	{
		
	private:

		

		dng_string fTableSemanticName;

		
		
		
		
		
		
		
		
		uint32 fPixelType = 0;

		
		
		
		
		
		
		

		dng_rgb_table fTable;

		
		
		

		std::shared_ptr<dng_memory_block> fStoredData;

	public:

		dng_masked_rgb_table ();

		dng_masked_rgb_table (dng_string tableSemanticName,
							  uint32 pixelType,
							  const dng_rgb_table &table);

		void Validate () const;
		
		void GetStream (dng_host &host,
						dng_stream &stream,
						uint64 tagEnd);
		
		void PutStream (dng_stream &stream) const;

		void AddDigest (dng_md5_printer_stream &printer) const;

		const dng_string & SemanticName () const
			{
			return fTableSemanticName;
			}

		const dng_rgb_table & Table () const
			{
			return fTable;
			}

		uint32 PixelType () const
			{
			return fPixelType;
			}

		#if qDNGValidate
		void Dump (uint32 indent) const;
		#endif

	private:

		static void CheckDivisions (uint32 divisions);

		static void CheckPixelType (uint32 pixelType);

		static void CheckGammaEncoding (dng_rgb_table::gamma_enum gamma);

		static void CheckGamutExtension (dng_rgb_table::gamut_enum gamut);

		static void CheckColorPrimaries (dng_rgb_table::primaries_enum primaries);
		
	};

class dng_masked_rgb_tables: private dng_uncopyable
	{

	public:
		
		static const uint32 kMaxTables = 20;

		enum composite_method
			{
			kWeightedSum = 0,
			kSequential	 = 1,
			};

	private:

		composite_method fCompositeMethod = kWeightedSum;

		std::vector<std::shared_ptr<dng_masked_rgb_table> > fTables;

	public:

		dng_masked_rgb_tables ();

		dng_masked_rgb_tables
			(const std::vector<std::shared_ptr<dng_masked_rgb_table> > &tables,
			 composite_method compositeMethod);

		const std::vector<std::shared_ptr<dng_masked_rgb_table> > & Tables () const
			{
			return fTables;
			}
		
		bool IsNOP () const;

		void Validate () const;

		void AddDigest (dng_md5_printer_stream &printer) const;

		void PutStream (dng_stream &stream) const;

		static dng_masked_rgb_tables * GetStream (dng_host &host,
												  dng_stream &stream,
												  bool isDraft,
												  uint32 tagCount);

		composite_method CompositeMethod () const
			{
			return fCompositeMethod;
			}

		bool UseSequentialMethod () const
			{
			return (fCompositeMethod == kSequential);
			}

		bool UseWeightedSumMethod () const
			{
			return !UseSequentialMethod ();
			}

		void SetCompositeMethod (composite_method method)
			{
			fCompositeMethod = method;
			}

		#if qDNGValidate
		void Dump () const;
		#endif
		
	};

class dng_rgb_to_rgb_table_data
	{

	public:

		dng_rgb_table fTable;

		bool fNeedMatrix;

		dng_matrix fEncodeMatrix;
		dng_matrix fDecodeMatrix;
		
		AutoPtr<dng_1d_table> fEncodeTable;
		AutoPtr<dng_1d_table> fDecodeTable;

		AutoPtr<dng_1d_table> fTable1D [3];

	public:

		dng_rgb_to_rgb_table_data (dng_host &host,
								   const dng_rgb_table &table);

		virtual ~dng_rgb_to_rgb_table_data ();

		virtual void Process_32 (dng_pixel_buffer &buffer,
								 dng_pixel_buffer *optMaskBuffer, 
								 uint32 optMaskPlane,
								 const dng_rect &dstArea,
								 uint32 bufferStartPlane,
								 bool needOverrange);

		void AddDigest (dng_md5_printer_stream &printer) const;

	};

class dng_masked_rgb_table_render_data
	{

	public:

		bool fUseSequentialMethod = false;
		
		
		

		std::vector<std::pair<dng_masked_rgb_table_sptr,
							  dng_semantic_mask> > fMaskedTables;

		
		

		std::vector<dng_rgb_to_rgb_table_data_sptr> fMaskedTableData;

		
		
		
		dng_masked_rgb_table_sptr fBackgroundTable;

		AutoPtr<dng_rgb_to_rgb_table_data> fBackgroundTableData;

		
		
		
		
		

		uint32 fBackgroundTableIndex = 0;

		
		
		
		
		

		dng_fingerprint fInputDigest;

	public:

		void Initialize (const dng_negative &negative,
						 const dng_camera_profile &profile);

		bool IsNOP () const
			{
			return (fMaskedTables.empty () && !fBackgroundTable);
			}

		void PrepareRGBtoRGBTableData (dng_host &host);
		
	};

void MoveBigTablesToDictionary (dng_xmp &xmp,
								dng_big_table_dictionary &dictionary);

void DualParseXMP (dng_host &host,
				   dng_xmp &xmp,
				   dng_big_table_dictionary &dictionary,
				   const void *blockData,
				   uint32 blockSize);

#endif	
	

