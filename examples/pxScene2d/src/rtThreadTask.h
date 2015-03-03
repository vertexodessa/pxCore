#ifndef RT_THREAD_TASK_H
#define RT_THREAD_TASK_H

typedef void (*rtThreadTaskFunction)(void *argp);

class rtThreadTask
{
public:
public:
  rtThreadTask(rtThreadTaskFunction func, void* data);
  ~rtThreadTask();
  void execute();

private:
  rtThreadTaskFunction mFunctionPointer;
  void* mData;
};

#endif //RT_THREAD_TASK_H