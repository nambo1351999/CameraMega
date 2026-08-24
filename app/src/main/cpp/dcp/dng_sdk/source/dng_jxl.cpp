

#include "dng_jxl.h"

#include "jxl/encode_cxx.h"
#include "jxl/decode_cxx.h"
#include "jxl/resizable_parallel_runner_cxx.h"

#include "dng_abort_sniffer.h"
#include "dng_area_task.h"
#include "dng_bmff.h"
#include "dng_color_space.h"
#include "dng_file_stream.h"
#include "dng_globals.h"
#include "dng_host.h"
#include "dng_image.h"
#include "dng_info.h"
#include "dng_memory_stream.h"
#include "dng_negative.h"
#include "dng_parse_utils.h"
#include "dng_pixel_buffer.h"
#include "dng_safe_arithmetic.h"
#include "dng_tag_codes.h"
#include "dng_utils.h"
#include "dng_xmp.h"
#include "dng_xy_coord.h"

#include <atomic>
#include <memory>
#include <unordered_map>

class jxl_memory_block
	{
		
	public:

		dng_memory_allocator &fAllocator;

		void *fPhysicalBuffer = nullptr;

		void *fLogicalBuffer = nullptr;

		

		AutoPtr<dng_memory_block> fBlock;

		
		

		bool fUseMalloc = false;

	public:

		jxl_memory_block (dng_memory_allocator &allocator,
			 const uint64 bytesNeeded)
			:	fAllocator (allocator)
			{

			if (bytesNeeded > uint64 (0x80000000))
				fUseMalloc = true;

			if (fUseMalloc)
				{

				

				size_t mallocSize = (size_t) (160u + bytesNeeded);

				if (mallocSize <= bytesNeeded)
					{

					ThrowOverflow ();

					}

				fPhysicalBuffer = allocator.Malloc (mallocSize);

				if (!fPhysicalBuffer)
					{

					ThrowMemoryFull ();

					}

				fLogicalBuffer = (void *) DNG_ALIGN_SIMD (fPhysicalBuffer);

				}

			else
				{

				fBlock.Reset (allocator.Allocate (uint32 (bytesNeeded)));

				fPhysicalBuffer = fBlock->Buffer ();

				fLogicalBuffer = fPhysicalBuffer;

				}

			}

		~jxl_memory_block ()
			{

			if (fUseMalloc)
				fAllocator.Free (fPhysicalBuffer);

			fPhysicalBuffer = nullptr;

			fLogicalBuffer = nullptr;

			}

		void * Buffer ()
			{
			return fLogicalBuffer;
			}
		
	};

class dng_jxl_decoder_callback_data
	{

	public:
		
		dng_pixel_buffer fTemplateBuffer;

		dng_pixel_buffer fWholeBuffer;

		AutoPtr<jxl_memory_block> fBlock;		

		AutoPtr<dng_image> fImage;			 

		AutoPtr<dng_image> fAlphaMask;		 

		bool fAlphaPremultiplied = false;

	};

static void dng_jxl_decoder_callback_func (void* opaque,
										  size_t x,
										  size_t y,
										  size_t num_pixels,
										  const void* pixels)
	{

	#if qLogJXL && 0
	
	printf ("dng_jxl_decoder_callback_func: %u, %u, %u\n",
			(unsigned) x,
			(unsigned) y,
			(unsigned) num_pixels);
	
	#endif

	
	
	
	auto *cbData = (dng_jxl_decoder_callback_data *) opaque;

	auto buffer = cbData->fTemplateBuffer;

	buffer.fArea.t = (int32) y;
	buffer.fArea.b = (int32) (y + 1);
	buffer.fArea.l = (int32) x;
	buffer.fArea.r = (int32) (x + num_pixels);

	buffer.fData = (void *) pixels;

	#if 0

	
	
	
	
	
	
	
	
	
	
	cbData->fImage->Put (buffer);

	#else

	
	
	

	cbData->fWholeBuffer.CopyArea (buffer,
								   buffer.fArea,
								   0,		 
								   0,		 
								   buffer.fPlanes);

	#endif
	
	}

class dng_jxl_parallel_runner_data
	{

	public:
		
		dng_host *fHost = nullptr;

		dng_atomic_error_code fErrorCode { dng_error_none };
		
	};

static void CheckResult (JxlDecoderStatus status,
						 const char *msg,
						 const dng_jxl_parallel_runner_data *data,
						 const dng_error_code defaultErrorCode = dng_error_jxl_decoder)
	{
	
	if (status != JXL_DEC_SUCCESS)
		{
		
		#if qLogJXL
		
		printf ("JXL encoder (%s) error with status %d",
				msg,
				(int) status);

		#endif

		
		

		if (data)
			{

			dng_error_code code = data->fErrorCode;

			if (code != dng_error_none)
				{

				if (code == dng_error_user_canceled)
					{
					Throw_dng_error (code, "JXL decoder - user cancelled", msg);
					}

				else
					{
					Throw_dng_error (code, "JXL decoder", msg);
					}
				
				}
			
			}

		Throw_dng_error (defaultErrorCode, "JXL decoder", msg);
		
		}
	
	}

static void CheckResult (JxlEncoderStatus status,
						 const char *msg,
						 const dng_jxl_parallel_runner_data *data,
						 const dng_error_code defaultErrorCode = dng_error_jxl_encoder)
	{
	
	if (status != JXL_ENC_SUCCESS)
		{

		#if qLogJXL
		
		printf ("JXL encoder (%s) error with status %d",
				msg,
				(int) status);

		#endif

		
		

		if (data)
			{

			dng_error_code code = data->fErrorCode;

			if (code != dng_error_none)
				{

				if (code == dng_error_user_canceled)
					{
					Throw_dng_error (code, "JXL encoder - user cancelled", msg);
					}

				else
					{
					Throw_dng_error (code, "JXL encoder", msg);
					}
				
				}
			
			}

		Throw_dng_error (defaultErrorCode, "JXL encoder", msg);
		
		}
	
	}

static void * dng_jxl_alloc (void *opaque, size_t size) 
	{

	#if 0

	(void) opaque;

	return malloc (size);

	#else
	
	auto allocator = (dng_memory_allocator *) opaque;

	return allocator->Malloc (size);

	#endif
	
	}

static void dng_jxl_free (void *opaque, void *addr)
	{

	#if 0

	(void) opaque;

	free (addr);

	#else
	
	auto allocator = (dng_memory_allocator *) opaque;

	return allocator->Free (addr);

	#endif
	
	}

static JxlParallelRetCode dng_jxl_parallel_runner (void* runner_opaque, 
												   void* jpegxl_opaque,
												   JxlParallelRunInit init,
												   JxlParallelRunFunction func,
												   uint32_t start_range,
												   uint32_t end_range)
	{

	auto *data = (dng_jxl_parallel_runner_data *) runner_opaque;

	const size_t maxThreadCount = (size_t) data->fHost->PerformAreaTaskThreads ();

	JxlParallelRetCode init_ret = (*init) (jpegxl_opaque,
										   maxThreadCount);

	if (init_ret != 0)
		return init_ret;

	auto &result = data->fErrorCode;
	
	try
		{

		dng_range_parallel_task::Do
			(*data->fHost,
			 dng_range_parallel_task::info ((int32) start_range,
											(int32) end_range,
											1,	 
											(uint32) maxThreadCount),
			 "dng_jxl_parallel_runner_task",
			 [func, jpegxl_opaque] (const dng_range_parallel_task::range &ra)
			 {

			 dng_abort_sniffer::SniffForAbort (ra.fSniffer);

			 for (int32 i = ra.fBegin; i < ra.fEnd; i++)
				 {

				 (*func) (jpegxl_opaque, i, (size_t) ra.fThreadIndex);

				 }

			 });

		

		return 0;

		}

	
	

	catch (const dng_exception &except)
		{
		
		dng_error_code codeError = except.ErrorCode ();
		dng_error_code codeNormal = dng_error_none;

		(void) result.compare_exchange_strong (codeNormal,
											   codeError);		
		
		}

	catch (...)
		{
		
		dng_error_code codeError = dng_error_unknown;
		dng_error_code codeNormal = dng_error_none;

		(void) result.compare_exchange_strong (codeNormal,
											   codeError);		
		
		}

	

	return JXL_PARALLEL_RET_RUNNER_ERROR;
	
	}

class dng_jxl_io_buffer
	{

	public:

		AutoPtr<dng_memory_block> fBlock;

		const size_t fMaxBufferSize;

		const size_t fChunkSize;

		size_t fDataSize = 0;

		size_t fIndex = 0;					 

	public:

		dng_jxl_io_buffer (dng_memory_allocator &allocator,
						   size_t maxBufferSize,
						   size_t chunkSize)

			:	fMaxBufferSize (maxBufferSize)
			,	fChunkSize (chunkSize)

			{

			DNG_REQUIRE (fMaxBufferSize > 2 * fChunkSize,
						 "invalid dng_jxl_io_buffer config");

			fBlock.Reset (allocator.Allocate ((uint32) maxBufferSize));

			}

		uint8 * Ptr ()
			{
			return fBlock->Buffer_uint8 () + fIndex;
			}

		size_t DataSize () const
			{
			return fDataSize;
			}

		void Reset ()
			{
			fDataSize = 0;
			fIndex    = 0;
			}

		void ReadChunk (dng_stream &stream,
						size_t remaining)
			{

			if (stream.Position () > stream.Length () ||
				fIndex > fMaxBufferSize				 ||
				fDataSize > fMaxBufferSize			 ||
				remaining > fDataSize)
				{
				ThrowBadFormat ("Invalid JXL IO buffer state");
				}

			size_t bytes_remaining_in_stream =
				(size_t) (stream.Length () - stream.Position ());

			if (bytes_remaining_in_stream == 0)
				{
				#if qLogJXL
				printf ("dng_jxl_io_buffer -- EOF reached in ReadChunk");
				#endif
				ThrowEndOfFile ("dng_jxl_io_buffer");
				}

			
			
			
			
			size_t next_read_size = std::min (fChunkSize,
											  bytes_remaining_in_stream);

			
			
			
			size_t prev_read_size = fDataSize - remaining;

			

			if (prev_read_size > fMaxBufferSize - fIndex)
				{
				ThrowBadFormat ("Invalid JXL IO buffer state");
				}

			fIndex += prev_read_size;

			if (remaining > fMaxBufferSize - fIndex)
				{
				ThrowBadFormat ("Invalid JXL IO buffer state");
				}

			
			
			
			

			size_t bytes_available_in_buffer = fMaxBufferSize - fIndex - remaining;

			if (next_read_size > bytes_available_in_buffer && fIndex != 0)
				{

				memmove (fBlock->Buffer (),
						 fBlock->Buffer_uint8 () + fIndex,
						 remaining);

				fIndex = 0;

				bytes_available_in_buffer = fMaxBufferSize - remaining;

				}

			if (next_read_size > bytes_available_in_buffer)
				{
				next_read_size = bytes_available_in_buffer;
				}

			if (next_read_size == 0)
				{
				ThrowBadFormat ("JXL decoder retained too much input");
				}

			

			auto ptr = Ptr () + remaining;
			
			stream.Get (ptr, (uint32) next_read_size);

			

			fDataSize = remaining + next_read_size;

			}
		
	};

static void EnsureUseBoxes (JxlEncoder *enc,
							bool &once_flag)
	{
	
	if (!once_flag)
		{
		
		once_flag = true;

		CheckResult (JxlEncoderUseBoxes (enc),
					 "JxlEncoderUseBoxes",
					 nullptr);
		
		}
	
	}

