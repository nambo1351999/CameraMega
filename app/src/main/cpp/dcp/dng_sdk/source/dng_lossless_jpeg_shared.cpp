

 

#include "dng_lossless_jpeg.h"

#include "dng_assertions.h"
#include "dng_exceptions.h"
#include "dng_memory.h"
#include "dng_safe_arithmetic.h"
#include "dng_simd_type.h"
#include "dng_stream.h"
#include "dng_tag_codes.h"

#include <algorithm>

#include "dng_fast_module.h"

#ifndef qSupportCanon_sRAW
#define qSupportCanon_sRAW 1
#endif

#ifndef qSupportSony_sRAW
#define qSupportSony_sRAW 1
#endif

#ifndef qSupportHasselblad_3FR
#define qSupportHasselblad_3FR 1
#endif

 
struct HuffmanTable
	{
	
	
	uint8 bits[17];
	uint8 huffval[256];

	

	uint16 mincode[17];
	int32 maxcode[18];
	int16 valptr[17];
	int32 numbits[256];
	int32 value[256];
	
	uint16 ehufco[256];
	int8 ehufsi[256];
	
	};

 
static void FixHuffTbl (HuffmanTable *htbl)
	{
	
	int32 l;
	int32 i;
	
	const uint32 bitMask [] =
		{
		0xffffffff, 0x7fffffff, 0x3fffffff, 0x1fffffff,
		0x0fffffff, 0x07ffffff, 0x03ffffff, 0x01ffffff,
		0x00ffffff, 0x007fffff, 0x003fffff, 0x001fffff,
		0x000fffff, 0x0007ffff, 0x0003ffff, 0x0001ffff,
		0x0000ffff, 0x00007fff, 0x00003fff, 0x00001fff,
		0x00000fff, 0x000007ff, 0x000003ff, 0x000001ff,
		0x000000ff, 0x0000007f, 0x0000003f, 0x0000001f,
		0x0000000f, 0x00000007, 0x00000003, 0x00000001
		};
		
	
	

	int8 huffsize [257];
	
	int32 p = 0;
	
	for (l = 1; l <= 16; l++)
		{
		
		for (i = 1; i <= (int32) htbl->bits [l]; i++)
			huffsize [p++] = (int8) l;

		}
		
	huffsize [p] = 0;
	
	int32 lastp = p;

	
	

	uint16 huffcode [257];
	
	uint16 code = 0;
	
	int32 si = huffsize [0];
	
	p = 0;
	
	while (huffsize [p])
		{
		
		while (((int32) huffsize [p]) == si) 
			{
			huffcode [p++] = code;
			code++;
			}
			
		code <<= 1;
		
		si++;
		
		}

	
	
	
	

	memset (htbl->ehufsi, 0, sizeof (htbl->ehufsi));

	for (p = 0; p < lastp; p++)
		{
		
		htbl->ehufco [htbl->huffval [p]] = huffcode [p];
		htbl->ehufsi [htbl->huffval [p]] = huffsize [p];
		
		}
	
	
 
	p = 0;
	
	for (l = 1; l <= 16; l++)
		{
		
		if (htbl->bits [l])
			{
			
			htbl->valptr  [l] = (int16) p;
			htbl->mincode [l] = huffcode [p];
			
			p += htbl->bits [l];
			
			htbl->maxcode [l] = huffcode [p - 1];
			
			}
			
		else 
			{
			htbl->maxcode [l] = -1;
			}

		}

	

	htbl->maxcode[17] = 0xFFFFFL;

	
	
	
	
	

	memset (htbl->numbits, 0, sizeof (htbl->numbits));
	
	for (p = 0; p < lastp; p++)
		{
		
		int32 size = huffsize [p];
		
		if (size <= 8)
			{
			
			int32 value = htbl->huffval [p];
			
			code = huffcode [p];
			
			int32 ll = code << (8  -size);
			
			int32 ul = (size < 8 ? ll | bitMask [24 + size]
								 : ll);

			if (ul >= static_cast<int32> (sizeof(htbl->numbits) / sizeof (htbl->numbits [0])) ||
				ul >= static_cast<int32> (sizeof(htbl->value  ) / sizeof (htbl->value	[0])))
				{
				ThrowBadFormat ();
				}
				
			for (i = ll; i <= ul; i++)
				{
				htbl->numbits [i] = size;
				htbl->value	  [i] = value;
				}
				
			}

		}

	}

static void ValidateDecoderHuffTbl (const HuffmanTable *htbl)
	{

	int32 totalCodes = 0;
	int32 openSlots  = 1;

	for (int32 l = 1; l <= 16; l++)
		{

		totalCodes += htbl->bits [l];

		if (totalCodes > 256)
			{
			ThrowBadFormat ();
			}

		openSlots = (openSlots << 1) - htbl->bits [l];

		if (openSlots < 0)
			{
			ThrowBadFormat ();
			}

		}

	if (totalCodes == 0)
		{
		ThrowBadFormat ();
		}

	for (int32 p = 0; p < totalCodes; p++)
		{

		if (htbl->huffval [p] > 16)
			{
			ThrowBadFormat ();
			}

		}

	}

 
struct JpegComponentInfo
	{
	
	
	int16 componentId;		
	int16 componentIndex;	

	
	int16 hSampFactor;		
	int16 vSampFactor;		

	
	int16 dcTblNo;

	};

 
struct DecompressInfo
	{
	
	 
	int32 imageWidth;
	int32 imageHeight;
	int32 dataPrecision;

	
	JpegComponentInfo *compInfo;
	int16 numComponents;

	
	JpegComponentInfo *curCompInfo[4];
	int16 compsInScan;

	
	int16 MCUmembership[10];

	
	HuffmanTable *dcHuffTblPtrs[4];

	
	int32 Ss;
	int32 Pt;

	
	int32 restartInterval;
	int32 restartInRows; 

	
	int32 restartRowsToGo;	
	int16 nextRestartNum;	
	
	};

typedef uint16 ComponentType;		

typedef ComponentType *MCU;			

template <SIMDType simd>
class dng_lossless_decoder: private dng_uncopyable
	{
	
	private:
	
		dng_stream *fStream;		
		
		dng_spooler *fSpooler;		
				
		bool fBug16;				

		dng_memory_data huffmanBuffer [4];
		
		dng_memory_data compInfoBuffer;
		
		DecompressInfo info;
		
		dng_memory_data mcuBuffer1;
		dng_memory_data mcuBuffer2;
		dng_memory_data mcuBuffer3;
		dng_memory_data mcuBuffer4;
	
		MCU *mcuROW1;
		MCU *mcuROW2;
		
		uint64 getBuffer;			
		int32 bitsLeft;				
				
		#if qSupportHasselblad_3FR
		bool fHasselblad3FR;
		#endif

	public:
	
		dng_lossless_decoder (dng_stream *stream,
							  dng_spooler *spooler,
							  bool bug16);
	
		void StartRead (uint32 &imageWidth,
						uint32 &imageHeight,
						uint32 &imageChannels);

		void FinishRead ();
		
		#if qSupportHasselblad_3FR
	
		bool IsHasselblad3FR ()
			{
			return fHasselblad3FR;
			}
		
		#endif

	private:

		DNG_ALWAYS_INLINE uint8 GetJpegChar ()
			{
			return fStream->Get_uint8 ();
			}
			
		DNG_ALWAYS_INLINE void UnGetJpegChar ()
			{
			fStream->SetReadPosition (fStream->Position () - 1);
			}
			
		uint16 Get2bytes ();
	
		void SkipVariable ();

		void GetDht ();

		void GetDri ();

		void GetApp0 ();

		void GetSof (int32 code);

		void GetSos ();

		void GetSoi ();
		
		int32 NextMarker ();

		JpegMarker ProcessTables ();
		
		void ReadFileHeader ();

		int32 ReadScanHeader ();

		void DecoderStructInit ();

		void HuffDecoderInit ();

		void ProcessRestart ();

		int32 QuickPredict (int32 col,
							int32 curComp,
							MCU *curRowBuf,
							MCU *prevRowBuf);

		void FillBitBuffer (int32 nbits);

		int32 show_bits8 ();

		void flush_bits (int32 nbits);

		int32 get_bits (int32 nbits);

		int32 get_bit ();

		int32 HuffDecode (HuffmanTable *htbl);

		void HuffExtend (int32 &x, int32 s);

		void PmPutRow (MCU *buf,
					   int32 numComp,
					   int32 numCol,
					   int32 row);

		void DecodeFirstRow (MCU *curRowBuf);

		void DecodeImage ();
		
	};

template <SIMDType simd>
dng_lossless_decoder<simd>::dng_lossless_decoder (dng_stream *stream,
												  dng_spooler *spooler,
												  bool bug16)
									
	:	fStream	 (stream )
	,	fSpooler (spooler)
	,	fBug16	 (bug16	 )
	
	,	compInfoBuffer ()
	,	info		   ()
	,	mcuBuffer1	   ()
	,	mcuBuffer2	   ()
	,	mcuBuffer3	   ()
	,	mcuBuffer4	   ()
	,	mcuROW1		   (NULL)
	,	mcuROW2		   (NULL)
	,	getBuffer	   (0)
	,	bitsLeft	   (0)
	
	#if qSupportHasselblad_3FR
	,	fHasselblad3FR (false)
	#endif
	
	{
	
	memset (&info, 0, sizeof (info));
	
	}

