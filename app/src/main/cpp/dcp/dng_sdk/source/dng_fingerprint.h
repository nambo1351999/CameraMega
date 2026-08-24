

#ifndef __dng_fingerprint__
#define __dng_fingerprint__

#include "dng_exceptions.h"
#include "dng_types.h"
#include "dng_stream.h"
#include "dng_string.h"

#include <cstring>
#include <unordered_set>
#include <vector>

class dng_fingerprint
	{

	friend struct dng_fingerprint_less_than;
	friend class dng_md5_printer;
	
	public:
	
		static const size_t kDNGFingerprintSize = 16;

	private:
		
		uint8 data [kDNGFingerprintSize];
		
	public:
	
		dng_fingerprint ();
		
		explicit dng_fingerprint (const char *hex);
		
		dng_fingerprint (const dng_fingerprint& print);

		dng_fingerprint& operator= (const dng_fingerprint& print);

		

		bool IsNull () const;

		

		bool IsValid () const
			{
			return !IsNull ();
			}

		

		void Clear ()
			{
			*this = dng_fingerprint ();
			}

		const uint8 * Data () const
			{
			return data;
			}

		uint8 * MutableData ()
			{
			return data;
			}

		
			
		bool operator== (const dng_fingerprint &print) const;
		
		

		bool operator!= (const dng_fingerprint &print) const
			{
			return !(*this == print);
			}
			
		
			
		bool operator< (const dng_fingerprint &print) const;
		
		
		
			
		uint32 Collapse32 () const; 

		
		
		
		

		void ToUtf8HexString (char resultStr [2 * kDNGFingerprintSize + 1]) const;

		

		dng_string ToUtf8HexString () const;
		
		
		

		static int HexCharToNum (char hexChar);
		
		
		
		
		
		
		
		

		bool FromUtf8HexString (const char inputStr [2 * kDNGFingerprintSize + 1]);

		
		
		
		
		
		
		

		bool FromUtf8HexString (const dng_string &inputStr);

		
		
		

		dng_string ToUtf8ClosestUUIDString () const;

	};

struct dng_fingerprint_less_than
	{

	

	bool operator() (const dng_fingerprint &a, 
					 const dng_fingerprint &b) const
		{

		return memcmp (a.data, 
					   b.data, 
					   sizeof (a.data)) < 0;

		}

	};

struct dng_fingerprint_hash
	{

	

	size_t operator () (const dng_fingerprint &digest) const
		{

		return (size_t) digest.Collapse32 ();

		}

	};

typedef std::unordered_set<dng_fingerprint,
						   dng_fingerprint_hash> dng_fingerprint_table;

typedef std::vector<dng_fingerprint> dng_fingerprint_vector;

