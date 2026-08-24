

#ifndef __dng_stream__
#define __dng_stream__

#include "dng_flags.h"

#include "dng_auto_ptr.h"
#include "dng_classes.h"
#include "dng_types.h"
#include "dng_memory.h"
#include "dng_rational.h"
#include "dng_uncopyable.h"
#include "dng_utils.h"

#ifndef qDNGStreamCheckForUnflushedStreams
#define qDNGStreamCheckForUnflushedStreams (qDNGValidate)
#endif

const uint64 kDNGStreamInvalidOffset = (uint64) (int64) -1;

class dng_stream: private dng_uncopyable
	{
	
	public:
	
		enum
			{
			
			kSmallBufferSize =	8 * 1024,
			kBigBufferSize	 = 64 * 1024,
			
			kDefaultBufferSize = kSmallBufferSize
			
			};
	
	private:
	
		bool fSwapBytes;
		
		bool fHaveLength;
		
		uint64 fLength;
		
		const uint64 fOffsetInOriginalFile;
	
		uint64 fPosition;
		
		AutoPtr<dng_memory_block> fMemBlock;
		
		uint8 *fBuffer;
		
		uint32 fBufferSize;
		
		uint64 fBufferStart;
		uint64 fBufferEnd;
		uint64 fBufferLimit;
		
		bool fBufferDirty;
		
		dng_abort_sniffer *fSniffer;
		
	protected:
	
		dng_stream (dng_abort_sniffer *sniffer = NULL,
					uint32 bufferSize = kDefaultBufferSize,
					uint64 offsetInOriginalFile = kDNGStreamInvalidOffset);
		
		virtual uint64 DoGetLength ();
	
		virtual void DoRead (void *data,
							 uint32 count,
							 uint64 offset);
							 
		virtual void DoSetLength (uint64 length);
							 
		virtual void DoWrite (const void *data,
							  uint32 count,
							  uint64 offset);

		#if qDNGStreamCheckForUnflushedStreams

		
		
		

		void DestructionOfUnflushedInstancesIsAllowed ()
			{
			fBufferDirty = false;
			}

		#endif

	public:
	
		
		
		
		
		

		dng_stream (const void *data,
					uint32 count,
					uint64 offsetInOriginalFile = kDNGStreamInvalidOffset);
		
		virtual ~dng_stream ();
		
		
		

		bool SwapBytes () const
			{
			return fSwapBytes;
			}
		
		
		
		

		void SetSwapBytes (bool swapBytes)
			{
			fSwapBytes = swapBytes;
			}

		
		

		bool BigEndian () const;
		
		
		

		void SetBigEndian (bool bigEndian = true);
		
		
		

		bool LittleEndian () const
			{
			return !BigEndian ();
			}
		
		
		

		void SetLittleEndian (bool littleEndian = true)
			{
			SetBigEndian (!littleEndian);
			}
			
		
		
		uint32 BufferSize () const
			{
			return fBufferSize;
			}

		
				
		void SetBufferSize (dng_memory_allocator &allocator,
							uint32 newBufferSize);

		
		
			
		uint64 Length ()
			{
			
			if (!fHaveLength)
				{
				
				fLength = DoGetLength ();
				
				fHaveLength = true;
				
				}
				
			return fLength;
			
			}

		
		

		uint64 Position () const
			{
			return fPosition;
			}
			
		
		
		
		

		uint64 PositionInOriginalFile () const;
		
		
		
		

		uint64 OffsetInOriginalFile () const;
		
		
		

		const void * Data () const;
		
		
		
		

		dng_memory_block * AsMemoryBlock (dng_memory_allocator &allocator,
										  uint32 numLeadingZeroBytes = 0);

		

		void SetReadPosition (uint64 offset);
		
		
		

		void Skip (uint64 delta)
			{
			SetReadPosition (SafeUint64Add (Position (), delta));
			}
		
		

		bool DataInBuffer (uint64 count,
						   uint64 offset)
			{
			return (offset >= fBufferStart &&
					count  <= fBufferEnd   &&
					offset <= fBufferEnd - count);
			}
		
		
		
		
		
		
		
		
		void Get (void *data, uint32 count, uint32 maxOverRead=0);

		void Get (dng_fingerprint &digest);

		
		
		void SetWritePosition (uint64 offset);
		
		

		void Flush ();

		
		

		void SetLength (uint64 length);

		
		
		

		void Put (const void *data, uint32 count);

		
		
		
		
		

		void Put_swap4 (const void *data,
						uint32 countMul4);
		
		
		
		
		
		

		void Put_swap8 (const void *data,
						uint32 countMul8);

		
		
		
		
		
		uint8 Get_uint8 ()
			{
			
			
			
			if (fPosition >= fBufferStart && fPosition < fBufferEnd)
				{
				
				return fBuffer [fPosition++ - fBufferStart];
				
				}
				
			
			
			uint8 x;
			
			Get (&x, 1);
			
			return x;
				
			}
		
		
		
		
		void Put_uint8 (uint8 x)
			{
			
			if (fBufferDirty			   &&
				fPosition  >= fBufferStart &&
				fPosition  <= fBufferEnd   &&
				fPosition  <  fBufferLimit)
				{
				
				fBuffer [fPosition - fBufferStart] = x;
				
				fPosition++;
				
				if (fBufferEnd < fPosition)
					fBufferEnd = fPosition;
					
				fLength = Max_uint64 (Length (), fPosition);
				
				}
				
			else
				{
				
				Put (&x, 1);
				
				}
	
			}
			
		
		
		
		
		
		
		uint16 Get_uint16 ();
		
		
		
		

		void Put_uint16 (uint16 x);
		
		
		
		
		
		
		
		uint32 Get_uint32 ();

#if !qDNGBigEndian
		inline 
		uint32 Get_uint32_LE ()
			{
	
			uint32 x;
	
			Get (&x, 4, 3); 

			

			return x;
	
			}
#endif

		
		
		

		void Put_uint32 (uint32 x);
		
		
		
		
		
		
		
		uint64 Get_uint64 ();
		
		
		
		

		void Put_uint64 (uint64 x);
		
		
		
		
		

		int8 Get_int8 ()
			{
			return (int8) Get_uint8 ();
			}
			
		
		

		void Put_int8 (int8 x)
			{
			Put_uint8 ((uint8) x);
			}

		

		void Put_bool (bool x)
			{
			Put_uint8 (x ? 1 : 0);
			}

		

		void Put_size (size_t x)
			{
			static_assert (sizeof (size_t) <= 8, "size_t > 8 bytes");
			Put_uint64 (uint64 (x));
			}

		
		
		
		
		

		int16 Get_int16 ()
			{
			return (int16) Get_uint16 ();
			}
			
		
		
		

		void Put_int16 (int16 x)
			{
			Put_uint16 ((uint16) x);
			}

		
		
		
		
		

		int32 Get_int32 ()
			{
			return (int32) Get_uint32 ();
			}
			
		
		
		

		void Put_int32 (int32 x)
			{
			Put_uint32 ((uint32) x);
			}

		
		
		
			
		void Put (const dng_rect &r);

		void Put (const dng_rect_real64 &r);

		
		
		
			
		void Put (const dng_fingerprint &digest);

		void Put (const dng_point &pt);

		void Put (const dng_point_real64 &pt);

		void Put (const dng_srational &value);

		void Put (const dng_urational &value);

		void Put (const dng_string &value);

		
		
		
		
		

		int64 Get_int64 ()
			{
			return (int64) Get_uint64 ();
			}
			
		
		
		

		void Put_int64 (int64 x)
			{
			Put_uint64 ((uint64) x);
			}
			
		
		
		
		
		

		real32 Get_real32 ();
		
		
		
		

		void Put_real32 (real32 x);
		
		
		
		
		
		

		real64 Get_real64 ();
		
		
		
		

		void Put_real64 (real64 x);
		
		
		
		
		
		
		
		
		

		
		
		
		
		
		
		

		void Get_CString (char *data,
						  uint32 maxLength,
						  uint32 maxStreamBytes = 0xFFFFFFFFu);

		
		

		void Put_CString (const char *data);

		
		
		
		
		
		
		
		
		
		
		
		

		void Get_UString (char *data,
						  uint32 maxLength,
						  uint32 maxStreamBytes = 0xFFFFFFFFu);
						  
		
		
		
		void PutZeros (uint64 count);
		
		
		
		void PadAlign2 ();
		
		
		
		void PadAlign4 ();
		
		
		
		
		
		
		
		

		uint32 TagValue_uint32 (uint32 tagType);

		
		
		
		
		
		
		

		uint64 TagValue_uint64 (uint32 tagType);

		
		
		
		
		
		
		

		int32 TagValue_int32 (uint32 tagType);
		
		
		
		
		
		
		
		

		int64 TagValue_int64 (uint32 tagType);
		
		
		
		
		
		
		
		

		dng_urational TagValue_urational (uint32 tagType);

		
		
		
		
		
		
		
		
		dng_srational TagValue_srational (uint32 tagType);

		
		
		
		
		
		
		

		real64 TagValue_real64 (uint32 tagType);

		
		
		
		dng_abort_sniffer * Sniffer () const
			{
			return fSniffer;
			}
			
		
		
		
		void SetSniffer (dng_abort_sniffer *sniffer)
			{
			fSniffer = sniffer;
			}
			
		
		
		
		
		virtual void CopyToStream (dng_stream &dstStream,
								   uint64 count);
								   
		
		
		
		void DuplicateStream (dng_stream &dstStream);
		
	};
	