template <SIMDType simd>
uint16 dng_lossless_decoder<simd>::Get2bytes ()
	{
	
	uint16 a = GetJpegChar ();
	
	return (uint16) ((a << 8) + GetJpegChar ());
	
	}

 
template <SIMDType simd>
void dng_lossless_decoder<simd>::SkipVariable ()
	{
	
	uint32 raw = Get2bytes ();

	if (raw < 2)
		{
		ThrowBadFormat ();
		}

	fStream->Skip (raw - 2);

	}

 
template <SIMDType simd>
void dng_lossless_decoder<simd>::GetDht ()
	{

	uint32 raw = Get2bytes ();

	if (raw < 2)
		{
		ThrowBadFormat ();
		}

	int32 length = (int32) (raw - 2);

	while (length > 0)
		{

		if (length < 1 + 16)
			{
			ThrowBadFormat ();
			}

		int32 index = GetJpegChar ();
		
		if (index < 0 || index >= 4)
			{
			ThrowBadFormat ();
			}

		HuffmanTable *&htblptr = info.dcHuffTblPtrs [index];

		if (htblptr == NULL)
			{
			
			huffmanBuffer [index] . Allocate (sizeof (HuffmanTable));
			
			htblptr = (HuffmanTable *) huffmanBuffer [index] . Buffer ();
			
			}

		htblptr->bits [0] = 0;
		
		int32 count = 0;
		
		for (int32 i = 1; i <= 16; i++)
			{
			
			htblptr->bits [i] = GetJpegChar ();
			
			count += htblptr->bits [i];
			
			}

		if (count > 256) 
			{
			ThrowBadFormat ();
			}

		if (count > length - (1 + 16))
			{
			ThrowBadFormat ();
			}

		for (int32 j = 0; j < count; j++)
			{
			
			htblptr->huffval [j] = GetJpegChar ();
			
			}

		length -= 1 + 16 + count;

		}
		
	}

template <SIMDType simd>
void dng_lossless_decoder<simd>::GetDri ()
	{
	
	if (Get2bytes () != 4)
		{
		ThrowBadFormat ();
		}
	
	info.restartInterval = Get2bytes ();

	}

template <SIMDType simd>
void dng_lossless_decoder<simd>::GetApp0 ()
	{

	SkipVariable ();
	
	}

 
template <SIMDType simd>
void dng_lossless_decoder<simd>::GetSof (int32 )
	{
	
	int32 length = Get2bytes ();

	info.dataPrecision = GetJpegChar ();
	info.imageHeight   = Get2bytes	 ();
	info.imageWidth	   = Get2bytes	 ();
	info.numComponents = GetJpegChar ();

	
	
	
	 
	if ((info.imageHeight	<= 0) ||
		(info.imageWidth	<= 0) || 
		(info.numComponents <= 0))
		{
		ThrowBadFormat ();
		}

	

	const int32 MinPrecisionBits = 2;
	const int32 MaxPrecisionBits = 16;

	if ((info.dataPrecision < MinPrecisionBits) ||
		(info.dataPrecision > MaxPrecisionBits))
		{
		ThrowBadFormat ();
		}
		
	

	if (length != (info.numComponents * 3 + 8))
		{
		ThrowBadFormat ();
		}
		
	
	
	
	

	compInfoBuffer.Allocate (static_cast<uint32> (info.numComponents),
							 sizeof (JpegComponentInfo));
	
	info.compInfo = (JpegComponentInfo *) compInfoBuffer.Buffer ();
								 
	

	for (int32 ci = 0; ci < info.numComponents; ci++)
		{
		
		JpegComponentInfo *compptr = &info.compInfo [ci];
		
		compptr->componentIndex = (int16) ci;
		
		compptr->componentId = GetJpegChar ();
		
		int32 c = GetJpegChar ();
		
		compptr->hSampFactor = (int16) ((c >> 4) & 15);
		compptr->vSampFactor = (int16) ((c	   ) & 15);
		
		(void) GetJpegChar ();	 
		
		}

	}

template <SIMDType simd>
void dng_lossless_decoder<simd>::GetSos ()
	{
	
	int32 length = Get2bytes ();

	

	int32 n = GetJpegChar ();
	info.compsInScan = (int16) n;
	
	
	
	length -= 3;

	if (length != (n * 2 + 3) || n < 1 || n > 4)
		{
		ThrowBadFormat ();
		}
	
	

	for (int32 i = 0; i < n; i++)
		{
		
		int32 cc = GetJpegChar ();
		int32 c	 = GetJpegChar ();
		
		int32 ci;
		
		for (ci = 0; ci < info.numComponents; ci++)
			{
			
			if (cc == info.compInfo[ci].componentId)
				{
				break;
				}
				
			}

		if (ci >= info.numComponents) 
			{
			ThrowBadFormat ();
			}

		JpegComponentInfo *compptr = &info.compInfo [ci];
		
		info.curCompInfo [i] = compptr;
		
		compptr->dcTblNo = (int16) ((c >> 4) & 15);
		
		}

	

	info.Ss = GetJpegChar (); 
	
	(void) GetJpegChar ();
	
	info.Pt = GetJpegChar () & 0x0F;
	
	}

template <SIMDType simd>
void dng_lossless_decoder<simd>::GetSoi ()
	{

	
	 
	info.restartInterval = 0;
	
	}

template <SIMDType simd>
int32 dng_lossless_decoder<simd>::NextMarker ()
	{
	
	int32 c;

	do
		{

		
		
		do 
			{
			c = GetJpegChar ();
			}
		while (c != 0xFF);
		
		
		
		do 
			{
			c = GetJpegChar();
			} 
		while (c == 0xFF);
		
		}
	while (c == 0);		

	return c;
	
	}

template <SIMDType simd>
JpegMarker dng_lossless_decoder<simd>::ProcessTables ()
	{
	
	while (true)
		{
	
		int32 c = NextMarker ();
	
		switch (c)
			{
			
			case M_SOF0:
			case M_SOF1:
			case M_SOF2:
			case M_SOF3:
			case M_SOF5:
			case M_SOF6:
			case M_SOF7:
			case M_JPG:
			case M_SOF9:
			case M_SOF10:
			case M_SOF11:
			case M_SOF13:
			case M_SOF14:
			case M_SOF15:
			case M_SOI:
			case M_EOI:
			case M_SOS:
				return (JpegMarker) c;

			case M_DHT:
				GetDht ();
				break;

			case M_DQT:
				break;

			case M_DRI:
				GetDri ();
				break;

			case M_APP0:
				GetApp0 ();
				break;

			case M_RST0:	
			case M_RST1:
			case M_RST2:
			case M_RST3:
			case M_RST4:
			case M_RST5:
			case M_RST6:
			case M_RST7:
			case M_TEM:
				break;

			default:		
				SkipVariable ();
				break;
				
			}
			
		}

		return M_ERROR;
	}

 
template <SIMDType simd>
void dng_lossless_decoder<simd>::ReadFileHeader ()
	{
	
	
	

	int32 c	 = GetJpegChar ();
	int32 c2 = GetJpegChar ();
	
	if ((c != 0xFF) || (c2 != M_SOI)) 
		{
		ThrowBadFormat ();
		}
		
	

	GetSoi ();

	

	c = ProcessTables ();

	switch (c)
		{
		
		case M_SOF0:
		case M_SOF1:
		case M_SOF3:
			GetSof (c);
			break;

		default:
			ThrowBadFormat ();
			break;
			
		}

	}

template <SIMDType simd>
int32 dng_lossless_decoder<simd>::ReadScanHeader ()
	{

	

	int32 c = ProcessTables ();

	switch (c)
		{
		
		case M_SOS:
			GetSos ();
			return 1;

		case M_EOI:
			return 0;

		default:
			ThrowBadFormat ();
			break;
			
		}
		
	return 0;
	
	}

