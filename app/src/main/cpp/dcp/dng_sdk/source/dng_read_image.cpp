

#include "dng_read_image.h"

#include "dng_abort_sniffer.h"
#include "dng_area_task.h"
#include "dng_bottlenecks.h"
#include "dng_exceptions.h"
#include "dng_flags.h"
#include "dng_globals.h"
#include "dng_host.h"
#include "dng_image.h"
#include "dng_ifd.h"
#include "dng_jpeg_image.h"
#include "dng_jxl.h"
#include "dng_lossless_jpeg.h"
#include "dng_mutex.h"
#include "dng_memory.h"
#include "dng_pixel_buffer.h"
#include "dng_safe_arithmetic.h"
#include "dng_tag_types.h"
#include "dng_tag_values.h"
#include "dng_uncopyable.h"
#include "dng_utils.h"

#include "zlib.h"

#if qDNGUseLibJPEG
#include "dng_jpeg_memory_source.h"
#include "dng_jpeglib.h"
#endif

#include <limits>

static void DecodeDelta8 (uint8 *dPtr,
						  uint32 rows,
						  uint32 cols,
						  uint32 channels)
	{
	
	const uint32 dRowStep = SafeUint32Mult (cols, channels);
	
	for (uint32 row = 0; row < rows; row++)
		{
		
		for (uint32 col = 1; col < cols; col++)
			{
			
			for (uint32 channel = 0; channel < channels; channel++)
				{
				
				dPtr [col * channels + channel] += dPtr [(col - 1) * channels + channel];
				
				}
			
			}
		
		dPtr += dRowStep;
		
		}

	}

static void DecodeDelta16 (uint16 *dPtr,
						   uint32 rows,
						   uint32 cols,
						   uint32 channels)
	{
	
	const uint32 dRowStep = SafeUint32Mult (cols, channels);
	
	for (uint32 row = 0; row < rows; row++)
		{
		
		for (uint32 col = 1; col < cols; col++)
			{
			
			for (uint32 channel = 0; channel < channels; channel++)
				{
				
				dPtr [col * channels + channel] += dPtr [(col - 1) * channels + channel];
				
				}
			
			}
		
		dPtr += dRowStep;
		
		}

	}
	

static void DecodeDelta32 (uint32 *dPtr,
						   uint32 rows,
						   uint32 cols,
						   uint32 channels)
	{
	
	const uint32 dRowStep = SafeUint32Mult (cols, channels);
	
	for (uint32 row = 0; row < rows; row++)
		{
		
		for (uint32 col = 1; col < cols; col++)
			{
			
			for (uint32 channel = 0; channel < channels; channel++)
				{
				
				dPtr [col * channels + channel] += dPtr [(col - 1) * channels + channel];
				
				}
			
			}
		
		dPtr += dRowStep;
		
		}

	}
	

inline void DecodeDeltaBytes (uint8 *bytePtr, int32 cols, int32 channels)
	{
	
	if (channels == 1)
		{
		
		uint8 b0 = bytePtr [0];
		
		bytePtr += 1;
		
		for (int32 col = 1; col < cols; ++col)
			{
			
			b0 += bytePtr [0];
			
			bytePtr [0] = b0;
			
			bytePtr += 1;

			}
			
		}
	
	else if (channels == 3)
		{
		
		uint8 b0 = bytePtr [0];
		uint8 b1 = bytePtr [1];
		uint8 b2 = bytePtr [2];
		
		bytePtr += 3;
		
		for (int32 col = 1; col < cols; ++col)
			{
			
			b0 += bytePtr [0];
			b1 += bytePtr [1];
			b2 += bytePtr [2];
			
			bytePtr [0] = b0;
			bytePtr [1] = b1;
			bytePtr [2] = b2;
			
			bytePtr += 3;

			}
			
		}
		
	else if (channels == 4)
		{
		
		uint8 b0 = bytePtr [0];
		uint8 b1 = bytePtr [1];
		uint8 b2 = bytePtr [2];
		uint8 b3 = bytePtr [3];
		
		bytePtr += 4;
		
		for (int32 col = 1; col < cols; ++col)
			{
			
			b0 += bytePtr [0];
			b1 += bytePtr [1];
			b2 += bytePtr [2];
			b3 += bytePtr [3];
			
			bytePtr [0] = b0;
			bytePtr [1] = b1;
			bytePtr [2] = b2;
			bytePtr [3] = b3;
			
			bytePtr += 4;

			}
			
		}
		
	else
		{
		
		for (int32 col = 1; col < cols; ++col)
			{
			
			for (int32 chan = 0; chan < channels; ++chan)
				{
				
				bytePtr [chan + channels] += bytePtr [chan];
				
				}
				
			bytePtr += channels;

			}
			
		}

	}
							

static void DecodeFPDelta (uint8 *input,
						   uint8 *output,
						   int32 cols,
						   int32 channels,
						   int32 bytesPerSample)
	{
	
	DecodeDeltaBytes (input,
					  SafeInt32Mult (cols, bytesPerSample),
					  channels);
	
	int32 rowIncrement = SafeInt32Mult (cols, channels);
	
	if (bytesPerSample == 2)
		{
		
		#if qDNGBigEndian
		const uint8 *input0 = input;
		const uint8 *input1 = input + rowIncrement;
		#else
		const uint8 *input1 = input;
		const uint8 *input0 = input + rowIncrement;
		#endif
		
		for (int32 col = 0; col < rowIncrement; ++col)
			{
			
			output [0] = input0 [col];
			output [1] = input1 [col];
			
			output += 2;
				
			}
			
		}
		
	else if (bytesPerSample == 3)
		{

		const int32 rowIncrement2 = SafeInt32Mult (rowIncrement, 2);
		
		const uint8 *input0 = input;
		const uint8 *input1 = input + rowIncrement;
		const uint8 *input2 = input + rowIncrement2;
		
		for (int32 col = 0; col < rowIncrement; ++col)
			{
			
			output [0] = input0 [col];
			output [1] = input1 [col];
			output [2] = input2 [col];
			
			output += 3;
				
			}
			
		}
		
	
	

	else if (bytesPerSample == 4)
		{

		const int32 rowIncrement2 = SafeInt32Mult (rowIncrement, 2);
		const int32 rowIncrement3 = SafeInt32Mult (rowIncrement, 3);
		
		#if qDNGBigEndian
		const uint8 *input0 = input;
		const uint8 *input1 = input + rowIncrement;
		const uint8 *input2 = input + rowIncrement2;
		const uint8 *input3 = input + rowIncrement3;
		#else
		const uint8 *input3 = input;
		const uint8 *input2 = input + rowIncrement;
		const uint8 *input1 = input + rowIncrement2;
		const uint8 *input0 = input + rowIncrement3;
		#endif
		
		for (int32 col = 0; col < rowIncrement; ++col)
			{
			
			output [0] = input0 [col];
			output [1] = input1 [col];
			output [2] = input2 [col];
			output [3] = input3 [col];
			
			output += 4;
				
			}
			
		}

	else
		{

		ThrowBadFormat ();

		}
		
	}	
						

bool DecodePackBits (dng_stream &stream,
					 uint8 *dPtr,
					 int32 dstCount)
	{

	while (dstCount > 0)
		{
		
		int32 runCount = (int8) stream.Get_uint8 ();

		if (runCount >= 0)
			{
			
			++runCount;
			
			dstCount -= runCount;

			if (dstCount < 0)
				return false;
				
			stream.Get (dPtr, runCount);
			
			dPtr += runCount;
			
			}
			
		else
			{
			
			runCount = -runCount + 1;

			dstCount -= runCount;

			if (dstCount < 0)
				return false;
				
			uint8 x = stream.Get_uint8 ();
			
			while (runCount--)
				{
				
				*(dPtr++) = x;
				
				}

			}
			
		}

	return true;

	}

class dng_lzw_expander: private dng_uncopyable
	{
	
	private:
	
		enum
			{
			kResetCode = 256,
			kEndCode   = 257,
			kTableSize = 4096
			};
		
		struct LZWExpanderNode
			{
			int16 prefix;
			int16 final;
			int16 depth;
			int16 fake_for_padding;
			};

		dng_memory_data fBuffer;

		LZWExpanderNode *fTable;
		
		const uint8 *fSrcPtr;
		
		int32 fSrcCount;
		
		int32 fByteOffset;

		uint32 fBitBuffer;
		int32 fBitBufferCount;
		
		int32 fNextCode;
		
		int32 fCodeSize;
		
	public:
	
		dng_lzw_expander ();

		bool Expand (const uint8 *sPtr,
					 uint8 *dPtr,
					 int32 sCount,
					 int32 dCount);
	
	private:
	
		void InitTable ();
	
		void AddTable (int32 w, int32 k);
		
		bool GetCodeWord (int32 &code);

	};

dng_lzw_expander::dng_lzw_expander ()

	:	fBuffer			 ()
	,	fTable			 (NULL)
	,	fSrcPtr			 (NULL)
	,	fSrcCount		 (0)
	,	fByteOffset		 (0)
	,	fBitBuffer		 (0)
	,	fBitBufferCount	 (0)
	,	fNextCode		 (0)
	,	fCodeSize		 (0)
	
	{
	
	fBuffer.Allocate ((kTableSize + 1) * sizeof (LZWExpanderNode));
	
	fTable = (LZWExpanderNode *) fBuffer.Buffer ();

	}

void dng_lzw_expander::InitTable ()
	{

	fCodeSize = 9;

	fNextCode = 258;
		
	LZWExpanderNode *node = &fTable [0];
	
	for (int32 code = 0; code <= kTableSize; code++)
		{
		
		node->prefix  = -1;
		node->final	  = (int16) code;
		node->depth	  = 1;
		
		node++;
		
		}
		
	}

void dng_lzw_expander::AddTable (int32 w, int32 k)
	{
	
	DNG_ASSERT ((w >= 0) && (w <= kTableSize),
				"bad w value in dng_lzw_expander::AddTable");

	LZWExpanderNode *parentNode = &fTable [w];
	
	int32 nextCode = fNextCode;
	
	fNextCode++;
	
	DNG_ASSERT ((nextCode >= 0) && (nextCode <= kTableSize),
				"bad fNextCode value in dng_lzw_expander::AddTable");
	
	LZWExpanderNode *node = &fTable [nextCode];
	
	node->prefix  = (int16) w;
	node->final	  = (int16) k;
	node->depth	  = 1 + parentNode->depth;
	
	if (nextCode + 1 == (1 << fCodeSize) - 1)
		{
		if (fCodeSize != 12)
			fCodeSize++;
		}
		
	}