class dng_stream_double_buffered : public dng_stream
	{
	
	private:
	
		dng_stream &fStream;
		
	public:
	
		dng_stream_double_buffered (dng_stream &stream,
									uint32 bufferSize = kDefaultBufferSize)
		
			:	dng_stream ((dng_abort_sniffer *) NULL,
							bufferSize,
							stream.OffsetInOriginalFile ())
		
			,	fStream (stream)
		
			{
			SetBigEndian (fStream.BigEndian ());
			}
		
	protected:
	
		virtual uint64 DoGetLength ()
			{
			return fStream.Length ();
			}
	
		virtual void DoRead (void *data,
							 uint32 count,
							 uint64 offset)
			{
			fStream.SetReadPosition (offset);
			fStream.Get (data, count);
			}

	};

class dng_stream_contiguous_read_hint
	{
	
	private:
	
		dng_stream &fStream;
		
		dng_memory_allocator &fAllocator;
		
		uint32 fOldBufferSize;
		
	public:
		
		dng_stream_contiguous_read_hint (dng_stream &stream,
										 dng_memory_allocator &allocator,
										 uint64 offset,
										 uint64 count);
		
		~dng_stream_contiguous_read_hint ();
	 
	};

class TempBigEndian
	{
	
	private:
	
		dng_stream & fStream;
		
		bool fOldSwap;
		
	public:
	
		TempBigEndian (dng_stream &stream,
					   bool bigEndian = true);
						 
		virtual ~TempBigEndian ();
		
	};
			

class TempLittleEndian: public TempBigEndian
	{
	
	public:
	
		TempLittleEndian (dng_stream &stream,
						  bool littleEndian = true)
			
			:	TempBigEndian (stream, !littleEndian)
			
			{
			}
	
		virtual ~TempLittleEndian ()
			{
			}

	};
				

class TempStreamSniffer: private dng_uncopyable
	{
	
	private:
	
		dng_stream & fStream;
		
		dng_abort_sniffer *fOldSniffer;
		
	public:
	
		TempStreamSniffer (dng_stream &stream,
						   dng_abort_sniffer *sniffer);
						 
		~TempStreamSniffer ();
		
	};
				

class PreserveStreamReadPosition: private dng_uncopyable
	{
	
	private:
	
		dng_stream & fStream;
	
		uint64 fPosition;
	
	public:
	
		PreserveStreamReadPosition (dng_stream &stream)

			:	fStream	  (stream)
			,	fPosition (stream.Position ())

			{
			}
			
		~PreserveStreamReadPosition ()
			{
			fStream.SetReadPosition (fPosition);
			}
	
	};
				

#endif
				

