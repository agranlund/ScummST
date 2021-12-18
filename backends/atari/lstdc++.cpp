//-------------------------------------------------------------------------
// libstdc++ replacement
//
// This file is distributed under the GPL v2, or at your option any
// later version.  Read COPYING for details.
// (c)2021, Anders Granlund
//-------------------------------------------------------------------------
extern "C" void* malloc(unsigned long n);
extern "C" void free(void* p);
extern "C" void atexit(void (*f)(void));

void* operator new(unsigned long n)
{
	void * const p = malloc(n);
	return p;
}

void* operator new[] (unsigned long n)
{
	void * const p = malloc(n);
	return p;
}

void operator delete(void * p)
{
	free(p);
}

void operator delete[](void* p)
{
	free(p);
}

extern "C" void __cxa_pure_virtual()
{
}

extern "C" void __cxa_guard_acquire()
{
}

extern "C" void __cxa_guard_release()
{
}

extern "C" void __do_global_ctors()
{
	typedef void (*ctor_ptr) (void);
	extern ctor_ptr __CTOR_LIST__[];
	unsigned long count = (unsigned long) __CTOR_LIST__[0];
	for (unsigned long i=1; i<count+1; i++)
	{
		if (__CTOR_LIST__[i] == 0)
			break;
		__CTOR_LIST__[i]();
	}
}

extern "C" void __do_global_dtors()
{
	typedef void (*dtor_ptr) (void);
	extern dtor_ptr __DTOR_LIST__[];
	unsigned long count = (unsigned long) __DTOR_LIST__[0];
	for (unsigned long i=1; i<count+1; i++)
	{
		if (__DTOR_LIST__[i] == 0)
			break;
		__DTOR_LIST__[i]();
	}
}

extern "C" void __main()
{
	__do_global_ctors();
	atexit(__do_global_dtors);
}