#ifndef qLogStreamOutput
#define qLogStreamOutput (qDNGValidate && 0)
#endif

#ifndef qLogStreamIntput
#define qLogStreamIntput (qDNGValidate && 0)
#endif

class jxl_data_writer
	{

	public:

		dng_host &fHost;

		size_t fPosition = 0ULL;
		
		size_t fFinalizedPosition = 0ULL;

		std::vector<uint8> fOutput;
		
	public:

		jxl_data_writer (dng_host &host)
			:	fHost (host)
			{
			}

		void * get_buffer (size_t *size)
			{

			if (!size)
				return nullptr;

			if (*size == 0)
				return nullptr;

			

			try
				{
				fHost.SniffForAbort ();
				}

			catch (...)
				{
				*size = 0;
				return nullptr;
				}

			#if qLogStreamOutput

			printf ("output: get_buffer (size %d)\n",
					int (*size));

			#endif

			if (fPosition + *size > fOutput.size ())
				fOutput.resize (fPosition + *size, 0xDA);
			
			*size = fOutput.size () - fPosition;

			return fOutput.data () + fPosition;

			}

		void release_buffer (size_t written_bytes)
			{

			#if qLogStreamOutput
			
			printf ("output: release_buffer (wb=%d)\n",
					int (written_bytes));

			#endif

			fPosition += written_bytes;
			
			}

		void seek (size_t position)
			{

			#if qLogStreamOutput
			
			printf ("output: seek (pos=%d)\n",
					int (position));

			#endif
			
			fPosition = position;
			
			}

		void set_finalized_position (size_t finalized_position)
			{
			
			#if qLogStreamOutput
			
			printf ("output: set_finalized_position (pos=%d)\n",
					int (finalized_position));
			
			#endif
			
			fFinalizedPosition = finalized_position;
			
			}
		
	};

class jxl_data_writer2
	{

	private:

		static constexpr size_t kMaxSize = 1073741824;

		dng_host &fHost;

		dng_stream &fStream;

		AutoPtr<dng_memory_block> fBlock;

		size_t fBufferSize = 0ULL;

		size_t fPosition = 0ULL;

		size_t fFinalizedPosition = 0ULL;

		size_t fStartPosition = 0ULL;

	public:

		jxl_data_writer2 (dng_host &host,
						  dng_stream &stream)
			:	fHost (host)
			,	fStream (stream)
			{
			}

		void * get_buffer (size_t *size)
			{

			if (!size)
				return nullptr;

			if (*size == 0)
				return nullptr;

			

			try
				{
				fHost.SniffForAbort ();
				}

			catch (...)
				{
				*size = 0;
				return nullptr;
				}

			#if qLogStreamOutput

			printf ("output: get_buffer (size %d)\n",
					int (*size));

			#endif

			if ((*size) > kMaxSize)
				{
				*size = 0;
				return nullptr;
				}

			const uint32 requestedBytes = uint32 (*size);

			if (!fBlock.Get () ||
				(fBlock->LogicalSize () < requestedBytes))
				{

				
				
				const uint32 roundedBytes = ((requestedBytes + 1048575) & uint32 (~1048575));

				fBlock.Reset (fHost.Allocate (roundedBytes));
				
				}

			fStartPosition = fPosition;

			fBufferSize = (size_t) fBlock->LogicalSize ();

			*size = fBufferSize;

			return fBlock->Buffer ();

			}

		void release_buffer (size_t written_bytes)
			{

			#if qLogStreamOutput
			
			printf ("output: release_buffer (wb=%d)\n",
					int (written_bytes));

			#endif

			if (written_bytes > fBufferSize)
				{
				ThrowBadFormat ("Invalid JXL output buffer state");
				}

			if (written_bytes > size_t (-1) - fPosition)
				{
				ThrowOverflow ("JXL output position overflow");
				}

			fPosition += written_bytes;

			}
		
		void set_finalized_position (size_t finalized_position)
			{

			#if qLogStreamOutput
			
			printf ("output: set_finalized_position (sp=%d, pos=%d)\n",
					int (fStartPosition),
					int (finalized_position));

			#endif

			fFinalizedPosition = finalized_position;

			if (finalized_position < fStartPosition)
				{
				ThrowBadFormat ("Invalid JXL finalized position");
				}

			size_t finalizedBytes = finalized_position - fStartPosition;

			if (finalizedBytes > fBufferSize)
				{
				ThrowBadFormat ("Invalid JXL finalized byte count");
				}

			if (finalizedBytes > 0)
				{

				fStream.Put (fBlock->Buffer (),
							 uint32 (finalizedBytes));

				}
			
			fStartPosition = finalized_position;
			
			}

	};

static void * jxl_output_get_buffer (void *opaque,
									 size_t *size)
	{
	
	auto writer = (jxl_data_writer2 *) opaque;

	if (!writer)
		{
		*size = 0;
		return nullptr;
		}

	return writer->get_buffer (size);
	
	}

static void jxl_output_release_buffer (void *opaque,
									   size_t written_bytes)
	{
	
	auto writer = (jxl_data_writer2 *) opaque;

	if (writer)
		writer->release_buffer (written_bytes);
		
	}

static void output_set_finalized_position (void *opaque,
										   uint64_t finalized_position)
	{
	
	auto writer = (jxl_data_writer2 *) opaque;

	if (writer)
		writer->set_finalized_position (size_t (finalized_position));
	
	}

class jxl_image_chunk_reader
	{

	private:

		dng_host &fHost;

		const dng_image &fImage;

		JxlPixelFormat fPixelFormat;

		std::mutex fMutex;

		std::unordered_map<const void *,
						   std::shared_ptr<dng_memory_block> > fTable;

	public:

		jxl_image_chunk_reader (dng_host &host,
								const dng_image &srcImage,
								const JxlPixelFormat &pixelFormat)
			:	fHost (host)
			,	fImage (srcImage)
			,	fPixelFormat (pixelFormat)
			{
			}

		void get_color_channels_pixel_format (JxlPixelFormat *pixel_format)
			{
			if (pixel_format)
				*pixel_format = fPixelFormat;
			}
		
		const void * get_color_channel_data_at (size_t xpos,
												size_t ypos,
												size_t xsize,
												size_t ysize,
												size_t *row_offset)
			{

			#if qLogStreamIntput

			printf ("input: get_color_channel_data_at (x=%zu, y=%zu, w=%zu, h=%zu)\n",
					xpos,
					ypos,
					xsize,
					ysize);

			#endif

			if (xpos  > 0x7fffffff ||
				ypos  > 0x7fffffff ||
				xsize > 0x7fffffff ||
				ysize > 0x7fffffff)
				{

				ThrowOverflow ("coordinate too large");

				}

			dng_pixel_buffer buffer;

			buffer.fArea.t = int32 (ypos);
			buffer.fArea.l = int32 (xpos);
			buffer.fArea.b = SafeInt32Add (int32 (ypos), int32 (ysize));
			buffer.fArea.r = SafeInt32Add (int32 (xpos), int32 (xsize));

			buffer.fPlanes = fImage.Planes ();

			

			DNG_REQUIRE (buffer.fPlanes    <= 0x7fffffff, "planes too large");
			DNG_REQUIRE (buffer.fArea.W () <= 0x7fffffff, "buffer too wide");

			buffer.fPlaneStep = 1;
			buffer.fColStep	  = buffer.fPlanes;
			buffer.fRowStep	  = SafeInt32Mult (buffer.fPlanes,
											   buffer.fArea.W ());

			buffer.fPixelType = fImage.PixelType ();
			buffer.fPixelSize = TagTypeSize (buffer.fPixelType);

			const uint32 bytesNeeded = SafeUint32Mult (buffer.fRowStep,
													   buffer.fArea.H (),
													   buffer.fPixelSize);

			std::shared_ptr<dng_memory_block> block (fHost.Allocate (bytesNeeded));
			
			buffer.fData = block->Buffer ();

			const void *ptr = buffer.fData;

			

			fImage.Get (buffer);

			

			if (row_offset)
				{
				
				*row_offset = (size_t) SafeUint32Mult ((uint32) buffer.fRowStep, buffer.fPixelSize);
				
				}

			
			
				{
				std::lock_guard<std::mutex> lock (fMutex);
				fTable.insert
					(std::pair<const void *, std::shared_ptr<dng_memory_block> >
						 (ptr,
						  block));
				}

			return ptr;

			}

		void release_buffer (const void *buf)
			{

			#if qLogStreamIntput

			printf ("input: release (%llu)\n",
					(unsigned long long) (buf));

			#endif	
			
			

				{
				std::lock_guard<std::mutex> lock (fMutex);
				fTable.erase (buf);
				}

			}

	public:

		static void GetPixelFormat (void *opaque,
									JxlPixelFormat *pixel_format)
			{

			auto reader = (jxl_image_chunk_reader *) opaque;

			if (reader)
				reader->get_color_channels_pixel_format (pixel_format);

			}

		static const void * GetData (void *opaque,
									 size_t xpos,
									 size_t ypos,
									 size_t xsize,
									 size_t ysize,
									 size_t *row_offset)
			{

			auto reader = (jxl_image_chunk_reader *) opaque;

			if (reader)
				return reader->get_color_channel_data_at (xpos,
														  ypos,
														  xsize,
														  ysize,
														  row_offset);

			return nullptr;

			}

		static void Release (void *opaque,
							 const void *buf)
			{

			auto reader = (jxl_image_chunk_reader *) opaque;

			if (reader)
				reader->release_buffer (buf);

			}
		
	};

