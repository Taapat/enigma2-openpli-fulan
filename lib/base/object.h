#ifndef __base_object_h
#define __base_object_h

#include <assert.h>
#include <lib/base/smartptr.h>
#include <lib/base/elock.h>

//#define OBJECT_DEBUG

#include <lib/base/eerror.h>

typedef int RESULT;

class iObject
{
private:
		/* we don't allow the default operator here, as it would break the refcount. */
	void operator=(const iObject &);
protected:
	void operator delete(void *p) { ::operator delete(p); }
	virtual ~iObject() { }
#ifdef SWIG
	virtual void AddRef()=0;
	virtual void Release()=0;
#endif
public:
#ifndef SWIG
	virtual void AddRef()=0;
	virtual void Release()=0;
#endif
};

#ifndef SWIG

class oRefCount
{
        int ref;
public:
	oRefCount(): ref(0) {}
        operator int&() {return ref;}
#ifdef OBJECT_DEBUG
      ~oRefCount()
      { 
         if (ref)
            eDebug("OBJECT_DEBUG FATAL: %p has %d references!", this, ref);
         else
            eDebug("OBJECT_DEBUG refcount ok! (%p)", this); 
      }
#endif       
};

		#define DECLARE_REF(x) 			\
			public: \
					void AddRef(); 		\
					void Release();		\
			private:\
					oRefCount ref;

		#define DEFINE_REF(c) \
			void c::AddRef() \
			{ \
				++ref; \
			} \
			void c::Release() \
	 		{ \
				if (!(--ref)) \
					delete this; \
			}

#else  // SWIG
	#define DECLARE_REF(x) \
		private: \
			void AddRef(); \
			void Release();
#endif  // SWIG

#endif  // __base_object_h