template <SIMDType simd>
void dng_lossless_decoder<simd>::DecoderStructInit ()
	{
	
	int32 ci;
	
	bool isVerifiedSamplingFactorSpecialCase = false;

	#if qSupportCanon_sRAW
	
	bool canon_sRAW = (info.numComponents == 3) &&
					  (info.compsInScan   == 3) &&
					  (info.compInfo [0].hSampFactor == 2) &&
					  (info.compInfo [1].hSampFactor == 1) &&
					  (info.compInfo [2].hSampFactor == 1) &&
					  (info.compInfo [0].vSampFactor == 1) &&
					  (info.compInfo [1].vSampFactor == 1) &&
					  (info.compInfo [2].vSampFactor == 1) &&
					  (info.dataPrecision == 15) &&
					  (info.Ss == 1) &&
					  ((info.imageWidth & 1) == 0);

	bool canon_sRAW2 = (info.numComponents == 3) &&
					   (info.compsInScan   == 3) &&
					   (info.compInfo [0].hSampFactor == 2) &&
					   (info.compInfo [1].hSampFactor == 1) &&
					   (info.compInfo [2].hSampFactor == 1) &&
					   (info.compInfo [0].vSampFactor == 2) &&
					   (info.compInfo [1].vSampFactor == 1) &&
					   (info.compInfo [2].vSampFactor == 1) &&
					   (info.dataPrecision == 15) &&
					   (info.Ss == 1) &&
					   ((info.imageWidth  & 1) == 0) &&
					   ((info.imageHeight & 1) == 0);
	
	if (canon_sRAW || canon_sRAW2)
		{
		isVerifiedSamplingFactorSpecialCase = true;
		}

	#endif
	
	#if qSupportSony_sRAW
		
	bool sony_sRAW = (info.numComponents == 3) &&
					 (info.compsInScan   == 3) &&
					 (info.compInfo [0].hSampFactor == 2) &&
					 (info.compInfo [1].hSampFactor == 1) &&
					 (info.compInfo [2].hSampFactor == 1) &&
					 (info.compInfo [0].vSampFactor == 2) &&
					 (info.compInfo [1].vSampFactor == 1) &&
					 (info.compInfo [2].vSampFactor == 1) &&
					 (info.dataPrecision == 16) &&
					 (info.Ss == 1) &&
					 ((info.imageWidth  & 1) == 0) &&
					 ((info.imageHeight & 1) == 0);

	bool sony_sRAW2 = (info.numComponents == 3) &&
					 (info.compsInScan   == 3) &&
					 (info.compInfo [0].hSampFactor == 2) &&
					 (info.compInfo [1].hSampFactor == 1) &&
					 (info.compInfo [2].hSampFactor == 1) &&
					 (info.compInfo [0].vSampFactor == 1) &&
					 (info.compInfo [1].vSampFactor == 1) &&
					 (info.compInfo [2].vSampFactor == 1) &&
					 (info.dataPrecision == 16) &&
					 (info.Ss == 1) &&
					 ((info.imageWidth  & 1) == 0) &&
					 ((info.imageHeight & 1) == 0);
					   
	if (sony_sRAW || sony_sRAW2)
		{
		isVerifiedSamplingFactorSpecialCase = true;
		}
	
	#endif
	
	if (!isVerifiedSamplingFactorSpecialCase)
		{
	
		

		for (ci = 0; ci < info.numComponents; ci++)
			{
			
			JpegComponentInfo *compPtr = &info.compInfo [ci];
			
			if (compPtr->hSampFactor != 1 ||
				compPtr->vSampFactor != 1) 
				{
				ThrowBadFormat ();
				}
		
			}
			
		}
	
	

	if (info.compsInScan < 0 || 
		info.compsInScan > 4)
		{
		ThrowBadFormat ();
		}

	for (ci = 0; ci < info.compsInScan; ci++)
		{
		info.MCUmembership [ci] = (int16) ci;
		}

	
	
	
	
	

	int32 mcuSize = info.compsInScan * (uint32) sizeof (ComponentType);
	
	mcuBuffer1.Allocate (info.imageWidth, sizeof (MCU));
	mcuBuffer2.Allocate (info.imageWidth, sizeof (MCU));
	
	mcuROW1 = (MCU *) mcuBuffer1.Buffer ();
	mcuROW2 = (MCU *) mcuBuffer2.Buffer ();
	
	mcuBuffer3.Allocate (info.imageWidth, mcuSize);
	mcuBuffer4.Allocate (info.imageWidth, mcuSize);
	
	mcuROW1 [0] = (ComponentType *) mcuBuffer3.Buffer ();
	mcuROW2 [0] = (ComponentType *) mcuBuffer4.Buffer ();
	
	for (int32 j = 1; j < info.imageWidth; j++)
		{
		
		mcuROW1 [j] = mcuROW1 [j - 1] + info.compsInScan;
		mcuROW2 [j] = mcuROW2 [j - 1] + info.compsInScan;
	
		}
	
	}

template <SIMDType simd>
void dng_lossless_decoder<simd>::HuffDecoderInit ()
	{
	
	
 
	getBuffer = 0;
	bitsLeft  = 0;
	
	

	for (int16 ci = 0; ci < info.compsInScan; ci++)
		{
		
		JpegComponentInfo *compptr = info.curCompInfo [ci];
		
		
		
		if (compptr->dcTblNo < 0 || compptr->dcTblNo > 3)
			{
			ThrowBadFormat ();
			}

		if (info.dcHuffTblPtrs [compptr->dcTblNo] == NULL) 
			{ 
			ThrowBadFormat ();
			}

		HuffmanTable *htbl = info.dcHuffTblPtrs [compptr->dcTblNo];

		
		

		ValidateDecoderHuffTbl (htbl);

		
		
		

		FixHuffTbl (htbl);

		}

	

	info.restartInRows	 = info.restartInterval / info.imageWidth;
	info.restartRowsToGo = info.restartInRows;
	info.nextRestartNum	 = 0;
	
	}

template <SIMDType simd>
void dng_lossless_decoder<simd>::ProcessRestart ()
	{
	
	
	
	fStream->SetReadPosition (fStream->Position () - bitsLeft / 8);
	
	bitsLeft  = 0;
	getBuffer = 0;
	
	

	int32 c;

	do
		{
		
		
		
		do 
			{ 
			c = GetJpegChar ();
			}
		while (c != 0xFF);
		
		
		
		do
			{
			c = GetJpegChar ();
			}
		while (c == 0xFF);
		
		}
	while (c == 0);		
	
	

	if (c != (M_RST0 + info.nextRestartNum))
		{
		ThrowBadFormat ();
		}

	

	info.restartRowsToGo = info.restartInRows;
	info.nextRestartNum	 = (info.nextRestartNum + 1) & 7;
	
	}

 
template <SIMDType simd>
DNG_ALWAYS_INLINE int32 dng_lossless_decoder<simd>::QuickPredict (int32 col,
																  int32 curComp,
																  MCU *curRowBuf,
																  MCU *prevRowBuf)
	{
	
	int32 diag	= prevRowBuf [col - 1] [curComp];
	int32 upper = prevRowBuf [col	 ] [curComp];
	int32 left	= curRowBuf	 [col - 1] [curComp];

	switch (info.Ss)
		{
		
		case 0:
			return 0;
			
		case 1:
			return left;

		case 2:
			return upper;

		case 3:
			return diag;

		case 4:
			return left + upper - diag;

		case 5:
			return left + ((upper - diag) >> 1);

		case 6:
			return upper + ((left - diag) >> 1);

		case 7:
			return (left + upper) >> 1;

		default:
			{
			ThrowBadFormat ();
			return 0;
			}
			  
		}
	 
	}
	

template <SIMDType simd>
DNG_ALWAYS_INLINE void dng_lossless_decoder<simd>::FillBitBuffer (int32 nbits)
	{
	
	const int32 kMinGetBits = sizeof (uint32) * 8 - 7;
	
	#if qSupportHasselblad_3FR
	
	if (fHasselblad3FR)
		{
		
		while (bitsLeft < kMinGetBits)
			{
			
			int32 c0 = 0;
			int32 c1 = 0;
			int32 c2 = 0;
			int32 c3 = 0;
			
			try
				{
				c0 = GetJpegChar ();
				c1 = GetJpegChar ();
				c2 = GetJpegChar ();
				c3 = GetJpegChar ();
				}
				
			catch (dng_exception &except)
				{
				
				
				
				if (except.ErrorCode () != dng_error_end_of_file)
					{
					throw; 
					}
					
				
				
				
				
				
					
				
				
				
				if (!((c0 == 0xFF && c1 == 0xD9) ||
					  (c1 == 0xFF && c2 == 0xD9)))
					{
					throw; 
					}
				
				
					
				}
			
			getBuffer = (getBuffer << 8) | c3;
			getBuffer = (getBuffer << 8) | c2;
			getBuffer = (getBuffer << 8) | c1;
			getBuffer = (getBuffer << 8) | c0;
			
			bitsLeft += 32;
			
			}
			
		return;
		
		}
	
	#endif
	
	while (bitsLeft < kMinGetBits)
		{
		
		int32 c = GetJpegChar ();

		

		if (c == 0xFF)
			{
			
			int32 c2 = GetJpegChar ();
			
			if (c2 != 0)
				{

				
				

				UnGetJpegChar ();
				UnGetJpegChar ();

				
				

				if (bitsLeft >= nbits)
					break;

				
				
				
				

				c = 0;
				
				}
				
			}
			
		getBuffer = (getBuffer << 8) | c;
		
		bitsLeft += 8;
		
		}
 
	}

template <SIMDType simd>
DNG_ALWAYS_INLINE int32 dng_lossless_decoder<simd>::show_bits8 ()
	{
	
	if (bitsLeft < 8)
		FillBitBuffer (8);
		
	return (int32) ((getBuffer >> (bitsLeft - 8)) & 0xff);
	
	}

template <SIMDType simd>
DNG_ALWAYS_INLINE void dng_lossless_decoder<simd>::flush_bits (int32 nbits)
	{
	
	bitsLeft -= nbits;
	
	}

#define qOptGetBitsMath 0