bool dng_lzw_expander::GetCodeWord (int32 &code)
	{

	
	
	
	int32 codeSize = fCodeSize;

	code = fBitBuffer >> (32 - codeSize);

	if (fBitBufferCount >= codeSize)
		{
		
		

		fBitBuffer	   <<= codeSize;
		fBitBufferCount -= codeSize;
		
		}

	else
		{
		
		

		const int32 bitsSoFar = fBitBufferCount;

		if (fByteOffset >= fSrcCount)
			return false;

		

		const uint8 *ptr = fSrcPtr + fByteOffset;

		uint32 avail = fSrcCount - fByteOffset;

		uint32 b0 = (avail > 0) ? ptr [0] : 0;
		uint32 b1 = (avail > 1) ? ptr [1] : 0;
		uint32 b2 = (avail > 2) ? ptr [2] : 0;
		uint32 b3 = (avail > 3) ? ptr [3] : 0;

		fBitBuffer = (((((b0 << 8) | b1) << 8) | b2) << 8) | b3;

		fBitBufferCount = 32;

		uint32 consumed = Min_uint32 (avail, 4);

		fByteOffset += consumed;

		
		
		const int32 bitsUsed = codeSize - bitsSoFar;

		
		
		const int32 bitsNotUsed = 32 - bitsUsed;

		code |= fBitBuffer >> bitsNotUsed;

		fBitBuffer	   <<= bitsUsed;
		fBitBufferCount -= bitsUsed;

		}

	return true;

	}

bool dng_lzw_expander::Expand (const uint8 *sPtr,
							   uint8 *dPtr,
							   int32 sCount,
							   int32 dCount)
	{

	if (sCount < 0 || dCount < 0)
		{
		return false;
		}

	if (dCount == 0)
		{
		return true;
		}
	
	void *dStartPtr = dPtr;
	
	fSrcPtr = sPtr;
	
	fSrcCount = sCount;
	
	fByteOffset = 0;
	
	
	
	while (true)
		{
		
		InitTable ();
			
		int32 code;
			
		do
			{
			
			if (!GetCodeWord (code)) 
				return false;
				
			DNG_ASSERT (code <= fNextCode,
						"Unexpected LZW code in dng_lzw_expander::Expand");
			
			}
		while (code == kResetCode);
		
		
		

		if (code == kEndCode)
			{
			return dCount == 0;
			}
		
		if (code > kEndCode) 
			return false;
		
		int32 oldCode = code;
		int32 inChar  = code;

		(void) inChar;
		
		*(dPtr++) = (uint8) code;
		
		if (--dCount == 0) 
			return true;
		
		while (true)
			{
			
			if (!GetCodeWord (code)) 
				return false;

			if (code == kResetCode) 
				break;
			
			
			

			if (code == kEndCode)
				{
				return dCount == 0;
				}
			
			const int32 inCode = code;

			bool repeatLastPixel = false;
			
			if (code >= fNextCode)
				{
					
				
				
				
				code = oldCode;	

				repeatLastPixel = true;

				}
			
			
			
			if (code > fNextCode)
				return false;

			const int32 depth = fTable [code].depth;

			if (depth < dCount)
				{

				dCount -= depth;
				
				dPtr += depth;

				uint8 *ptr = dPtr;
				
				
				
				const LZWExpanderNode *localTable = fTable;
				
				int32 localCode = code;
				
				
				
				while (localCode >= kResetCode)
					{
					
					if (ptr <= dStartPtr)
						return false;		

					const LZWExpanderNode &node = localTable [localCode];
					
					uint8 tempFinal = (uint8) node.final;
					
					localCode = node.prefix;

					

					if (localCode < 0 || localCode > kTableSize)
						return false;

					*(--ptr) = tempFinal;

					}
				
				code = localCode;

				inChar = localCode;

				if (ptr <= dStartPtr)
					return false;		

				*(--ptr) = (uint8) inChar;

				}
		
			else
				{
				
				
				
				
				const int32 skip = depth - dCount;

				for (int32 i = 0; i < skip ; i++)
					{
					const LZWExpanderNode &node = fTable [code];
					code = node.prefix;
					}

				int32 depthUsed = depth - skip;

				dCount -= depthUsed;

				(void) dCount;
				
				dPtr += depthUsed;

				uint8 *ptr = dPtr;

				while (code >= 0)
					{
					
					if (ptr <= dStartPtr)
						return false;		

					const LZWExpanderNode &node = fTable [code];

					*(--ptr) = (uint8) node.final;

					code = node.prefix;
					
					

					if (code > kTableSize)
						return false;

					}

				return true;

				}

			if (repeatLastPixel)
				{
				
				*(dPtr++) = (uint8) inChar;
				
				if (--dCount == 0)
					return true;
	
				}
				
			if (fNextCode < kTableSize)
				{
				
				AddTable (oldCode, code);
				
				}
			
			oldCode = inCode;
			
			}
			
		}	

	return false;
	
	}

static void ReorderSubTileBlocks (dng_host &host,
								  const dng_ifd &ifd,
								  dng_pixel_buffer &buffer,
								  AutoPtr<dng_memory_block> &tempBuffer)
	{
	
	uint32 tempBufferSize = ComputeBufferSize (buffer.fPixelType,
											   buffer.fArea.Size (),
											   buffer.fPlanes, 
											   padNone);
							
	if (!tempBuffer.Get () || tempBuffer->LogicalSize () < tempBufferSize)
		{
		
		tempBuffer.Reset (host.Allocate (tempBufferSize));
		
		}
	
	uint32 blockRows = ifd.fSubTileBlockRows;
	uint32 blockCols = ifd.fSubTileBlockCols;

	
	

	if (blockRows == 0 ||
		blockCols == 0 ||
		(buffer.fArea.H () % blockRows) != 0 ||
		(buffer.fArea.W () % blockCols) != 0)
		{
		ThrowBadFormat ();
		}
	
	uint32 rowBlocks = buffer.fArea.H () / blockRows;
	uint32 colBlocks = buffer.fArea.W () / blockCols;
	
	int32 rowStep = SafeInt32Mult (buffer.fRowStep,
								   (int32) buffer.fPixelSize);

	int32 colStep = SafeInt32Mult (buffer.fColStep,
								   (int32) buffer.fPixelSize);

	int32 rowBlockStep = SafeInt32Mult (rowStep, (int32) blockRows);
	int32 colBlockStep = SafeInt32Mult (colStep, (int32) blockCols);

	uint32 blockColBytes = SafeUint32Mult (blockCols,
										   buffer.fPlanes,
										   buffer.fPixelSize);
	
	const uint8 *s0 = (const uint8 *) buffer.fData;
		  uint8 *d0 = tempBuffer->Buffer_uint8 ();
	
	for (uint32 rowBlock = 0; rowBlock < rowBlocks; rowBlock++)
		{
		
		uint8 *d1 = d0;
		
		for (uint32 colBlock = 0; colBlock < colBlocks; colBlock++)
			{
			
			uint8 *d2 = d1;
			
			for (uint32 blockRow = 0; blockRow < blockRows; blockRow++)
				{
				
				for (uint32 j = 0; j < blockColBytes; j++)
					{
					
					d2 [j] = s0 [j];
					
					}
					
				s0 += blockColBytes;
				
				d2 += rowStep;
				
				}
			
			d1 += colBlockStep;
			
			}
			
		d0 += rowBlockStep;
		
		}
		
	
		
	DoCopyBytes (tempBuffer->Buffer (),
				 buffer.fData,
				 tempBufferSize);
	
	}

class dng_image_spooler: public dng_spooler,
						 private dng_uncopyable
	{
	
	private:
	
		dng_host &fHost;
		
		const dng_ifd &fIFD;
	
		dng_image &fImage;
	
		dng_rect fTileArea;
		
		uint32 fPlane;
		uint32 fPlanes;
		
		dng_memory_block &fBlock;
		
		AutoPtr<dng_memory_block> &fSubTileBuffer;
		
		dng_rect fTileStrip;
		
		uint8 *fBuffer;
		
		uint32 fBufferCount;
		uint32 fBufferSize;
		
	public:
	
		dng_image_spooler (dng_host &host,
						   const dng_ifd &ifd,
						   dng_image &image,
						   const dng_rect &tileArea,
						   uint32 plane,
						   uint32 planes,
						   dng_memory_block &block,
						   AutoPtr<dng_memory_block> &subTileBuffer);
		
		virtual ~dng_image_spooler ();
			
		virtual void Spool (const void *data,
							uint32 count);
							
	};

dng_image_spooler::dng_image_spooler (dng_host &host,
									  const dng_ifd &ifd,
									  dng_image &image,
									  const dng_rect &tileArea,
									  uint32 plane,
									  uint32 planes,
									  dng_memory_block &block,
									  AutoPtr<dng_memory_block> &subTileBuffer)

	:	fHost (host)
	,	fIFD (ifd)
	,	fImage (image)
	,	fTileArea (tileArea)
	,	fPlane (plane)
	,	fPlanes (planes)
	,	fBlock (block)
	,	fSubTileBuffer (subTileBuffer)
	
	,	fTileStrip ()
	,	fBuffer (NULL)
	,	fBufferCount (0)
	,	fBufferSize (0)
	
	{
	
	uint32 bytesPerRow = SafeUint32Mult (fTileArea.W (),
										 fPlanes,
										 (uint32) sizeof (uint16));

	DNG_REQUIRE (bytesPerRow > 0,
				 "Bad bytesPerRow in dng_image_spooler");

	
	
	
	

	if (ifd.fSubTileBlockRows == 0)
		{
		ThrowBadFormat ();
		}

	uint32 stripLength = Pin_uint32 (ifd.fSubTileBlockRows,
									 fBlock.LogicalSize () / bytesPerRow,
									 fTileArea.H ());

	stripLength = stripLength / ifd.fSubTileBlockRows
							  * ifd.fSubTileBlockRows;
	
	fTileStrip	 = fTileArea;
	fTileStrip.b = fTileArea.t + stripLength;
	
	fBuffer = (uint8 *) fBlock.Buffer ();
	
	fBufferCount = 0;
	fBufferSize	 = SafeUint32Mult (bytesPerRow, stripLength);
				  
	}

dng_image_spooler::~dng_image_spooler ()
	{
	
	}

void dng_image_spooler::Spool (const void *data,
							   uint32 count)
	{
	
	while (count)
		{
		
		uint32 block = Min_uint32 (count, fBufferSize - fBufferCount);
		
		if (block == 0)
			{
			return;
			}
		
		DoCopyBytes (data,
					 fBuffer + fBufferCount,
					 block);
				
		data = ((const uint8 *) data) + block;
		
		count -= block;
		
		fBufferCount += block;
		
		if (fBufferCount == fBufferSize)
			{
			
			fHost.SniffForAbort ();
			
			dng_pixel_buffer buffer (fTileStrip, 
									 fPlane, 
									 fPlanes, 
									 ttShort,
									 pcInterleaved, 
									 fBuffer);
			
			if (fIFD.fSubTileBlockRows > 1)
				{
			
				ReorderSubTileBlocks (fHost,
									  fIFD,
									  buffer,
									  fSubTileBuffer);
									  
				}

			fImage.Put (buffer);
			
			uint32 stripLength = fTileStrip.H ();
			
			fTileStrip.t = fTileStrip.b;
			
			fTileStrip.b = Min_int32 (fTileStrip.t + stripLength,
									  fTileArea.b);
			
			fBufferCount = 0;
			
			fBufferSize = SafeUint32Mult (fTileStrip.W (),
										  fTileStrip.H (),
										  fPlanes,
										  (uint32) sizeof (uint16));
	
			}

		}
	
	}

dng_read_image::dng_read_image ()

	:	fJPEGTables ()
	
	{
	
	}

dng_read_image::~dng_read_image ()
	{
	
	}

