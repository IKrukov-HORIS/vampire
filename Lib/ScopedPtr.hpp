/**
 * @file ScopedPtr.hpp
 * Defines class ScopedPtr.
 */

#ifndef __ScopedPtr__
#define __ScopedPtr__

#include "Forwards.hpp"

#include "Debug/Assertion.hpp"
#include "Debug/Tracer.hpp"

namespace Lib
{

/**
 * Wrapper containing a pointer to an object which is deleted
 * when the wrapper is destroyed
 */
template<typename T>
class ScopedPtr {
private:
  ScopedPtr(const ScopedPtr& ptr);
  ScopedPtr& operator=(const ScopedPtr& ptr);
public:
  inline
  ScopedPtr() : _obj(0) {}
  /**
   * Create a scoped pointer containing pointer @b obj
   */
  inline
  explicit ScopedPtr(T* obj)
  : _obj(obj) {ASS(obj);}
  inline
  ~ScopedPtr()
  {
    if(_obj) {
      checked_delete(_obj);
    }
  }
  void operator=(T* obj)
  {
    CALL("SmartPtr::operator=");

    if(_obj) {
      checked_delete(_obj);
    }
    _obj = obj;
  }

  inline
  operator bool() const { return _obj; }

  inline
  T* operator->() const
  {
    CALL("ScopedPtr::operator->");
    ASS(_obj);

    return _obj;
  }
  inline
  T& operator*() const
  {
    CALL("ScopedPtr::operator*");
    ASS(_obj);

    return *_obj;
  }

  inline
  T* ptr() const { return _obj; }

  inline
  bool isEmpty() const { return !_obj; }

  template<class Target>
  inline
  Target* pcast() const { return static_cast<Target*>(_obj); }

private:
  T* _obj;
};

}

#endif // __ScopedPtr__
