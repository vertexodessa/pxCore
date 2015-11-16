#ifndef RTNODE_H
#define RTNODE_H


#include "rtRefT.h"
#include "rtObject.h"

#include "uv.h"
#include "include/v8.h"
#include "include/libplatform/libplatform.h"


class rtNodeContext;
typedef rtRefT<rtNodeContext> rtNodeContextRef;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct args_
{
  int    argc;
  char **argv;

  args_() { argc = 0; argv = NULL; }
  args_(int n = 0, char** a = NULL) : argc(n), argv(a) {}
}
args_t;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class rtNodeContext  // V8
{
public:
  rtNodeContext();
  ~rtNodeContext();

  void addObject(std::string const& name, rtObjectRef const& obj);

  rtObjectRef run(std::string js);
  rtObjectRef run_thread(std::string js);

  unsigned long AddRef()
  {
    return rtAtomicInc(&mRefCount);
  }

  unsigned long Release()
  {
    long l = rtAtomicDec(&mRefCount);
    if (l == 0) delete this;
    return l;
  }

  std::string   js_file;

  v8::Isolate                   *mIsolate;
  v8::Persistent<v8::Context>    mContext;
  v8::Persistent<v8::Object>     rtWrappers;

  v8::Persistent<v8::ObjectTemplate>  globalTemplate;

private:
  rtObjectRef run_snippet(std::string js);
  rtObjectRef run_file(std::string file);

  int startThread(std::string js);

  int mRefCount;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class rtNode
{
public:
  rtNode();

  void init(int argc, char** argv);
  void term();

  rtNodeContextRef getGlobalContext() const;

  rtNodeContextRef createContext(bool ownThread = false);

private:
  v8::Platform  *mPlatform;
  v8::Extension *mPxNodeExtension;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif // RTNODE_H

