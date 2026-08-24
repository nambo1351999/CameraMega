

#ifndef __dng_auto_ptr__
#define __dng_auto_ptr__

#include <memory>
#include <stddef.h>
#include <stdlib.h>

#include "dng_memory.h"
#include "dng_uncopyable.h"

template<class T>
class AutoPtr: private dng_uncopyable
	{
	
	private:
	
		T *p_;
		
	public:

		

		AutoPtr () : p_ (0) { }
	
		
		
		

		explicit AutoPtr (T *p) :  p_( p ) { }

		

		~AutoPtr ();

		

		void Alloc ();

		
		

		T *Get () const { return p_; }

		
		

		T *Release ();

		
		
		
		

		void Reset (T *p);

		
		

		void Reset ();

		
		

		T *operator-> () const { return p_; }

		
		

		T &operator* () const { return *p_; }
		
		
		
		friend inline void Swap (AutoPtr< T > &x, AutoPtr< T > &y)
			{
			T* temp = x.p_;
			x.p_ = y.p_;
			y.p_ = temp;
			}
		
	};

template<class T>
AutoPtr<T>::~AutoPtr ()
	{
	
	delete p_;
	p_ = 0;
	
	}

template<class T>
T *AutoPtr<T>::Release ()
	{
	T *result = p_;
	p_ = 0;
	return result;
	}

template<class T>
void AutoPtr<T>::Reset (T *p)
	{
	
	if (p_ != p)
		{
		if (p_ != 0)
			delete p_;
		p_ = p;
		}
	
	}

template<class T>
void AutoPtr<T>::Reset ()
	{
	
	if (p_ != 0)
		{
		delete p_;
		p_ = 0;
		}
	
	}

template<class T>
void AutoPtr<T>::Alloc ()
	{
	this->Reset (new T);
	}

template<typename T>
class AutoArray: private dng_uncopyable
	{

	public:
		
		

		AutoArray () { }
		
		
		
		

		explicit AutoArray (size_t count)
			: fVector (new dng_std_vector<T> (count))
			{
			}

		
		
		
		

		void Reset (size_t count)
			{
			fVector.reset (new dng_std_vector<T> (count));
			}

		
		
		

		T &operator[] (ptrdiff_t i)
			{
			return (*fVector) [i];
			}
		const T &operator[] (ptrdiff_t i) const
			{
			return (*fVector) [i];
			}

		

		T *Get ()
			{
			if (fVector)
				return fVector->data ();
			else
				return nullptr;
			}
		
		const T *Get () const
			{
			if (fVector)
				return fVector->data ();
			else
				return nullptr;
			}

	private:

		

		AutoArray (const AutoArray &);

		const AutoArray & operator= (const AutoArray &);

	private:

		std::unique_ptr<dng_std_vector<T> > fVector;

	};

#endif

