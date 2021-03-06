// pxCore CopyRight 2007-2015 John Robinson
// rtNode.cpp

#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <string>
#include <fstream>

#include <iostream>
#include <sstream>

#ifndef WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "node.h"
#include "node_javascript.h"
#include "node_contextify_mods.h"

#include "env.h"
#include "env-inl.h"

#ifndef WIN32
#pragma GCC diagnostic pop
#endif

#include "rtNode.h"

#include "rtThreadQueue.h"

extern rtThreadQueue gUIThreadQueue;


#ifdef USE_CONTEXTIFY_CLONES
#warning Using USE_CONTEXTIFY_CLONES !!
#else
#warning NOT Using USE_CONTEXTIFY_CLONES !!
#endif

#ifdef RUNINMAIN

//#include "pxEventLoop.h"
//extern pxEventLoop* gLoop;

#define ENTERSCENELOCK()
#define EXITSCENELOCK()
#else
#define ENTERSCENELOCK() rtWrapperSceneUpdateEnter();
#define EXITSCENELOCK()  rtWrapperSceneUpdateExit();
#endif


using namespace v8;
using namespace node;

extern args_t *s_gArgs;

namespace node
{
extern bool use_debug_agent;
extern bool debug_wait_connect;
}

static int exec_argc;
static const char** exec_argv;
static rtAtomic sNextId = 100;


args_t *s_gArgs;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef WIN32
static DWORD __rt_main_thread__;
#else
static pthread_t __rt_main_thread__;
#endif

// rtIsMainThread() - Previously:  identify the MAIN thread of 'node' which running JS code.
//
// rtIsMainThread() - Currently:   identify BACKGROUND thread which running JS code.
//
bool rtIsMainThread()
{
  // Since this is single threaded version we're always on the js thread
  return true;
}

#if 0
static inline bool file_exists(const char *file)
{
  struct stat buffer;
  return (stat (file, &buffer) == 0);
}
#endif

rtNodeContext::rtNodeContext(Isolate *isolate) :
     mIsolate(isolate), mEnv(NULL), mRefCount(0)
{
  assert(isolate); // MUST HAVE !
  mId = rtAtomicInc(&sNextId);

  createEnvironment();
}

#ifdef USE_CONTEXTIFY_CLONES
rtNodeContext::rtNodeContext(Isolate *isolate, rtNodeContextRef clone_me) :
      mIsolate(isolate), mEnv(NULL), mRefCount(0)
{
  assert(mIsolate); // MUST HAVE !
  mId = rtAtomicInc(&sNextId);

  clonedEnvironment(clone_me);
}
#endif

void rtNodeContext::createEnvironment()
{
  Locker                locker(mIsolate);
  Isolate::Scope isolate_scope(mIsolate);
  HandleScope     handle_scope(mIsolate);

  // Create a new context.
  Local<Context> local_context = Context::New(mIsolate);

  local_context->SetEmbedderData(HandleMap::kContextIdIndex, Integer::New(mIsolate, mId));

  mContext.Reset(mIsolate, local_context); // local to persistent

  Context::Scope context_scope(local_context);

  Handle<Object> global = local_context->Global();

  // Register wrappers.
  rtObjectWrapper::exportPrototype(mIsolate, global);
  rtFunctionWrapper::exportPrototype(mIsolate, global);

  mRtWrappers.Reset(mIsolate, global);

  // Create Environment.
  mEnv = CreateEnvironment(mIsolate,
                           uv_default_loop(),
                           local_context,
                           s_gArgs->argc,
                           s_gArgs->argv,
                           exec_argc,
                           exec_argv);

  // Start debug agent when argv has --debug
  if (use_debug_agent)
  {
    printf("use_debug_agent\n");
    StartDebug(mEnv, debug_wait_connect);
  }

  // Load Environment.
  LoadEnvironment(mEnv);

  // Enable debugger
  if (use_debug_agent)
  {
    EnableDebug(mEnv);
  }
}

#ifdef USE_CONTEXTIFY_CLONES

