#include "rtThreadPoolNative.h"

#include <process.h>

void launchThread(void* threadPool)
{
  rtThreadPoolNative* pool = (rtThreadPoolNative*)threadPool;
  pool->startThread();
}

rtThreadPoolNative::rtThreadPoolNative(int numberOfThreads) 
  : mNumberOfThreads(numberOfThreads)
  , mRunning(false)
  , mThreadTaskMutex()
  , mThreadTaskCondition()
  , mThreads()
  , mThreadTasks()
{
  initialize();
}

rtThreadPoolNative::~rtThreadPoolNative()
{
  if (mRunning)
    destroy();
}

bool rtThreadPoolNative::initialize()
{
  mRunning = true;
  for (int i = 0; i < mNumberOfThreads; i++)
  {
    HANDLE hThread = (HANDLE)_beginthread(launchThread, 0, this);
    if (hThread != (HANDLE)-1L)
      return false;
    mThreads.push_back(hThread);
  }
  return true;
}

void rtThreadPoolNative::destroy()
{
  mThreadTaskMutex.lock();
  mRunning = false; //mRunning is accessed by other threads
  mThreadTaskMutex.unlock();
  //broadcast to all the threads that we are shutting down
  mThreadTaskCondition.broadcast();

  typedef std::vector<HANDLE>::iterator iterator;
  for (iterator i = mThreads.begin(); i != mThreads.end(); ++i)
    WaitForSingleObject(*i, INFINITE);
}

void rtThreadPoolNative::startThread()
{
  rtThreadTask* threadTask = NULL;
  while (true)
  {
    mThreadTaskMutex.lock();
    while (mRunning && (mThreadTasks.empty()))
    {
      mThreadTaskCondition.wait(mThreadTaskMutex.getNativeMutexDescription());
    }
    if (!mRunning)
    {
      mThreadTaskMutex.unlock();
      return;
    }
    threadTask = mThreadTasks.front();
    mThreadTasks.pop_front();
    mThreadTaskMutex.unlock();

    if (threadTask != NULL)
    {
      threadTask->execute();
      delete threadTask;
      threadTask = NULL;
    }
  }
}

void rtThreadPoolNative::executeTask(rtThreadTask* threadTask)
{
    mThreadTaskMutex.lock();
    mThreadTasks.push_back(threadTask);
    mThreadTaskCondition.signal();
    mThreadTaskMutex.unlock();
}