#if qOptGetBitsMath
#define MASK_ME(nbits) (0x0FFFF >> (16 - nbits))
static const int32 get_bits_mask[17] = {
	MASK_ME(0),
	MASK_ME(1),
	MASK_ME(2),
	MASK_ME(3),
	MASK_ME(4),
	MASK_ME(5),
	MASK_ME(6),
	MASK_ME(7),
	MASK_ME(8),
	MASK_ME(9),
	MASK_ME(10),
	MASK_ME(11),
	MASK_ME(12),
	MASK_ME(13),
	MASK_ME(14),
	MASK_ME(15),
	MASK_ME(16)
};
#endif

template <SIMDType simd>
DNG_ALWAYS_INLINE int32 dng_lossless_decoder<simd>::get_bits (int32 nbits)
	{
	
	if (nbits > 16)
		{
		ThrowBadFormat ();
		}
	
	if (bitsLeft < nbits)
		FillBitBuffer (nbits);

	#if qOptGetBitsMath

	return (int32) ((getBuffer >> (bitsLeft -= nbits)) & get_bits_mask [nbits]);

	#else
		
	return (int32) ((getBuffer >> (bitsLeft -= nbits)) & (0x0FFFF >> (16 - nbits)));

	#endif
	
	}

template <SIMDType simd>
DNG_ALWAYS_INLINE int32 dng_lossless_decoder<simd>::get_bit ()
	{
	
	if (!bitsLeft)
		FillBitBuffer (1);
		
	return (int32) ((getBuffer >> (--bitsLeft)) & 1);
	
	}

template <SIMDType simd>
DNG_ALWAYS_INLINE int32 dng_lossless_decoder<simd>::HuffDecode (HuffmanTable *htbl)
	{
	
	
	
	

	int32 code = show_bits8 ();
	
	if (htbl->numbits [code])
		{
		
		flush_bits (htbl->numbits [code]);
		
		return htbl->value [code];
		
		}
		
	else
		{
		
		flush_bits (8);
		
		int32 l = 8;
		
		while (code > htbl->maxcode [l]) 
			{
			code = (code << 1) | get_bit ();
			l++;
			}

		

		if (l > 16)
			{
			return 0;		
			}
		else
			{

			const int32 offset = (int32) (code - htbl->mincode [l]);

			
			
			

			if ((uint32) offset >= (uint32) htbl->bits [l])
				{
				return 0;
				}

			return htbl->huffval [htbl->valptr [l] + offset];

			}
			
		}
		
	}

template <SIMDType simd>
DNG_ALWAYS_INLINE void dng_lossless_decoder<simd>::HuffExtend (int32 &x, int32 s)
	{

	
	
	

	const uint32 shift		 = (uint32) s;
	const int32 signThreshold = (int32) (0x08000u >> (16 - shift));
	const uint32 extendMask	 = (1u << shift) - 1u;
	const uint32 needsExtend  = (x < signThreshold) ? 0xffffffffu : 0u;

	x -= (int32) (needsExtend & extendMask);

	}

template <SIMDType simd>
DNG_ALWAYS_INLINE
void dng_lossless_decoder<simd>::PmPutRow (MCU *buf,
										   int32 numComp,
										   int32 numCol,
										   int32 )
	{
	
	uint16 *sPtr = &buf [0] [0];
	
	uint32 pixels = SafeUint32Mult ((uint32) numCol,
									(uint32) numComp);
	
	fSpooler->Spool (sPtr, SafeUint32Mult (pixels,
										   (uint32) sizeof (uint16)));
		
	}

 
template <SIMDType simd>
void dng_lossless_decoder<simd>::DecodeFirstRow (MCU *curRowBuf)
	{
	
	int32 compsInScan = info.compsInScan;
	
	

	for (int32 curComp = 0; curComp < compsInScan; curComp++) 
		{
		
		int32 ci = info.MCUmembership [curComp];
		
		JpegComponentInfo *compptr = info.curCompInfo [ci];
		
		HuffmanTable *dctbl = info.dcHuffTblPtrs [compptr->dcTblNo];

		

		int32 d = 0;
	
		int32 s = HuffDecode (dctbl);
		
		if (s)
			{
			
			if (s == 16 && !fBug16)
				{
				d = -32768;
				}
			
			else
				{
				d = get_bits (s);
				HuffExtend (d, s);
				}

			}

		

		int32 Pr = info.dataPrecision;
		int32 Pt = info.Pt;
	
		curRowBuf [0] [curComp] = (ComponentType) (d + (1 << (Pr-Pt-1)));
		
		}
		
	
	
	int32 numCOL = info.imageWidth;
	
	for (int32 col = 1; col < numCOL; col++)
		{

		for (int32 curComp = 0; curComp < compsInScan; curComp++)
			{
			
			int32 ci = info.MCUmembership [curComp];
			
			JpegComponentInfo *compptr = info.curCompInfo [ci];
			
			HuffmanTable *dctbl = info.dcHuffTblPtrs [compptr->dcTblNo];

			

			int32 d = 0;
		
			int32 s = HuffDecode (dctbl);
			
			if (s)
				{

				if (s == 16 && !fBug16)
					{
					d = -32768;
					}
				
				else
					{
					d = get_bits (s);
					HuffExtend (d, s);
					}

				}
				
			

			curRowBuf [col] [curComp] = (ComponentType) (d + curRowBuf [col-1] [curComp]);
			
			}
			
		}
		
	

	if (info.restartInRows)
		{
		info.restartRowsToGo--;
		}
		
	}

 
