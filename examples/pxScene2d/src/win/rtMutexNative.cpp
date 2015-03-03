#include "rtMutexNative.h"


rtMutexNative::rtMutexNative()
{
  mMutex = CreateMutex(NULL, FALSE, NULL);
}

rtMutexNative::~rtMutexNative()
{
  CloseHandle(mMutex);
}

void rtMutexNative::lock()
{
  WaitForSingleObject(mMutex, INFINITE);
}

void rtMutexNative::unlock()
{
  ReleaseMutex(mMutex);
}

rtMutexNativeDesc rtMutexNative::getNativeMutexDescription()
{
  return mMutex;
}

rtThreadConditionNative::rtThreadConditionNative()
{
  mCond = CreateEvent(NULL, FALSE, FALSE, NULL);
}

rtThreadConditionNative::~rtThreadConditionNative()
{
  CloseHandle(mCond);
}

void rtThreadConditionNative::wait(rtMutexNativeDesc desc)
{
  SignalObjectAndWait(desc, mCond, INFINITE, FALSE);
}

void rtThreadConditionNative::signal()
{
  PulseEvent(mCond);
}

void rtThreadConditionNative::broadcast()
{
  PulseEvent(mCond);
}