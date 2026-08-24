

#include "dng_string_list.h"

dng_string_list::dng_string_list ()

	:	fList ()
	
	{
	
	}

dng_string_list::~dng_string_list ()
	{
	
	}

void dng_string_list::Allocate (uint32 minSize)
	{
	
	fList.reserve (minSize);
	
	}
					 

void dng_string_list::Insert (uint32 index, 
							  const dng_string &s)
	{
	
	fList.insert (fList.begin () + index, s);
		
	}
					 

bool dng_string_list::Contains (const dng_string &s) const
	{
	
	for (uint32 j = 0; j < Count (); j++)
		{
		
		if ((*this) [j] == s)
			{
			
			return true;
			
			}
			
		}
		
	return false;
	
	}
					 

void dng_string_list::Clear ()
	{
	
	fList.clear ();
	
	}

