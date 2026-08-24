

#include "dng_fingerprint.h"

#include "dng_assertions.h"
#include "dng_flags.h"
#include "dng_types.h"

#include <cstdint>

dng_fingerprint::dng_fingerprint ()
	{
	
	for (uint32 j = 0; j < kDNGFingerprintSize; j++)
		{
		
		data [j] = 0;
		
		}
		
	}

dng_fingerprint::dng_fingerprint (const char *hex)
	{
	
	if (!hex || strlen (hex) != kDNGFingerprintSize * 2 || !FromUtf8HexString (hex))
		{
		
		Clear ();
		
		}
		
	}
		

dng_fingerprint::dng_fingerprint (const dng_fingerprint& print)

	{

	if (this != &print)
		{

		for (uint32 j = 0; j < kDNGFingerprintSize; j++)
			{

			data [j] = print.data [j];

			}
		}

	}

dng_fingerprint& dng_fingerprint::operator= (const dng_fingerprint& print)
	{

	if (this != &print)
		{

		for (uint32 j = 0; j < kDNGFingerprintSize; j++)
			{

			data [j] = print.data [j];

			}
		}

	return *this;

	}

bool dng_fingerprint::IsNull () const
	{
	
	for (uint32 j = 0; j < kDNGFingerprintSize; j++)
		{
		
		if (data [j] != 0)
			{
			
			return false;
			
			}
			
		}
		
	return true;
	
	}

bool dng_fingerprint::operator== (const dng_fingerprint &print) const
	{
	
	for (uint32 j = 0; j < kDNGFingerprintSize; j++)
		{
		
		if (data [j] != print.data [j])
			{
			
			return false;
			
			}
			
		}
		
	return true;
	
	}

bool dng_fingerprint::operator< (const dng_fingerprint &print) const
	{
	
	for (uint32 j = 0; j < kDNGFingerprintSize; j++)
		{
		
		if (data [j] != print.data [j])
			{
		
			return data [j] < print.data [j];
			
			}
			
		}
		
	return false;
	
	}

uint32 dng_fingerprint::Collapse32 () const
	{
	
	uint32 x = 0;
	
	for (uint32 j = 0; j < 4; j++)
		{
		
		uint32 y = 0;
		
		for (uint32 k = 0; k < 4; k++)
			{
			
			y = (y << 8) + (uint32) data [j * 4 + k];
			
			}
			
		x = x ^ y;
		
		}
		
	return x;
	
	}

static char NumToHexChar (unsigned int c)
	{

	if (c < 10)
		{
		return (char) ('0' + c);
		}

	else
		{
		return (char) ('A' + c - 10);
		}

	}

void dng_fingerprint::ToUtf8HexString (char resultStr [2 * kDNGFingerprintSize + 1]) const
	{
	
	for (size_t i = 0; i < kDNGFingerprintSize; i++)
		{
		
		unsigned char c = data [i];

		resultStr [i * 2	] = NumToHexChar (c >> 4);
		resultStr [i * 2 + 1] = NumToHexChar (c & 15);
		
		}
	
	resultStr [kDNGFingerprintSize * 2] = '\0';

	}

dng_string dng_fingerprint::ToUtf8HexString () const
	{
	
	char buf [2 * kDNGFingerprintSize + 1];

	ToUtf8HexString (buf);

	return dng_string (buf);
	
	}

int dng_fingerprint::HexCharToNum (char hexChar)
	{
	
	if (hexChar >= '0' && hexChar <= '9')
		{
		return hexChar - '0';
		}

	else if (hexChar >= 'A' && hexChar <= 'F')
		{
		return hexChar - 'A' + 10;
		}

	else if (hexChar >= 'a' && hexChar <= 'f')
		{
		return hexChar - 'a' + 10;
		}
	
	return -1;
	
	}

bool dng_fingerprint::FromUtf8HexString (const char inputStr [2 * kDNGFingerprintSize + 1])
	{
	
	for (size_t i = 0; i < kDNGFingerprintSize; i++)
		{
		
		int highNibble = HexCharToNum (inputStr [i * 2]);

		if (highNibble < 0)
			{
			return false;
			}
		
		int lowNibble = HexCharToNum (inputStr [i * 2 + 1]);

		if (lowNibble < 0)
			{
			return false;
			}

		data [i] = (uint8) ((highNibble << 4) + lowNibble);
		
		}
	
	return true;

	}

dng_string dng_fingerprint::ToUtf8ClosestUUIDString () const
	{

	

	uint8 d [kDNGFingerprintSize];

	memcpy (d, data, kDNGFingerprintSize);

	d [6] = (d [6] & 0x0F) | 0x30;   
	d [8] = (d [8] & 0x3F) | 0x80;   

	char buf [37];

	snprintf (buf, sizeof (buf),
			  "%02x%02x%02x%02x-"
			  "%02x%02x-"
			  "%02x%02x-"
			  "%02x%02x-"
			  "%02x%02x%02x%02x%02x%02x",
			  d [0],  d [1],  d [2],  d [3],
			  d [4],  d [5],
			  d [6],  d [7],
			  d [8],  d [9],
			  d [10], d [11], d [12], d [13], d [14], d [15]);

	return dng_string (buf);

	}