template <SIMDType simd>
void dng_lossless_decoder<simd>::DecodeImage ()
	{
	
	#if qSupportSony_sRAW
		
	bool sony_sRAW = (info.numComponents == 3) &&
					 (info.compInfo [0].hSampFactor == 2) &&
					 (info.compInfo [1].hSampFactor == 1) &&
					 (info.compInfo [2].hSampFactor == 1) &&
					 (info.compInfo [0].vSampFactor == 2) &&
					 (info.compInfo [1].vSampFactor == 1) &&
					 (info.compInfo [2].vSampFactor == 1) &&
					 (info.dataPrecision == 16) &&
					 (info.Ss == 1) &&
					 ((info.imageWidth  & 1) == 0) &&
					 ((info.imageHeight & 1) == 0);
		
	bool sony_sRAW2 = (info.numComponents == 3) &&
					 (info.compInfo [0].hSampFactor == 2) &&
					 (info.compInfo [1].hSampFactor == 1) &&
					 (info.compInfo [2].hSampFactor == 1) &&
					 (info.compInfo [0].vSampFactor == 1) &&
					 (info.compInfo [1].vSampFactor == 1) &&
					 (info.compInfo [2].vSampFactor == 1) &&
					 (info.dataPrecision == 16) &&
					 (info.Ss == 1) &&
					 ((info.imageWidth  & 1) == 0) &&
					 ((info.imageHeight & 1) == 0);

	#endif
	
	#define swap(type,a,b) {type c; c=(a); (a)=(b); (b)=c;}

	int32 numCOL	  = info.imageWidth;
	int32 numROW	  = info.imageHeight;
	int32 compsInScan = info.compsInScan;
	
	
	
	HuffmanTable *ht [4];

	memset (ht, 0, sizeof (ht));
	
	for (int32 curComp = 0; curComp < compsInScan; curComp++)
		{
		
		int32 ci = info.MCUmembership [curComp];
		
		JpegComponentInfo *compptr = info.curCompInfo [ci];
		
		ht [curComp] = info.dcHuffTblPtrs [compptr->dcTblNo];

		}
		
	MCU *prevRowBuf = mcuROW1;
	MCU *curRowBuf	= mcuROW2;
	
	#if qSupportCanon_sRAW && qSupportSony_sRAW
		
	
	
	if (info.compInfo [0].hSampFactor == 2 &&
		info.compInfo [0].vSampFactor == 1 &&
		compsInScan >= 3)
		{

		for (int32 row = 0; row < numROW; row++)
			{

			

			int32 p0;
			int32 p1;
			int32 p2;
			
			if (row == 0)
				{
				
				if (sony_sRAW2)
					{
					p0 = 1 << 15;
					}
				
				else 
					{
					p0 = 1 << 14;
					}
				
				p1 = 1 << 14;
				p2 = 1 << 14;
				
				}
				
			else
				{
				p0 = prevRowBuf [0] [0];
				p1 = prevRowBuf [0] [1];
				p2 = prevRowBuf [0] [2];
				}
			
			for (int32 col = 0; col < numCOL; col += 2)
				{
				
				
				
					{
				
					int32 d = 0;
				
					int32 s = HuffDecode (ht [0]);
					
					if (s)
						{

						if (s == 16)
							{
							d = -32768;
							}
						
						else
							{
							d = get_bits (s);
							HuffExtend (d, s);
							}

						}
						
					p0 += d;
					
					curRowBuf [col] [0] = (ComponentType) p0;
				
					}
				
				
				
					{
				
					int32 d = 0;
				
					int32 s = HuffDecode (ht [0]);
					
					if (s)
						{

						if (s == 16)
							{
							d = -32768;
							}
						
						else
							{
							d = get_bits (s);
							HuffExtend (d, s);
							}

						}
						
					p0 += d;
					
					curRowBuf [col + 1] [0] = (ComponentType) p0;
				
					}
				
				
				
					{
				
					int32 d = 0;
				
					int32 s = HuffDecode (ht [1]);
					
					if (s)
						{

						if (s == 16)
							{
							d = -32768;
							}
						
						else
							{
							d = get_bits (s);
							HuffExtend (d, s);
							}

						}
						
					p1 += d;
					
					curRowBuf [col	  ] [1] = (ComponentType) p1;
					curRowBuf [col + 1] [1] = (ComponentType) p1;
				
					}
				
				
				
					{
				
					int32 d = 0;
				
					int32 s = HuffDecode (ht [2]);
					
					if (s)
						{

						if (s == 16)
							{
							d = -32768;
							}
						
						else
							{
							d = get_bits (s);
							HuffExtend (d, s);
							}

						}
						
					p2 += d;
					
					curRowBuf [col	  ] [2] = (ComponentType) p2;
					curRowBuf [col + 1] [2] = (ComponentType) p2;
				
					}
								
				}
			
			PmPutRow (curRowBuf, compsInScan, numCOL, row);

			swap (MCU *, prevRowBuf, curRowBuf);
			
			}
			
		return;
		
		}
	
	if (info.compInfo [0].hSampFactor == 2 &&
		info.compInfo [0].vSampFactor == 2 &&
		compsInScan >= 3)
		{

		for (int32 row = 0; row < numROW; row += 2)
			{

			
			
			if (sony_sRAW)
				{
			
				

				int32 p00 = 0;
				int32 p01 = 0;
				int32 p10 = 0;
				int32 p11 = 0;
				
				int32 p1 = 0;
				int32 p2 = 0;
				
				if (row == 0)
					{
					
					p00 = 1 << 15;
					p1  = 1 << 14;
					p2  = 1 << 14;
					
					}
					
				else
					{
					p00 = prevRowBuf [0] [0];
					p1  = prevRowBuf [0] [1];
					p2  = prevRowBuf [0] [2];
					}

				for (int32 col = 0; col < numCOL; col += 2)
					{
			
					
					
						{
					
						int32 d = 0;
					
						int32 s = HuffDecode (ht [0]);
						
						if (s)
							{

							if (s == 16)
								{
								d = -32768;
								}
							
							else
								{
								d = get_bits (s);
								HuffExtend (d, s);
								}

							}

						p00 = d + ((col == 0) ? p00 : p01);

						prevRowBuf [col] [0] = (ComponentType) p00;
					
						}
					
					
					
						{
					
						int32 d = 0;
					
						int32 s = HuffDecode (ht [0]);
						
						if (s)
							{

							if (s == 16)
								{
								d = -32768;
								}
							
							else
								{
								d = get_bits (s);
								HuffExtend (d, s);
								}

							}

						p01 = p00 + d;
						
						prevRowBuf [col + 1] [0] = (ComponentType) p01;
					
						}
					
					
					
						{
					
						int32 d = 0;
					
						int32 s = HuffDecode (ht [0]);
						
						if (s)
							{

							if (s == 16)
								{
								d = -32768;
								}
							
							else
								{
								d = get_bits (s);
								HuffExtend (d, s);
								}

							}
						
						p10 = d + ((col == 0) ? p00 : p11);

						curRowBuf [col] [0] = (ComponentType) p10;
					
						}
					
					
					
						{
					
						int32 d = 0;
					
						int32 s = HuffDecode (ht [0]);
						
						if (s)
							{

							if (s == 16)
								{
								d = -32768;
								}
							
							else
								{
								d = get_bits (s);
								HuffExtend (d, s);
								}

							}
							
						p11 = p10 + d;
						
						curRowBuf [col + 1] [0] = (ComponentType) p11;
					
						}
					
					
					
						{
					
						int32 d = 0;
					
						int32 s = HuffDecode (ht [1]);
						
						if (s)
							{

							if (s == 16)
								{
								d = -32768;
								}
							
							else
								{
								d = get_bits (s);
								HuffExtend (d, s);
								}

							}
							
						p1 += d;
						
						prevRowBuf [col	   ] [1] = (ComponentType) p1;
						prevRowBuf [col + 1] [1] = (ComponentType) p1;

						curRowBuf [col	  ] [1] = (ComponentType) p1;
						curRowBuf [col + 1] [1] = (ComponentType) p1;
					
						}
					
					
					
						{
					
						int32 d = 0;
					
						int32 s = HuffDecode (ht [2]);
						
						if (s)
							{

							if (s == 16)
								{
								d = -32768;
								}
							
							else
								{
								d = get_bits (s);
								HuffExtend (d, s);
								}

							}
							
						p2 += d;
						
						prevRowBuf [col	   ] [2] = (ComponentType) p2;
						prevRowBuf [col + 1] [2] = (ComponentType) p2;
					
						curRowBuf [col	  ] [2] = (ComponentType) p2;
						curRowBuf [col + 1] [2] = (ComponentType) p2;
					
						}
									
					}
				
				PmPutRow (prevRowBuf, compsInScan, numCOL, row);
				PmPutRow (curRowBuf, compsInScan, numCOL, row);
				
				}
			
			
			
			else
				{
				
				
				
				int32 p0;
				int32 p1;
				int32 p2;
				
				if (row == 0)
					{
					p0 = 1 << 14;
					p1 = 1 << 14;
					p2 = 1 << 14;
					}
					
				else
					{
					p0 = prevRowBuf [0] [0];
					p1 = prevRowBuf [0] [1];
					p2 = prevRowBuf [0] [2];
					}
				
				for (int32 col = 0; col < numCOL; col += 2)
					{
					
					
					
						{
					
						int32 d = 0;
					
						int32 s = HuffDecode (ht [0]);
						
						if (s)
							{

							if (s == 16)
								{
								d = -32768;
								}
							
							else
								{
								d = get_bits (s);
								HuffExtend (d, s);
								}

							}
							
						p0 += d;
						
						prevRowBuf [col] [0] = (ComponentType) p0;
					
						}
					
					
					
						{
					
						int32 d = 0;
					
						int32 s = HuffDecode (ht [0]);
						
						if (s)
							{

							if (s == 16)
								{
								d = -32768;
								}
							
							else
								{
								d = get_bits (s);
								HuffExtend (d, s);
								}

							}
							
						p0 += d;
						
						prevRowBuf [col + 1] [0] = (ComponentType) p0;
					
						}
					
					
					
						{
					
						int32 d = 0;
					
						int32 s = HuffDecode (ht [0]);
						
						if (s)
							{

							if (s == 16)
								{
								d = -32768;
								}
							
							else
								{
								d = get_bits (s);
								HuffExtend (d, s);
								}

							}
							
						p0 += d;
						
						curRowBuf [col] [0] = (ComponentType) p0;
					
						}
					
					
					
						{
					
						int32 d = 0;
					
						int32 s = HuffDecode (ht [0]);
						
						if (s)
							{

							if (s == 16)
								{
								d = -32768;
								}
							
							else
								{
								d = get_bits (s);
								HuffExtend (d, s);
								}

							}
							
						p0 += d;
						
						curRowBuf [col + 1] [0] = (ComponentType) p0;
					
						}
					
					
					
						{
					
						int32 d = 0;
					
						int32 s = HuffDecode (ht [1]);
						
						if (s)
							{

							if (s == 16)
								{
								d = -32768;
								}
							
							else
								{
								d = get_bits (s);
								HuffExtend (d, s);
								}

							}
							
						p1 += d;
						
						prevRowBuf [col	   ] [1] = (ComponentType) p1;
						prevRowBuf [col + 1] [1] = (ComponentType) p1;

						curRowBuf [col	  ] [1] = (ComponentType) p1;
						curRowBuf [col + 1] [1] = (ComponentType) p1;
					
						}
					
					
					
						{
					
						int32 d = 0;
					
						int32 s = HuffDecode (ht [2]);
						
						if (s)
							{

							if (s == 16)
								{
								d = -32768;
								}
							
							else
								{
								d = get_bits (s);
								HuffExtend (d, s);
								}

							}
							
						p2 += d;
						
						prevRowBuf [col	   ] [2] = (ComponentType) p2;
						prevRowBuf [col + 1] [2] = (ComponentType) p2;
					
						curRowBuf [col	  ] [2] = (ComponentType) p2;
						curRowBuf [col + 1] [2] = (ComponentType) p2;
					
						}
									
					}
				
				PmPutRow (prevRowBuf, compsInScan, numCOL, row);
				PmPutRow (curRowBuf, compsInScan, numCOL, row);
				
				}

			}
			
		return;
		
		}

	#endif
	
	#if qSupportHasselblad_3FR
	
	
	

	if (info.Ss == 8 &&
		compsInScan == 1 &&
		(numCOL & 1) == 0)
		{
		
		fHasselblad3FR = true;
		
		for (int32 row = 0; row < numROW; row++)
			{
			
			int32 p0 = 32768;
			int32 p1 = 32768;
			
			for (int32 col = 0; col < numCOL; col += 2)
				{
				
				int32 s0 = HuffDecode (ht [0]);
				int32 s1 = HuffDecode (ht [0]);
				
				if (s0)
					{
					int32 d = get_bits (s0);
					if (s0 == 16)
						{
						d = -32768;
						}
					else
						{
						HuffExtend (d, s0);
						}
					p0 += d;
					}

				if (s1)
					{
					int32 d = get_bits (s1);
					if (s1 == 16)
						{
						d = -32768;
						}
					else
						{
						HuffExtend (d, s1);
						}
					p1 += d;
					}

				curRowBuf [col	  ] [0] = (ComponentType) p0;
				curRowBuf [col + 1] [0] = (ComponentType) p1;
				
				}
			
			PmPutRow (curRowBuf, compsInScan, numCOL, row);

			}

		return;
		
		}
	
	#endif
	
	
	
	

	DecodeFirstRow (mcuROW1);
	
	PmPutRow (mcuROW1, compsInScan, numCOL, 0);
	
	

	for (int32 row = 1; row < numROW; row++)
		{

		

		if (info.restartInRows)
			{
			
			if (info.restartRowsToGo == 0)
				{
				
				ProcessRestart ();
			
				
				
				DecodeFirstRow (curRowBuf);
				
				PmPutRow (curRowBuf, compsInScan, numCOL, row);
				
				swap (MCU *, prevRowBuf, curRowBuf);
				
				continue;
				
				}
				
			info.restartRowsToGo--;
		   
			}
			
		

		for (int32 curComp = 0; curComp < compsInScan; curComp++)
			{
			
			

			int32 d = 0;
		
			int32 s = HuffDecode (ht [curComp]);
			
			if (s)
				{

				if (s == 16 && !fBug16)
					{
					d = -32768;
					}
				
				else
					{
					d = get_bits (s);
					HuffExtend (d, s);
					}

				}
				
			

			curRowBuf [0] [curComp] = (ComponentType) (d + prevRowBuf [0] [curComp]);
			
			}

		
		

		if (compsInScan == 2 && info.Ss == 1 && numCOL > 1)
			{
			
			
			
			
			uint16 *dPtr = &curRowBuf [1] [0];
			
			int32 prev0 = dPtr [-2];
			int32 prev1 = dPtr [-1];
			
			for (int32 col = 1; col < numCOL; col++)
				{
				
				int32 s = HuffDecode (ht [0]);
				
				if (s)
					{
					
					int32 d;

					if (s == 16 && !fBug16)
						{
						d = -32768;
						}
					
					else
						{
						d = get_bits (s);
						HuffExtend (d, s);
						}

					prev0 += d;
					
					}
					
				s = HuffDecode (ht [1]);
				
				if (s)
					{
					
					int32 d;

					if (s == 16 && !fBug16)
						{
						d = -32768;
						}
					
					else
						{
						d = get_bits (s);
						HuffExtend (d, s);
						}

					prev1 += d;
					
					}
				
				dPtr [0] = (uint16) prev0;
				dPtr [1] = (uint16) prev1;
				
				dPtr += 2;
				
				}
				
			}
			
		else
			{
			
			for (int32 col = 1; col < numCOL; col++)
				{
				
				for (int32 curComp = 0; curComp < compsInScan; curComp++)
					{
					
					

					int32 d = 0;
				
					int32 s = HuffDecode (ht [curComp]);
					
					if (s)
						{
						
						if (s == 16 && !fBug16)
							{
							d = -32768;
							}
						
						else
							{
							d = get_bits (s);
							HuffExtend (d, s);
							}

						}
						
					
					
					int32 predictor = QuickPredict (col,
													curComp,
													curRowBuf,
													prevRowBuf);
												  
					

					curRowBuf [col] [curComp] = (ComponentType) (d + predictor);
					
					}
					
				}

			}

		PmPutRow (curRowBuf, compsInScan, numCOL, row);
		
		swap (MCU *, prevRowBuf, curRowBuf);
		
		}
		
	#undef swap
	
	}

