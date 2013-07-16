//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
//  contact: immarespond at gmail dot com
#ifndef __REFERENCE_COUNTED_OBJECT_H__
#define __REFERENCE_COUNTED_OBJECT_H__


// Reference counted interface. Inherit this class and access it with ReferenceCountedPtr
class ReferenceCountedObject
{
public:
	ReferenceCountedObject():_refNb(0){}
	~ReferenceCountedObject(){}
	void ref()
	{
		_refNb++;
	}
	void unref()
	{
		if ((--_refNb) == 0)
			delete this;
	}
	int count() const
	{
		return _refNb;
	}

private:
	int _refNb;

	ReferenceCountedObject(const ReferenceCountedObject& other);
	void operator=(const ReferenceCountedObject& other);
};

//class defining a pointer to a template reference counted object.

template<class T> class RefCPtr
    {
      T* _obj;
    public:

      RefCPtr<T>() : _obj(0)
      { 
      }

      explicit RefCPtr<T>(T* obj) : _obj(obj)
      { 
        if (obj)
          obj->ref();
      }
 
      RefCPtr<T>(const RefCPtr<T>& other) : _obj(other._obj)
      {
        if (_obj)
          _obj->ref();
      }
      

      ~RefCPtr<T>()
      {
        if (_obj)
          _obj->unref();
      }
      
      void operator=(const RefCPtr<T>& other)
      {
        if (_obj == other._obj)
          return;

        if (_obj)
          _obj->unref();
        _obj = other._obj;
        if (_obj)
          _obj->ref();
      }
     
      T& operator*()
      {
        return *_obj;
      }

      const T& operator*() const
      {
        return *_obj;
      }
      
      T* operator->()
      {
        return _obj;
      }

      const T* operator->() const
      {
        return _obj;
      }

      int count() const 
      {
        if(_obj)
          return _obj->count();
        return 0;
      }
  
      template<class U> static RefCPtr<T> down_cast(RefCPtr<U> other)
      {
        if (T* obj = dynamic_cast<T*>(&*other))
          return RefCPtr<T>(obj);
        return RefCPtr<T>();
      }
    
      operator bool() const
      {
        return _obj != 0;
      }
   
      void allocate()
      {
        if (_obj) {
          _obj->unref();
        }
        _obj = new T();
        _obj->ref();
      }

  
      template<class P1> void allocate(const P1& p1)
      {
        if (_obj) {
          _obj->unref();
        }
        _obj = new T(p1);
        _obj->ref();
      }
      
      template<class P1, class P2> void allocate(const P1& p1, const P2& p2)
      {
        if (_obj) {
          _obj->unref();
        }
        _obj = new T(p1, p2);
        _obj->ref();
      }
      
   
      template<class P1, class P2, class P3> void allocate(const P1& p1, const P2& p2, const P3& p3)
      {
        if (_obj) {
          _obj->unref();
        }
        _obj = new T(p1, p2, p3);
        _obj->ref();
      }
    
      void clear()
      {
        if (_obj) {
          _obj->unref();
        }
        _obj = 0;
      }
    };

  




#endif //__REFERENCE_COUNTED_OBJECT_H__