void rtNodeContext::clonedEnvironment(rtNodeContextRef clone_me)
{
  Locker                locker(mIsolate);
  Isolate::Scope isolate_scope(mIsolate);
  HandleScope     handle_scope(mIsolate);

  // Get parent Local context...
  Local<Context> local_context = clone_me->getLocalContext();
  Context::Scope context_scope(local_context);

  // Create dummy sandbox for ContextifyContext::makeContext() ...
  Local<Object> sandbox = Object::New(mIsolate);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  rtValue       val_array = clone_me->get(SANDBOX_IDENTIFIER);
  rtObjectRef       array = val_array.toObject();

  int len = array.get<int>("length");

  rtString s;
  for(int i = 0; i < len; i++)
  {
    array.get<rtString>( (uint32_t) i, s);  // get 'name' for object
    rtValue obj = clone_me->get(s);         // get object for 'name'

    if( obj.isEmpty() == false)
    {
        // Copy to var/module 'sandbox' under construction...
        Local<Value> module = local_context->Global()->Get( String::NewFromUtf8(mIsolate, s.cString() ) );
        sandbox->Set( String::NewFromUtf8(mIsolate, s.cString()), module);
    }
    else
    {
      printf("## FATAL:   '%s' is empty !! - UNEXPECTED\n", s.cString()); fflush(stdout);
    }
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // Clone a new context.
  {
    Local<Context>  clone_local = node::makeContext(mIsolate, sandbox); // contextify context with 'sandbox'
    clone_local->SetEmbedderData(HandleMap::kContextIdIndex, Integer::New(mIsolate, mId));

    mContext.Reset(mIsolate, clone_local); // local to persistent

    Context::Scope context_scope(clone_local);

    Handle<Object> clone_global = clone_local->Global();

    // Register wrappers in this cloned context...
      rtObjectWrapper::exportPrototype(mIsolate, clone_global);
    rtFunctionWrapper::exportPrototype(mIsolate, clone_global);

    mRtWrappers.Reset(mIsolate, clone_global);
}

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}

#endif // USE_CONTEXTIFY_CLONES

rtNodeContext::~rtNodeContext()
{
  if(mEnv)
  {
    Locker                locker(mIsolate);
    Isolate::Scope isolate_scope(mIsolate);
    HandleScope     handle_scope(mIsolate);

    RunAtExit(mEnv);

    mEnv->Dispose();
    mEnv = NULL;
  }

  if(exec_argv)
  {
    delete[] exec_argv;
    exec_argv = NULL;
    exec_argc = 0;
  }

  // TODO:  Might not be needed in ST case...
  //
  // Un-Register wrappers.
  // rtObjectWrapper::destroyPrototype();
  // rtFunctionWrapper::destroyPrototype();

  // clear out persistent javascript handles
  HandleMap::clearAllForContext(mId);

  mContext.Reset();
  mRtWrappers.Reset();

  Release();

  // NOTE: 'mIsolate' is owned by rtNode.  Don't destroy here !
}


void rtNodeContext::add(const char *name, rtValue const& val)
{
  if(name == NULL)
  {
    rtLogError(" rtNodeContext::add() - no symbolic name for rtValue");
    return;
  }
  else if(this->has(name))
  {
    rtLogError(" rtNodeContext::add() - ALREADY HAS '%s' ... over-writing.", name);
   // return; // Allow for "Null"-ing erasure.
  }

  Locker                locker(mIsolate);
  Isolate::Scope isolate_scope(mIsolate);
  HandleScope     handle_scope(mIsolate);    // Create a stack-allocated handle scope.

  // Get a Local context...
  Local<Context> local_context = node::PersistentToLocal<Context>(mIsolate, mContext);
  Context::Scope context_scope(local_context);

  local_context->Global()->Set( String::NewFromUtf8(mIsolate, name), rt2js(local_context, val));
}


rtValue rtNodeContext::get(const char *name)
{
  if(name == NULL)
  {
    rtLogError(" rtNodeContext::get() - no symbolic name for rtValue");
    return rtValue(0);
  }

  // Get a Local context...
  Local<Context> local_context = node::PersistentToLocal<Context>(mIsolate, mContext);
  Context::Scope context_scope(local_context);

  Handle<Object> global = local_context->Global();

  // Get the object
  Local<Value> object = global->Get( String::NewFromUtf8(mIsolate, name) );

  if(object->IsUndefined() || object->IsNull() )
  {
    rtLogError("FATAL: '%s' is Undefined ", name);
    return rtValue();
  }
  else
  {
    rtWrapperError error; // TODO - handle error
    return js2rt(local_context, object, &error);
  }
}

bool rtNodeContext::has(const char *name)
{
  if(name == NULL)
  {
    rtLogError(" rtNodeContext::has() - no symbolic name for rtValue");
    return false;
  }

  Locker                locker(mIsolate);
  Isolate::Scope isolate_scope(mIsolate);
  HandleScope     handle_scope(mIsolate);    // Create a stack-allocated handle scope.

  // Get a Local context...
  Local<Context> local_context = node::PersistentToLocal<Context>(mIsolate, mContext);
  Context::Scope context_scope(local_context);

  Handle<Object> global = local_context->Global();

  TryCatch try_catch;
  Handle<Value> value = global->Get(String::NewFromUtf8(mIsolate, name) );

  if (try_catch.HasCaught())
  {
     printf("\n ## has() - HasCaught()  ... ERROR");
     return false;
  }

  // No need to check if |value| is empty because it's taken care of
  // by TryCatch above.

  return ( !value->IsUndefined() && !value->IsNull() );
}


rtObjectRef rtNodeContext::runScript(const char *script, const char *args /*= NULL*/)
{
  if(script == NULL)
  {
    rtLogError(" %s  ... no script given.",__PRETTY_FUNCTION__);

    return  rtObjectRef(0);// JUNK
  }

  // rtLogDebug(" %s  ... Running...",__PRETTY_FUNCTION__);

  return runScript(std::string(script), args);
}

rtObjectRef rtNodeContext::runScript(const std::string &script, const char* /* args = NULL*/)
{
  if(script.empty())
  {
    rtLogError(" %s  ... no script given.",__PRETTY_FUNCTION__);

    return  rtObjectRef(0);// JUNK
  }

  {//scope
    Locker                locker(mIsolate);
    Isolate::Scope isolate_scope(mIsolate);
    HandleScope     handle_scope(mIsolate);    // Create a stack-allocated handle scope.

    // Get a Local context...
    Local<Context> local_context = node::PersistentToLocal<Context>(mIsolate, mContext);
    Context::Scope context_scope(local_context);

    Local<String> source = String::NewFromUtf8(mIsolate, script.c_str());

    // Compile the source code.
    Local<Script> run_script = Script::Compile(source);

    // Run the script to get the result.
    Local<Value> result = run_script->Run();

    // Convert the result to an UTF8 string and print it.
    String::Utf8Value utf8(result);
  }//scope

  return rtObjectRef(0);// JUNK
}

std::string readFile(const char *file)
{
  std::ifstream       src_file(file);
  std::stringstream   src_script;

  src_script << src_file.rdbuf(); // slurp up file

  std::string s = src_script.str();

  return s;
}

rtObjectRef rtNodeContext::runFile(const char *file, const char* /*args = NULL*/)
{
  if(file == NULL)
  {
    rtLogError(" %s  ... no script given.",__PRETTY_FUNCTION__);

    return  rtObjectRef(0);// JUNK
  }

  // Read the script file
  js_script = readFile(file);

  return runScript(js_script);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

rtNode::rtNode() /*: mPlatform(NULL)*/
{
  char const* s = getenv("RT_TEST_GC");
  if (s && strlen(s) > 0)
    mTestGc = true;
  else
    mTestGc = false;

  if (mTestGc)
    rtLogWarn("*** PERFORMANCE WARNING *** : gc being invoked in render thread");

// TODO Please make this better... less hard coded...

                              //0123456 789ABCDEF012 345 67890ABCDEF
  static const char *args2   = "rtNode\0--expose-gc\0-e\0console.log(\"rtNode Initalized\");\0\0";
  static const char *argv2[] = {&args2[0], &args2[7], &args2[19], &args2[22], NULL};
  int          argc   = sizeof(argv2)/sizeof(char*) - 1;

  static args_t aa(argc, (char**)argv2);

  s_gArgs = &aa;


  char **argv = aa.argv;

  __rt_main_thread__ = pthread_self(); //  NB

  nodePath();

  mIsolate     = Isolate::New();
  node_isolate = mIsolate; // Must come first !!

  init(argc, argv);
}

rtNode::~rtNode()
{
  term();
}

void rtNode::pump()
{
  Locker                locker(mIsolate);
  Isolate::Scope isolate_scope(mIsolate);
  HandleScope     handle_scope(mIsolate);    // Create a stack-allocated handle scope.

  uv_run(uv_default_loop(), UV_RUN_NOWAIT);//UV_RUN_ONCE);

  // Enable this to expedite garbage collection for testing... warning perf hit
  if (mTestGc)
  {
    static int sGcTickCount = 0;

    if (sGcTickCount++ > 60)
    {
      Local<Context> local_context = node::PersistentToLocal<Context>(mIsolate, mContext);
      Context::Scope contextScope(local_context);
      mIsolate->RequestGarbageCollectionForTesting(Isolate::kFullGarbageCollection);
      sGcTickCount = 0;
    }
  }
}

#if 0
rtNode::forceGC()
{
  mIsolate->RequestGarbageCollectionForTesting(Isolate::kFullGarbageCollection);
}
#endif

void rtNode::nodePath()
{
  const char* NODE_PATH = ::getenv("NODE_PATH");

  if(NODE_PATH == NULL)
  {
    char cwd[1024] = {};

    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
      ::setenv("NODE_PATH", cwd, 1); // last arg is 'overwrite' ... 0 means DON'T !

      //printf("\n\n INFO: NODE_PATH = [%s] << NEW\n", cwd);
    }
    else
    {
      rtLogError(" - failed to set NODE_PATH");
    }
  }
  // else
  // {
  //    printf("\nINFO:  NODE_PATH =  [%s]  <<<<< ALREADY SET !\n", NODE_PATH);
  // }
}

void rtNode::init(int argc, char** argv)
{
  // Hack around with the argv pointer. Used for process.title = "blah".
  argv = uv_setup_args(argc, argv);


#if 0
#warning Using DEBUG AGENT...
  use_debug_agent = true; // JUNK
#endif

  if(node_is_initialized == false)
  {
    Init(&argc, const_cast<const char**>(argv), &exec_argc, &exec_argv);

//    mPlatform = platform::CreateDefaultPlatform();
//    V8::InitializePlatform(mPlatform);

    V8::Initialize();
    node_is_initialized = true;

    Locker                locker(mIsolate);
    Isolate::Scope isolate_scope(mIsolate);
    HandleScope     handle_scope(mIsolate);    // Create a stack-allocated handle scope.

    Local<Context> ctx = Context::New(mIsolate);
    ctx->SetEmbedderData(HandleMap::kContextIdIndex, Integer::New(mIsolate, 99));
    mContext.Reset(mIsolate, ctx);
  }
}

void rtNode::term()
{
#ifdef USE_CONTEXTIFY_CLONES
  if( mRefContext.getPtr() )
  {
    mRefContext->Release();
  }
#endif

return; // JUNK - Probably leaks like a sieve !!!! Stops crash on STB

  if(node_isolate)
  {
// JRJRJR  Causing crash???  ask Hugh

    printf("\n++++++++++++++++++ DISPOSE\n\n");

    node_isolate->Dispose();
    node_isolate = NULL;
    mIsolate     = NULL;
  }

  if(node_is_initialized)
  {
    V8::Dispose();

    node_is_initialized = false;

  //  V8::ShutdownPlatform();
  //  if(mPlatform)
  //  {
  //    delete mPlatform;
  //    mPlatform = NULL;
  //  }

  //  if(mPxNodeExtension)
  //  {
  //    delete mPxNodeExtension;
  //    mPxNodeExtension = NULL;
  //  }
  }
}

rtNodeContextRef rtNode::getGlobalContext() const
{
  return rtNodeContextRef();
}

rtNodeContextRef rtNode::createContext(bool ownThread)
{
  UNUSED_PARAM(ownThread);    // not implemented yet.

  rtNodeContextRef ctxref;

#ifdef USE_CONTEXTIFY_CLONES
  if(mRefContext.getPtr() == NULL)
  {
    // printf("\n createContext()  >>  REFERENCE CREATED  !!!!!!");

    mRefContext = new rtNodeContext(mIsolate);

    ctxref = mRefContext;

    // Populate 'sandbox' vars in JS...
    mRefContext->runFile(SANDBOX_JS);
  }
  else
  {
    // printf("\n createContext()  >>  CLONE CREATED !!!!!!");

    ctxref = new rtNodeContext(mIsolate, mRefContext); // CLONE !!!
  }
#else

    ctxref = new rtNodeContext(mIsolate);

#endif

  return ctxref;
}