static uint8 ReverseBits8 (uint8 x)
	{

	x = (uint8) (((x & 0x55) << 1) | ((x & 0xAA) >> 1));
	x = (uint8) (((x & 0x33) << 2) | ((x & 0xCC) >> 2));
	x = (uint8) (((x & 0x0F) << 4) | ((x & 0xF0) >> 4));

	return x;

	}

static void ApplyFillOrder (uint8 *data,
							uint32 count,
							uint32 fillOrder)
	{

	
	
	

	if (fillOrder == 2)
		{

		for (uint32 j = 0; j < count; j++)
			{
			data [j] = ReverseBits8 (data [j]);
			}

		}

	}

bool dng_read_image::ReadUncompressed (dng_host &host,
									   const dng_ifd &ifd,
									   dng_stream &stream,
									   dng_image &image,
									   const dng_rect &tileArea,
									   uint32 plane,
									   uint32 planes,
									   AutoPtr<dng_memory_block> &uncompressedBuffer,
									   AutoPtr<dng_memory_block> &subTileBlockBuffer)
	{
	
	uint32 rows			 = tileArea.H ();
	uint32 samplesPerRow = tileArea.W ();
	
	if (ifd.fPlanarConfiguration == pcRowInterleaved)
		{
		rows = SafeUint32Mult (rows, planes);
		}
	else
		{
		samplesPerRow = SafeUint32Mult (samplesPerRow, planes);
		}
	
	uint32 samplesPerTile = SafeUint32Mult (samplesPerRow, rows);
		
	if (uncompressedBuffer.Get () == NULL)
		{

		#if qDNGValidate
		
		ReportError ("Fuzz: Missing uncompressed buffer");
		
		#endif

		ThrowBadFormat ();
		
		}
	
	uint32 bitDepth = ifd.fBitsPerSample [plane];
	
	uint32 pixelType = ttUndefined;
	
	if (bitDepth == 8)
		{
		
		pixelType = ttByte;
				
		stream.Get (uncompressedBuffer->Buffer (), samplesPerTile);

		ApplyFillOrder (uncompressedBuffer->Buffer_uint8 (),
						samplesPerTile,
						ifd.fFillOrder);
		
		}
		
	else if (bitDepth == 16 && ifd.fSampleFormat [0] == sfFloatingPoint)
		{
		
		pixelType = ttFloat;
		
		uint32 *p_uint32 = (uint32 *) uncompressedBuffer->Buffer ();
			
		for (uint32 j = 0; j < samplesPerTile; j++)
			{
			
			p_uint32 [j] = DNG_HalfToFloat (stream.Get_uint16 ());
											
			}
		
		}
	
	else if (bitDepth == 24 && ifd.fSampleFormat [0] == sfFloatingPoint)
		{
		
		pixelType = ttFloat;
		
		uint32 *p_uint32 = (uint32 *) uncompressedBuffer->Buffer ();
			
		for (uint32 j = 0; j < samplesPerTile; j++)
			{
			
			uint8 input [3];
			
			if (stream.LittleEndian ())
				{
				input [2] = stream.Get_uint8 ();
				input [1] = stream.Get_uint8 ();
				input [0] = stream.Get_uint8 ();
				}
				
			else
				{
				input [0] = stream.Get_uint8 ();
				input [1] = stream.Get_uint8 ();
				input [2] = stream.Get_uint8 ();
				}
			
			p_uint32 [j] = DNG_FP24ToFloat (input);
											
			}
		
		}
	
	else if (bitDepth == 16)
		{
		
		pixelType = ttShort;

		const uint32 byteCount = SafeUint32Mult (samplesPerTile, 2);
		
		stream.Get (uncompressedBuffer->Buffer (),
					byteCount);

		ApplyFillOrder (uncompressedBuffer->Buffer_uint8 (),
						byteCount,
						ifd.fFillOrder);
		
		if (stream.SwapBytes ())
			{
			
			DoSwapBytes16 ((uint16 *) uncompressedBuffer->Buffer (),
						   samplesPerTile);
					
			}
				
		}
		
	else if (bitDepth == 32)
		{
		
		pixelType = image.PixelType ();

		const uint32 byteCount = SafeUint32Mult (samplesPerTile, 4);
		
		stream.Get (uncompressedBuffer->Buffer (),
					byteCount);

		ApplyFillOrder (uncompressedBuffer->Buffer_uint8 (),
						byteCount,
						ifd.fFillOrder);
		
		if (stream.SwapBytes ())
			{
			
			DoSwapBytes32 ((uint32 *) uncompressedBuffer->Buffer (),
						   samplesPerTile);
					
			}
				
		}
		
	else if (bitDepth == 12)
		{
		
		pixelType = ttShort;
		
		uint16 *p = (uint16 *) uncompressedBuffer->Buffer ();
			
		uint32 evenSamples = samplesPerRow >> 1;
		
		for (uint32 row = 0; row < rows; row++)
			{
			
			for (uint32 j = 0; j < evenSamples; j++)
				{
				
				uint32 b0 = stream.Get_uint8 ();
				uint32 b1 = stream.Get_uint8 ();
				uint32 b2 = stream.Get_uint8 ();
								
				p [0] = (uint16) ((b0 << 4) | (b1 >> 4));
				p [1] = (uint16) (((b1 << 8) | b2) & 0x0FFF);
				
				p += 2;
				
				}
				
			if (samplesPerRow & 1)
				{
				
				uint32 b0 = stream.Get_uint8 ();
				uint32 b1 = stream.Get_uint8 ();
				
				p [0] = (uint16) ((b0 << 4) | (b1 >> 4));
				
				p += 1;
				
				}
			
			}
		
		}
		
	else if (bitDepth > 8 && bitDepth < 16)
		{
		
		pixelType = ttShort;
		
		uint16 *p = (uint16 *) uncompressedBuffer->Buffer ();
			
		uint32 bitMask = (1 << bitDepth) - 1;
							
		for (uint32 row = 0; row < rows; row++)
			{
			
			uint32 bitBuffer  = 0;
			uint32 bufferBits = 0;
			
			for (uint32 j = 0; j < samplesPerRow; j++)
				{
				
				while (bufferBits < bitDepth)
					{
					
					bitBuffer = (bitBuffer << 8) | stream.Get_uint8 ();
					
					bufferBits += 8;
					
					}
									
				p [j] = (uint16) ((bitBuffer >> (bufferBits - bitDepth)) & bitMask);
				
				bufferBits -= bitDepth;
				
				}
				
			p += samplesPerRow;
			
			}
			
		}
		
	else if (bitDepth > 16 && bitDepth < 32)
		{
		
		pixelType = ttLong;
		
		uint32 *p = (uint32 *) uncompressedBuffer->Buffer ();
			
		uint32 bitMask = ((uint32) 1 << bitDepth) - 1;
							
		for (uint32 row = 0; row < rows; row++)
			{
			
			uint64 bitBuffer  = 0;
			uint32 bufferBits = 0;
			
			for (uint32 j = 0; j < samplesPerRow; j++)
				{
				
				while (bufferBits < bitDepth)
					{
					
					bitBuffer = (bitBuffer << 8) | stream.Get_uint8 ();
					
					bufferBits += 8;
					
					}
									
				p [j] = ((uint32) (bitBuffer >> (bufferBits - bitDepth))) & bitMask;
				
				bufferBits -= bitDepth;
				
				}
				
			p += samplesPerRow;
			
			}
			
		}
		
	else
		{
		
		return false;
		
		}
		
	dng_pixel_buffer buffer (tileArea, 
							 plane, 
							 planes, 
							 pixelType,
							 ifd.fPlanarConfiguration, 
							 uncompressedBuffer->Buffer ());

	DecodePredictor (host,
					 ifd,
					 buffer);
	
	if (ifd.fSampleBitShift)
		{
		
		buffer.ShiftRight (ifd.fSampleBitShift);
		
		}
		
	if (ifd.fSubTileBlockRows > 1)
		{
		
		ReorderSubTileBlocks (host,
							  ifd,
							  buffer,
							  subTileBlockBuffer);
		
		}
		
	image.Put (buffer);
	
	return true;
		
	}

#if qDNGUseLibJPEG

static void dng_error_exit (j_common_ptr cinfo)
	{
	
	
	
	(*cinfo->err->output_message) (cinfo);
	
	

	switch (cinfo->err->msg_code)
		{
		
		case JERR_OUT_OF_MEMORY:
			{
			ThrowMemoryFull ();
			break;
			}
			
		default:
			{
			ThrowBadFormat ();
			}
			
		}
			
	}

static void dng_output_message (j_common_ptr cinfo)
	{
	
	
	
	char buffer [JMSG_LENGTH_MAX];

	(*cinfo->err->format_message) (cinfo, buffer);
	
	
	
	ReportWarning ("libjpeg", buffer);

	}

#endif

void dng_read_image::DecodeLossyJPEG (dng_host &host,
									  dng_image &image,
									  const dng_rect &tileArea,
									  uint32 plane,
									  uint32 planes,
									  uint32 ,
									  uint32 jpegDataSize,
									  uint8 *jpegDataInMemory,
									  bool )
	{
	
	#if qDNGUseLibJPEG
	
	struct jpeg_decompress_struct cinfo;
	
	
	
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error (&jerr);
	
	jerr.error_exit		= dng_error_exit;
	jerr.output_message = dng_output_message;
	
	try
		{
		
		

		jpeg_create_decompress (&cinfo);
		
		
		
		size_t jpegDataSizeAsSizet = 0;
		
		ConvertUnsigned (jpegDataSize, &jpegDataSizeAsSizet);

		jpeg_source_mgr memorySource =
			CreateJpegMemorySource (jpegDataInMemory,
									jpegDataSizeAsSizet);

		cinfo.src = &memorySource;
			
		
			
		jpeg_read_header (&cinfo, TRUE);
		
		
		
			{

			

			if (cinfo.num_components < 0)
				{
				ThrowBadFormat ("invalid cinfo.num_components");
				}
			
			

			uint32 imageWidthAsUint32	 = 0;
			uint32 imageHeightAsUint32	 = 0;
			uint32 numComponentsAsUint32 = 0;

			ConvertUnsigned (cinfo.image_width,	 &imageWidthAsUint32);
			ConvertUnsigned (cinfo.image_height, &imageHeightAsUint32);

			
			

			ConvertUnsigned (static_cast<unsigned> (cinfo.num_components),
							 &numComponentsAsUint32);
			
			

			if (imageWidthAsUint32	  != tileArea.W () ||
				imageHeightAsUint32	  != tileArea.H () ||
				numComponentsAsUint32 != planes)
				{
				ThrowBadFormat ("JPEG dimensions do not match tile");
				}

			}
			
		
		
		jpeg_start_decompress (&cinfo);
		
		
		
		dng_pixel_buffer buffer (tileArea, 
								 plane, 
								 planes, 
								 ttByte, 
								 pcInterleaved,
								 NULL);

		buffer.fArea.b = tileArea.t + 1;
		
		buffer.fDirty = true;
		
		AutoPtr<dng_memory_block> bufferData (host.Allocate (buffer.fRowStep));
		
		buffer.fData = bufferData->Buffer ();
		
		uint8 *sampArray [1];
		
		sampArray [0] = bufferData->Buffer_uint8 ();
		
		
			
		while (buffer.fArea.t < tileArea.b)
			{
			
			jpeg_read_scanlines (&cinfo, sampArray, 1);
			
			image.Put (buffer);
			
			buffer.fArea.t = buffer.fArea.b;
			buffer.fArea.b = buffer.fArea.t + 1;
			
			}
			
		
			
		jpeg_finish_decompress (&cinfo);

		jpeg_destroy_decompress (&cinfo);
			
		}
		
	catch (...)
		{
		
		jpeg_destroy_decompress (&cinfo);
		
		throw;
		
		}
	
	#else
				
	
	
	
	(void) host;
	(void) image;
	(void) tileArea;
	(void) plane;
	(void) planes;
	(void) jpegDataSize;
	(void) jpegDataInMemory;
	
	ThrowProgramError ("Missing lossy JPEG decoder");
	
	#endif
	
	}
	

