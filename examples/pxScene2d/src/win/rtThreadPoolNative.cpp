#include "rtThreadPoolNative.h"

#include <process.h>

static void workerStart(void* argp)
{
  rtThreadPoolNative* pool = reinterpret_cast<rtThreadPoolNative *>(argp);
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
  rtScopedLock lock(mThreadTaskMutex);

  mRunning = true;
  for (int i = 0; i < mNumberOfThreads; i++)
  {
    HANDLE hThread = (HANDLE)_beginthread(workerStart, 0, this);
    if (hThread == (HANDLE)-1L)
      return false;
    mThreads.push_back(hThread);
  }
  return true;
}

void rtThreadPoolNative::destroy()
{
  {
    rtScopedLock lock(mThreadTaskMutex);
    mRunning = false;
    mThreadTaskCondition.broadcast();
  }

  typedef std::vector<HANDLE>::iterator iterator;
  for (iterator i = mThreads.begin(); i != mThreads.end(); ++i)
    WaitForSingleObject(*i, INFINITE); // same as pthread_join()

  mThreads.clear();
}

void rtThreadPoolNative::startThread()
{
  rtThreadTask* threadTask = NULL;
  while (true)
  {
    {
      rtScopedLock lock(mThreadTaskMutex);
      while (mRunning && mThreadTasks.empty())
        mThreadTaskCondition.wait(mThreadTaskMutex.getNativeMutexDescription());

      if (!mRunning)
        return;

      threadTask = mThreadTasks.front();
      mThreadTasks.pop_front();
    }

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
  rtScopedLock lock(mThreadTaskMutex);
  mThreadTasks.push_back(threadTask);
  mThreadTaskCondition.signal();
}