bool dng_fingerprint::FromUtf8HexString (const dng_string &inputStr)
	{

	
	
	
	
	

	if (inputStr.Length () < kDNGFingerprintSize * 2)
		{
		return false;
		}

	return FromUtf8HexString (inputStr.Get ());

	}

dng_md5_printer::dng_md5_printer ()

	:	final  (false)
	,	result ()
	
	{
	
	Reset ();
	
	}
								

void dng_md5_printer::Reset ()
	{
	
	
	
	count [0] = 0;
	count [1] = 0;

	

	state [0] = 0x67452301;
	state [1] = 0xefcdab89;
	state [2] = 0x98badcfe;
	state [3] = 0x10325476;

	
	
	final = false;
	
	}

void dng_md5_printer::ProcessPtr (const void *data,
								  uint32 inputLen)
	{
	
	DNG_ASSERT (!final, "Fingerprint already finalized!");
	
	const uint8 *input = (const uint8 *) data;
	
	
	
	uint32 index = (count [0] >> 3) & 0x3F;

	
	
	if ((count [0] += inputLen << 3) < (inputLen << 3))
		{
		count [1]++;
		}
		
	count [1] += inputLen >> 29;

	
	
	uint32 i = 0;

	uint32 partLen = 64 - index;

	if (inputLen >= partLen)
		{
		
		memcpy (&buffer [index],
				input,
				partLen);
				
		MD5Transform (state, buffer);

		for (i = partLen; i + 63 < inputLen; i += 64)
			{
			
			MD5Transform (state, &input [i]);
			
			}

		index = 0;
		
		}
		
	
	
	memcpy (&buffer [index],
			&input [i],
			inputLen - i);
	
	}

void dng_md5_printer::Process_bool (bool x)
	{

	

	
	
	
	std::uint8_t value = x ? 1 : 0;

	static_assert (sizeof (value) == 1, "uint8_t not 1 byte?");

	ProcessPtr (&value, 1);

	}

void dng_md5_printer::Process_size (size_t x)
	{

	

	
	

	std::uint64_t value = (std::uint64_t) x;

	static_assert (sizeof (value) == 8, "uint64_t not 8 bytes?");

	ProcessPtr (&value, 8);
	
	}
		

const dng_fingerprint & dng_md5_printer::Result ()
	{
	
	if (!final)
		{
		
		static uint8 PADDING [64] =
			{
			0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
			};

		
		
		uint8 bits [8];

		Encode (bits, count, 8);

		
		
		uint32 index = (count [0] >> 3) & 0x3f;
		
		uint32 padLen = (index < 56) ? (56 - index) : (120 - index);
		
		ProcessPtr (PADDING, padLen);

		

		ProcessPtr (bits, 8);

		
		
		Encode (result.data, state, 16);

		
		
		final = true;
		
		}
	
	return result;

	}

void dng_md5_printer::Encode (uint8 *output,
							  const uint32 *input,
							  uint32 len)
	{

	uint32 i, j;
	
	for (i = 0, j = 0; j < len; i++, j += 4)
		{
		output [j  ] = (uint8) ((input [i]		) & 0xff);
		output [j+1] = (uint8) ((input [i] >>  8) & 0xff);
		output [j+2] = (uint8) ((input [i] >> 16) & 0xff);
		output [j+3] = (uint8) ((input [i] >> 24) & 0xff);
		}

	}

void dng_md5_printer::Decode (uint32 *output,
							  const uint8 *input,
							  uint32 len)
	{
	
	
	
	if (((uintptr) input) & 3)
		{

		uint32 i, j;
	
		for (i = 0, j = 0; j < len; i++, j += 4)
			{
			
			output [i] = (((uint32) input [j  ])	  ) |
						 (((uint32) input [j+1]) <<	 8) |
						 (((uint32) input [j+2]) << 16) |
						 (((uint32) input [j+3]) << 24);
	   
			}
			
		}
		
	
		
	else
		{
		
		len = len >> 2;
		
		const uint32 *sPtr = (const uint32 *) input;
		
		uint32 *dPtr = output;
		
		while (len--)
			{
			
			#if qDNGBigEndian
			
			uint32 data = *(sPtr++);
			
			data = (data >> 24) |
				   ((data >> 8) & 0x0000FF00) |
				   ((data << 8) & 0x00FF0000) |
				   (data << 24);
				   
			*(dPtr++) = data;
			
			#else
			
			*(dPtr++) = *(sPtr++);
			
			#endif

			}
				
		}
   
	}