static dng_memory_block * ReadJPEGDataToBlock (dng_host &host,
											   dng_stream &stream,
											   dng_memory_block *tablesBlock,
											   uint64 tileOffset,
											   uint32 tileByteCount,
											   bool patchFirstByte)
	{
	
	if (tileByteCount <= 2)
		{
		ThrowEndOfFile ();
		}
		
	uint32 tablesByteCount = tablesBlock ? tablesBlock->LogicalSize () : 0;
	
	if (tablesByteCount && tablesByteCount < 4)
		{
		ThrowEndOfFile ();
		}
		
	
	
	
	
	
	

	if (tablesByteCount)
		{
		
		

		if (tileOffset > std::numeric_limits<uint64>::max () - 2)
			{
			ThrowEndOfFile ();
			}
		
		tablesByteCount -= 2;
		
		tileOffset	  += 2;
		tileByteCount -= 2;
		
		}
		
	
	
	AutoPtr<dng_memory_block> buffer 
		(host.Allocate (SafeUint32Add (tablesByteCount, tileByteCount)));
													  
	
	
	if (tablesByteCount)
		{
		
		DoCopyBytes (tablesBlock->Buffer (),
					 buffer->Buffer (),
					 tablesByteCount);
		
		}
		
	
	
	stream.SetReadPosition (tileOffset);
	
	stream.Get (buffer->Buffer_uint8 () + tablesByteCount, tileByteCount);
		
	
		
	if (patchFirstByte)
		{
		
		buffer->Buffer_uint8 () [0] = 0xFF;
		
		}
		
	
	
	return buffer.Release ();
	
	}

bool dng_read_image::ReadBaselineJPEG (dng_host &host,
									   const dng_ifd &ifd,
									   dng_stream &stream,
									   dng_image &image,
									   const dng_rect &tileArea,
									   uint32 plane,
									   uint32 planes,
									   uint32 tileByteCount,
									   uint8 *jpegDataInMemory,
									   bool usingMultipleThreads)
	{
	
	
	
	if (fJPEGTables.Get () || !jpegDataInMemory)
		{
		
		AutoPtr<dng_memory_block> jpegDataBlock;
	
		jpegDataBlock.Reset (ReadJPEGDataToBlock (host,
												  stream,
												  fJPEGTables.Get (),
												  stream.Position (),
												  tileByteCount,
												  ifd.fPatchFirstJPEGByte));
												  
		DecodeLossyJPEG (host,
						 image,
						 tileArea,
						 plane,
						 planes,
						 ifd.fPhotometricInterpretation,
						 jpegDataBlock->LogicalSize (),
						 jpegDataBlock->Buffer_uint8 (),
						 usingMultipleThreads);
		
		}
		
	else
		{
	
		if (ifd.fPatchFirstJPEGByte && tileByteCount)
			{
			jpegDataInMemory [0] = 0xFF;
			}

		DecodeLossyJPEG (host,
						 image,
						 tileArea,
						 plane,
						 planes,
						 ifd.fPhotometricInterpretation,
						 tileByteCount,
						 jpegDataInMemory,
						 usingMultipleThreads);
		
		}
				
	return true;
	
	}
	

bool dng_read_image::ReadLosslessJPEG (dng_host &host,
									   const dng_ifd &ifd,
									   dng_stream &stream,
									   dng_image &image,
									   const dng_rect &tileArea,
									   uint32 plane,
									   uint32 planes,
									   uint32 tileByteCount,
									   AutoPtr<dng_memory_block> &uncompressedBuffer,
									   AutoPtr<dng_memory_block> &subTileBlockBuffer)
	{
	
	

	if (tileArea.IsEmpty ())
		{
		return true;
		}

	dng_safe_uint32 bytesPerRow =
		(dng_safe_uint32 (tileArea.W ()) * planes *
		 static_cast<uint32> (sizeof (uint16)));

	

	if (ifd.fSubTileBlockRows == 0)
		{
		ThrowBadFormat ();
		}

	uint32 rowsPerStrip = Pin_uint32 (ifd.fSubTileBlockRows,
									  kImageBufferSize / bytesPerRow.Get (),
									  tileArea.H ());

	rowsPerStrip = rowsPerStrip / ifd.fSubTileBlockRows
								* ifd.fSubTileBlockRows;
									  
	dng_safe_uint32 bufferSize = bytesPerRow * rowsPerStrip;
	
	if (uncompressedBuffer.Get () &&
		uncompressedBuffer->LogicalSize () < bufferSize.Get ())
		{
		
		uncompressedBuffer.Reset ();
		
		}
		
	if (uncompressedBuffer.Get () == NULL)
		{
		
		uncompressedBuffer.Reset (host.Allocate (bufferSize.Get ()));
									
		}
	
	dng_image_spooler spooler (host,
							   ifd,
							   image,
							   tileArea,
							   plane,
							   planes,
							   *uncompressedBuffer.Get (),
							   subTileBlockBuffer);
			

				   
	dng_safe_uint32 decodedSize = (dng_safe_uint32 (tileArea.W ()) *
								   tileArea.H () *
								   planes * 
								   (uint32) sizeof (uint16));
							
	bool bug16 = ifd.fLosslessJPEGBug16;
	
	uint64 tileOffset = stream.Position ();

	DoDecodeLosslessJPEG (stream,
						  spooler,
						  decodedSize.Get (),
						  decodedSize.Get (),
						  bug16,
						  tileOffset + tileByteCount);

	return true;
	
	}

bool dng_read_image::ReadJXL (dng_host &host,
							  const dng_ifd &ifd,
							  dng_stream &stream,
							  dng_image &image,
							  const dng_rect &tileArea,
							  uint32 tileByteCount,
							  uint8 *jxlCompressedRawBitStream,
							  bool usingMultipleThreads)
	{

	(void) ifd;
	
	dng_jxl_decoder decoder;

	decoder.fNeedBoxMeta     = false;
	decoder.fNeedImage       = true;
	decoder.fUsePixelBuffer  = true;
	decoder.fUseSingleThread = usingMultipleThreads;
		
	if (!jxlCompressedRawBitStream)
		{
		
		decoder.Decode (host,
						stream);

		}

	else
		{
		
		dng_stream tempStream (jxlCompressedRawBitStream,
							   tileByteCount);

		decoder.Decode (host,
						tempStream);

		}

	

	DNG_REQUIRE (decoder.fMainPixelBuffer.Get (),
				 "missing fMainPixelBuffer in ReadJXL");

	auto &buffer = *decoder.fMainPixelBuffer;

	

	DNG_REQUIRE (buffer.fArea.Size () == tileArea.Size (),
				 "mismatch tile size in ReadJXL");

	

	DNG_REQUIRE (buffer.fPixelType == image.PixelType (),
				 "mismatch pixel type in ReadJXL");

	

	DNG_REQUIRE (buffer.fPlanes == image.Planes (),
				 "mismatch plane count in ReadJXL");

	

	buffer.fArea = tileArea;

	image.Put (buffer);

	return true;
	
	}

bool dng_read_image::CanReadTile (const dng_ifd &ifd)
	{
	
	if (ifd.fSampleFormat [0] != sfUnsignedInteger &&
		ifd.fSampleFormat [0] != sfFloatingPoint)
		{
		return false;
		}

	switch (ifd.fCompression)
		{
		
		case ccUncompressed:
			{
			
			if (ifd.fSampleFormat [0] == sfFloatingPoint)
				{

				if (ifd.fPredictor != cpNullPredictor ||
					ifd.fFillOrder != 1)
					{
					return false;
					}
				
				return (ifd.fBitsPerSample [0] == 16 ||
						ifd.fBitsPerSample [0] == 24 ||
						ifd.fBitsPerSample [0] == 32);
						
				}

			
			
			

			if (ifd.fPredictor != cpNullPredictor		   &&
				ifd.fPredictor != cpHorizontalDifference   &&
				ifd.fPredictor != cpHorizontalDifferenceX2 &&
				ifd.fPredictor != cpHorizontalDifferenceX4)
				{
				return false;
				}

			if (ifd.fPredictor != cpNullPredictor &&
				ifd.fBitsPerSample [0] != 8	 &&
				ifd.fBitsPerSample [0] != 16 &&
				ifd.fBitsPerSample [0] != 32)
				{
				return false;
				}

			if (ifd.fPredictor != cpNullPredictor &&
				ifd.fPlanarConfiguration == pcRowInterleaved)
				{
				return false;
				}

			if (ifd.fFillOrder != 1 &&
				ifd.fFillOrder != 2)
				{
				return false;
				}

			if (ifd.fFillOrder == 2 &&
				ifd.fBitsPerSample [0] != 8	 &&
				ifd.fBitsPerSample [0] != 16 &&
				ifd.fBitsPerSample [0] != 32)
				{
				return false;
				}
				
			return ifd.fBitsPerSample [0] >= 8 &&
				   ifd.fBitsPerSample [0] <= 32;
			
			}
			
		case ccJPEG:
			{
			
			if (ifd.fSampleFormat [0] != sfUnsignedInteger)
				{
				return false;
				}
				
			if (ifd.IsBaselineJPEG ())
				{
				
				
				
				return true;
				
				}
				
			else
				{
				
				
				
				return ifd.fBitsPerSample [0] >= 8 &&
					   ifd.fBitsPerSample [0] <= 16;
				
				}
				
			break;
			
			}

		case ccJXL:
			{

			if (ifd.fSamplesPerPixel != 1 &&
				ifd.fSamplesPerPixel != 3)
				{
				return false;
				}

			if (ifd.fSampleFormat [0] == sfUnsignedInteger)
				{
				
				return ifd.fBitsPerSample [0] >= 8 &&
					   ifd.fBitsPerSample [0] <= 16;
				
				}

			else
				{
				
				return ifd.fBitsPerSample [0] >= 16 &&
					   ifd.fBitsPerSample [0] <= 32;
				
				}
				
			break;
			
			}

		case ccLZW:
		case ccDeflate:
		case ccOldDeflate:
		case ccPackBits:
			{
			
			if (ifd.fSampleFormat [0] == sfFloatingPoint)
				{
				
				if (ifd.fCompression == ccPackBits)
					{
					return false;
					}
				
				if (ifd.fPredictor != cpNullPredictor	&&
					ifd.fPredictor != cpFloatingPoint	&&
					ifd.fPredictor != cpFloatingPointX2 &&
					ifd.fPredictor != cpFloatingPointX4)
					{
					return false;
					}
					
				if (ifd.fBitsPerSample [0] != 16 &&
					ifd.fBitsPerSample [0] != 24 &&
					ifd.fBitsPerSample [0] != 32)
					{
					return false;
					}
				
				}
				
			else
				{

				if (ifd.fPredictor != cpNullPredictor		   &&
					ifd.fPredictor != cpHorizontalDifference   &&
					ifd.fPredictor != cpHorizontalDifferenceX2 &&
					ifd.fPredictor != cpHorizontalDifferenceX4)
					{
					return false;
					}
					
				if (ifd.fBitsPerSample [0] != 8	 &&
					ifd.fBitsPerSample [0] != 16 &&
					ifd.fBitsPerSample [0] != 32)
					{
					return false;
					}
					
				}
					
			return true;
			
			}
			
		default:
			{
			break;
			}
			
		}
		
	return false;

	}
	