class jxl_buffer_chunk_reader
	{

	private:

		dng_host &fHost;

		const dng_pixel_buffer &fBuffer;

		JxlPixelFormat fPixelFormat;

		std::mutex fMutex;

		std::unordered_map<const void *,
						   std::shared_ptr<dng_memory_block> > fTable;

	public:

		jxl_buffer_chunk_reader (dng_host &host,
								 const dng_pixel_buffer &srcBuffer,
								 const JxlPixelFormat &pixelFormat)
			:	fHost (host)
			,	fBuffer (srcBuffer)
			,	fPixelFormat (pixelFormat)
			{

			#if qLogStreamIntput

			printf ("--- srcBuffer: t=%d, l=%d, b=%d, r=%d (w=%d, h=%d)\n",
					srcBuffer.fArea.t,
					srcBuffer.fArea.l,
					srcBuffer.fArea.b,
					srcBuffer.fArea.r,
					int (srcBuffer.fArea.W ()),
					int (srcBuffer.fArea.H ()));

			#endif
			
			}

		void get_color_channels_pixel_format (JxlPixelFormat *pixel_format)
			{
			if (pixel_format)
				*pixel_format = fPixelFormat;
			}
		
		const void * get_color_channel_data_at (size_t xpos,
												size_t ypos,
												size_t xsize,
												size_t ysize,
												size_t *row_offset)
			{

			#if qLogStreamIntput

			printf ("input: get_color_channel_data_at (x=%zu, y=%zu, w=%zu, h=%zu)\n",
					xpos,
					ypos,
					xsize,
					ysize);

			#endif

			if (xpos  > 0x7fffffff ||
				ypos  > 0x7fffffff ||
				xsize > 0x7fffffff ||
				ysize > 0x7fffffff)
				{

				ThrowOverflow ("coordinate too large");

				}
			
			dng_pixel_buffer buffer = fBuffer;

			buffer.fArea.t = SafeInt32Add (int32 (ypos),   fBuffer.fArea.t);
			buffer.fArea.l = SafeInt32Add (int32 (xpos),   fBuffer.fArea.l);
			buffer.fArea.b = SafeInt32Add (buffer.fArea.t, int32 (ysize));
			buffer.fArea.r = SafeInt32Add (buffer.fArea.l, int32 (xsize));

			

			DNG_REQUIRE (buffer.fPlanes    <= 0x7fffffff, "planes too large");
			DNG_REQUIRE (buffer.fArea.W () <= 0x7fffffff, "buffer too wide");

			buffer.fPlaneStep = 1;
			buffer.fColStep	  = buffer.fPlanes;
			buffer.fRowStep	  = SafeInt32Mult (buffer.fPlanes,
											   buffer.fArea.W ());

			const uint32 bytesNeeded = SafeUint32Mult (buffer.fRowStep,
													   buffer.fArea.H (),
													   buffer.fPixelSize);

			std::shared_ptr<dng_memory_block> block (fHost.Allocate (bytesNeeded));
			
			buffer.fData = block->Buffer ();

			const void *ptr = buffer.fData;

			

			#if 0

			dng_copy_buffer_task task (fBuffer,
									   buffer);

			fHost.PerformAreaTask (task,
								   buffer.fArea);

			#else

			buffer.CopyArea (fBuffer,
							 buffer.fArea,
							 0,
							 0,
							 buffer.fPlanes);

			#endif

			

			if (row_offset)
				{
				
				*row_offset = (size_t) SafeUint32Mult ((uint32) buffer.fRowStep, buffer.fPixelSize);

				}

			
			
				{
				std::lock_guard<std::mutex> lock (fMutex);
				fTable.insert
					(std::pair<const void *, std::shared_ptr<dng_memory_block> >
						 (ptr,
						  block));
				}

			return ptr;

			}

		void release_buffer (const void *buf)
			{

			#if qLogStreamIntput

			printf ("input: release (%llu)\n",
					(unsigned long long) (buf));

			#endif	
			
			

				{
				std::lock_guard<std::mutex> lock (fMutex);
				fTable.erase (buf);
				}

			}

	public:

		static void GetPixelFormat (void *opaque,
									JxlPixelFormat *pixel_format)
			{

			auto reader = (jxl_buffer_chunk_reader *) opaque;

			if (reader)
				reader->get_color_channels_pixel_format (pixel_format);

			}

		static const void * GetData (void *opaque,
									 size_t xpos,
									 size_t ypos,
									 size_t xsize,
									 size_t ysize,
									 size_t *row_offset)
			{

			auto reader = (jxl_buffer_chunk_reader *) opaque;

			if (reader)
				return reader->get_color_channel_data_at (xpos,
														  ypos,
														  xsize,
														  ysize,
														  row_offset);

			return nullptr;

			}

		static void Release (void *opaque,
							 const void *buf)
			{

			auto reader = (jxl_buffer_chunk_reader *) opaque;

			if (reader)
				reader->release_buffer (buf);

			}
		
	};

static JxlEncoderPtr EncodeJXL_Common (dng_host &host,
									   dng_pixel_buffer &buffer,
									   const dng_jxl_encode_settings &settings,
									   const bool useStreamingEncoder,
									   const bool useContainer,
									   const dng_jxl_color_space_info &colorSpaceInfo,
									   const dng_metadata *metadata,
									   const bool includeExif,
									   const bool includeXMP,
									   const bool includeIPTC,
									   const dng_bmff_box_list *additionalBoxes,
									   dng_jxl_parallel_runner_data &parallelData,
									   JxlPixelFormat &outPixelFormat,
									   JxlEncoderFrameSettings **outFrameSettings)
	{
	
	const bool isLossless = (settings.Distance () <= 0.0f);

	uint32 &srcPixelType = buffer.fPixelType;

	DNG_REQUIRE (srcPixelType == ttByte		 ||
				 srcPixelType == ttShort	 ||
				 srcPixelType == ttHalfFloat ||
				 srcPixelType == ttFloat,
				 "Unsupported srcPixelType in EncodeJXL");

	if (srcPixelType == ttByte)
		DNG_REQUIRE (buffer.fPixelSize == 1,
					 "unexpected ttByte pixel size");

	else if (srcPixelType == ttShort)
		DNG_REQUIRE (buffer.fPixelSize == 2,
					 "unexpected ttShort pixel size");

	else if (srcPixelType == ttHalfFloat)
		DNG_REQUIRE (buffer.fPixelSize == 2,
					 "unexpected ttHalfFloat pixel size");

	else if (srcPixelType == ttFloat)
		{
		
		DNG_REQUIRE (buffer.fPixelSize == 2 ||
					 buffer.fPixelSize == 4,
					 "unexpected ttFlaot pixel size");

		if (buffer.fPixelSize == 2)
			srcPixelType = ttHalfFloat;

		}

	
	
	
	const uint32 planes = buffer.Planes ();

	DNG_REQUIRE (planes == 1 ||				 
				 planes == 2 ||				 
				 planes == 3 ||				 
				 planes == 4,				 
				 "Unsupported plane count in EncodeJXL");

	

	JxlMemoryManager memManager;

	memManager.opaque = (void *) &host.Allocator ();
	memManager.alloc  = dng_jxl_alloc;
	memManager.free   = dng_jxl_free;

	

	auto encoder = JxlEncoderMake (&memManager);

	auto enc = encoder.get ();

	DNG_REQUIRE (enc, "JXL encoder - JxlEncoderMake failed");

	

	parallelData.fHost = &host;

	if (!settings.UseSingleThread ())
		{

		#if qLogJXL
		printf ("Encoding using multiple threads\n");
		#endif

		#if 0

		auto nativeRunner = JxlResizableParallelRunnerMake (nullptr);

		const size_t maxThreadCount = (size_t) host.PerformAreaTaskThreads ();

		JxlResizableParallelRunnerSetThreads (nativeRunner.get (),
											  maxThreadCount);

		CheckResult (JxlEncoderSetParallelRunner (enc,
												  JxlResizableParallelRunner,
												  nativeRunner.get ()),
					 "JxlDecoderSetParallelRunner",
					 &parallelData);

		(void) dng_jxl_parallel_runner;

		#else

		CheckResult (JxlEncoderSetParallelRunner (enc,
												  dng_jxl_parallel_runner,
												  (void *) &parallelData),
					 "JxlEncoderSetParallelRunner",
					 &parallelData);

		#endif

		}

	#if qLogJXL

	else
		{
		
		printf ("Encoding using 1 thread\n");
		
		}

	#endif

	
	
	CheckResult
		(JxlEncoderUseContainer (enc,
								 useContainer ? JXL_TRUE : JXL_FALSE),
		 "JxlEncoderUseContainer",
		 &parallelData);

	#if qLogJXL

	if (!useContainer)
		{
		
		if (metadata && (includeExif || includeXMP || includeIPTC))
			{
			
			printf ("EncodeJXL called with useContainer==false"
					" -- ignoring metadata");
			
			}
		
		}

	#endif	

	

	if ((metadata || additionalBoxes) && useContainer)
		{

		bool useBoxesOnceFlag = false;

		bool didAddBox = false;
		
		

		if (includeExif && metadata)
			{

			const dng_resolution *resolution = nullptr;

			AutoPtr<dng_memory_block> exifData
				(metadata->BuildExifBlock (host.Allocator (),
										   resolution,
										   false,
										   nullptr,
										   kNumLeadingZeroBytesForEXIF));

			uint32 exifSize = exifData->LogicalSize ();

			if (exifSize)
				{

				JxlBoxType type = { 'E', 'x', 'i', 'f' };

				JXL_BOOL compressBox = JXL_FALSE;

				EnsureUseBoxes (enc, useBoxesOnceFlag);

				CheckResult (JxlEncoderAddBox (enc,
											   type,
											   exifData->Buffer_uint8 (),
											   (size_t) exifData->LogicalSize (),
											   compressBox),
							 "JxlEncoderAddBox-Exif",
							 &parallelData);

				didAddBox = true;

				}

			} 
			
		

		if (includeXMP && metadata && metadata->GetXMP ())
			{

			
			

			AutoPtr<dng_memory_block> xmpBlock
				(metadata->GetXMP ()->Serialize (true));

			uint32 xmpSize = xmpBlock->LogicalSize ();

			if (xmpSize > 0)
				{

				JxlBoxType type = { 'x', 'm', 'l', ' ' };

				
				
				

				JXL_BOOL compressBox = JXL_FALSE;

				EnsureUseBoxes (enc, useBoxesOnceFlag);

				CheckResult (JxlEncoderAddBox (enc,
											   type,
											   xmpBlock->Buffer_uint8 (),
											   (size_t) xmpSize,
											   compressBox),
							 "JxlEncoderAddBox-XMP",
							 &parallelData);

				didAddBox = true;

				}

			} 

		

		if (includeIPTC && metadata)
			{

			auto iptcData = metadata->IPTCData   ();
			auto iptcSize = metadata->IPTCLength ();

			if (iptcData && iptcSize)
				{

				JxlBoxType type = { 'x', 'm', 'l', ' ' };

				JXL_BOOL compressBox = JXL_FALSE;

				EnsureUseBoxes (enc, useBoxesOnceFlag);

				CheckResult (JxlEncoderAddBox (enc,
											   type,
											   (const uint8 *) iptcData,
											   (size_t) iptcSize,
											   compressBox),
							 "JxlEncoderAddBox-IPTC",
							 &parallelData);

				didAddBox = true;

				}

			}

		

		if (additionalBoxes)
			{
			
			for (const auto &box : *additionalBoxes)
				{
				
				if (!box)
					continue;

				if (box->fName.Length () != 4)
					continue;

				if (!box->fContent)
					continue;

				const char *name = box->fName.Get ();

				JxlBoxType type =
					{
					name [0],
					name [1],
					name [2],
					name [3]
					};
				
				JXL_BOOL compressBox = JXL_FALSE;

				EnsureUseBoxes (enc, useBoxesOnceFlag);

				CheckResult (JxlEncoderAddBox (enc,
											   type,
											   (const uint8 *) box->fContent->Buffer (),
											   (size_t) box->fContent->LogicalSize (),
											   compressBox),
							 "JxlEncoderAddBox-extra",
							 &parallelData);

				didAddBox = true;

				}
			
			}

		
		

		if (didAddBox)
			{
			
			JxlEncoderCloseBoxes (enc);
			
			}

		} 

	

	bool useJXLColorEncoding = false;
	
	JxlColorEncoding colorEncoding;

	if (colorSpaceInfo.fJxlColorEncoding.Get ())
		{
		
		
		
		colorEncoding = *colorSpaceInfo.fJxlColorEncoding;

		useJXLColorEncoding = true;
		
		}

	else if (colorSpaceInfo.fICCProfile.Get ())
		{
		
		
		
		}

	else
		{
		
		

		memset (&colorEncoding, 0, sizeof (colorEncoding));

		colorEncoding.color_space		= JXL_COLOR_SPACE_RGB;
		colorEncoding.white_point		= JXL_WHITE_POINT_D65;
		colorEncoding.primaries			= JXL_PRIMARIES_SRGB;
		colorEncoding.transfer_function = JXL_TRANSFER_FUNCTION_SRGB;

		useJXLColorEncoding = true;
		
		}

	

	const uint32 bitsPerSample = buffer.fPixelSize * 8;

		{

		JxlBasicInfo info;

		JxlEncoderInitBasicInfo (&info);

		info.have_container		   = useContainer;

		info.xsize				   = buffer.fArea.W ();
		info.ysize				   = buffer.fArea.H ();

		info.bits_per_sample       = bitsPerSample;

		

		if (srcPixelType == ttFloat)
			info.exponent_bits_per_sample = 8;	 

		else if (srcPixelType == ttHalfFloat)
			info.exponent_bits_per_sample = 5;	 

		else
			{
			info.exponent_bits_per_sample = 0;	 
			info.bits_per_sample = Min_uint32 (info.bits_per_sample, 16);
			}

		
		

		

		

		if (isLossless)
			info.uses_original_profile = JXL_TRUE;

		
		
		
		
		
		

		else if (settings.UseOriginalColorEncoding ())
			info.uses_original_profile = JXL_TRUE;

		else
			info.uses_original_profile = useJXLColorEncoding ? JXL_FALSE : JXL_TRUE;

		

		const bool hasAlpha = (planes == 2 ||
							   planes == 4);

		if (hasAlpha)
			{

			info.num_extra_channels = 1;

			info.alpha_bits = bitsPerSample;

			info.num_color_channels = planes - 1;

			}

		else
			{

			info.num_extra_channels = 0;

			info.alpha_bits = 0;

			info.num_color_channels = planes;

			}

		
		#if 0
		info.have_preview = JXL_TRUE;
		info.preview.xsize = 200;
		info.preview.ysize = 200;
		#endif

		
		info.intrinsic_xsize	   = info.xsize;
		info.intrinsic_ysize	   = info.ysize;

		info.intensity_target = float (colorSpaceInfo.fIntensityTargetNits);

		CheckResult (JxlEncoderSetBasicInfo (enc, &info),
					 "JxlEncoderSetBasicInfo",
					 &parallelData);

		}

	
	

	if (useJXLColorEncoding)
		{
			
		CheckResult (JxlEncoderSetColorEncoding (enc, &colorEncoding),
					 "JxlEncoderSetColorEncoding",
					 &parallelData);

		}

	else
		{

		DNG_REQUIRE (colorSpaceInfo.fICCProfile.Get (),
					 "invalid icc profile");

		

		const void *profileData = colorSpaceInfo.fICCProfile->Buffer ();
		uint32		profileSize = colorSpaceInfo.fICCProfile->LogicalSize ();

		if (profileData && profileSize)
			{

			CheckResult (JxlEncoderSetICCProfile (enc,
												  (const uint8 *) profileData,
												  (size_t) profileSize),
						 "JxlEncoderSetICCProfile",
						 &parallelData);

			}

		else
			{

			#if qLogJXL
			printf ("Writing untagged JXL file");
			#endif
			
			}
		
		}

	

	auto frameSettings = JxlEncoderFrameSettingsCreate (enc, NULL);

	DNG_REQUIRE (frameSettings,
				 "JXL encoder - JxlEncoderFrameSettingsCreate failed");

	*outFrameSettings = frameSettings;
	
	

	if (isLossless)
		{

		

		#if qLogJXL
		printf ("Using lossless encoding\n");
		#endif

		CheckResult (JxlEncoderSetFrameLossless (frameSettings,
												 JXL_TRUE),
					 "JxlEncoderSetFrameLossless",
					 &parallelData);

		}

	else
		{

		

		const real32 distance = settings.Distance ();

		#if qLogJXL
		printf ("using lossy encoding with distance %.3f\n",
				distance);
		#endif

		CheckResult (JxlEncoderSetFrameDistance (frameSettings,
												 distance),
					 "JxlEncoderSetFrameDistance",
					 &parallelData);

		}

	

	CheckResult
		(JxlEncoderFrameSettingsSetOption (frameSettings,	
										   JXL_ENC_FRAME_SETTING_EFFORT,
										   (int) settings.Effort ()),
		 "JxlEncoderFrameSettingsSetOption",
		 &parallelData);

	

	if (useStreamingEncoder)
		{

		CheckResult
			(JxlEncoderFrameSettingsSetOption (frameSettings,	
											   JXL_ENC_FRAME_SETTING_BUFFERING,
											   (int) 3),
			 "JxlEncoderFrameSettingsSetOption - buffering",
			 &parallelData);

		}

	

	
	
	

	#if 0

	printf ("lossless: %s\n", isLossless ? "yes" : "no");

	printf ("effort: %u\n", unsigned (settings.Effort ()));

	printf ("distance: %f\n", float (settings.Distance ()));

	#endif

	if (!isLossless)
		{

		

		CheckResult
			(JxlEncoderFrameSettingsSetOption (frameSettings,	
											   JXL_ENC_FRAME_SETTING_DECODING_SPEED,
											   (int) settings.DecodeSpeed ()),
			 "JxlEncoderFrameSettingsSetOption",
			 &parallelData);

		
		

		CheckResult
			(JxlEncoderFrameSettingsSetOption (frameSettings,	
											   JXL_ENC_FRAME_SETTING_PATCHES,
											   0),
			 "JxlEncoderFrameSettingsSetOption-Patches",
			 &parallelData);

		if (settings.Distance () > 0.02f)
			{

			

			CheckResult
				(JxlEncoderFrameSettingsSetOption (frameSettings,	
												   JXL_ENC_FRAME_SETTING_GABORISH,
												   1),
				 "JxlEncoderFrameSettingsSetOption-Gaborish",
				 &parallelData);

			
			

			CheckResult
				(JxlEncoderFrameSettingsSetOption (frameSettings,	
												   JXL_ENC_FRAME_SETTING_EPF,
												   3),
				 "JxlEncoderFrameSettingsSetOption-EPF",
				 &parallelData);

			}

		

		

		} 

	

	

	

	
	

	auto &pixelFormat = outPixelFormat;

	memset (&pixelFormat, 0, sizeof (pixelFormat));

	pixelFormat.num_channels = planes;

	switch (srcPixelType)
		{
		
		case ttByte:
			pixelFormat.data_type = JXL_TYPE_UINT8;
			break;
			
		case ttShort:
			pixelFormat.data_type = JXL_TYPE_UINT16;
			break;
			
		case ttHalfFloat:
			pixelFormat.data_type = JXL_TYPE_FLOAT16;
			break;
			
		case ttFloat:
			pixelFormat.data_type = JXL_TYPE_FLOAT;
			break;
			
		default:
			{
			#if qLogJXL
			printf ("unexpected srcPixelType");
			#endif
			ThrowProgramError ("unexpected srcPixelType");
			break;
			}
		
		}

	
	

	#if 0

	dng_pixel_buffer previewPixelBuffer;

	previewPixelBuffer.fArea = dng_rect (200, 200); 

	previewPixelBuffer.fPlanes = planes;

	previewPixelBuffer.fPlaneStep = 1;
	previewPixelBuffer.fColStep   = (int32) planes;
	previewPixelBuffer.fRowStep   = SafeInt32Mult ((int32) planes,
												   (int32) previewPixelBuffer.fArea.W ());

	previewPixelBuffer.fPixelType = srcPixelType;
	previewPixelBuffer.fPixelSize = TagTypeSize (previewPixelBuffer.fPixelType);

	uint32 previewBytesNeeded = SafeUint32Mult (previewPixelBuffer.fRowStep,
												previewPixelBuffer.fArea.H (),
												previewPixelBuffer.fPixelSize);

	AutoPtr<dng_memory_block> previewBlock (host.Allocate (previewBytesNeeded));

	previewPixelBuffer.fData = previewBlock->Buffer ();

	

	auto previewFrameSettings = JxlEncoderFrameSettingsCreate (enc,
															   frameSettings);

	DNG_REQUIRE (previewFrameSettings,
				 "JXL encoder - JxlEncoderFrameSettingsCreate failed");

	CheckResult
		(JxlEncoderAddImageFrame (previewFrameSettings,
								  &pixelFormat,
								  previewPixelBuffer.fData,
								  previewBytesNeeded),
		 "JxlEncoderAddImageFrame-preview");

	#endif	
	
	
	
	
	

	#if 0
	
	CheckResult (JxlEncoderSetFrameName (frameSettings,
										 "main"),
				 "JxlEncoderSetFrameName",
				 &parallelData);

	#endif

	return encoder;
	
	}

