

#ifndef __dng_uncopyable__
#define __dng_uncopyable__

class dng_uncopyable
	{

	protected:

		dng_uncopyable ()
			{
			}

		~dng_uncopyable ()
			{
			}

	private:
		
		dng_uncopyable (const dng_uncopyable &);
		
		dng_uncopyable & operator= (const dng_uncopyable &);
		
	};

#endif	
	

