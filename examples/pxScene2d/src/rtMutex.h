#ifndef RT_MUTEX_H
#define RT_MUTEX_H

#include "rtCore.h"

class rtMutex : public rtMutexNative
{
public:
    rtMutex() {}
    ~rtMutex() {}
};

class rtThreadCondition : public rtThreadConditionNative
{
public:
  rtThreadCondition() {}
  ~rtThreadCondition() {}
};

class rtScopedLock
{
public:
  rtScopedLock(rtMutex& m) : mMutex(&m)
  {
    mMutex->lock();
  }

  ~rtScopedLock()
  {
    mMutex->unlock();
  }
private:
  rtScopedLock(const rtScopedLock& ) : mMutex(NULL) { }
  rtScopedLock const& operator = (const rtScopedLock& ) { return *this; }
private:
  rtMutex* mMutex;
};

#endif //RT_MUTEX_H