static void EncodeJXL (dng_host &host,
					   dng_stream &stream,
					   const dng_image &srcImage,
					   const dng_jxl_encode_settings &settings,
					   const bool useContainer,
					   const dng_jxl_color_space_info &colorSpaceInfo,
					   const dng_metadata *metadata,
					   const bool includeExif,
					   const bool includeXMP,
					   const bool includeIPTC,
					   const dng_bmff_box_list *additionalBoxes)
	{

	#if qDNGValidate && 0
	dng_timer timerOuter ("EncodeJXL-Image");
	#endif

	dng_pixel_buffer buffer;

	buffer.fArea	  = srcImage.Bounds ();
	buffer.fPlanes	  = srcImage.Planes ();
	
	buffer.fPixelType = srcImage.PixelType ();
	buffer.fPixelSize = TagTypeSize (buffer.fPixelType);

	dng_jxl_parallel_runner_data parallelData;

	JxlPixelFormat pixelFormat;

	JxlEncoderFrameSettings *frameSettings = nullptr;

	
	

	const bool useStreamingEncoder = (srcImage.Bounds ().LongSide () > 2048);

	auto encoder = EncodeJXL_Common (host,
									 buffer,
									 settings,
									 useStreamingEncoder,
									 useContainer,
									 colorSpaceInfo,
									 metadata,
									 includeExif,
									 includeXMP,
									 includeIPTC,
									 additionalBoxes,
									 parallelData,
									 pixelFormat,
									 &frameSettings);

	auto enc = encoder.get ();

	jxl_data_writer2 writer (host, stream);

	JxlEncoderOutputProcessor outputProcessor { };

	outputProcessor.opaque                 = &writer;
	outputProcessor.get_buffer             = jxl_output_get_buffer;
	outputProcessor.release_buffer         = jxl_output_release_buffer;
	outputProcessor.seek                   = nullptr;			 
	outputProcessor.set_finalized_position = output_set_finalized_position;

	CheckResult (JxlEncoderSetOutputProcessor (enc,
											   outputProcessor),
				 "JxlEncoderSetOutputProcessor-main",
				 &parallelData);

	

	jxl_image_chunk_reader reader (host,
								   srcImage,
								   pixelFormat);

	JxlChunkedFrameInputSource inputSource { };

	inputSource.opaque							= &reader;
	inputSource.get_color_channels_pixel_format = jxl_image_chunk_reader::GetPixelFormat;
	inputSource.get_color_channel_data_at		= jxl_image_chunk_reader::GetData;
	inputSource.release_buffer					= jxl_image_chunk_reader::Release;
	
	CheckResult (JxlEncoderAddChunkedFrame (frameSettings,
											JXL_TRUE, 
											inputSource),
				 "JxlEncoderAddChunkedFrame-main",
				 &parallelData);

	stream.Flush ();

	

	JxlEncoderCloseInput (enc);

	}

