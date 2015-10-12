#ifndef RTNODE_H
#define RTNODE_H


#include "rtRefT.h"
#include "rtObject.h"

#include "uv.h"
#include "include/v8.h"
#include "include/libplatform/libplatform.h"


class rtNodeContext;
typedef rtRefT<rtNodeContext> rtNodeContextRef;


//===============================================================================================

class rtNodeContext
{
public:
  rtNodeContext();
  ~rtNodeContext();

  void addObject(std::string const& name, rtObjectRef const& obj);

  rtObjectRef run(std::string js);
  rtObjectRef run_file(std::string js);

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

  v8::Isolate                   *mIsolate;

private:
  v8::Persistent<v8::Context>    mContext;

  int mRefCount;
};

//===============================================================================================

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

//===============================================================================================

#endif // RTNODE_H