class dng_md5_printer
	{
	
	protected:
	
		dng_md5_printer ();
		
	public:
	
		virtual ~dng_md5_printer ()
			{
			}
		
		

		void Reset ();
		
		
		
		

		virtual void ProcessPtr (const void *data,
								 uint32 inputLen);
					  
		
		

		void ProcessString (const char *text)
			{
			
			ProcessPtr (text, (uint32) strlen (text));
			
			}

		void Process (const dng_fingerprint &digest)
			{

			ProcessPtr (digest.data,
						uint32 (sizeof (digest.data)));

			}

		void Process (const dng_string &str)
			{

			ProcessPtr (str.Get	   (),
						str.Length ());

			}

		
		

		void Process_bool (bool x);
		
		
		

		void Process_size (size_t x);
		
		

		virtual const dng_fingerprint & Result ();
		
	private:
	
		static void Encode (uint8 *output,
							const uint32 *input,
							uint32 len);

		static void Decode (uint32 *output,
							const uint8 *input,
							uint32 len);
							
		

		static inline uint32 F (uint32 x,
								uint32 y,
								uint32 z)
			{
			return (x & y) | (~x & z);
			}
			
		static inline uint32 G (uint32 x,
								uint32 y,
								uint32 z)
			{
			return (x & z) | (y & ~z);
			}
			
		static inline uint32 H (uint32 x,
								uint32 y,
								uint32 z)
			{
			return x ^ y ^ z;
			}
			
		static inline uint32 I (uint32 x,
								uint32 y,
								uint32 z)
			{
			return y ^ (x | ~z);
			}
			
		
		
		DNG_ATTRIB_NO_SANITIZE("unsigned-integer-overflow")
		static inline void FF (uint32 &a,
							   uint32 b,
							   uint32 c,
							   uint32 d,
							   uint32 x,
							   uint32 s,
							   uint32 ac)
			{
			a += F (b, c, d) + x + ac;
			a = (a << s) | (a >> (32 - s));
			a += b;
			}

		DNG_ATTRIB_NO_SANITIZE("unsigned-integer-overflow")
		static inline void GG (uint32 &a,
							   uint32 b,
							   uint32 c,
							   uint32 d,
							   uint32 x,
							   uint32 s,
							   uint32 ac)
			{
			a += G (b, c, d) + x + ac;
			a = (a << s) | (a >> (32 - s));
			a += b;
			}

		DNG_ATTRIB_NO_SANITIZE("unsigned-integer-overflow")
		static inline void HH (uint32 &a,
							   uint32 b,
							   uint32 c,
							   uint32 d,
							   uint32 x,
							   uint32 s,
							   uint32 ac)
			{
			a += H (b, c, d) + x + ac;
			a = (a << s) | (a >> (32 - s));
			a += b;
			}

		DNG_ATTRIB_NO_SANITIZE("unsigned-integer-overflow")
		static inline void II (uint32 &a,
							   uint32 b,
							   uint32 c,
							   uint32 d,
							   uint32 x,
							   uint32 s,
							   uint32 ac)
			{
			a += I (b, c, d) + x + ac;
			a = (a << s) | (a >> (32 - s));
			a += b;
			}

		static void MD5Transform (uint32 state [4],
								  const uint8 block [64]);
		
	private:
	
		uint32 state [4];
		
		uint32 count [2];
		
		uint8 buffer [64];
		
		bool final;
		
		dng_fingerprint result;
		
	};

class dng_md5_printer_stream : public dng_stream, public dng_md5_printer
	{
	
	private:
	
		uint64 fNextOffset;

	public:

		

		dng_md5_printer_stream ()
		
			:	fNextOffset (0)
			
			{
			}

		void ProcessPtr (const void *data,
						 uint32 inputLen) override
			{
			
			
			

			Flush ();
			
			dng_md5_printer::ProcessPtr (data,
										 inputLen);
			
			}
					  
		uint64 DoGetLength () override
			{
			
			return fNextOffset;
			
			}
	
		void DoRead (void * ,
					 uint32 ,
					 uint64 ) override
			{
			
			ThrowProgramError ();
			
			}
							 
		void DoSetLength (uint64 length) override
			{
			
			if (length != fNextOffset)
				{
				ThrowProgramError ();
				}
				
			}
							 
		void DoWrite (const void *data,
					  uint32 count2,
					  uint64 offset) override
			{
			
			if (offset != fNextOffset)
				{
				ThrowProgramError ();
				}
				
			dng_md5_printer::ProcessPtr (data, count2);
			
			fNextOffset += count2;
			
			}

		const dng_fingerprint & Result () override
			{
			
			Flush ();
			
			return dng_md5_printer::Result ();
			
			}

	};

class dng_md5_printer_le_stream : public dng_md5_printer_stream
	{

	public:

		dng_md5_printer_le_stream ()
			{
			SetLittleEndian ();
			}
	
	};

class dng_md5_direct_printer: public dng_md5_printer
	{

	public:

		dng_md5_direct_printer ()
			{
			}

	};

#endif	
	