template <SIMDType simd>
void dng_lossless_decoder<simd>::StartRead (uint32 &imageWidth,
											uint32 &imageHeight,
											uint32 &imageChannels)
	{ 
	
	ReadFileHeader	  ();
	ReadScanHeader	  ();
	DecoderStructInit ();
	HuffDecoderInit	  ();
	
	imageWidth	  = info.imageWidth;
	imageHeight	  = info.imageHeight;
	imageChannels = info.compsInScan;
	
	}

template <SIMDType simd>
void dng_lossless_decoder<simd>::FinishRead ()
	{
	
	DecodeImage ();
		
	}

template <SIMDType simd>
class dng_lossless_encoder
	{
	
	private:
	
		const uint16 *fSrcData;
		
		uint32 fSrcRows;
		uint32 fSrcCols;
		uint32 fSrcChannels;
		uint32 fSrcBitDepth;
		
		int32 fSrcRowStep;
		int32 fSrcColStep;
	
		dng_stream &fStream;
	
		HuffmanTable huffTable [4];
		
		uint32 freqCount [4] [257];
		
		

		uint64 huffPutBuffer;
		uint64 huffPutBits;
		
		
		
		int numBitsTable [256];
		
		std::vector<uint8> streamBuffer;
		size_t streamBufferOffset;

	public:
	
		dng_lossless_encoder (const uint16 *srcData,
							  uint32 srcRows,
							  uint32 srcCols,
							  uint32 srcChannels,
							  uint32 srcBitDepth,
							  int32 srcRowStep,
							  int32 srcColStep,
							  dng_stream &stream);
		
		void Encode ();
		
	private:
	
		void EmitByte (uint8 value);
	
		void EmitBits (int code, int size);

		void FlushBits ();

		void PushBits ();

		void FlushBuffer();

		int EmitBitsToBuffer (int buffered_bits,
							  uint64 bit_buffer);

		int EncodeOneDiffToBuffer (int diff,
								   HuffmanTable *dctbl,
								   int buffered_bits,
								   uint64 &bit_buffer);

		void CountOneDiff (int diff, uint32 *countTable);

		void EncodeOneDiff (int diff, HuffmanTable *dctbl);
		
		void FreqCountSet ();

		void HuffEncode ();

		void GenHuffCoding (HuffmanTable *htbl, uint32 *freq);

		void HuffOptimize ();

		void EmitMarker (JpegMarker mark);

		void Emit2bytes (int value);

		void EmitDht (int index);

		void EmitSof (JpegMarker code);

		void EmitSos ();

		void WriteFileHeader ();

		void WriteScanHeader ();

		void WriteFileTrailer ();

	};
	

template <SIMDType simd>
dng_lossless_encoder<simd>::dng_lossless_encoder (const uint16 *srcData,
											uint32 srcRows,
											uint32 srcCols,
											uint32 srcChannels,
											uint32 srcBitDepth,
											int32 srcRowStep,
											int32 srcColStep,
											dng_stream &stream)
									
	:	fSrcData	 (srcData	 )
	,	fSrcRows	 (srcRows	 )
	,	fSrcCols	 (srcCols	 )
	,	fSrcChannels (srcChannels)
	,	fSrcBitDepth (srcBitDepth)
	,	fSrcRowStep	 (srcRowStep )
	,	fSrcColStep	 (srcColStep )
	,	fStream		 (stream	 )
	
	,	huffPutBuffer (0)
	,	huffPutBits	  (0)

	,	streamBufferOffset (0)

	{
	
	
	
	numBitsTable [0] = 0;
		
	for (int i = 1; i < 256; i++)
		{
		
		int temp = i;
		int nbits = 1;
		
		while (temp >>= 1)
			{
			nbits++;
			}
			
		numBitsTable [i] = nbits;
		
		}
		
	
	
	
	
	
	
	
	
	
	

	
	

	uint32 streamBufferExtent32 = SafeUint32Mult (srcCols,
												  srcChannels);

	const uint32 bytesPerSample = SafeUint32Add (srcBitDepth, 7u) / 8u;

	streamBufferExtent32 = SafeUint32Mult (streamBufferExtent32,
										   bytesPerSample);

	streamBufferExtent32 = SafeUint32Mult (streamBufferExtent32, 2u);
	streamBufferExtent32 = SafeUint32Mult (streamBufferExtent32, 2u);
	streamBufferExtent32 = SafeUint32Add  (streamBufferExtent32, 1u);

	
	

	uint32 streamHeaderExtent32 = SafeUint32Mult (srcChannels, 296u);

	streamHeaderExtent32 = SafeUint32Add (streamHeaderExtent32, 64u);

	const size_t streamBufferExtent = std::max<size_t> (streamBufferExtent32,
														streamHeaderExtent32);
	streamBuffer.resize (streamBufferExtent);

	}

template <SIMDType simd>
inline void dng_lossless_encoder<simd>::EmitByte (uint8 value)
	{

	if (streamBufferOffset >= streamBuffer.size ())
		{
		ThrowProgramError ("Lossless JPEG output buffer overflow");
		}
	
	streamBuffer[streamBufferOffset++] = value;
	
	}
	