DNG_ATTRIB_NO_SANITIZE("unsigned-integer-overflow")
void dng_md5_printer::MD5Transform (uint32 state [4],
									const uint8 block [64])
	{
	
	enum
		{
		S11 = 7,
		S12 = 12,
		S13 = 17,
		S14 = 22,
		S21 = 5,
		S22 = 9,
		S23 = 14,
		S24 = 20,
		S31 = 4,
		S32 = 11,
		S33 = 16,
		S34 = 23,
		S41 = 6,
		S42 = 10,
		S43 = 15,
		S44 = 21
		};
		
	#if qDNGBigEndian

	uint32 x [16];

	Decode (x, block, 64);
	
	#else

	uint32 temp [16];

	const uint32 *x;
	
	if (((uintptr) block) & 3)
		{
		
		Decode (temp, block, 64);
		
		x = temp;
		
		}
		
	else
		x = (const uint32 *) block;
		
	#endif

	uint32 a = state [0];
	uint32 b = state [1];
	uint32 c = state [2];
	uint32 d = state [3];
	
	
	FF (a, b, c, d, x[ 0], S11, 0xd76aa478); 
	FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); 
	FF (c, d, a, b, x[ 2], S13, 0x242070db); 
	FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); 
	FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); 
	FF (d, a, b, c, x[ 5], S12, 0x4787c62a); 
	FF (c, d, a, b, x[ 6], S13, 0xa8304613); 
	FF (b, c, d, a, x[ 7], S14, 0xfd469501); 
	FF (a, b, c, d, x[ 8], S11, 0x698098d8); 
	FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); 
	FF (c, d, a, b, x[10], S13, 0xffff5bb1); 
	FF (b, c, d, a, x[11], S14, 0x895cd7be); 
	FF (a, b, c, d, x[12], S11, 0x6b901122); 
	FF (d, a, b, c, x[13], S12, 0xfd987193); 
	FF (c, d, a, b, x[14], S13, 0xa679438e); 
	FF (b, c, d, a, x[15], S14, 0x49b40821); 

	
	GG (a, b, c, d, x[ 1], S21, 0xf61e2562); 
	GG (d, a, b, c, x[ 6], S22, 0xc040b340); 
	GG (c, d, a, b, x[11], S23, 0x265e5a51); 
	GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); 
	GG (a, b, c, d, x[ 5], S21, 0xd62f105d); 
	GG (d, a, b, c, x[10], S22,	 0x2441453); 
	GG (c, d, a, b, x[15], S23, 0xd8a1e681); 
	GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); 
	GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); 
	GG (d, a, b, c, x[14], S22, 0xc33707d6); 
	GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); 
	GG (b, c, d, a, x[ 8], S24, 0x455a14ed); 
	GG (a, b, c, d, x[13], S21, 0xa9e3e905); 
	GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); 
	GG (c, d, a, b, x[ 7], S23, 0x676f02d9); 
	GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); 

	
	HH (a, b, c, d, x[ 5], S31, 0xfffa3942); 
	HH (d, a, b, c, x[ 8], S32, 0x8771f681); 
	HH (c, d, a, b, x[11], S33, 0x6d9d6122); 
	HH (b, c, d, a, x[14], S34, 0xfde5380c); 
	HH (a, b, c, d, x[ 1], S31, 0xa4beea44); 
	HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); 
	HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); 
	HH (b, c, d, a, x[10], S34, 0xbebfbc70); 
	HH (a, b, c, d, x[13], S31, 0x289b7ec6); 
	HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); 
	HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); 
	HH (b, c, d, a, x[ 6], S34,	 0x4881d05); 
	HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); 
	HH (d, a, b, c, x[12], S32, 0xe6db99e5); 
	HH (c, d, a, b, x[15], S33, 0x1fa27cf8); 
	HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); 

	
	II (a, b, c, d, x[ 0], S41, 0xf4292244); 
	II (d, a, b, c, x[ 7], S42, 0x432aff97); 
	II (c, d, a, b, x[14], S43, 0xab9423a7); 
	II (b, c, d, a, x[ 5], S44, 0xfc93a039); 
	II (a, b, c, d, x[12], S41, 0x655b59c3); 
	II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); 
	II (c, d, a, b, x[10], S43, 0xffeff47d); 
	II (b, c, d, a, x[ 1], S44, 0x85845dd1); 
	II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); 
	II (d, a, b, c, x[15], S42, 0xfe2ce6e0); 
	II (c, d, a, b, x[ 6], S43, 0xa3014314); 
	II (b, c, d, a, x[13], S44, 0x4e0811a1); 
	II (a, b, c, d, x[ 4], S41, 0xf7537e82); 
	II (d, a, b, c, x[11], S42, 0xbd3af235); 
	II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); 
	II (b, c, d, a, x[ 9], S44, 0xeb86d391); 

	state [0] += a;
	state [1] += b;
	state [2] += c;
	state [3] += d;

	}