static void EncodeJXL (dng_host &host,
					   dng_stream &stream,
					   const dng_pixel_buffer &inBuffer,
					   const dng_jxl_encode_settings &settings,
					   const bool useContainer,
					   const dng_jxl_color_space_info &colorSpaceInfo,
					   const dng_metadata *metadata,
					   const bool includeExif,
					   const bool includeXMP,
					   const bool includeIPTC,
					   const dng_bmff_box_list *additionalBoxes)
	{

	#if qDNGValidate && 0
	dng_timer timerOuter ("EncodeJXL");
	#endif

	dng_pixel_buffer buffer = inBuffer;

	dng_jxl_parallel_runner_data parallelData;

	JxlPixelFormat pixelFormat;

	JxlEncoderFrameSettings *frameSettings = nullptr;

	const bool useStreamingEncoder = (inBuffer.Area ().LongSide () > 2048);

	auto encoder = EncodeJXL_Common (host,
									 buffer,
									 settings,
									 useStreamingEncoder,
									 useContainer,
									 colorSpaceInfo,
									 metadata,
									 includeExif,
									 includeXMP,
									 includeIPTC,
									 additionalBoxes,
									 parallelData,
									 pixelFormat,
									 &frameSettings);

	auto enc = encoder.get ();

	jxl_data_writer2 writer (host, stream);

	JxlEncoderOutputProcessor outputProcessor { };

	outputProcessor.opaque                 = &writer;
	outputProcessor.get_buffer             = jxl_output_get_buffer;
	outputProcessor.release_buffer         = jxl_output_release_buffer;
	outputProcessor.seek                   = nullptr;			 
	outputProcessor.set_finalized_position = output_set_finalized_position;

	CheckResult (JxlEncoderSetOutputProcessor (enc,
											   outputProcessor),
				 "JxlEncoderSetOutputProcessor-main",
				 &parallelData);

	

	jxl_buffer_chunk_reader reader (host,
									buffer,
									pixelFormat);

	JxlChunkedFrameInputSource inputSource { };

	inputSource.opaque							= &reader;
	inputSource.get_color_channels_pixel_format = jxl_buffer_chunk_reader::GetPixelFormat;
	inputSource.get_color_channel_data_at		= jxl_buffer_chunk_reader::GetData;
	inputSource.release_buffer					= jxl_buffer_chunk_reader::Release;
	
	CheckResult (JxlEncoderAddChunkedFrame (frameSettings,
											JXL_TRUE, 
											inputSource),
				 "JxlEncoderAddChunkedFrame-main",
				 &parallelData);

	stream.Flush ();

	

	JxlEncoderCloseInput (enc);

	}

class dng_jxl_box_reader
	{

	private:

		enum
			{
			kInitialJXLBoxBufferSize = 4096,
			kMaxJXLBoxDataSize	   = 128 * 1024 * 1024
			};
		
	public:

		dng_host &fHost;

		JxlDecoder *fDec = nullptr;

		bool fHasCurrentBox = false;
		
		JxlBoxType fType;
		
		std::vector<uint8> fData;
		
		size_t fIndex = 0;
		
		dng_info *fInfo = nullptr;

		dng_jxl_decoder &fDecoder;

		
		

		uint64 fCurrentOffset = 0;

		
		

		uint64 fCurrentRawSize = 0;

	public:

		dng_jxl_box_reader (dng_host &host,
							JxlDecoder *dec,
							dng_jxl_decoder &decoder,
							uint64 startPosition)
			:	fHost (host)
			,	fDec (dec)
			,	fInfo (decoder.fInfo)
			,	fDecoder (decoder)
			,	fCurrentOffset (startPosition)
			{
			}

		void HandleDecBox ()
			{

			
			

			CheckFinishBox ();

			fCurrentOffset += fCurrentRawSize;

			

			CheckResult (JxlDecoderGetBoxType (fDec,
											   fType,
											   JXL_TRUE), 
						 "JxlDecoderGetBoxType",
						 nullptr);

			

			uint64_t size;

			CheckResult (JxlDecoderGetBoxSizeRaw (fDec, &size),
						 "JxlDecoderGetBoxSizeRaw",
						 nullptr);

			fCurrentRawSize = uint64 (size);

			#if qLogJXL

			printf ("jxl box: type: \"%c%c%c%c\" size: %llu\n",
					fType [0],
					fType [1],
					fType [2],
					fType [3],
					(unsigned long long) size);

			#endif

			

			if (fType [0] == 'j' &&
				fType [1] == 'x' &&
				fType [2] == 'l' &&
				fType [3] == 'c')
				{

				

				

				}

			
			

			#if 1

			else if (size > kMaxJXLBoxDataSize)
				 {

				 #if qLogJXL
					
				 printf ("jxl: Skipping large box with size %llu",
						 (unsigned long long) size);

				 #endif
					
				 }

			#endif

			else
				{

				

				fIndex = 0;

				if (fData.empty ())
					{
					fData.resize (kInitialJXLBoxBufferSize);
					}

				CheckResult (JxlDecoderSetBoxBuffer (fDec,
													 fData.data (),
													 fData.size ()),
							 "JxlDecoderSetBoxBuffer",
							 nullptr);

				fHasCurrentBox = true;

				}

			}

		void HandleNeedMoreOutput ()
			{
		
			size_t remaining = JxlDecoderReleaseBoxBuffer (fDec);

			if (fData.empty () ||
				remaining > fData.size ())
				{
				ThrowBadFormat ("Invalid JXL box buffer state");
				}

			fIndex = fData.size () - remaining;

			if (fData.size () >= kMaxJXLBoxDataSize)
				{
				ThrowBadFormat ("JXL box too large");
				}

			size_t newSize = fData.size () * 2;

			if (newSize <= fData.size () ||
				newSize > kMaxJXLBoxDataSize)
				{
				newSize = kMaxJXLBoxDataSize;
				}

			if (newSize <= fIndex)
				{
				ThrowBadFormat ("JXL box too large");
				}

			fData.resize (newSize);

			size_t availableSize = newSize - fIndex;
			
			#if qLogJXL
			printf ("box read output: remain=%llu\n",
					(unsigned long long) remaining);
			#endif

			CheckResult (JxlDecoderSetBoxBuffer (fDec,
												 &fData [fIndex],
												 availableSize),
						 "JxlDecoderSetBoxBuffer",
						 nullptr);
		
			}

		void CheckFinishBox ()
			{

			if (fHasCurrentBox)
				{

				size_t remaining = JxlDecoderReleaseBoxBuffer (fDec);

				if (remaining > fData.size ())
					{
					ThrowBadFormat ("Invalid JXL box buffer state");
					}

				fData.resize (fData.size () - remaining);

				fHasCurrentBox = false;

				FinishBox ();

				}

			}

	private:

		
		

		void FinishBox ()
			{

			
			

			fDecoder.fCurrentBoxStreamOffset = fCurrentOffset;
			fDecoder.fCurrentBoxRawSize      = fCurrentRawSize;

			#if qLogJXL

			printf ("finished up box type %c%c%c%c: final byte size %llu\n",
					fType [0],
					fType [1],
					fType [2],
					fType [3],
					(unsigned long long) fData.size ());

			#endif

			if (fType [0] == 'E' &&
				fType [1] == 'x' &&
				fType [2] == 'i' &&
				fType [3] == 'f')
				{

				fDecoder.ProcessExifBox (fHost,
										 fData);

				} 

			else if ((fType [0] == 'X' &&
					  fType [1] == 'M' &&
					  fType [2] == 'L' &&
					  fType [3] == ' ') ||
					 
					 (fType [0] == 'x' &&
					  fType [1] == 'm' &&
					  fType [2] == 'l' &&
					  fType [3] == ' '))
					 
				{

				fDecoder.ProcessXMPBox (fHost,
										fData);

				} 

			else
				{
				
				

				const char rawTypeName [] =
					{
					fType [0],
					fType [1],
					fType [2],
					fType [3],
					'\0'
					};

				dng_string typeName (rawTypeName);

				fDecoder.ProcessBox (fHost,
									 typeName,
									 fData);
				
				}

			

			#if qDNGValidate && 0

			char buf [256];

			snprintf (buf,
					  256,
					  "%c%c%c%c.txt",
					  fType [0],
					  fType [1],
					  fType [2],
					  fType [3]);

			dng_file_stream outStream (buf, true);

			outStream.Put (fData.data (), (uint32) fData.size ());

			outStream.Flush ();

			#endif	

			}		
		
	};

class dng_put_alpha_buffer_task : public dng_area_task
	{
		
	private:

		const dng_pixel_buffer &fSrc;

		dng_image &fDst;

		int32 fSrcAlphaPlane = 0;

	public:

		dng_put_alpha_buffer_task (const dng_pixel_buffer &buffer,
								   dng_image &image)
			
			:	dng_area_task ("dng_put_alpha_buffer_task")
				
			,	fSrc (buffer)
				
			,	fDst (image)
				
			{
			
			DNG_REQUIRE (buffer.Planes () == 2 ||
						 buffer.Planes () == 4,
						 "Unsupported plane count in dng_stage_put_alpha_buffer");

			fSrcAlphaPlane = int32 (buffer.Planes () - 1);

			}

		dng_rect RepeatingTile1 () const override
			{
			
			return fDst.RepeatingTile ();
			
			}
			
		void Process (uint32 ,
					  const dng_rect &tile,
					  dng_abort_sniffer * ) override
			{

			dng_pixel_buffer temp = fSrc;

			temp.fData = (void *) fSrc.ConstPixel (tile.t,
												   tile.l,
												   fSrcAlphaPlane);

			temp.fArea = tile;

			temp.fPlane  = 0;
			temp.fPlanes = 1;

			fDst.Put (temp);
		
			}
		
	};

dng_jxl_decoder::~dng_jxl_decoder ()
	{
	}

