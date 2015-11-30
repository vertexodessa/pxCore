// pxCore CopyRight 2007-2015 John Robinson
// rtNode.cpp

#include <string>
#include <fstream>

#include <iostream>
#include <sstream>

#include "pxCore.h"

#include "rtNode.h"

#include "jsbindings/rtObjectWrapper.h"
#include "jsbindings/rtFunctionWrapper.h"

#include "node.h"
#include "node_javascript.h"
#include "env.h"
#include "env-inl.h"

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


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef WIN32
static DWORD __rt_main_thread__;
#else
static pthread_t __rt_main_thread__;
static pthread_t __rt_render_thread__;
static pthread_mutex_t gInitLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gInitCond  = PTHREAD_COND_INITIALIZER;
static bool gInitComplete = false;
#endif

bool rtIsMainThread()
{
#ifdef WIN32
  return GetCurrentThreadId() == __rt_main_thread__;
#else
  return pthread_self() == __rt_main_thread__;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

pthread_t worker;

void *jsThread(void *ptr)
{
//  printf("jsThread() - ENTER\n");

  if(ptr)
  {
    rtNodeContext *instance = (rtNodeContext *) ptr;

    if(instance)
    {
      instance->runThread(instance->js_file.c_str());
    }
    else
    {
      fprintf(stderr, "FATAL - No Instance... jsThread() exiting...");
    }
  }

  printf("jsThread() - EXITING...\n");

  return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

rtNodeContext::rtNodeContext() :
  mIsolate(NULL), mRefCount(0)
{
  // Create a new Isolate and make it the current one.
  mIsolate = Isolate::New();

  Isolate::Scope isolate_scope(mIsolate);
  HandleScope     handle_scope(mIsolate);

  // Create a new context.
  mContext.Reset(mIsolate, Context::New(mIsolate));

  // Get a Local context.
  Local<Context> local_context = node::PersistentToLocal<Context>(mIsolate, mContext);
  Context::Scope context_scope(local_context);

  Handle<Object> global = local_context->Global();

  // Register wrappers.
    rtObjectWrapper::exportPrototype(mIsolate, global);
  rtFunctionWrapper::exportPrototype(mIsolate, global);

  rtWrappers.Reset(mIsolate, global);

  mEnv = CreateEnvironment(
        mIsolate,
        uv_default_loop(),
        local_context,
        s_gArgs->argc,
        s_gArgs->argv,
        exec_argc,
        exec_argv);

  // Start debug agent when argv has --debug
  if (use_debug_agent)
  {
    StartDebug(mEnv, debug_wait_connect);
  }

  LoadEnvironment(mEnv);

  // Enable debugger
  if (use_debug_agent)
  {
    EnableDebug(mEnv);
  }
}


rtNodeContext::~rtNodeContext()
{
  mContext.Reset();

  mIsolate->Dispose();
}


void rtNodeContext::add(const char *name, rtValue const& val)
{
  Isolate::Scope isolate_scope(mIsolate);
  HandleScope     handle_scope(mIsolate);    // Create a stack-allocated handle scope.

  // Get a Local context...
  Local<Context> local_context = node::PersistentToLocal<Context>(mIsolate, mContext);
  Context::Scope context_scope(local_context);

  Handle<Object> global = local_context->Global();

  global->Set(String::NewFromUtf8(mIsolate, name), rt2js(mIsolate, val));

//  global->Set(String::NewFromUtf8(mIsolate, name),
//              rtObjectWrapper::createFromObjectReference(mIsolate, val));
}


static inline bool file_exists(const char *file)
{
  struct stat buffer;
  return (stat (file, &buffer) == 0);
}


rtObjectRef rtNodeContext::runFile(const char *file)
{
  if(file_exists(file) == false)
  {
    printf("DEBUG:  %15s()    - NO FILE\n", __FUNCTION__);

    return rtObjectRef(0);  // ERROR
  }

  std::ifstream       js_file(file);
  std::stringstream   script;

  script << js_file.rdbuf(); // slurp up file

  std::string s = script.str();

  startThread(s.c_str());

  return rtObjectRef(0);// JUNK

  //return startThread(s.c_str());
}

rtObjectRef rtNodeContext::runScript(const char *script)
{
  {//scope

    Locker                locker(mIsolate);
    Isolate::Scope isolate_scope(mIsolate);
    HandleScope     handle_scope(mIsolate);    // Create a stack-allocated handle scope.

    // Enter the context for compiling and running ... Persistent > Local

    // Get a Local context...
    Local<Context> local_context = node::PersistentToLocal<Context>(mIsolate, mContext);
    Context::Scope context_scope(local_context);

    Local<String> source = String::NewFromUtf8(mIsolate, script);

    // Compile the source code.
    Local<Script> run_script = Script::Compile(source);

    printf("DEBUG:  %15s()    - SCRIPT = %s\n", __FUNCTION__, script);

    // Run the script to get the result.
    Local<Value> result = run_script->Run();

    // Convert the result to an UTF8 string and print it.
    String::Utf8Value utf8(result);

    printf("DEBUG:  %15s()    - RESULT = %s\n", __FUNCTION__, *utf8);  // TODO:  Probably need an actual RESULT return mechanisim

    return rtObjectRef(0);// JUNK
  }//scope
}

int rtNodeContext::startThread(const char *js)
{
  //strcpy(js_file, js);

  js_file = js;

  if(pthread_create(&worker, NULL, jsThread, (void *) this))
  {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }
}


rtObjectRef rtNodeContext::runThread(const char *js)
{
//  int exec_argc = 0;
//  const char** exec_argv;

  {//scope

    Locker                locker(mIsolate);
    Isolate::Scope isolate_scope(mIsolate);
    HandleScope     handle_scope(mIsolate);    // Create a stack-allocated handle scope.

    Local<Context> local_context = node::PersistentToLocal<Context>(mIsolate, mContext);

    Context::Scope context_scope(local_context);

    Local<String> source = String::NewFromUtf8(mIsolate, js);

    // Compile the source code.
    Local<Script> script = Script::Compile(source);

    // Run the script to get the result.
    Local<Value> result = script->Run();

    // Convert the result to an UTF8 string and print it.
    String::Utf8Value utf8(result);

    int code;
    bool more;
    do
    {
      more = uv_run(mEnv->event_loop(), UV_RUN_ONCE);

      if (more == false)
      {
        EmitBeforeExit(mEnv);

        // Emit `beforeExit` if the loop became alive either after emitting
        // event, or after running some callbacks.
        more = uv_loop_alive(mEnv->event_loop());

        if (uv_run(mEnv->event_loop(), UV_RUN_NOWAIT) != 0)
        {
          more = true;
        }
      }
    } while (more == true);

    code = EmitExit(mEnv);
    RunAtExit(mEnv);

  }//scope

  return  rtObjectRef(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

rtNode::rtNode() : mPlatform(NULL), mPxNodeExtension(NULL)
{

}

void rtNode::init(int argc, char** argv)
{
//  int exec_argc;
//  const char** exec_argv;

  // Hack around with the argv pointer. Used for process.title = "blah".
  argv = uv_setup_args(argc, argv);

  use_debug_agent = true; // JUNK

  Init(&argc, const_cast<const char**>(argv), &exec_argc, &exec_argv);

  V8::Initialize();
  node_is_initialized = true;
}

void rtNode::term()
{
  V8::Dispose();
  V8::ShutdownPlatform();

  if(mPlatform)
  {
    delete mPlatform;
    mPlatform = NULL;
  }

  if(mPxNodeExtension)
  {
    delete mPxNodeExtension;
    mPxNodeExtension = NULL;
  }
}

rtNodeContextRef rtNode::getGlobalContext() const
{
  return rtNodeContextRef();
}

rtNodeContextRef rtNode::createContext(bool ownThread)
{
  rtNodeContextRef ctxref = new rtNodeContext();

  node_isolate = ctxref->mIsolate; // Must come first !!

  return ctxref;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