bool dng_read_image::NeedsCompressedBuffer (const dng_ifd &ifd)
	{
	
	if (ifd.fCompression == ccLZW		 ||
		ifd.fCompression == ccDeflate	 ||
		ifd.fCompression == ccOldDeflate ||
		ifd.fCompression == ccPackBits)
		{
		return true;
		}
		
	return false;
	
	}
	

void dng_read_image::ByteSwapBuffer (dng_host & ,
									 dng_pixel_buffer &buffer)
	{
	
	DNG_REQUIRE (buffer.fArea.H () == 0 || buffer.fRowStep > 0,
				 "buffer.fRowStep");

	uint32 pixels = SafeUint32Mult ((uint32) buffer.fRowStep, buffer.fArea.H ());
	
	switch (buffer.fPixelSize)
		{
		
		case 2:
			{
			
			DoSwapBytes16 ((uint16 *) buffer.fData,
						   pixels);
						   
			break;
			
			}
			
		case 4:
			{
			
			DoSwapBytes32 ((uint32 *) buffer.fData,
						   pixels);
						   
			break;
			
			}
			
		default:
			break;
			
		}

	}
							

void dng_read_image::DecodePredictor (dng_host & ,
									  const dng_ifd &ifd,
									  dng_pixel_buffer &buffer)
	{
	
	switch (ifd.fPredictor)
		{
		
		case cpNullPredictor:
			{
			
			return;
			
			}
		
		case cpHorizontalDifference:
		case cpHorizontalDifferenceX2:
		case cpHorizontalDifferenceX4:
			{
			
			int32 xFactor = 1;
			
			if (ifd.fPredictor == cpHorizontalDifferenceX2)
				{
				xFactor = 2;
				}
				
			else if (ifd.fPredictor == cpHorizontalDifferenceX4)
				{
				xFactor = 4;
				}
			
			switch (buffer.fPixelType)
				{
				
				case ttByte:
					{
					
					DecodeDelta8 ((uint8 *) buffer.fData,
								  buffer.fArea.H (),
								  buffer.fArea.W () / xFactor,
								  buffer.fPlanes	* xFactor);
					
					return;
					
					}
					
				case ttShort:
					{
					
					DecodeDelta16 ((uint16 *) buffer.fData,
								   buffer.fArea.H (),
								   buffer.fArea.W () / xFactor,
								   buffer.fPlanes	 * xFactor);
					
					return;
					
					}
					
				case ttLong:
					{
					
					DecodeDelta32 ((uint32 *) buffer.fData,
								   buffer.fArea.H (),
								   buffer.fArea.W () / xFactor,
								   buffer.fPlanes	 * xFactor);
					
					return;
					
					}
					
				default:
					break;
					
				}
			
			break;
			
			}
			
		default:
			break;
		
		}
		
	ThrowBadFormat ();
	
	}
							

void dng_read_image::ReadTile (dng_host &host,
							   const dng_ifd &ifd,
							   dng_stream &stream,
							   dng_image &image,
							   const dng_rect &tileArea,
							   uint32 plane,
							   uint32 planes,
							   uint32 tileByteCount,
							   std::shared_ptr<dng_memory_block> &compressedBuffer,
							   AutoPtr<dng_memory_block> &uncompressedBuffer,
							   AutoPtr<dng_memory_block> &subTileBlockBuffer,
							   bool usingMultipleThreads)
	{
	
	switch (ifd.fCompression)
		{
		
		case ccLZW:
		case ccDeflate:
		case ccOldDeflate:
		case ccPackBits:
			{
			
			

			dng_safe_uint32 bytesPerSample = (ifd.fBitsPerSample [0] >> 3);

			const bool floatingPointPredictor =
				ifd.fPredictor == cpFloatingPoint	||
				ifd.fPredictor == cpFloatingPointX2 ||
				ifd.fPredictor == cpFloatingPointX4;

			
			

			if (floatingPointPredictor &&
				ifd.fSampleFormat [0] != sfFloatingPoint)
				{
				ThrowBadFormat ();
				}

			dng_safe_uint32 sampleCount = (dng_safe_uint32 (planes)		   * 
										   dng_safe_uint32 (tileArea.W ()) * 
										   dng_safe_uint32 (tileArea.H ()));

			dng_safe_uint32 uncompressedSize = sampleCount * bytesPerSample;

			bool decodedFullTile = false;

			
			
			uint32 pixelType = ttUndefined;
			
			if (ifd.fSampleFormat [0] == sfFloatingPoint)
				{
				pixelType = ttFloat;
				}
			
			else if (ifd.fBitsPerSample [0] == 8)
				{
				pixelType = ttByte;
				}
				
			else if (ifd.fBitsPerSample [0] == 16)
				{
				pixelType = ttShort;
				}
				
			else if (ifd.fBitsPerSample [0] == 32)
				{
				pixelType = ttLong;
				}
				
			else
				{
				ThrowBadFormat ();
				}

			dng_pixel_buffer buffer (tileArea, 
									 plane, 
									 planes, 
									 pixelType, 
									 pcInterleaved,
									 NULL);

			
			

			buffer.fPixelSize = bytesPerSample.Get ();

			dng_safe_uint32 bufferSize = uncompressedSize;
			
			
			
			
			if (floatingPointPredictor)
				{

				bufferSize += (dng_safe_uint32 (dng_safe_int32 (buffer.fRowStep)) * 
							   dng_safe_uint32 (buffer.fPixelSize));

				}
				
			
			
			
			if (buffer.fPixelType == ttFloat)
				{
				bufferSize = Max_uint32 (bufferSize.Get (),
										 (sampleCount * 4u).Get ());
				}
			
			
			
			
			
			if (uncompressedBuffer.Get () &&
				uncompressedBuffer->LogicalSize () < bufferSize.Get ())
				{
				
				uncompressedBuffer.Reset ();
				
				}
				
			if (uncompressedBuffer.Get () == NULL)
				{

				uncompressedBuffer.Reset (host.Allocate (bufferSize.Get ()));
											
				}
				
			buffer.fData = uncompressedBuffer->Buffer ();
			
			
			
			if (floatingPointPredictor)
				{
				
				buffer.fData = (uint8 *) buffer.fData +
							   buffer.fRowStep * buffer.fPixelSize;
							   
				}
			
			
			
			if (ifd.fCompression == ccLZW)
				{
				
				dng_lzw_expander expander;
				
				if (!expander.Expand (compressedBuffer->Buffer_uint8 (),
									  (uint8 *) buffer.fData,
									  tileByteCount,
									  uncompressedSize.Get ()))
					{
					ThrowBadFormat ();
					}
				
				}
				
			else if (ifd.fCompression == ccPackBits)
				{
				
				dng_stream subStream (compressedBuffer->Buffer_uint8 (),
									  tileByteCount);
											 
				if (!DecodePackBits (subStream,
									 (uint8 *) buffer.fData,
									 uncompressedSize.Get ()))
					{
					ThrowBadFormat ();
					}
				
				}
			
			else
				{
				
				uLongf dstLen = uncompressedSize.Get ();
							
				int err = uncompress ((Bytef *) buffer.fData,
									  &dstLen,
									  (const Bytef *) compressedBuffer->Buffer (),
									  tileByteCount);

				
				
				
				

				if (err == Z_BUF_ERROR && !floatingPointPredictor)
					{

					dng_rect fullTileArea (tileArea);

					fullTileArea.r = fullTileArea.l + ifd.fTileWidth;
					fullTileArea.b = fullTileArea.t + ifd.fTileLength;

					if (fullTileArea != tileArea)
						{

						dng_safe_uint32 fullSampleCount =
							(dng_safe_uint32 (planes) *
							 dng_safe_uint32 (fullTileArea.W ()) *
							 dng_safe_uint32 (fullTileArea.H ()));

						dng_safe_uint32 fullUncompressedSize =
							fullSampleCount * bytesPerSample;

						dng_pixel_buffer fullBuffer (fullTileArea,
													 plane,
													 planes,
													 pixelType,
													 pcInterleaved,
													 NULL);

						fullBuffer.fPixelSize = bytesPerSample.Get ();

						dng_safe_uint32 fullBufferSize =
							fullUncompressedSize;

						if (fullBuffer.fPixelType == ttFloat)
							{
							fullBufferSize =
								Max_uint32 (fullBufferSize.Get (),
											(fullSampleCount * 4u).Get ());
							}

						if (uncompressedBuffer.Get () &&
							uncompressedBuffer->LogicalSize () <
								fullBufferSize.Get ())
							{
							uncompressedBuffer.Reset ();
							}

						if (uncompressedBuffer.Get () == NULL)
							{
							uncompressedBuffer.Reset (
								host.Allocate (fullBufferSize.Get ()));
							}

						fullBuffer.fData = uncompressedBuffer->Buffer ();

						uLongf fullDstLen = fullUncompressedSize.Get ();

						err = uncompress ((Bytef *) fullBuffer.fData,
										  &fullDstLen,
										  (const Bytef *)
											compressedBuffer->Buffer (),
										  tileByteCount);

						if (err == Z_OK &&
							fullDstLen == fullUncompressedSize.Get ())
							{
							buffer = fullBuffer;
							sampleCount = fullSampleCount;
							uncompressedSize = fullUncompressedSize;
							dstLen = fullDstLen;
							decodedFullTile = true;
							}

						}

					}

				if (err != Z_OK)
					{
					
					if (err == Z_MEM_ERROR)
						{
						ThrowMemoryFull ();
						}
						
					else if (err == Z_DATA_ERROR)
						{
						
						
						
						
						}
						
					else
						{
						ThrowBadFormat ();
						}
						
					}
					
				if (dstLen != uncompressedSize.Get ())
					{
					ThrowBadFormat ();
					}
				
				}
				
			
			
			if (floatingPointPredictor)
				{
				
				int32 xFactor = 1;
				
				if (ifd.fPredictor == cpFloatingPointX2)
					{
					xFactor = 2;
					}
					
				else if (ifd.fPredictor == cpFloatingPointX4)
					{
					xFactor = 4;
					}
				
				for (int32 row = tileArea.t; row < tileArea.b; row++)
					{
					
					uint8 *srcPtr = (uint8 *) buffer.DirtyPixel (row	, tileArea.l, plane);
					uint8 *dstPtr = (uint8 *) buffer.DirtyPixel (row - 1, tileArea.l, plane);
					
					DecodeFPDelta (srcPtr,
								   dstPtr,
								   tileArea.W () / xFactor,
								   planes		 * xFactor,
								   bytesPerSample.Get ());
					
					}
				
				buffer.fData = (uint8 *) buffer.fData -
							   buffer.fRowStep * buffer.fPixelSize;
							   
				}
				
			else
				{
				
				
				
				if (stream.SwapBytes ())
					{
					
					ByteSwapBuffer (host,
									buffer);
									
					}
					
				
				
				DecodePredictor (host,
								 ifd,
								 buffer);
								 
				}
				
			
			
			if (buffer.fPixelType == ttFloat && buffer.fPixelSize == 2)
				{
				
				uint16 *srcPtr = (uint16 *) buffer.fData;
				uint32 *dstPtr = (uint32 *) buffer.fData;
				
				for (int32 index = sampleCount.Get () - 1; index >= 0; index--)
					{
					
					dstPtr [index] = DNG_HalfToFloat (srcPtr [index]);
					
					}
					
				buffer.fPixelSize = 4;
				
				}
				
			else if (buffer.fPixelType == ttFloat && buffer.fPixelSize == 3)
				{
				
				uint8  *srcPtr = ((uint8  *) buffer.fData) + (sampleCount.Get () - 1) * 3;
				uint32 *dstPtr = ((uint32 *) buffer.fData) + (sampleCount.Get () - 1);
				
				if (stream.BigEndian () || ifd.fPredictor == cpFloatingPoint   ||
										   ifd.fPredictor == cpFloatingPointX2 ||
										   ifd.fPredictor == cpFloatingPointX4)
					{
				
					for (uint32 index = 0; index < sampleCount.Get (); index++)
						{
						
						*(dstPtr--) = DNG_FP24ToFloat (srcPtr);
						
						srcPtr -= 3;
						
						}
						
					}
					
				else
					{
					
					for (uint32 index = 0; index < sampleCount.Get (); index++)
						{
						
						uint8 input [3];
						
						input [2] = srcPtr [0];
						input [1] = srcPtr [1];
						input [0] = srcPtr [2];
						
						*(dstPtr--) = DNG_FP24ToFloat (input);
						
						srcPtr -= 3;
						
						}
						
					}
					
				buffer.fPixelSize = 4;
				
				}
			
			

			if (decodedFullTile)
				{
				buffer.fArea = tileArea;
				}
			
			image.Put (buffer);
			
			return;
			
			}
	
		case ccUncompressed:
			{
			
			if (ReadUncompressed (host,
								  ifd,
								  stream,
								  image,
								  tileArea,
								  plane,
								  planes,
								  uncompressedBuffer,
								  subTileBlockBuffer))
				{
				
				return;
				
				}
				
			break;
			
			}
			
		case ccJPEG:
			{
			
			if (ifd.IsBaselineJPEG ())
				{
				
				
				
				if (ReadBaselineJPEG (host,
									  ifd,
									  stream,
									  image,
									  tileArea,
									  plane,
									  planes,
									  tileByteCount,
									  compressedBuffer ? compressedBuffer->Buffer_uint8 () : NULL,
									  usingMultipleThreads))
					{
					
					return;
					
					}
				
				}
				
			else
				{
				
				
				
				if (ReadLosslessJPEG (host,
									  ifd,
									  stream,
									  image,
									  tileArea,
									  plane,
									  planes,
									  tileByteCount,
									  uncompressedBuffer,
									  subTileBlockBuffer))
					{
					
					return;
					
					}
				
				}
			
			break;
			
			}
			
		case ccLossyJPEG:
			{
			
			if (ReadBaselineJPEG (host,
								  ifd,
								  stream,
								  image,
								  tileArea,
								  plane,
								  planes,
								  tileByteCount,
								  compressedBuffer ? compressedBuffer->Buffer_uint8 () : NULL,
								  usingMultipleThreads))
				{
				
				return;
				
				}
							
			break;
			
			}

		case ccJXL:
			{

			DNG_REQUIRE (plane == 0,
						 "Unexpected plane in ReadTile for ccJXL");

			DNG_REQUIRE (planes == 1 ||
						 planes == 3,
						 "Unexpected planes in ReadTile for ccJXL");
			
			if (ReadJXL (host,
						 ifd,
						 stream,
						 image,
						 tileArea,
						 tileByteCount,
						 compressedBuffer ? compressedBuffer->Buffer_uint8 () : NULL,
						 usingMultipleThreads))
				{
				
				return;
				
				}
							
			break;
			
			}

		default:
			break;
			
		}
		
	ThrowBadFormat ();
		
	}