template <SIMDType simd>
void dng_lossless_encoder<simd>::PushBits ()
	{
	while (huffPutBits >= 8)
		{
		uint8 c = (uint8) (huffPutBuffer >> (huffPutBits - 8));
		EmitByte (c);
		if (c == 0xff)
			{
			EmitByte(0x00);
			}
		huffPutBits -= 8;
		}
	}

 
template <SIMDType simd>
inline void dng_lossless_encoder<simd>::EmitBits (int code, int size)
	{
	
	DNG_ASSERT (size != 0, "Bad Huffman table entry");

	if ((huffPutBits + size) > 64)
		{
		PushBits();
			}
	huffPutBuffer <<= size;
	huffPutBuffer |= code;
	huffPutBits += size;
	}

template <SIMDType simd>
void dng_lossless_encoder<simd>::FlushBits ()
	{
	
	

	EmitBits (0x007F, 7);

	PushBits();

	

	huffPutBuffer = 0;
	huffPutBits	  = 0;
	
	}

template <SIMDType simd>
void dng_lossless_encoder<simd>::FlushBuffer()
	{
	  fStream.Put(&streamBuffer[0], (uint32) streamBufferOffset);
	  streamBufferOffset = 0;
	}

template <SIMDType simd>
inline void dng_lossless_encoder<simd>::CountOneDiff (int diff, uint32 *countTable)
	{
	
	
	 
	int temp = diff;
	
	if (temp < 0)
		{
		
		temp = -temp;
 
		}

	

	int nbits = temp >= 256 ? numBitsTable [temp >> 8  ] + 8
							: numBitsTable [temp & 0xFF];
			
	

	countTable [nbits] ++;
	
	}

template <SIMDType simd>
inline void dng_lossless_encoder<simd>::EncodeOneDiff (int diff, HuffmanTable *dctbl)
	{

	
	 
	int temp  = diff;
	int temp2 = diff;
	
	if (temp < 0)
		{
		
		temp = -temp;
		
		
		
		

		temp2--;
		
		}

	

	int nbits = temp >= 256 ? numBitsTable [temp >> 8  ] + 8
							: numBitsTable [temp & 0xFF];

	

	EmitBits (dctbl->ehufco [nbits],
			  dctbl->ehufsi [nbits]);

	
	
	
	
	
	
	

	if (nbits & 15)
		{
		
		EmitBits (temp2 & (0x0FFFF >> (16 - nbits)),
				  nbits);
		
		}

	}

template <SIMDType simd>
inline int dng_lossless_encoder<simd>::EmitBitsToBuffer (int buffered_bits,
														 uint64 bit_buffer)
	{
	DNG_ASSERT(buffered_bits < 64, "buffered_bits too big(3)");
	while (buffered_bits >= 8)
			{
		uint8 c = (uint8)(bit_buffer >> (buffered_bits - 8));

		if (streamBufferOffset >= streamBuffer.size ())
			{
			ThrowProgramError ("Lossless JPEG output buffer overflow");
			}

		streamBuffer[streamBufferOffset++] = c;
		if (c == 0xff)
			{

			if (streamBufferOffset >= streamBuffer.size ())
				{
				ThrowProgramError ("Lossless JPEG output buffer overflow");
				}

			streamBuffer[streamBufferOffset++] = 0x00;
			}
		buffered_bits -= 8;
		}
	return buffered_bits;
	}

template <SIMDType simd>
inline int dng_lossless_encoder<simd>::EncodeOneDiffToBuffer (int diff,
															  HuffmanTable *dctbl,
															  int buffered_bits,
															  uint64 &bit_buffer)
	{
	
	DNG_ASSERT(buffered_bits < 64, "buffered_bits too big(1)");
	
	if (buffered_bits > 32)
		{
		buffered_bits = EmitBitsToBuffer(buffered_bits, bit_buffer);
		}
		
	

	int temp  = diff;
	int temp2 = diff;

	if (temp < 0)
		{

		temp = -temp;

		
		
		

		temp2--;

		}

	

	int nbits = temp >= 256 ? numBitsTable [temp >> 8  ] + 8
							: numBitsTable [temp & 0xFF];

	
	int bits_bits = dctbl->ehufsi [nbits];
	bit_buffer <<= bits_bits;
	bit_buffer |= dctbl->ehufco [nbits];
	buffered_bits += bits_bits;

	
	

	
	
	
	

	if (nbits & 15)
		{
		bit_buffer <<= nbits;
		bit_buffer |= temp2 & (0x0FFFF >> (16 - nbits));
		buffered_bits += nbits;
		}

	DNG_ASSERT(buffered_bits < 64, "buffered_bits too big(2)");
	
	return buffered_bits;
	
	}

template <SIMDType simd>
void dng_lossless_encoder<simd>::FreqCountSet ()
	{
	
	memset (freqCount, 0, sizeof (freqCount));
	
	DNG_ASSERT ((int32)fSrcRows >= 0, "dng_lossless_encoder::FreqCountSet: fSrcRpws too large.");

	for (int32 row = 0; row < (int32)fSrcRows; row++)
		{
		
		const uint16 *sPtr = fSrcData + row * fSrcRowStep;
		
		
		
		int32 predictor [4] = { 0, 0, 0, 0 };
		
		for (int32 channel = 0; channel < (int32)fSrcChannels; channel++)
			{
			
			if (row == 0)
				predictor [channel] = 1 << (fSrcBitDepth - 1);
				
			else
				predictor [channel] = sPtr [channel - fSrcRowStep];
			
			}
			
		
		
		if (fSrcChannels == 2)
			{
			
			int32 pred0 = predictor [0];
			int32 pred1 = predictor [1];
			
			uint32 srcCols	  = fSrcCols;
			int32  srcColStep = fSrcColStep;
			
			for (uint32 col = 0; col < srcCols; col++)
				{
				
				int32 pixel0 = sPtr [0];
				int32 pixel1 = sPtr [1];
				
				int16 diff0 = (int16) (pixel0 - pred0);
				int16 diff1 = (int16) (pixel1 - pred1);
				
				CountOneDiff (diff0, freqCount [0]);
				CountOneDiff (diff1, freqCount [1]);
				
				pred0 = pixel0;
				pred1 = pixel1;
					
				sPtr += srcColStep;
					
				}
			
			}
			
		
			
		else
			{
			
			for (uint32 col = 0; col < fSrcCols; col++)
				{
				
				for (uint32 channel = 0; channel < fSrcChannels; channel++)
					{
					
					int32 pixel = sPtr [channel];
					
					int16 diff = (int16) (pixel - predictor [channel]);
					
					CountOneDiff (diff, freqCount [channel]);
					
					predictor [channel] = pixel;
					
					}
					
				sPtr += fSrcColStep;
					
				}
				
			}
			
		}

	}

template <SIMDType simd>
void dng_lossless_encoder<simd>::HuffEncode ()
	{
	
	DNG_ASSERT ((int32)fSrcRows >= 0, "dng_lossless_encoder::HuffEncode: fSrcRows too large.");

	uint64 bit_buffer = 0;
	int buffered_bits = 0;

	if (fSrcChannels == 2)
		{
		bit_buffer = huffPutBuffer;
		buffered_bits = (int) huffPutBits;
		}

	for (int32 row = 0; row < (int32)fSrcRows; row++)
		{
		
		const uint16 *sPtr = fSrcData + row * fSrcRowStep;
		
		
		
		int32 predictor [4] = { 0, 0, 0, 0 };
		
		for (int32 channel = 0; channel < (int32)fSrcChannels; channel++)
			{
			
			if (row == 0)
				predictor [channel] = 1 << (fSrcBitDepth - 1);
				
			else
				predictor [channel] = sPtr [channel - fSrcRowStep];
			
			}
			
		
		
		if (fSrcChannels == 2)
			{
			
			int32 pred0 = predictor [0];
			int32 pred1 = predictor [1];
			
			uint32 srcCols	  = fSrcCols;
			int32  srcColStep = fSrcColStep;
			
			for (uint32 col = 0; col < srcCols; col++)
				{
				
				int32 pixel0 = sPtr [0];
				int32 pixel1 = sPtr [1];
				
				int16 diff0 = (int16) (pixel0 - pred0);
				int16 diff1 = (int16) (pixel1 - pred1);
				
				buffered_bits = EncodeOneDiffToBuffer (diff0, &huffTable [0], buffered_bits, bit_buffer);
				buffered_bits = EncodeOneDiffToBuffer (diff1, &huffTable [1], buffered_bits, bit_buffer);

				pred0 = pixel0;
				pred1 = pixel1;
					
				sPtr += srcColStep;
					
				}
			
			buffered_bits = EmitBitsToBuffer(buffered_bits, bit_buffer);

			}
			
		
			
		else
			{
			
			for (uint32 col = 0; col < fSrcCols; col++)
				{
				
				for (uint32 channel = 0; channel < fSrcChannels; channel++)
					{
					
					int32 pixel = sPtr [channel];
					
					int16 diff = (int16) (pixel - predictor [channel]);
					
					EncodeOneDiff (diff, &huffTable [channel]);
					
					predictor [channel] = pixel;
					
					}
					
				sPtr += fSrcColStep;
					
				}
				
			}
			
		FlushBuffer();
		
		}
  
	if (fSrcChannels == 2)
		{
		huffPutBuffer = bit_buffer;
		huffPutBits = buffered_bits;
		}

	FlushBits ();
	
	}

