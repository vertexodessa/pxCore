#include "rtMutexNative.h"


rtMutexNative::rtMutexNative()
{
  InitializeCriticalSection(&mMutex);
}

rtMutexNative::~rtMutexNative()
{
}

void rtMutexNative::lock()
{
  EnterCriticalSection(&mMutex);
}

void rtMutexNative::unlock()
{
  LeaveCriticalSection(&mMutex);
}

rtMutexNativeDesc rtMutexNative::getNativeMutexDescription()
{
  return &mMutex;
}

rtThreadConditionNative::rtThreadConditionNative()
{
  InitializeConditionVariable(&mCond);
}

rtThreadConditionNative::~rtThreadConditionNative()
{

}

void rtThreadConditionNative::wait(rtMutexNativeDesc desc)
{
  SleepConditionVariableCS(&mCond, desc, INFINITE);
}

void rtThreadConditionNative::signal()
{
  WakeConditionVariable(&mCond);
}

void rtThreadConditionNative::broadcast()
{
  WakeAllConditionVariable(&mCond);
}