bool dng_read_image::CanRead (const dng_ifd &ifd)
	{
	
	if (ifd.fImageWidth	 < 1 ||
		ifd.fImageLength < 1)
		{
		return false;
		}
		
	if (ifd.fSamplesPerPixel < 1)
		{
		return false;
		}
		
	if (ifd.fBitsPerSample [0] < 1)
		{
		return false;
		}
	
	for (uint32 j = 1; j < Min_uint32 (ifd.fSamplesPerPixel,
									   kMaxSamplesPerPixel); j++)
		{
		
		if (ifd.fBitsPerSample [j] !=
			ifd.fBitsPerSample [0])
			{
			return false;
			}
			
		if (ifd.fSampleFormat [j] !=
			ifd.fSampleFormat [0])
			{
			return false;
			}

		}
		
	if ((ifd.fPlanarConfiguration != pcInterleaved	 ) &&
		(ifd.fPlanarConfiguration != pcPlanar		 ) &&
		(ifd.fPlanarConfiguration != pcRowInterleaved))
		{
		return false;
		}
		
	if (ifd.fUsesStrips == ifd.fUsesTiles)
		{
		return false;
		}
		
	uint32 tileCount = ifd.TilesPerImage ();
	
	if (tileCount < 1)
		{
		return false;
		}
		
	bool needTileByteCounts = (ifd.TileByteCount (ifd.TileArea (0, 0)) == 0);
		
	if (tileCount == 1)
		{
		
		if (needTileByteCounts)
			{
			
			if (ifd.fTileByteCount [0] < 1)
				{
				return false;
				}
			
			}
		
		}
		
	else
		{
		
		if (ifd.fTileOffsetsCount != tileCount)
			{
			return false;
			}
			
		if (needTileByteCounts)
			{
			
			if (ifd.fTileByteCountsCount != tileCount)
				{
				return false;
				}
			
			}
		
		}
		
	if (!CanReadTile (ifd))
		{
		return false;
		}
		
	return true;
	
	}
	

dng_read_tiles_task::dng_read_tiles_task (dng_read_image &readImage,
										  dng_host &host,
										  const dng_ifd &ifd,
										  dng_stream &stream,
										  dng_image &image,
										  dng_lossy_compressed_image *lossyImage,
										  dng_fingerprint *lossyTileDigest,
										  uint32 outerSamples,
										  uint32 innerSamples,
										  uint32 tilesDown,
										  uint32 tilesAcross,
										  uint64 *tileOffset,
										  uint32 *tileByteCount,
										  uint32 compressedSize,
										  uint32 uncompressedSize)

	:	dng_area_task ("dng_read_tiles_task")

	,	fReadImage		  (readImage)
	,	fHost			  (host)
	,	fIFD			  (ifd)
	,	fStream			  (stream)
	,	fImage			  (image)
	,	fLossyImage		  (lossyImage)
	,	fLossyTileDigest  (lossyTileDigest)
	,	fOuterSamples	  (outerSamples)
	,	fInnerSamples	  (innerSamples)
	,	fTilesDown		  (tilesDown)
	,	fTilesAcross	  (tilesAcross)
	,	fTileOffset		  (tileOffset)
	,	fTileByteCount	  (tileByteCount)
	,	fCompressedSize	  (compressedSize)
	,	fUncompressedSize (uncompressedSize)
	,	fMutex			  ("dng_read_tiles_task")
	,	fNextTileIndex	  (0)

	{

	fMinTaskArea = 16 * 16;
	fUnitCell	 = dng_point (16, 16);
	fMaxTileSize = dng_point (16, 16);

	}
	

void dng_read_tiles_task::Process (uint32 ,
								   const dng_rect & ,
								   dng_abort_sniffer *sniffer)
	{
			
	std::shared_ptr<dng_memory_block> compressedBuffer;
	
	AutoPtr<dng_memory_block> uncompressedBuffer;
	AutoPtr<dng_memory_block> subTileBlockBuffer;

	if (!fLossyImage)
		{
		compressedBuffer.reset (fHost.Allocate (fCompressedSize));
		}

	if (fUncompressedSize)
		{
		uncompressedBuffer.Reset (fHost.Allocate (fUncompressedSize));
		}

	while (true)
		{

		uint32 tileIndex;
		uint32 byteCount;

			{

			dng_lock_mutex lock (&fMutex);

			if (fNextTileIndex == fOuterSamples * fTilesDown * fTilesAcross)
				{
				return;
				}

			tileIndex = fNextTileIndex++;

			ReadTask (tileIndex,
					  byteCount,
					  compressedBuffer.get ());

			}

		ProcessTask (tileIndex,
					 byteCount,
					 sniffer,
					 compressedBuffer,
					 uncompressedBuffer,
					 subTileBlockBuffer);

		}

	}

void dng_read_tiles_task::ReadTask (uint32 tileIndex,
									uint32 &byteCount,
									dng_memory_block *compressedBuffer)
	{
	
	TempStreamSniffer noSniffer (fStream, NULL);

	fStream.SetReadPosition (fTileOffset [tileIndex]);

	byteCount = fTileByteCount [tileIndex];

	if (fLossyImage)
		{

		fLossyImage->fData [tileIndex] . reset (fHost.Allocate (byteCount));

		}

	fStream.Get (fLossyImage ? fLossyImage->fData [tileIndex]->Buffer ()
							 : compressedBuffer->Buffer (),
				 byteCount);	

	}

void dng_read_tiles_task::ProcessTask (uint32 tileIndex,
									   uint32 byteCount,
									   dng_abort_sniffer *sniffer,
									   std::shared_ptr<dng_memory_block> &compressedBuffer,
									   AutoPtr<dng_memory_block> &uncompressedBuffer,
									   AutoPtr<dng_memory_block> &subTileBlockBuffer)
	{
	
	dng_abort_sniffer::SniffForAbort (sniffer);

	if (fLossyTileDigest)
		{

		dng_md5_direct_printer printer;

		printer.ProcessPtr (compressedBuffer->Buffer (),
							byteCount);

		fLossyTileDigest [tileIndex] = printer.Result ();

		}

	dng_stream tileStream (fLossyImage ? fLossyImage->fData [tileIndex]->Buffer ()
									   : compressedBuffer->Buffer (),
						   byteCount);

	tileStream.SetLittleEndian (fStream.LittleEndian ());

	uint32 plane = tileIndex / (fTilesDown * fTilesAcross);

	uint32 rowIndex = (tileIndex - plane * fTilesDown * fTilesAcross) / fTilesAcross;

	uint32 colIndex = tileIndex - (plane * fTilesDown + rowIndex) * fTilesAcross;

	dng_rect tileArea = fIFD.TileArea (rowIndex, colIndex);

	dng_host host (&fHost.Allocator (),
				   sniffer);				

	fReadImage.ReadTile (host,
						 fIFD,
						 tileStream,
						 fImage,
						 tileArea,
						 plane,
						 fInnerSamples,
						 byteCount,
						 fLossyImage ? fLossyImage->fData [tileIndex]
									 : compressedBuffer,
						 uncompressedBuffer,
						 subTileBlockBuffer,
						 true);

	}