void dng_jxl_decoder::Decode (dng_host &host,
							  dng_stream &stream)
	{

	const bool needBoxMeta = fNeedBoxMeta;
	const bool needImage   = fNeedImage;
	
	

	JxlMemoryManager memManager;

	memManager.opaque = (void *) &host.Allocator ();
	memManager.alloc  = dng_jxl_alloc;
	memManager.free   = dng_jxl_free;

	

	const uint64 startPosition = stream.Position ();

	

	auto decoder = JxlDecoderMake (&memManager);

	auto dec = decoder.get ();

	DNG_REQUIRE (dec, "JXL decoder - JxlDecoderMake failed");

	

	CheckResult (JxlDecoderSetKeepOrientation (dec, 1),
				 "JxlDecoderSetKeepOrientation",
				 nullptr);

	

	dng_jxl_parallel_runner_data parallelData;

	parallelData.fHost = &host;
	
	#if 1

	
	

	auto nativeRunner = JxlResizableParallelRunnerMake (nullptr);

	const size_t maxThreadCount =
		(size_t) (fUseSingleThread ? 1 : host.PerformAreaTaskThreads ());
	
	JxlResizableParallelRunnerSetThreads (nativeRunner.get (),
										  maxThreadCount);

	CheckResult (JxlDecoderSetParallelRunner (dec,
											  JxlResizableParallelRunner,
											  nativeRunner.get ()),
				 "JxlDecoderSetParallelRunner",
				 &parallelData);
	
	#else

	CheckResult (JxlDecoderSetParallelRunner (dec,
											  dng_jxl_parallel_runner,
											  (void *) &parallelData),
				 "JxlDecoderSetParallelRunner",
				 &parallelData);
	
	#endif

	

	
	

	int eventsWanted = (JXL_DEC_BASIC_INFO |
						JXL_DEC_COLOR_ENCODING);

	

	if (needBoxMeta)
		{
		eventsWanted = eventsWanted | JXL_DEC_BOX;
		}

	

	

	if (needImage)
		{
		eventsWanted = eventsWanted | JXL_DEC_FULL_IMAGE | JXL_DEC_FRAME;
		}
	
	CheckResult (JxlDecoderSubscribeEvents (dec, eventsWanted),
				 "JxlDecoderSubscribeEvents",
				 &parallelData);

	

	CheckResult (JxlDecoderSetDecompressBoxes (dec, JXL_TRUE),
				 "JxlDecoderSetDecompressBoxes",
				 &parallelData);

	

	
	

	dng_jxl_io_buffer inputBuffer (host.Allocator (),
								   17 * 1024 * 1024,  
								   8  * 1024 * 1024); 

	JxlBasicInfo basicInfo;

	bool foundBasicInfo = false;

	JxlFrameHeader frameHeader;

	dng_jxl_box_reader boxReader (host, dec, *this, startPosition);

	dng_jxl_decoder_callback_data cbData;

	

	for (;;)
		{

		
		
		
		
		JxlDecoderStatus status = JxlDecoderProcessInput (dec);

		

		if (status == JXL_DEC_ERROR)
			{
			#if qLogJXL
			printf ("Unknown jxl decoder error");
			#endif
			ThrowBadFormat ("JXL_DEC_ERROR");
			}
		  
		

		else if (status == JXL_DEC_SUCCESS)
			{
			
			
			
			#if qLogJXL
			printf ("jxl decode success\n");
			#endif

			boxReader.CheckFinishBox ();
		
			break;
		
			}

		

		else if (status == JXL_DEC_NEED_MORE_INPUT)
			{
			
			
			

			#if qLogJXL
			printf ("--- JXL_DEC_NEED_MORE_INPUT\n");
			#endif
			
			size_t remaining = JxlDecoderReleaseInput (dec);

			inputBuffer.ReadChunk (stream, remaining);

			CheckResult (JxlDecoderSetInput (dec,
											 inputBuffer.Ptr (),
											 inputBuffer.DataSize ()),
						 "JxlDecoderSetInput",
						 &parallelData);

			

			if (stream.Position () >= stream.Length ())
				{
				JxlDecoderCloseInput (dec);
				}
			
			}

		
		
		else if (status == JXL_DEC_BASIC_INFO)
			{

			CheckResult (JxlDecoderGetBasicInfo (dec, &basicInfo),
						 "JxlDecoderGetBasicInfo",
						 &parallelData);

			foundBasicInfo = true;

			#if qDNGValidate

			if (needBoxMeta && gVerbose)
				{

				printf ("\nJXL basic info:\n\n");

				printf ("dimensions: %ux%u\n", basicInfo.xsize, basicInfo.ysize);
				printf ("have_container: %d\n", basicInfo.have_container);
				printf ("uses_original_profile: %d\n", basicInfo.uses_original_profile);
				printf ("bits_per_sample: %d\n", basicInfo.bits_per_sample);

				if (basicInfo.exponent_bits_per_sample)
					printf ("float, with exponent_bits_per_sample: %d\n",
						   basicInfo.exponent_bits_per_sample);
				
				if (basicInfo.intensity_target != 255.f ||
					basicInfo.min_nits != 0.f ||
					basicInfo.relative_to_max_display != 0 ||
					basicInfo.relative_to_max_display != 0.f)
					{
					printf ("intensity_target: %f\n", basicInfo.intensity_target);
					printf ("min_nits: %f\n", basicInfo.min_nits);
					printf ("relative_to_max_display: %d\n", basicInfo.relative_to_max_display);
					printf ("linear_below: %f\n", basicInfo.linear_below);
					}
				
				printf ("have_preview: %d\n", basicInfo.have_preview);
				
				if (basicInfo.have_preview)
					{
					printf ("preview xsize: %u\n", basicInfo.preview.xsize);
					printf ("preview ysize: %u\n", basicInfo.preview.ysize);
					}
				
				printf ("have_animation: %d\n", basicInfo.have_animation);
				
				if (basicInfo.have_animation)
					{

					printf ("ticks per second (numerator / denominator): %u / %u\n",
							basicInfo.animation.tps_numerator, basicInfo.animation.tps_denominator);

					printf ("num_loops: %u\n", basicInfo.animation.num_loops);

					printf ("have_timecodes: %d\n", basicInfo.animation.have_timecodes);

					}
				
				printf ("intrinsic xsize: %u\n", basicInfo.intrinsic_xsize);
				printf ("intrinsic ysize: %u\n", basicInfo.intrinsic_ysize);

				const char* const orientation_string[8] =
					{
					"Normal",          "Flipped horizontally",
					"Upside down",     "Flipped vertically",
					"Transposed",      "90 degrees clockwise",
					"Anti-Transposed", "90 degrees counter-clockwise"
					};
				
				if (basicInfo.orientation > 0 && basicInfo.orientation < 9)
					{
					printf ("orientation: %d (%s)\n", basicInfo.orientation,
							orientation_string[basicInfo.orientation - 1]);
					}

				else
					{
					fprintf (stderr, "Invalid orientation\n");
					}
				
				printf ("num_color_channels: %d\n", basicInfo.num_color_channels);
				printf ("num_extra_channels: %d\n", basicInfo.num_extra_channels);

				
				
				const char* const ec_type_names [7] =
					{
					"Alpha",
					"Depth",
					"Spot color",
					"Selection mask",
					"K (of CMYK)",
					"CFA (Bayer data)",
					"Thermal"
					};
				
				for (uint32_t i = 0; i < basicInfo.num_extra_channels; i++)
					{

					JxlExtraChannelInfo extra;
					
					CheckResult
						(JxlDecoderGetExtraChannelInfo (dec,
														i,
														&extra),
						 "JxlDecoderGetExtraChannelInfo",
						 nullptr);
					
					printf ("extra channel %u:\n", i);
					
					printf ("  type: %s\n",
							(extra.type < 7 ? ec_type_names [extra.type]
							 : (extra.type == JXL_CHANNEL_OPTIONAL
								? "Unknown but can be ignored"
								: "Unknown, please update your libjxl")));
					
					printf ("  bits_per_sample: %u\n",
							extra.bits_per_sample);
					
					if (extra.exponent_bits_per_sample > 0)
						printf ("  float, with exponent_bits_per_sample: %u\n",
								extra.exponent_bits_per_sample);
					
					if (extra.dim_shift > 0)
						printf ("  dim_shift: %u (upsampled %ux)\n",
								extra.dim_shift,
								1 << extra.dim_shift);
					
					if (extra.name_length)
						{

						
						
						

						const uint32 nameLen =
							Min_uint32 (extra.name_length, 65535u);

						std::vector<char> temp (nameLen + 1);

						char *name = temp.data ();

						CheckResult (JxlDecoderGetExtraChannelName
									 (dec,
									  i,
									  name,
									  nameLen + 1),
									 "JxlDecoderGetExtraChannelName",
									 nullptr);

						printf ("  name: %s\n", name);

						}
					
					if (extra.type == JXL_CHANNEL_ALPHA)
						printf ("  alpha_premultiplied: %d (%s)\n",
								extra.alpha_premultiplied,
								extra.alpha_premultiplied ? "Premultiplied"
														  : "Non-premultiplied");
					
					if (extra.type == JXL_CHANNEL_SPOT_COLOR)
						printf ("  point_color: (%f, %f, %f) with opacity %f\n",
								extra.spot_color[0],
								extra.spot_color[1],
								extra.spot_color[2],
								extra.spot_color[3]);
					
					if (extra.type == JXL_CHANNEL_CFA)
						printf ("  cfa_channel: %u\n", extra.cfa_channel);
					
					} 

				} 

			#endif	
			
			} 

		

		else if (status == JXL_DEC_COLOR_ENCODING)
			{

			

			
			
			
			

			#if qDNGValidate
			if (gVerbose)
				printf("\nJXL color profile info:\n");
			#endif

			JxlColorEncoding color_encoding;
			
			if (JXL_DEC_SUCCESS ==
				JxlDecoderGetColorAsEncodedProfile (dec,
													
													JXL_COLOR_PROFILE_TARGET_ORIGINAL,
													&color_encoding))
				{

				

				

				fColorSpaceInfo.fJxlColorEncoding.Reset (new JxlColorEncoding);

				*fColorSpaceInfo.fJxlColorEncoding = color_encoding;

				#if qDNGValidate

				if (gVerbose)
					{

					printf ("  format: JPEG XL encoded color profile\n");

					const char* const cs_string [4] =
						{
						"RGB color",
						"Grayscale",
						"XYB",
						"Unknown"
						};

					const char* const wp_string [12] =
						{
						"",
						"D65",
						"Custom",
						"",
						"",
						"",
						"",
						"",
						"",
						"",
						"E",
						"P3"
						};

					const char* const pr_string [12] =
						{
						"", "sRGB", "Custom", "",
						"", "", "", "",
						"", "Rec.2100", "", "P3"
						};

					const char* const tf_string [19] =
						{
						"", "709", "Unknown", "", "",
						"", "",   "",    "Linear", "",
						"", "",    "",        "sRGB",
						"", "", "PQ", "DCI", "HLG"
						};

					const char* const ri_string [4] =
						{
						"Perceptual",
						"Relative",
						"Saturation",
						"Absolute"
						};

					printf ("  color_space: %d (%s)\n",
							color_encoding.color_space,
							cs_string [color_encoding.color_space]);

					printf ("  white_point: %d (%s)\n",
							color_encoding.white_point,
						   wp_string[color_encoding.white_point]);

					if (color_encoding.white_point == JXL_WHITE_POINT_CUSTOM)
						{

						printf ("  white_point XY: %f %f\n",
								color_encoding.white_point_xy [0],
								color_encoding.white_point_xy [1]);

						}

					if (color_encoding.color_space == JXL_COLOR_SPACE_RGB ||
						color_encoding.color_space == JXL_COLOR_SPACE_UNKNOWN)
						{

						printf ("  primaries: %d (%s)\n",
								color_encoding.primaries,
								pr_string[color_encoding.primaries]);

						if (color_encoding.primaries == JXL_PRIMARIES_CUSTOM)
							{

							printf ("  red primaries XY: %f %f\n",
									color_encoding.primaries_red_xy [0],
									color_encoding.primaries_red_xy [1]);

							printf ("  green primaries XY: %f %f\n",
									color_encoding.primaries_green_xy [0],
									color_encoding.primaries_green_xy [1]);

							printf ("  blue primaries XY: %f %f\n",
									color_encoding.primaries_blue_xy [0],
									color_encoding.primaries_blue_xy [1]);

							}

						}

					if (color_encoding.transfer_function ==
						JXL_TRANSFER_FUNCTION_GAMMA)
						{

						printf ("  transfer_function: gamma: %f\n",
								color_encoding.gamma);

						}

					else
						{

						printf ("  transfer_function: %d (%s)\n",
								color_encoding.transfer_function,
								tf_string [color_encoding.transfer_function]);

						}

					printf ("  rendering_intent: %d (%s)\n",
							color_encoding.rendering_intent,
							ri_string [color_encoding.rendering_intent]);

					}

				#endif	

				}
			
			else
				{
				
				
				
				
				size_t profile_size;

				CheckResult (JxlDecoderGetICCProfileSize
							 (dec,
							  
							  JXL_COLOR_PROFILE_TARGET_DATA,
							  &profile_size),
							 "JxlDecoderGetICCProfileSize",
							 nullptr);
				
				if (profile_size < 132)
					{

					#if qLogJXL
					
					printf ("ICC profile size %llu too small -- ignoring",
							(unsigned long long) profile_size);

					#endif
					
					continue;
					
					}

				if (profile_size > 0xFFFFFFFF)
					{
					ThrowBadFormat ("ICC profile too large");
					}

				AutoPtr<dng_memory_block> profileBlock
					(host.Allocate ((uint32) profile_size));

				auto *profile = profileBlock->Buffer_uint8 ();
				
				CheckResult (JxlDecoderGetColorAsICCProfile
							 (dec,
							  
							  JXL_COLOR_PROFILE_TARGET_DATA,
							  profile,
							  profile_size),
							 "JxlDecoderGetColorAsICCProfile",
							 nullptr);

				fColorSpaceInfo.fICCProfile.Reset (profileBlock.Release ());

				#if qDNGValidate

				if (gVerbose)
					{

					printf ("  format: ICC profile\n");

					printf ("  ICC profile size: %u\n", (uint32) profile_size);

					printf ("  CMM type: \"%.4s\"\n", profile + 4);
					printf ("  color space: \"%.4s\"\n", profile + 16);
					printf ("  rendering intent: %d\n", (int) profile [67]);

					}

				#endif	

				}
			
			} 

		else if (status == JXL_DEC_FRAME)
			{

			CheckResult (JxlDecoderGetFrameHeader (dec, &frameHeader),
						 "JxlDecoderGetFrameHeader",
						 &parallelData);

			#if qDNGValidate

			if (gVerbose)
				{
						 
				printf ("frame:\n");

				if (frameHeader.name_length)
					{

					
					
					

					const uint32 nameLen =
						Min_uint32 (frameHeader.name_length, 65535u);

					std::vector<char> temp (nameLen + 1);

					char *name = temp.data ();

					CheckResult (JxlDecoderGetFrameName (dec,
														 name,
														 nameLen + 1),
								 "JxlDecoderGetFrameName",
								 &parallelData);

					printf ("  name: %s\n", name);

					}

				float ms = (frameHeader.duration * 1000.f *
							basicInfo.animation.tps_denominator /
							basicInfo.animation.tps_numerator);

				if (basicInfo.have_animation)
					{

					printf ("  duration: %u ticks (%f ms)\n",
							frameHeader.duration,
							ms);

					if (basicInfo.animation.have_timecodes)
						printf ("  time code: %X\n",
								frameHeader.timecode);

					}

				if (!frameHeader.name_length &&
					!basicInfo.have_animation)
					{
					printf ("  still frame, unnamed\n");
					}

				} 

			#endif	

			}

		

		else if (status == JXL_DEC_BOX)
			{
			
			boxReader.HandleDecBox ();
						 
			}

		

		else if (status == JXL_DEC_BOX_NEED_MORE_OUTPUT)
			{

			#if qLogJXL
			printf ("JXL_DEC_BOX_NEED_MORE_OUTPUT\n");
			#endif

			boxReader.HandleNeedMoreOutput ();

			}

		

		else if (status == JXL_DEC_NEED_IMAGE_OUT_BUFFER)
			{

			const uint32 colorPlanes = basicInfo.num_color_channels;

			if (colorPlanes != 1 &&
				colorPlanes != 3)
				{
				#if qLogJXL
				printf ("jxl: unsupported plane count");
				#endif
				ThrowNotYetImplemented ("unsupported plane count");
				}

			uint32 pixelType = ttByte;
			
			if (basicInfo.exponent_bits_per_sample == 0)
				{
				
				

				if (basicInfo.bits_per_sample <= 8)
					pixelType = ttByte;

				else
					{
					
					pixelType = ttShort;

					#if qLogJXL
					
					if (basicInfo.bits_per_sample > 16)
						printf ("jxl: bits per sample > 16; truncating to 16");

					#endif

					}

				}

			else
				{

				
				
				
				

				pixelType = ttFloat;
				
				}
			
			JxlPixelFormat format;

			
			

			

			
			

			const bool doReadAlpha = ((basicInfo.alpha_bits > 0) &&
									  !fUsePixelBuffer);

			const uint32 totalPlanes = doReadAlpha ? (colorPlanes + 1) : colorPlanes;

			format.num_channels = totalPlanes;

			if (pixelType == ttByte)
				format.data_type = JXL_TYPE_UINT8;				

			else if (pixelType == ttShort)
				format.data_type = JXL_TYPE_UINT16;				

			else
				format.data_type = JXL_TYPE_FLOAT;				

			format.endianness = JXL_NATIVE_ENDIAN;

			format.align = 0;

			#if 1

			
			

			dng_point size;

			DNG_REQUIRE (basicInfo.xsize <= 0x7fffffff &&
						 basicInfo.ysize <= 0x7fffffff,
						 "Image size exceeds supported size.");

			size.h = basicInfo.xsize;
			size.v = basicInfo.ysize;

			

			auto &buffer	  = cbData.fTemplateBuffer;

			buffer.fArea	  = dng_rect (size);

			buffer.fPlanes	  = totalPlanes;
			  
			buffer.fPlaneStep = 1;
			buffer.fColStep	  = totalPlanes;
			buffer.fRowStep	  = SafeInt32Mult (buffer.fColStep,
											   size.h);

			buffer.fPixelType = pixelType;
			buffer.fPixelSize = TagTypeSize (buffer.fPixelType);

			buffer.fData	  = nullptr;

			

			if (!fUsePixelBuffer)
				{

				cbData.fImage.Reset (host.Make_dng_image (dng_rect (size),
														  colorPlanes,
														  pixelType));

				if (doReadAlpha)
					{
					
					cbData.fAlphaMask.Reset (host.Make_dng_image (dng_rect (size),
																  1,
																  pixelType));

					cbData.fAlphaPremultiplied = basicInfo.alpha_premultiplied;
					
					}

				}

			
			
			

			auto &wholeBuffer = cbData.fWholeBuffer;

			wholeBuffer			   = buffer;

			wholeBuffer.fColStep   = 1;
			wholeBuffer.fPlaneStep = size.h;
			wholeBuffer.fRowStep   = SafeInt32Mult (wholeBuffer.fPlaneStep,
													totalPlanes);

			const uint64 bytesNeeded =
				(uint64) SafeInt64Mult (int64 (wholeBuffer.fRowStep),
										int64 (wholeBuffer.fArea.H ()),
										int64 (wholeBuffer.fPixelSize));

			cbData.fBlock.Reset (new jxl_memory_block (host.Allocator (),
													   bytesNeeded));
			
			cbData.fWholeBuffer.fData = cbData.fBlock->Buffer ();
			  
			CheckResult
				(JxlDecoderSetImageOutCallback
				 (dec,
				  &format,
				  dng_jxl_decoder_callback_func,
				  &cbData),
				 "JxlDecoderSetImageOutCallback",
				 &parallelData);

			#else

			(void) dng_jxl_decoder_callback_func;

			

			size_t size;

			CheckResult (JxlDecoderImageOutBufferSize (dec,
													   &format,
													   &size),
						 "JxlDecoderImageOutBufferSize",
						 &parallelData);

			DNG_REQUIRE (size <= 0xffffffff,
						 "JxlDecoderImageOutBufferSize too large");

			block.Reset (host.Allocate ((uint32) size));

			CheckResult (JxlDecoderSetImageOutBuffer (dec,
													  &format,
													  block->Buffer (),
													  size),
						 "JxlDecoderSetImageOutBuffer",
						 &parallelData);

			#endif	
			  
			}

		
		

		else if (status == JXL_DEC_FULL_IMAGE)
			{

			#if qLogJXL
			printf ("--- JXL_DEC_FULL_IMAGE\n");
			#endif

			if (needImage)
				{

				if (cbData.fImage.Get ())
					{
				
					#if 1

					

					auto &dstImage = *cbData.fImage;

					dng_put_buffer_task task (cbData.fWholeBuffer,
											  dstImage);

					host.PerformAreaTask (task,
										  dstImage.Bounds ());

					#else

					

					cbData.fImage->Put (cbData.fWholeBuffer);

					#endif

					}

				if (cbData.fAlphaMask.Get ())
					{
					
					auto &dstImage = *cbData.fAlphaMask;

					dng_put_alpha_buffer_task task (cbData.fWholeBuffer,
													dstImage);

					host.PerformAreaTask (task,
										  dstImage.Bounds ());
					
					}

				}

			}

		

		else
			{

			#if qLogJXL

			printf ("Unexpected jxl decoder status 0x%x\n",
					(unsigned) status);

			#endif

			ThrowBadFormat ("unhandled jxl decoder status");

			}
		
		} 

	

	
	

	if (foundBasicInfo)
		{

		const auto &src = basicInfo;

		if (fInfo)
			{

			auto &info = *fInfo;

			

			info.fMagic = tcJXL;

			

			if (!info.fExif.Get ())
				{
				info.fExif.Reset (host.Make_dng_exif ());
				}

			if (!info.fShared.Get ())
				{
				info.fShared.Reset (host.Make_dng_shared ());
				}

			
			

			if (fC2PAManifestOffset &&
				!info.fShared->fC2PAManifestOffset)
				{

				info.fShared->fC2PAManifestOffset = fC2PAManifestOffset;
				info.fShared->fC2PAManifestCount  = fC2PAManifestRawSize;

				}

			if (info.IFDCount () == 0)
				{
				info.fIFD.push_back (host.Make_dng_ifd ());
				}

			
			

			auto &ifd = *info.fIFD [0];

			ifd.fImageWidth        = (uint32) src.xsize;
			ifd.fImageLength       = (uint32) src.ysize;

			ifd.fBitsPerSample [0] = (uint32) src.bits_per_sample;

			ifd.fSamplesPerPixel   = (uint32) src.num_color_channels;
			
			ifd.fOrientation	   = (uint32) src.orientation;

			if (src.exponent_bits_per_sample > 0)
				ifd.fSampleFormat [0] = sfFloatingPoint;

			else
				ifd.fSampleFormat [0] = sfUnsignedInteger;

			
			
			ifd.fTileOffset [0]    = (uint64) startPosition;
			
			}

		

		fUsesOriginalProfile   = (src.uses_original_profile != JXL_FALSE);

		fMainImageSize.h	   = (int32) src.xsize;
		fMainImageSize.v	   = (int32) src.ysize;

		fMainImagePlanes	   = (uint32) src.num_color_channels;
		
		fBitsPerSample		   = (uint32) src.bits_per_sample;
		fExponentBitsPerSample = (uint32) src.exponent_bits_per_sample;
		
		fNumExtraChannels	   = (uint32) src.num_extra_channels;
		
		fHasPreview			   = (src.have_preview != JXL_FALSE);

		fOrientation.SetTIFF ((uint32) src.orientation);

		

		if (fUsePixelBuffer)
			{

			fMainPixelBuffer.Reset
				(new dng_pixel_buffer (cbData.fWholeBuffer));

			DNG_REQUIRE (!cbData.fBlock->fUseMalloc,
						 "Expected dng_memory_block use for pixel buffer JXL decode");
			
			DNG_REQUIRE (cbData.fBlock->fBlock.Get (),
						 "Missing dng_memory_block for pixel buffer JXL decode");
			
			fMainBlock.Reset (cbData.fBlock->fBlock.Release ());

			}

		else
			{
			
			fMainImage.Reset (cbData.fImage.Release ());

			fAlphaMask.Reset (cbData.fAlphaMask.Release ());

			if (fAlphaMask.Get ())
				fAlphaPremultiplied = cbData.fAlphaPremultiplied;

			}

		

		
		
		}
	
	}

