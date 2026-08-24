

#ifndef __dng_date_time__
#define __dng_date_time__

#include "dng_classes.h"
#include "dng_safe_arithmetic.h"
#include "dng_string.h"
#include "dng_types.h"

class dng_date_time
	{
	
	public:
		
		uint32 fYear;
		uint32 fMonth;
		uint32 fDay;
		uint32 fHour;
		uint32 fMinute;
		uint32 fSecond;
		
	public:
	
		

		dng_date_time ();

		
		
		
		
		
		
		

		dng_date_time (uint32 year,
					   uint32 month,
					   uint32 day,
					   uint32 hour,
					   uint32 minute,
					   uint32 second);

		
		

		bool IsValid () const;
		
		
		

		bool NotValid () const
			{
			return !IsValid ();
			}
			
		

		bool operator== (const dng_date_time &dt) const
			{
			return fYear   == dt.fYear	 &&
				   fMonth  == dt.fMonth	 &&
				   fDay	   == dt.fDay	 &&
				   fHour   == dt.fHour	 &&
				   fMinute == dt.fMinute &&
				   fSecond == dt.fSecond;
			}
			
		
		
		bool operator!= (const dng_date_time &dt) const
			{
			return !(*this == dt);
			}
			
		

		void Clear ();

		
		
		

		bool Parse (const char *s);
			
	};
	

class dng_time_zone
	{
	
	private:
	
		enum
			{
			
			kMaxOffsetHours = 15,
			kMinOffsetHours = -kMaxOffsetHours,
			
			kMaxOffsetMinutes = kMaxOffsetHours * 60,
			kMinOffsetMinutes = kMinOffsetHours * 60,
			
			kInvalidOffset = kMinOffsetMinutes - 1
			
			};
			
		
		
	
		int32 fOffsetMinutes;
		
	public:
	
		dng_time_zone ()
			:	fOffsetMinutes (kInvalidOffset)
			{
			}
			
		void Clear ()
			{
			fOffsetMinutes = kInvalidOffset;
			}
			
		void SetOffsetHours (int32 offset)
			{
			fOffsetMinutes = SafeInt32Mult (offset, 60);
			}
			
		void SetOffsetMinutes (int32 offset)
			{
			fOffsetMinutes = offset;
			}
			
		void SetOffsetSeconds (int32 offset)
			{
			fOffsetMinutes = (offset > 0) ? ((offset + 30) / 60)
										  : ((offset - 30) / 60);
			}

		bool IsValid () const
			{
			return fOffsetMinutes >= kMinOffsetMinutes &&
				   fOffsetMinutes <= kMaxOffsetMinutes;
			}
			
		bool NotValid () const
			{
			return !IsValid ();
			}
			
		int32 OffsetMinutes () const
			{
			return fOffsetMinutes;
			}
			
		bool IsExactHourOffset () const
			{
			return IsValid () && ((fOffsetMinutes % 60) == 0);
			}
			
		int32 ExactHourOffset () const
			{
			return fOffsetMinutes / 60;
			}
			
		dng_string Encode_ISO_8601 () const;
			
	};

class dng_date_time_info
	{
	
	private:
	
		
	
		bool fDateOnly;
		
		
		
		dng_date_time fDateTime;
		
		
		
		dng_string fSubseconds;
		
		
		
		dng_time_zone fTimeZone;
		
	public:
	
		dng_date_time_info ();
		
		bool IsValid () const;
		
		bool NotValid () const
			{
			return !IsValid ();
			}
			
		void Clear ()
			{
			*this = dng_date_time_info ();
			}
   
		bool IsDateOnly () const
			{
			return fDateOnly;
			}
			
		const dng_date_time & DateTime () const
			{
			return fDateTime;
			}
			
		void SetDateTime (const dng_date_time &dt)
			{
			fDateOnly = false;
			fDateTime = dt;
			}
			
		const dng_string & Subseconds () const
			{
			return fSubseconds;
			}
			
		void SetSubseconds (const dng_string &s)
			{
			fSubseconds = s;
			}
			
		const dng_time_zone & TimeZone () const
			{
			return fTimeZone;
			}
			
		void SetZone (const dng_time_zone &zone)
			{
			fTimeZone = zone;
			}
   
		void ClearZone ()
			{
			fTimeZone.Clear ();
			}
   
		void SetOffsetTime (const dng_string &s);
		
		dng_string OffsetTime () const;
			
		void Decode_ISO_8601 (const char *s);
		
		dng_string Encode_ISO_8601 () const;
		
		void Decode_IPTC_Date (const char *s);
		
		dng_string Encode_IPTC_Date () const;
	
		void Decode_IPTC_Time (const char *s);
		
		dng_string Encode_IPTC_Time () const;
		
	private:
	
		void SetDate (uint32 year,
					  uint32 month,
					  uint32 day);
					  
		void SetTime (uint32 hour,
					  uint32 minute,
					  uint32 second);
					  
	};

void CurrentDateTimeAndZone (dng_date_time_info &info);

void DecodeUnixTime (uint32 unixTime, dng_date_time &dt);

dng_time_zone LocalTimeZone (const dng_date_time &dt);

enum dng_date_time_format
	{
	dng_date_time_format_unknown			= 0, 
	dng_date_time_format_exif				= 1, 
	dng_date_time_format_unix_little_endian = 2, 
	dng_date_time_format_unix_big_endian	= 3	 
	};

class dng_date_time_storage_info
	{
	
	private:
	
		uint64 fOffset;
		
		dng_date_time_format fFormat;
	
	public:
	
		

		dng_date_time_storage_info ();
		
		

		dng_date_time_storage_info (uint64 offset,
									dng_date_time_format format);
		
		
		

		bool IsValid () const;
		
		

		
		
		

		uint64 Offset () const;
			
		
		
		
		

		dng_date_time_format Format () const;
	
	};

#endif
	