class dng_interleave_task : public dng_area_task
						  , private dng_uncopyable
	{
	
	public:
	
		const dng_image &fSrcImage;
			  dng_image &fDstImage;
			  
		int32 fRowFactor = 1;
		int32 fColFactor = 1;
		
		bool fEncode = false;
		
	private:
	
		enum
			{
			kMaxThreads = 4
			};
		
		AutoPtr<dng_memory_block> fSrcBuffer [kMaxThreads];
		AutoPtr<dng_memory_block> fDstBuffer [kMaxThreads];
		
	public:
	
		dng_interleave_task (const dng_image &srcImage,
							 dng_image &dstImage,
							 const int32 rowFactor,
							 const int32 colFactor,
							 bool encode)
								  
			:	dng_area_task ("dng_interleave_task")
								  
			,	fSrcImage  (srcImage)
			,	fDstImage  (dstImage)
			,	fRowFactor (rowFactor)
			,	fColFactor (colFactor)
			,	fEncode    (encode)
			
			{
			
			if (fRowFactor >= (int32) fSrcImage.Bounds ().H ())
				{
				fRowFactor = 1;
				}
			
			if (fColFactor >= (int32) fSrcImage.Bounds ().W ())
				{
				fColFactor = 1;
				}
			
			fMaxThreads = kMaxThreads;
			
			fMaxTileSize = dng_point (512, 512);
			
			}
	
		dng_rect RepeatingTile1 () const override
			{
			return fDstImage.RepeatingTile ();
			}
			
		void Start (uint32 threadCount,
					const dng_rect &dstArea,
					const dng_point &tileSize,
					dng_memory_allocator *allocator,
					dng_abort_sniffer *sniffer) override;

		void Process (uint32 threadIndex,
					  const dng_rect &tile,
					  dng_abort_sniffer *sniffer) override;
		
	};

void dng_interleave_task::Start (uint32 threadCount,
								 const dng_rect & ,
								 const dng_point &tileSize,
								 dng_memory_allocator *allocator,
								 dng_abort_sniffer * )
	{
	
	uint32 srcBufferSize =
		SafeUint32Mult ((tileSize.h + fColFactor - 1) / fColFactor,
						(tileSize.v + fRowFactor - 1) / fRowFactor,
						fDstImage.PixelSize (),
						fDstImage.Planes ());

	uint32 dstBufferSize =
		SafeUint32Mult ((uint32) tileSize.h,
						(uint32) tileSize.v,
						fDstImage.PixelSize (),
						fDstImage.Planes ());
	
	for (uint32 threadIndex = 0; threadIndex < threadCount; threadIndex++)
		{
		
		fSrcBuffer [threadIndex].Reset (allocator->Allocate (srcBufferSize));
		
		fDstBuffer [threadIndex].Reset (allocator->Allocate (dstBufferSize));
		
		}
	
	}

void dng_interleave_task::Process (uint32 threadIndex,
								   const dng_rect &tile,
								   dng_abort_sniffer * )
	{
	
	dng_pixel_buffer dstBuffer;
	
	dstBuffer.fArea      = tile;
	dstBuffer.fPlane     = 0;
	dstBuffer.fPlanes    = fDstImage.Planes ();
	dstBuffer.fPlaneStep = 1;
	dstBuffer.fColStep   = dstBuffer.fPlaneStep * dstBuffer.fPlanes;
	dstBuffer.fRowStep   = SafeInt32Mult (dstBuffer.fColStep,
										  (int32) dstBuffer.fArea.W ());
	dstBuffer.fPixelType = fDstImage.PixelType ();
	dstBuffer.fPixelSize = fDstImage.PixelSize ();
	dstBuffer.fData      = fDstBuffer [threadIndex]->Buffer ();
	dstBuffer.fDirty     = true;
	
	dng_pixel_buffer srcBuffer = dstBuffer;
	
	srcBuffer.fData = fSrcBuffer [threadIndex]->Buffer ();
	
	if (fEncode)
		{
		fSrcImage.Get (dstBuffer);
		}
		
	for (int32 rOffset = 0; rOffset < Min_int32 (fRowFactor, tile.H ()); rOffset++)
		{
		
		for (int32 cOffset = 0; cOffset < Min_int32 (fColFactor, tile.W ()); cOffset++)
			{
			
			int32 rField = (tile.t + rOffset) % fRowFactor;
			int32 cField = (tile.l + cOffset) % fColFactor;
			
			int32 rFieldOffset = rField * (fDstImage.Height () / fRowFactor) +
								 Min_int32 (rField, fDstImage.Height () % fRowFactor);
			
			int32 cFieldOffset = cField * (fDstImage.Width  () / fColFactor) +
								 Min_int32 (cField, fDstImage.Width  () % fColFactor);
								 
			srcBuffer.fArea.t = rFieldOffset + (tile.t + rOffset) / fRowFactor;
			srcBuffer.fArea.l = cFieldOffset + (tile.l + cOffset) / fColFactor;
			
			srcBuffer.fArea.b = srcBuffer.fArea.t + (tile.H () - rOffset + fRowFactor - 1) / fRowFactor;
			srcBuffer.fArea.r = srcBuffer.fArea.l + (tile.W () - cOffset + fColFactor - 1) / fColFactor;
			
			srcBuffer.fRowStep = SafeInt32Mult (srcBuffer.fColStep,
												(int32) srcBuffer.fArea.W ());
			
			if (!fEncode)
				{
				fSrcImage.Get (srcBuffer);
				}
				
			dng_pixel_buffer tmpBuffer = dstBuffer;
			
			tmpBuffer.fArea = srcBuffer.fArea;

			tmpBuffer.fData = dstBuffer.DirtyPixel (tile.t + rOffset,
													tile.l + cOffset);
													
			tmpBuffer.fRowStep = SafeInt32Mult (tmpBuffer.fRowStep, fRowFactor);
			tmpBuffer.fColStep = SafeInt32Mult (tmpBuffer.fColStep, fColFactor);
			
			if (fEncode)
				{
				srcBuffer.CopyArea (tmpBuffer,
									tmpBuffer.fArea,
									tmpBuffer.fPlane,
									tmpBuffer.fPlanes);
				}
			else
				{
				tmpBuffer.CopyArea (srcBuffer,
									srcBuffer.fArea,
									srcBuffer.fPlane,
									srcBuffer.fPlanes);
				}
			
			if (fEncode)
				{
				fDstImage.Put (srcBuffer);
				}
			
			}
		
		}
		
	if (!fEncode)
		{
		fDstImage.Put (dstBuffer);
		}
	
	}
	

void Interleave2D (dng_host &host,
				   const dng_image &srcImage,
				   dng_image &dstImage,
				   const int32 rowFactor,
				   const int32 colFactor,
				   bool encode)
	{
	
	#if qDNGValidate
	dng_timer timer ("Interleave2D");
	#endif
	
	DNG_REQUIRE (srcImage.Bounds    () == dstImage.Bounds    () &&
				 srcImage.Planes    () == dstImage.Planes    () &&
				 srcImage.PixelType () == dstImage.PixelType (),
				 "Mismatched src and dst in Interleave2D");
				 
	dng_interleave_task task (srcImage,
							  dstImage,
							  rowFactor,
							  colFactor,
							  encode);
								   
	host.PerformAreaTask (task, dstImage.Bounds ());
	
	}

