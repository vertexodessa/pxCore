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
  if(ptr)
  {
    rtNodeContext *instance = (rtNodeContext *) ptr;

    instance->run_thread(instance->js_file);
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
}


rtNodeContext::~rtNodeContext()
{
  mContext.Reset();

  mIsolate->Dispose();
}


void rtNodeContext::addObject(std::string const& name, rtObjectRef const& obj)
{
  Locker                locker(mIsolate);
  Isolate::Scope isolate_scope(mIsolate);
  HandleScope     handle_scope(mIsolate);    // Create a stack-allocated handle scope.

  // Get a Local context...
  Local<Context> local_context = node::PersistentToLocal<Context>(mIsolate, mContext);
  Context::Scope context_scope(local_context);

  Handle<Object> global = local_context->Global();

  global->Set(String::NewFromUtf8(mIsolate, name.c_str()),
              rtObjectWrapper::createFromObjectReference(mIsolate, obj));
}


inline bool file_exists(const std::string& name)
{
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}


rtObjectRef rtNodeContext::run_file(std::string file)
{
  //printf("DEBUG:  %15s()    - ENTER\n", __FUNCTION__);

  if(file_exists(file) == false)
  {
    return rtObjectRef(0);  // ERROR
  }

  startThread(file);

  return rtObjectRef(0);// JUNK

  //return startThread(file);
}


rtObjectRef rtNodeContext::run_snippet(std::string js)
{
  {//scope

 //   Locker                locker(mIsolate);
    Isolate::Scope isolate_scope(mIsolate);
    HandleScope     handle_scope(mIsolate);    // Create a stack-allocated handle scope.

    // Enter the context for compiling and running ... Persistent > Local

    // Get a Local context...
    Local<Context> local_context = node::PersistentToLocal<Context>(mIsolate, mContext);
    Context::Scope context_scope(local_context);

    Local<String> source = String::NewFromUtf8(mIsolate, js.c_str());

    // Compile the source code.
    Local<Script> script = Script::Compile(source);

    // Run the script to get the result.
    Local<Value> result = script->Run();

    // Convert the result to an UTF8 string and print it.
    String::Utf8Value utf8(result);

    return rtObjectRef(0);// JUNK
  }//scope
}

int rtNodeContext::startThread(std::string js)
{
  js_file = js;

  if(pthread_create(&worker, NULL, jsThread, (void *) this))
  {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }
}


rtObjectRef rtNodeContext::run_thread(std::string js)
{
  int exec_argc = 0;
  const char** exec_argv = NULL;

  {//scope

    Locker                locker(mIsolate);
    Isolate::Scope isolate_scope(mIsolate);
    HandleScope     handle_scope(mIsolate);    // Create a stack-allocated handle scope.

    Local<Context> local_context = node::PersistentToLocal<Context>(mIsolate, mContext);

    Context::Scope context_scope(local_context);

    Environment* env = CreateEnvironment(
          mIsolate,
          uv_default_loop(),
          local_context,
          s_gArgs->argc,
          s_gArgs->argv,
          exec_argc,
          exec_argv);

    LoadEnvironment(env);

    Local<String> source = String::NewFromUtf8(mIsolate, js.c_str());

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
      more = uv_run(env->event_loop(), UV_RUN_ONCE);

      if (more == false)
      {
        EmitBeforeExit(env);

        // Emit `beforeExit` if the loop became alive either after emitting
        // event, or after running some callbacks.
        more = uv_loop_alive(env->event_loop());

        if (uv_run(env->event_loop(), UV_RUN_NOWAIT) != 0)
        {
          more = true;
        }
      }
    } while (more == true);

    code = EmitExit(env);
    RunAtExit(env);

  }//scope

  return  rtObjectRef(0);
}



rtObjectRef rtNodeContext::run(std::string js)
{
  if(true) //file_exists(js) == true)
  {
//    printf("DEBUG:  %15s()    - EXISTS !!   [%s] \n", __FUNCTION__, js.c_str());

    std::ifstream       js_file(js.c_str());
    std::stringstream   script;

    script << js_file.rdbuf(); // slurp up file

    std::string s = script.str();

    startThread(s);

    return  rtObjectRef(0);
  }
  else
  {
    return run_snippet(js);
  }

  return  rtObjectRef(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

rtNode::rtNode() : mPlatform(NULL), mPxNodeExtension(NULL)
{

}

void rtNode::init(int argc, char** argv)
{
  int exec_argc;
  const char** exec_argv;

  Init(&argc, const_cast<const char**>(argv), &exec_argc, &exec_argv);

  // Hack around with the argv pointer. Used for process.title = "blah".
  argv = uv_setup_args(argc, argv);

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

  return ctxref;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