void dng_jxl_decoder::ProcessExifBox (dng_host &host,
									  const std::vector<uint8> &data)
	{

	

	if (fInfo && (data.size () > 4))
		{

		if (data.size () - 4 > 0xFFFFFFFF)
			{
			ThrowBadFormat ("Exif box too large");
			}

		dng_stream exifStream ((&data [0]) + 4,
							   uint32 (data.size () - 4));

		fInfo->Parse (host, exifStream);

		}
	
	}

void dng_jxl_decoder::ProcessXMPBox (dng_host &host,
									 const std::vector<uint8> &data)
	{
	
	#if qDNGValidate

	if (gVerbose)
		{

		if (data.size () > 0xFFFFFFFF)
			{
			ThrowBadFormat ("XMP box too large");
			}

		dng_memory_stream temp (host.Allocator ());

		uint32 count = (uint32) data.size ();

		temp.Put (data.data (), count);

		temp.SetReadPosition (0);

		DumpXMP (temp, count);

		}

	#endif	

	if (fInfo)
		{

		try
			{

			if (data.size () > 0xFFFFFFFF)
				{
				ThrowBadFormat ("XMP box too large");
				}

			dng_xmp xmp (host.Allocator ());

			uint32 count = (uint32) data.size ();

			xmp.Parse (host, data.data (), count);

			auto &block = fInfo->fXMPBlock;

			if (!block.Get ())
				{

				block.Reset (host.Allocate (count));

				memcpy (block->Buffer (),
						data.data (),
						(size_t) count);

				}

			}

		catch (...)
			{

			

			}

		}
	
	}

