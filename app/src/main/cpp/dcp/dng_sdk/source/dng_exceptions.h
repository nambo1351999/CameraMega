

#ifndef __dng_exceptions__
#define __dng_exceptions__

#include "dng_errors.h"
#include "dng_flags.h"

#include <exception>
#include <string>

#if defined(DNG_NO_RETURN) != defined(DNG_NO_RETURN_ENABLED)
#error DNG_NO_RETURN and DNG_NO_RETURN_ENABLED must both be defined or undefined here.
#endif

#ifndef DNG_NO_RETURN
#if defined(__cplusplus)
#if __cplusplus >= 201103L
#define DNG_NO_RETURN [[noreturn]]
#define DNG_NO_RETURN_ENABLED 1
#endif
#endif
#endif

#ifndef DNG_NO_RETURN
#ifdef __GNUC__
#define DNG_NO_RETURN __attribute__((noreturn))
#define DNG_NO_RETURN_ENABLED 1
#else
#define DNG_NO_RETURN
#define DNG_NO_RETURN_ENABLED 0
#endif
#endif

void ReportWarning (const char *message,
					const char *sub_message = NULL);
	

void ReportError (const char *message,
				  const char *sub_message = NULL);
	

class dng_exception : public std::exception
	{
	
	private:
	
		dng_error_code fErrorCode;
		
		std::string fErrorMsg;
	
	public:
	
		
		
		
		dng_exception (dng_error_code code)
		
			: fErrorCode (code)

			
			{
			fErrorMsg = "dng_error: ";
			fErrorMsg += std::to_string (fErrorCode);
			}
		
		#if qDNGVerboseExceptions
		
		dng_exception (dng_error_code code,
					   const char *message,
					   const char *sub_message)

			: fErrorCode (code)

			{
			
			fErrorMsg = "dng_error: ";
			fErrorMsg += std::to_string (fErrorCode);
			
			if (message)
				{
				fErrorMsg += ": ";
				fErrorMsg += message;
				}
			
			if (sub_message)
				{
				fErrorMsg += " (";
				fErrorMsg += sub_message;
				fErrorMsg += ")";
				}
			
			}
		
		#endif	

		virtual ~dng_exception ()
			{ 
			}

		
		
		
		dng_error_code ErrorCode () const
			{
			return fErrorCode;
			}

		virtual char const * what () const noexcept override
			{
			return fErrorMsg.c_str ();
			}

	};
	

DNG_NO_RETURN void Throw_dng_error (dng_error_code err,
									const char * message = NULL,
									const char * sub_message = NULL,
									bool silent = false);

inline void Fail_dng_error (dng_error_code err)
	{
	
	if (err != dng_error_none)
		{
		
		Throw_dng_error (err);
		
		}
		
	}

DNG_NO_RETURN inline void ThrowProgramError (const char * sub_message = NULL)
	{
	
	Throw_dng_error (dng_error_unknown, NULL, sub_message);
	
	}

DNG_NO_RETURN inline void ThrowOverflow (const char * sub_message = NULL)
	{
	
	Throw_dng_error (dng_error_overflow, NULL, sub_message);
	
	}

DNG_NO_RETURN inline void ThrowNotYetImplemented (const char * sub_message = NULL)
	{
	
	Throw_dng_error (dng_error_not_yet_implemented, NULL, sub_message);
	
	}

DNG_NO_RETURN inline void ThrowSilentError (const char *sub_message = NULL)
	{
	
	Throw_dng_error (dng_error_silent, NULL, sub_message);
	
	}

DNG_NO_RETURN inline void ThrowUserCanceled ()
	{
	
	Throw_dng_error (dng_error_user_canceled);
	
	}

DNG_NO_RETURN inline void ThrowHostInsufficient (const char * sub_message = NULL,
								   bool silent = false)
	{
	
	Throw_dng_error (dng_error_host_insufficient, NULL, sub_message, silent);
	
	}

DNG_NO_RETURN inline void ThrowMemoryFull (const char * sub_message = NULL)
	{
	
	Throw_dng_error (dng_error_memory, NULL, sub_message);
	
	}

DNG_NO_RETURN inline void ThrowBadFormat (const char * sub_message = NULL)
	{
	
	Throw_dng_error (dng_error_bad_format, NULL, sub_message);
	
	}

DNG_NO_RETURN inline void ThrowMatrixMath (const char * sub_message = NULL)
	{
	
	Throw_dng_error (dng_error_matrix_math, NULL, sub_message);
	
	}

DNG_NO_RETURN inline void ThrowOpenFile (const char * sub_message = NULL, bool silent = false)
	{
	
	Throw_dng_error (dng_error_open_file, NULL, sub_message, silent);
	
	}

DNG_NO_RETURN inline void ThrowReadFile (const char *sub_message = NULL)
	{
	
	Throw_dng_error (dng_error_read_file, NULL, sub_message);
	
	}

DNG_NO_RETURN inline void ThrowWriteFile (const char *sub_message = NULL)
	{
	
	Throw_dng_error (dng_error_write_file, NULL, sub_message);
	
	}

DNG_NO_RETURN inline void ThrowEndOfFile (const char *sub_message = NULL)
	{
	
	Throw_dng_error (dng_error_end_of_file, NULL, sub_message);
	
	}

DNG_NO_RETURN inline void ThrowFileIsDamaged ()
	{
	
	Throw_dng_error (dng_error_file_is_damaged);
	
	}

DNG_NO_RETURN inline void ThrowImageTooBigDNG ()
	{
	
	Throw_dng_error (dng_error_image_too_big_dng);
	
	}

DNG_NO_RETURN inline void ThrowImageTooBigTIFF ()
	{
	
	Throw_dng_error (dng_error_image_too_big_tiff);
	
	}

DNG_NO_RETURN inline void ThrowUnsupportedDNG ()
	{
	
	Throw_dng_error (dng_error_unsupported_dng);
	
	}

#endif
	

