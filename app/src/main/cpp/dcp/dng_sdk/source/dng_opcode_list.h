

#ifndef __dng_opcode_list__
#define __dng_opcode_list__

#include "dng_auto_ptr.h"
#include "dng_classes.h"
#include "dng_memory.h"
#include "dng_opcodes.h"
#include "dng_uncopyable.h"

#include <vector>

class dng_opcode_list: private dng_uncopyable
	{
	
	private:
	
		dng_std_vector<dng_opcode *> fList;
		
		bool fAlwaysApply;
		
		uint32 fStage;
		
	public:

		
	
		dng_opcode_list (uint32 stage);
		
		~dng_opcode_list ();

		
		
		bool IsEmpty () const
			{
			return fList.size () == 0;
			}

		
			
		bool NotEmpty () const
			{
			return !IsEmpty ();
			}

		
			
		bool AlwaysApply () const
			{
			return fAlwaysApply && NotEmpty ();
			}

		
		
			
		void SetAlwaysApply ()
			{
			fAlwaysApply = true;
			}

		
			
		uint32 Count () const
			{
			return (uint32) fList.size ();
			}

		
		
			
		dng_opcode & Entry (uint32 index)
			{
			return *fList [index];
			}

		
		
			
		const dng_opcode & Entry (uint32 index) const
			{
			return *fList [index];
			}

		

		void Clear ();
	
		

		void Remove (uint32 index);

		
		
		void Swap (dng_opcode_list &otherList);

		
		
		
		
		uint32 MinVersion (bool includeOptional) const;
		
		
		

		void Apply (dng_host &host,
					dng_negative &negative,
					AutoPtr<dng_image> &image);

		
					
		void Append (AutoPtr<dng_opcode> &opcode);

		
		
		
		dng_memory_block * Spool (dng_host &host) const;

		
		
		void FingerprintToStream (dng_stream &stream) const;

		
		
		
		
		

		void Parse (dng_host &host,
					dng_stream &stream,
					uint32 byteCount,
					uint64 streamOffset);

		
		
		
		

		void ApplyAreaScale (const dng_urational &scale);
		
	};

#endif
	