template <SIMDType simd>
void dng_lossless_encoder<simd>::GenHuffCoding (HuffmanTable *htbl, uint32 *freq)
	{
	
	int i;
	int j;
	
	const int MAX_CLEN = 32;		
	
	uint8 bits [MAX_CLEN + 1];	
	short codesize [257];			
	short others   [257];			
	
	memset (bits	, 0, sizeof (bits	 ));
	memset (codesize, 0, sizeof (codesize));
	
	for (i = 0; i < 257; i++)
		others [i] = -1;			

	
	
	

	freq [256] = 1;					

	
	
	while (true)
		{

		
		

		int c1 = -1;
		
		uint32 v = 0xFFFFFFFF;
		
		for (i = 0; i <= 256; i++)
			{
			
			if (freq [i] && freq [i] <= v)
				{
				v = freq [i];
				c1 = i;
				}
	
			}

		
		

		int c2 = -1;
		
		v = 0xFFFFFFFF;
		
		for (i = 0; i <= 256; i++)
			{
			
			if (freq [i] && freq [i] <= v && i != c1) 
				{
				v = freq [i];
				c2 = i;
				}
				
			}

		

		if (c2 < 0)
			break;
	
		

		freq [c1] += freq [c2];
		freq [c2] = 0;

		

		codesize [c1] ++;
		
		while (others [c1] >= 0)
			{
			c1 = others [c1];
			codesize [c1] ++;
			}
	
		

		others [c1] = (short) c2;
	
		

		codesize [c2] ++;
		
		while (others [c2] >= 0) 
			{
			c2 = others [c2];
			codesize [c2] ++;
			}

		}

	

	for (i = 0; i <= 256; i++)
		{
		
		if (codesize [i])
			{

			
			
			
			if (codesize [i] > MAX_CLEN)
				{
	   
				ThrowOverflow ("Huffman code size table overflow");
				
				}

			bits [codesize [i]]++;
			
			}

		}

	
	
	
	
	
	
	
	
	
  
	for (i = MAX_CLEN; i > 16; i--)
		{
		
		while (bits [i] > 0)
			{
			
			
			
			
			
			
			DNG_REPORT ("Info: Optimal huffman table bigger than 16 bits");
			
			ThrowProgramError ();
			
			
			
			j = i - 2;		
			
			while (bits [j] == 0)
				j--;
	  
			bits [i	   ] -= 2;		
			bits [i - 1] ++;		
			bits [j + 1] += 2;		
			bits [j	   ] --;		
			
			}
			
		}

	
	
	
	while (bits [i] == 0)		
		i--;
		
	bits [i] --;
  
	

	memcpy (htbl->bits, bits, sizeof (htbl->bits));
  
	
	
	
   
	int p = 0;
	
	for (i = 1; i <= MAX_CLEN; i++)
		{
		
		for (j = 0; j <= 255; j++)
			{
			
			if (codesize [j] == i)
				{
				htbl->huffval [p] = (uint8) j;
				p++;
				}

			}
			
		}
 
	}

template <SIMDType simd>
void dng_lossless_encoder<simd>::HuffOptimize ()
	{
	
	
	 
	FreqCountSet ();
	
	
	
	for (uint32 channel = 0; channel < fSrcChannels; channel++)
		{
		
		try
			{
			
			GenHuffCoding (&huffTable [channel], freqCount [channel]);
			
			}
			
		catch (...)
			{
			
			DNG_REPORT ("Info: Reverting to default huffman table");
			
			for (uint32 j = 0; j <= 256; j++)
				{
				
				freqCount [channel] [j] = (j <= 16 ? 1 : 0);
				
				}
			
			GenHuffCoding (&huffTable [channel], freqCount [channel]);
			
			}
		
		FixHuffTbl (&huffTable [channel]);
		
		}
 
	}

template <SIMDType simd>
void dng_lossless_encoder<simd>::EmitMarker (JpegMarker mark)
	{
	
	EmitByte (0xFF);
	EmitByte ((uint8) mark);
	
	}

template <SIMDType simd>
void dng_lossless_encoder<simd>::Emit2bytes (int value)
	{
	
	EmitByte ((value >> 8) & 0xFF);
	EmitByte (value & 0xFF);
 
	}

 
template <SIMDType simd>
void dng_lossless_encoder<simd>::EmitDht (int index)
	{
	
	int i;
	
	HuffmanTable *htbl = &huffTable [index];
	
	EmitMarker (M_DHT);

	int length = 0;
	
	for (i = 1; i <= 16; i++)
		length += htbl->bits [i];

	Emit2bytes (length + 2 + 1 + 16);
	
	EmitByte ((uint8) index);

	for (i = 1; i <= 16; i++)
		EmitByte (htbl->bits [i]);

	for (i = 0; i < length; i++)
		EmitByte (htbl->huffval [i]);

	}

template <SIMDType simd>
void dng_lossless_encoder<simd>::EmitSof (JpegMarker code)
	{
	
	EmitMarker (code);

	Emit2bytes (3 * fSrcChannels + 2 + 5 + 1);	

	EmitByte ((uint8) fSrcBitDepth);
	
	Emit2bytes (fSrcRows);
	Emit2bytes (fSrcCols);

	EmitByte ((uint8) fSrcChannels);

	for (uint32 i = 0; i < fSrcChannels; i++)
		{
		
		EmitByte ((uint8) i);
		
		EmitByte ((uint8) ((1 << 4) + 1));		
				 
		EmitByte (0);					
		
		}
   
	}

template <SIMDType simd>
void dng_lossless_encoder<simd>::EmitSos ()
	{
	
	EmitMarker (M_SOS);

	Emit2bytes (2 * fSrcChannels + 2 + 1 + 3);	

	EmitByte ((uint8) fSrcChannels);			

	for (uint32 i = 0; i < fSrcChannels; i++) 
		{ 
		
		
		
		EmitByte ((uint8) i);
		EmitByte ((uint8) (i << 4));
		
		}

	EmitByte (1);		
	EmitByte (0);		
	EmitByte (0);		
	
	}

template <SIMDType simd>
void dng_lossless_encoder<simd>::WriteFileHeader ()
	{
	
	EmitMarker (M_SOI);		
	
	EmitSof (M_SOF3);
	
	}

template <SIMDType simd>
void dng_lossless_encoder<simd>::WriteScanHeader ()
	{

	
	
	for (uint32 i = 0; i < fSrcChannels; i++)
		{
		
		EmitDht (i);
		
		}

	EmitSos ();
	 
	}

template <SIMDType simd>
void dng_lossless_encoder<simd>::WriteFileTrailer ()
	{
	
	EmitMarker (M_EOI);
	
	}

template <SIMDType simd>
void dng_lossless_encoder<simd>::Encode ()
	{
	
	DNG_ASSERT (fSrcChannels <= 4, "Too many components in scan");
	
	
	
	
	HuffOptimize ();

	

	WriteFileHeader (); 
	
	WriteScanHeader ();

	FlushBuffer ();

	
	
	HuffEncode ();

	FlushBuffer ();

	
	
	WriteFileTrailer ();

	FlushBuffer ();

	}

template <SIMDType simd>
void DecodeLosslessJPEG (dng_stream &stream,
						 dng_spooler &spooler,
						 uint32 minDecodedSize,
						 uint32 maxDecodedSize,
						 bool bug16,
						 uint64 endOfData)
	{

	dng_lossless_decoder<simd> decoder (&stream,
										&spooler,
										bug16);
	
	uint32 imageWidth;
	uint32 imageHeight;
	uint32 imageChannels;
	
	decoder.StartRead (imageWidth,
					   imageHeight,
					   imageChannels);
					   
	uint32 decodedSize = SafeUint32Mult (imageWidth,
										 imageHeight,
										 imageChannels,
										 (uint32) sizeof (uint16));
					   
	if (decodedSize < minDecodedSize ||
		decodedSize > maxDecodedSize)
		{
		ThrowBadFormat ();
		}
	
	decoder.FinishRead ();
	
	uint64 streamPos = stream.Position ();
	
	if (streamPos > endOfData)
		{
		
		bool throwBadFormat = true;
		
		
		
		
		
		#if qSupportHasselblad_3FR
		
		if (decoder.IsHasselblad3FR () &&
			streamPos - endOfData == 4)
			{
			throwBadFormat = false;
			}
			
		#endif
		
		if (throwBadFormat)
			{
			ThrowBadFormat ();
			}

		}
	
	}

template <SIMDType simd>
void EncodeLosslessJPEG (const uint16 *srcData,
						 uint32 srcRows,
						 uint32 srcCols,
						 uint32 srcChannels,
						 uint32 srcBitDepth,
						 int32 srcRowStep,
						 int32 srcColStep,
						 dng_stream &stream)
	{
	
	dng_lossless_encoder<simd> encoder (srcData,
										srcRows,
										srcCols,
										srcChannels,
										srcBitDepth,
										srcRowStep,
										srcColStep,
										stream);

	encoder.Encode ();

	}