void dng_read_image::Read (dng_host &host,
						   const dng_ifd &ifd,
						   dng_stream &stream,
						   dng_image &image,
						   dng_lossy_compressed_image *lossyImage,
						   dng_fingerprint *lossyDigest)
	{
	
	

	
	

	static const uint32 kLimit = 2 * kMaxImageSide;

	if (ifd.fImageWidth	 > kLimit ||
		ifd.fImageLength > kLimit)
		{
		
		ThrowBadFormat ("dng_read_image::Read image too large");
		
		}

	uint32 tileIndex;

	

	if (ifd.fRowInterleaveFactor	> 1	||
		ifd.fColumnInterleaveFactor > 1)
		{

		
		
		AutoPtr<dng_image> tempImage (host.Make_dng_image (image.Bounds (),
														   image.Planes (),
														   image.PixelType ()));
		
		dng_ifd tempIFD (ifd);
		
		tempIFD.fRowInterleaveFactor    = 1;
		tempIFD.fColumnInterleaveFactor = 1;

		Read (host,
			  tempIFD,
			  stream,
			  *tempImage,
			  lossyImage,
			  lossyDigest);

		
		
		Interleave2D (host,
					  *tempImage,
					  image,
					  ifd.fRowInterleaveFactor,
					  ifd.fColumnInterleaveFactor,
					  false);
						
		return;
		
		}

	
	
	uint32 innerSamples = 1;
	uint32 outerSamples = 1;
	
	if (ifd.fPlanarConfiguration == pcPlanar)
		{
		outerSamples = ifd.fSamplesPerPixel;
		}
	else
		{
		innerSamples = ifd.fSamplesPerPixel;
		}
	
	
	
	uint32 tilesAcross = ifd.TilesAcross ();
	uint32 tilesDown   = ifd.TilesDown	 ();
	
	uint32 tileCount = SafeUint32Mult (tilesAcross, tilesDown, outerSamples);
	
	
		
	dng_memory_data tileOffsetData (tileCount, sizeof (uint64));
	
	uint64 *tileOffset = tileOffsetData.Buffer_uint64 ();
	
	if (tileCount <= dng_ifd::kMaxTileInfo)
		{
		
		for (tileIndex = 0; tileIndex < tileCount; tileIndex++)
			{
			
			tileOffset [tileIndex] = ifd.fTileOffset [tileIndex];
			
			}
		
		}
		
	else
		{
		
		stream.SetReadPosition (ifd.fTileOffsetsOffset);
		
		for (tileIndex = 0; tileIndex < tileCount; tileIndex++)
			{
			
			tileOffset [tileIndex] = stream.TagValue_uint64 (ifd.fTileOffsetsType);
			
			}
		
		}
		
	
	
	for (tileIndex = 0; tileIndex < tileCount; tileIndex++)
		{
		
		#if qDNGValidate
		
		if (tileOffset [tileIndex] < 8)
			{
			
			ReportWarning ("Tile/Strip offset less than 8");
			
			}
		
		#endif
		
		if (tileOffset [tileIndex] >= stream.Length ())
			{
			
			ThrowBadFormat ();
			
			}
		
		}
		
	
	
	dng_memory_data tileByteCountData;
	
	uint32 *tileByteCount = NULL;
	
	
	
		
	uint32 uncompressedSize = 0;

	uint32 subTileLength = ifd.fTileLength;
	
	if (ifd.TileByteCount (ifd.TileArea (0, 0)) != 0)
		{
		
		uint32 bytesPerPixel = TagTypeSize (ifd.PixelType ());
		
		uint32 bytesPerRow = SafeUint32Mult (ifd.fTileWidth,
											 innerSamples,
											 bytesPerPixel);

		
		

		if (ifd.fSubTileBlockRows == 0)
			{
			ThrowBadFormat ();
			}

		subTileLength = Pin_uint32 (ifd.fSubTileBlockRows,
									kImageBufferSize / bytesPerRow,
									ifd.fTileLength);

		subTileLength = subTileLength / ifd.fSubTileBlockRows
									  * ifd.fSubTileBlockRows;
									  
		uncompressedSize = SafeUint32Mult (subTileLength, bytesPerRow);
									
		}
		
	
	
	else
		{
		
		tileByteCountData.Allocate (tileCount, sizeof (uint32));
		
		tileByteCount = tileByteCountData.Buffer_uint32 ();
		
		if (tileCount <= dng_ifd::kMaxTileInfo)
			{
			
			for (tileIndex = 0; tileIndex < tileCount; tileIndex++)
				{
				
				if (ifd.fTileByteCount [tileIndex] > 0xFFFFFFFF)
					{
					ThrowBadFormat ();
					}
				
				tileByteCount [tileIndex] = (uint32) ifd.fTileByteCount [tileIndex];
				
				}
			
			}
			
		else
			{
			
			stream.SetReadPosition (ifd.fTileByteCountsOffset);
			
			for (tileIndex = 0; tileIndex < tileCount; tileIndex++)
				{
				
				uint64 tileByteCount64 = stream.TagValue_uint64 (ifd.fTileByteCountsType);
				
				if (tileByteCount64 > 0xFFFFFFFF)
					{
					ThrowBadFormat ();
					}
				
				tileByteCount [tileIndex] = (uint32) tileByteCount64;
				
				}
			
			}
			
		
		
		for (tileIndex = 0; tileIndex < tileCount; tileIndex++)
			{
			
			if (tileByteCount [tileIndex] < 1 ||
				tileByteCount [tileIndex] > stream.Length ())
				{
				
				ThrowBadFormat ();
				
				}
			
			}
		
		}
		
	
		
	uint32 maxTileByteCount = 0;
	
	if (tileByteCount)
		{
		
		for (tileIndex = 0; tileIndex < tileCount; tileIndex++)
			{
			
			maxTileByteCount = Max_uint32 (maxTileByteCount,
										   tileByteCount [tileIndex]);
										   
			}
			
		}
		
	

	uint32 compressedSize = 0;
	
	bool needsCompressedBuffer = NeedsCompressedBuffer (ifd);
	
	if (needsCompressedBuffer)
		{
		
		if (!tileByteCount)
			{
			ThrowBadFormat ();
			}
		
		compressedSize = maxTileByteCount;
		
		}
		
	
	
	if (lossyImage)
		{
		
		if (ifd.IsBaselineJPEG () ||
			(ifd.fCompression == ccJXL))
			{
			
			lossyImage->fImageSize.h = ifd.fImageWidth;
			lossyImage->fImageSize.v = ifd.fImageLength;
			
			lossyImage->fTileSize.h = ifd.fTileWidth;
			lossyImage->fTileSize.v = ifd.fTileLength;
			
			lossyImage->fUsesStrips = ifd.fUsesStrips;

			lossyImage->fData.resize (tileCount);
						
			}

		else
			{
			
			lossyImage = NULL;
			
			}
		
		}
		
	
	
	if (ifd.fJPEGTablesOffset && ifd.fJPEGTablesCount)
		{
	
		if (ifd.IsBaselineJPEG ())
			{
			
			fJPEGTables.Reset (host.Allocate (ifd.fJPEGTablesCount));
			
			stream.SetReadPosition (ifd.fJPEGTablesOffset);
			
			stream.Get (fJPEGTables->Buffer		 (),
						fJPEGTables->LogicalSize ());
			
			}
			
		}
		
	std::vector<dng_fingerprint> lossyTileDigests;
	
	if (lossyDigest)
		lossyTileDigests.resize (tileCount);
			
	
	
	outerSamples = Min_uint32 (image.Planes (), outerSamples);
		
	
	
	
	
	uint64 contiguousByteCount = 0;
	
		{

		bool tilesInOrder = true;
		
		uint64 totalTileBytes = 0;
		
		uint64 minFileOffset = tileOffset [0];
		uint64 maxFileOffset = tileOffset [0];
		
		tileIndex = 0;
		
		for (uint32 plane = 0; plane < outerSamples; plane++)
			{
			
			for (uint32 rowIndex = 0; rowIndex < tilesDown; rowIndex++)
				{
				
				for (uint32 colIndex = 0; colIndex < tilesAcross; colIndex++)
					{
					
					uint64 thisOffset = tileOffset [tileIndex];
					
					uint64 thisByteCount;
					
					if (tileByteCount)
						{
						
						thisByteCount = tileByteCount [tileIndex];
						
						}
						
					else
						{
						
						thisByteCount = ifd.TileByteCount (ifd.TileArea (rowIndex, colIndex));
						
						}
						
					if (thisOffset < maxFileOffset)
						{
						tilesInOrder = false;
						}

					const uint64 tileEnd = SafeUint64Add (thisOffset,
														  thisByteCount);

					totalTileBytes = SafeUint64Add (totalTileBytes,
													thisByteCount);
					
					minFileOffset = Min_uint64 (minFileOffset, thisOffset);
					maxFileOffset = Max_uint64 (maxFileOffset, tileEnd);
					
					tileIndex++;
					
					}
					
				}
				
			}
			
		
		
		if (maxFileOffset > stream.Length ())
			{
			
			ThrowBadFormat ();
			
			}
			
		
			
		if (tilesInOrder)
			{
			
			
			
			uint64 totalFileBytes = maxFileOffset - minFileOffset;

			uint64 threshold = (totalFileBytes / 10) * 9 +
							   ((totalFileBytes % 10) * 9) / 10;
			
			if (totalTileBytes >= threshold)
				{
				
				contiguousByteCount = totalFileBytes;
				
				}
		
			}
			
		}
		
	dng_stream_contiguous_read_hint readHint (stream,
											  host.Allocator (),
											  tileOffset [0],
											  contiguousByteCount);
		
	
	
	bool useMultipleThreads = (outerSamples * tilesDown * tilesAcross >= 2) &&
							  (host.PerformAreaTaskThreads () > 1) &&
							  (maxTileByteCount > 0 && maxTileByteCount <= 1024 * 1024) &&
							  (subTileLength == ifd.fTileLength) &&
							  (ifd.fCompression != ccUncompressed);
	
	if (useMultipleThreads)
		{

		DoReadTiles (host,
					 ifd,
					 stream,
					 image,
					 lossyImage,
					 lossyDigest ? &lossyTileDigests [0] : nullptr,
					 outerSamples,
					 innerSamples,
					 tilesDown,
					 tilesAcross,
					 tileOffset,
					 tileByteCount,
					 maxTileByteCount,
					 uncompressedSize);
		
		}
		
	
	
	else
		{
		
		std::shared_ptr<dng_memory_block> compressedBuffer;
		AutoPtr<dng_memory_block> uncompressedBuffer;
		AutoPtr<dng_memory_block> subTileBlockBuffer;
		
		if (uncompressedSize)
			{
			uncompressedBuffer.Reset (host.Allocate (uncompressedSize));
			}
			
		if (compressedSize && !lossyImage)
			{
			compressedBuffer.reset (host.Allocate (compressedSize));
			}
			
		else if (lossyDigest)
			{
			compressedBuffer.reset (host.Allocate (maxTileByteCount));
			}
			
		tileIndex = 0;
		
		for (uint32 plane = 0; plane < outerSamples; plane++)
			{
			
			for (uint32 rowIndex = 0; rowIndex < tilesDown; rowIndex++)
				{
				
				for (uint32 colIndex = 0; colIndex < tilesAcross; colIndex++)
					{
					
					stream.SetReadPosition (tileOffset [tileIndex]);
					
					dng_rect tileArea = ifd.TileArea (rowIndex, colIndex);
					
					uint32 subTileCount = (tileArea.H () + subTileLength - 1) /
										  subTileLength;
										  
					for (uint32 subIndex = 0; subIndex < subTileCount; subIndex++)
						{
						
						host.SniffForAbort ();
					
						dng_rect subArea (tileArea);
						
						subArea.t = tileArea.t + subIndex * subTileLength;
						
						subArea.b = Min_int32 (subArea.t + subTileLength,
											   tileArea.b);
						
						uint32 subByteCount;
						
						if (tileByteCount)
							{
							subByteCount = tileByteCount [tileIndex];
							}
						else
							{
							subByteCount = ifd.TileByteCount (subArea);
							}
							
						if (lossyImage)
							{
							
							lossyImage->fData [tileIndex].reset (host.Allocate (subByteCount));
							
							stream.Get (lossyImage->fData [tileIndex]->Buffer (), subByteCount);
							
							stream.SetReadPosition (tileOffset [tileIndex]);
							
							}
							
						else if ((needsCompressedBuffer || lossyDigest) && subByteCount)
							{
							
							stream.Get (compressedBuffer->Buffer (), subByteCount);
							
							if (lossyDigest)
								{
								
								dng_md5_direct_printer printer;
								
								printer.ProcessPtr (compressedBuffer->Buffer (),
													subByteCount);
												 
								lossyTileDigests [tileIndex] = printer.Result ();
								
								}
							
							}
							
						ReadTile (host,
								  ifd,
								  stream,
								  image,
								  subArea,
								  plane,
								  innerSamples,
								  subByteCount,
								  lossyImage ? lossyImage->fData [tileIndex] : compressedBuffer,
								  uncompressedBuffer,
								  subTileBlockBuffer,
								  useMultipleThreads);
								  
						}
					
					tileIndex++;
					
					}
					
				}

			}
			
		}
		
	
	
	if (lossyDigest)
		{

		if (fJPEGTables.Get ())
			{
			
			dng_md5_direct_printer printer;
			
			printer.ProcessPtr (fJPEGTables->Buffer		 (),
								fJPEGTables->LogicalSize ());
							 
			lossyTileDigests.push_back (printer.Result ());
			
			}
			
		dng_md5_direct_printer printer2;

		for (const auto &digest : lossyTileDigests)
			printer2.Process (digest);
							  
		*lossyDigest = printer2.Result ();
		
		}
		
	
	
	if (lossyImage && fJPEGTables.Get ())
		((dng_jpeg_image *) lossyImage)->fJPEGTables.Reset (fJPEGTables.Release ());	

	}

void dng_read_image::DoReadTiles (dng_host &host,
								  const dng_ifd &ifd,
								  dng_stream &stream,
								  dng_image &image,
								  dng_lossy_compressed_image *lossyImage,
								  dng_fingerprint *lossyTileDigest,
								  uint32 outerSamples,
								  uint32 innerSamples,
								  uint32 tilesDown,
								  uint32 tilesAcross,
								  uint64 *tileOffset,
								  uint32 *tileByteCount,
								  uint32 compressedSize,
								  uint32 uncompressedSize)
	{
	
	uint32 threadCount = Min_uint32 (outerSamples * tilesDown * tilesAcross,
									 host.PerformAreaTaskThreads ());

	dng_read_tiles_task task (*this,
							  host,
							  ifd,
							  stream,
							  image,
							  lossyImage,
							  lossyTileDigest,
							  outerSamples,
							  innerSamples,
							  tilesDown,
							  tilesAcross,
							  tileOffset,
							  tileByteCount,
							  compressedSize,
							  uncompressedSize);

	host.PerformAreaTask (task,
						  dng_rect (0, 0, 16, 16 * threadCount));
	
	}

