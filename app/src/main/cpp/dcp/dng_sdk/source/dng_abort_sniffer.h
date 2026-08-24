

#ifndef __dng_abort_sniffer__
#define __dng_abort_sniffer__

#include "dng_classes.h"
#include "dng_flags.h"
#include "dng_string.h"
#include "dng_types.h"
#include "dng_uncopyable.h"

enum dng_priority
	{
	
	dng_priority_background = 0,
	dng_priority_low		= 1,
	dng_priority_medium		= 2,
	dng_priority_high		= 3,
	dng_priority_very_high	= 4,
	
	dng_priority_count,
	
	dng_priority_minimum = dng_priority_background,
	dng_priority_maximum = dng_priority_very_high
	
	};

class dng_set_minimum_priority
	{
	
	private:
	
		dng_priority fPriority;

		dng_string fName;
	
	public:
	
		dng_set_minimum_priority (dng_priority priority,
								  const char *name);
		
		~dng_set_minimum_priority ();
	
	};

class dng_abort_sniffer
	{
	
	friend class dng_sniffer_task;
	
	private:
	
		dng_priority fPriority;

	public:
	
		dng_abort_sniffer ();

		virtual ~dng_abort_sniffer ();

		
		
		dng_priority Priority () const
			{
			return fPriority;
			}
			
		
		
		void SetPriority (dng_priority priority);

		
		
		
		
		
		

		static void SniffForAbort (dng_abort_sniffer *sniffer);
		
		
		
		void SniffNoPriorityWait ()
			{
			Sniff ();
			}
	
		
		
		
		
		virtual bool ThreadSafe () const
			{
			return false;
			}

		
		
		
		
		
		virtual bool SupportsPriorityWait () const;

		
		

		virtual real64 SuggestedTimeBetweenSniffs () const
			{
			return 0.1;
			}

		
		
		
		
		

		virtual void SniffSlowHint ()
			{
			}

	protected:
	
		
		

		virtual void Sniff () = 0;
		
		
		
		
		
		

		virtual void StartTask (const char *name,
								real64 fract);

		

		virtual void EndTask ();

		
		
		

		virtual void UpdateProgress (real64 fract);

	};

class dng_sniffer_task: private dng_uncopyable
	{
	
	private:
	
		dng_abort_sniffer *fSniffer;
	
	public:
	
		
		
		
		
		
		

		dng_sniffer_task (dng_abort_sniffer *sniffer,
						  const char *name = NULL,
						  real64 fract = 0.0)
					 
			:	fSniffer (sniffer)
			
			{
			if (fSniffer)
				fSniffer->StartTask (name, fract);
			}
			
		virtual ~dng_sniffer_task ();

		
		

		void Sniff ()
			{
			dng_abort_sniffer::SniffForAbort (fSniffer);
			}

		
		
		

		void UpdateProgress (real64 fract)
			{
			if (fSniffer)
				fSniffer->UpdateProgress (fract);
			}
			
		
		
		

		void UpdateProgress (uint32 done,
							 uint32 total)
			{
			UpdateProgress ((real64) done /
							(real64) total);
			}
		
		

		void Finish ()
			{
			UpdateProgress (1.0);
			}
			
	};

#endif