void dng_jxl_decoder::ProcessBox (dng_host & ,
								  const dng_string &name,
								  const std::vector<uint8> &data)
	{

	
	
	
	
	
	
	
	
	

	if (name.Matches ("jumb", true)                              &&
		data.size () >= 12                                       &&
		memcmp (data.data () + 4, "jumdc2pa", 8) == 0           &&
		!fC2PAManifestOffset)
		{

		
		

		fC2PAManifestOffset  = fCurrentBoxStreamOffset;
		fC2PAManifestRawSize = (uint32) fCurrentBoxRawSize;

		}

	}

static bool DoParseJXL (dng_host &host,
						dng_stream &stream,
						dng_info &info)
	{
	
	try
		{

		dng_jxl_decoder decoder;

		decoder.fInfo = &info;

		decoder.fNeedImage = false;

		decoder.Decode (host,
						stream);

		return true;
		
		}

	catch (const dng_exception &except)
		{

		
		
		
		if (except.ErrorCode () == dng_error_jxl_decoder)
			{
			return false;
			}

		
		

		

		throw;
		
		}

	return false;
	
	}

static bool ParseJXL_Basic (dng_host &host,
							dng_stream &stream,
							dng_info &info)
	{
	
	if (stream.Length () < 2)
		{
		return false;
		}

	const auto pos = stream.Position ();

	
		
	if (stream.Get_uint8 () != 0xFF)
		{
		return false;
		}
		
	if (stream.Get_uint8 () != 0x0A)
		{
		return false;
		}

	#if qDNGValidate
	
	if (gVerbose)
		{
		printf ("\nJXL (basic) file found\n");
		}
		
	#endif
	
	

	stream.SetReadPosition (pos);

	return DoParseJXL (host, stream, info);

	}

static bool ParseJXL_Container (dng_host &host,
								dng_stream &stream,
								dng_info &info)
	{

	
	
	if (stream.Length () < 12)
		{
		return false;
		}

	const auto pos = stream.Position ();

	uint8 buffer [12];

	stream.Get (buffer, 12);

	uint8 jxlBuffer [] =
		{
		0x00, 0x00, 0x00, 0x0C,
		0x4A, 0x58, 0x4C, 0x20,
		0x0D, 0x0A, 0x87, 0x0A
		};

	if (memcmp (buffer, jxlBuffer, 12) != 0)
		{
		return false;
		}

	#if qDNGValidate
	
	if (gVerbose)
		{
		printf ("\nJXL (container) file found\n");
		}
		
	#endif
	
	

	stream.SetReadPosition (pos);

	return DoParseJXL (host, stream, info);

	}

bool ParseJXL (dng_host &host,
			   dng_stream &stream,
			   dng_info &info,
			   bool supportBasicCodeStream,
			   bool supportContainer)
	{
	
	const uint64 startPosition = stream.Position ();
	
	if (supportBasicCodeStream)
		{

		if (ParseJXL_Basic (host, stream, info))
			return true;

		stream.SetReadPosition (startPosition);

		}

	if (supportContainer)
		{
		
		if (ParseJXL_Container (host, stream, info))
			return true;
		
		}

	return false;

	}

void EncodeJXL_Tile (dng_host &host,
					 dng_stream &stream,
					 const dng_image &image,
					 const dng_jxl_color_space_info &colorSpaceInfo,
					 const dng_jxl_encode_settings &settings)
	{
	
	EncodeJXL (host,
			   stream,
			   image,
			   settings,
			   false,						 
			   colorSpaceInfo,
			   nullptr,						 
			   false,
			   false,
			   false,
			   nullptr);					 
	
	}

void EncodeJXL_Tile (dng_host &host,
					 dng_stream &stream,
					 const dng_pixel_buffer &buffer,
					 const dng_jxl_color_space_info &colorSpaceInfo,
					 const dng_jxl_encode_settings &settings)
	{
	
	EncodeJXL (host,
			   stream,
			   buffer,
			   settings,
			   false,						 
			   colorSpaceInfo,
			   nullptr,						 
			   false,
			   false,
			   false,
			   nullptr);					 
	
	}

void EncodeJXL_Container (dng_host &host,
						  dng_stream &stream,
						  const dng_image &image,
						  const dng_jxl_encode_settings &settings,
						  const dng_jxl_color_space_info &colorSpaceInfo,
						  const dng_metadata *metadata,
						  const bool includeExif,
						  const bool includeXMP,
						  const bool includeIPTC,
						  const dng_bmff_box_list *additionalBoxes)
	{

	EncodeJXL (host,
			   stream,
			   image,
			   settings,
			   true,						 
			   colorSpaceInfo,
			   metadata,
			   includeExif,
			   includeXMP,
			   includeIPTC,
			   additionalBoxes);

	}

void EncodeJXL_Container (dng_host &host,
						  dng_stream &stream,
						  const dng_pixel_buffer &buffer,
						  const dng_jxl_encode_settings &settings,
						  const dng_jxl_color_space_info &colorSpaceInfo,
						  const dng_metadata *metadata,
						  const bool includeExif,
						  const bool includeXMP,
						  const bool includeIPTC,
						  const dng_bmff_box_list *additionalBoxes)
	{

	EncodeJXL (host,
			   stream,
			   buffer,
			   settings,
			   true,						 
			   colorSpaceInfo,
			   metadata,
			   includeExif,
			   includeXMP,
			   includeIPTC,
			   additionalBoxes);

	}

real32 JXLQualityToDistance (uint32 quality)
	{

	quality = Pin_uint32 (kMinJXLCompressionQuality,
						  quality,
						  kMaxJXLCompressionQuality);

	

	static const real32 kTable [] =
		{
		0.0f,								 
		8.0f,								 
		6.0f,								 
		5.0f,								 
		4.0f,								 
		3.0f,								 
		2.0f,								 
		1.6f,								 
		1.3f,								 
		1.0f,								 
		0.7f,								 
		0.4f,								 
		0.1f,								 
		0.0f,								 
		};

	return kTable [quality];

	}

dng_jxl_encode_settings * JXLQualityToSettings (uint32 quality)
	{
	
	AutoPtr<dng_jxl_encode_settings> settings (new dng_jxl_encode_settings);

	real32 d = JXLQualityToDistance (quality);

	settings->SetDistance (d);

	if (d <= 0.0f)
		settings->SetFastest ();			 

	return settings.Release ();
	
	}

void PreviewColorSpaceToJXLEncoding (const PreviewColorSpaceEnum colorSpace,
									 const uint32 planes,
									 dng_jxl_color_space_info &info)
	{

	#if qLogJXL
	printf ("jxl preview info color space enum: %d\n",
			int (colorSpace));
	#endif

	info.fJxlColorEncoding.Reset (new JxlColorEncoding);

	auto &encoding = *info.fJxlColorEncoding;

	memset (&encoding, 0, sizeof (encoding));

	switch (colorSpace)
		{

		case previewColorSpace_GrayGamma22:
			{
			encoding.color_space	   = JXL_COLOR_SPACE_GRAY;
			encoding.white_point	   = JXL_WHITE_POINT_D65; 
			encoding.primaries		   = JXL_PRIMARIES_SRGB;  
			encoding.transfer_function = JXL_TRANSFER_FUNCTION_GAMMA;
			encoding.gamma			   = 1.0 / 2.2;
			break;
			}

		case previewColorSpace_sRGB:
			{
			encoding.color_space	   = JXL_COLOR_SPACE_RGB;
			encoding.white_point	   = JXL_WHITE_POINT_D65;
			encoding.primaries		   = JXL_PRIMARIES_SRGB;
			encoding.transfer_function = JXL_TRANSFER_FUNCTION_SRGB;
			break;
			}

		case previewColorSpace_AdobeRGB:
			{
			
			encoding.color_space	   = JXL_COLOR_SPACE_RGB;
			encoding.white_point	   = JXL_WHITE_POINT_D65;
			encoding.primaries		   = JXL_PRIMARIES_CUSTOM;
			encoding.transfer_function = JXL_TRANSFER_FUNCTION_GAMMA;
			encoding.gamma			   = 1.0 / 2.2;

			encoding.primaries_red_xy   [0] = 0.6400;
			encoding.primaries_red_xy   [1] = 0.3300;

			encoding.primaries_green_xy [0] = 0.2100;
			encoding.primaries_green_xy [1] = 0.7100;

			encoding.primaries_blue_xy  [0] = 0.1500;
			encoding.primaries_blue_xy  [1] = 0.0600;

			break;
			
			}

		case previewColorSpace_ProPhotoRGB:
			{

			encoding.color_space            = JXL_COLOR_SPACE_RGB;
			encoding.white_point            = JXL_WHITE_POINT_CUSTOM;
			encoding.primaries              = JXL_PRIMARIES_CUSTOM;
			encoding.transfer_function      = JXL_TRANSFER_FUNCTION_GAMMA;
			encoding.gamma                  = 1.0 / 1.8;

			encoding.primaries_red_xy   [0] = 0.734699;
			encoding.primaries_red_xy   [1] = 0.265301;

			encoding.primaries_green_xy [0] = 0.159597;
			encoding.primaries_green_xy [1] = 0.840403;

			encoding.primaries_blue_xy  [0] = 0.036598;
			encoding.primaries_blue_xy  [1] = 0.000105;

			dng_xy_coord white              = D50_xy_coord ();

			encoding.white_point_xy [0]     = white.x;
			encoding.white_point_xy [1]     = white.y;
			
			break;
			
			}

		case previewColorSpace_Unknown:
		default:
			{

			

			
			
			
			

			if (planes == 1)
				{
				encoding.color_space	   = JXL_COLOR_SPACE_GRAY;
				encoding.white_point	   = JXL_WHITE_POINT_D65; 
				encoding.primaries		   = JXL_PRIMARIES_2100;  
				encoding.transfer_function = JXL_TRANSFER_FUNCTION_LINEAR;
				}
			
			else
				{
				encoding.color_space	   = JXL_COLOR_SPACE_RGB;
				encoding.white_point	   = JXL_WHITE_POINT_D65;
				encoding.primaries		   = JXL_PRIMARIES_2100;
				encoding.transfer_function = JXL_TRANSFER_FUNCTION_LINEAR;
				}

			break;
			
			}
		
		}
	
	}

bool SupportsJXL (const dng_image &image)
	{
	
	
	

	

	uint32 planes	 = image.Planes ();
	uint32 pixelType = image.PixelType ();

	return ((planes == 1 ||					 
			 planes == 3)					 

			&&

			(pixelType == ttByte	  ||
			 pixelType == ttShort	  ||
			 pixelType == ttHalfFloat ||
			 pixelType == ttFloat));
	
	}

