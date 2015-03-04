#ifndef RT_MUTEX_NATIVE_H
#define RT_MUTEX_NATIVE_H

#include <Windows.h>
typedef PCRITICAL_SECTION rtMutexNativeDesc;

class rtThreadConditionNative;

class rtMutexNative
{
  friend class rtThreadConditionNative;

public:
  rtMutexNative();
  ~rtMutexNative();
  void lock();
  void unlock();
  rtMutexNativeDesc getNativeMutexDescription();
private:
  CRITICAL_SECTION mMutex;
};

class rtThreadConditionNative
{
public:
  rtThreadConditionNative();
  ~rtThreadConditionNative();
  void wait(rtMutexNativeDesc mutexNativeDesc);
  void signal();
  void broadcast();
private:
  CONDITION_VARIABLE mCond;
};

#endif //RT_MUTEX_NATIVE_H