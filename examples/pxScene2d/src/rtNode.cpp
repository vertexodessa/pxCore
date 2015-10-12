// pxCore CopyRight 2007-2015 John Robinson
// main.cpp

#ifdef WIN32
#include <windows.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#include <string>
#include <fstream>

#include <iostream>
#include <sstream>

#include <sys/stat.h>
#include <unistd.h>

#include "pxCore.h"


#include "rtNode.h"

#include "node.h"
#include "node_javascript.h"
#include "env.h"
#include "env-inl.h"

#define PX_NODE_EXTENSION       "px"
#define PX_NODE_EXTENSION_PATH  "./jsbindings/build/Debug/px.node"

using namespace v8;
using namespace node;

using v8::Array;
using v8::ArrayBuffer;

//#define USE_BASIC_EXAMPLE
//#define USE_NODESTART_EXAMPLE
#if 1

//extern node::ArrayBufferAllocator::the_singleton;

#else

//###
class ArrayBufferAllocator : public ArrayBuffer::Allocator
{
 public:
  // Impose an upper limit to avoid out of memory errors that bring down
  // the process.
  static const size_t kMaxLength = 0x3fffffff;
  static ArrayBufferAllocator the_singleton;
  virtual ~ArrayBufferAllocator() {}
  virtual void* Allocate(size_t length);
  virtual void* AllocateUninitialized(size_t length);
  virtual void Free(void* data, size_t length);
 private:
  ArrayBufferAllocator() {}
  ArrayBufferAllocator(const ArrayBufferAllocator&);
  void operator=(const ArrayBufferAllocator&);
};
#endif

ArrayBufferAllocator ArrayBufferAllocator::the_singleton;


void* ArrayBufferAllocator::Allocate(size_t length) {
  if (length > kMaxLength)
    return NULL;
  char* data = new char[length];
  memset(data, 0, length);
  return data;
}


void* ArrayBufferAllocator::AllocateUninitialized(size_t length) {
  if (length > kMaxLength)
    return NULL;
  return new char[length];
}


void ArrayBufferAllocator::Free(void* data, size_t length) {
  delete[] static_cast<char*>(data);
}



// process-relative uptime base, initialized at start-up
static double prog_start_time;
static bool debugger_running;
static uv_async_t dispatch_debug_messages_async;

static Isolate* node_isolate = NULL;

// Called from V8 Debug Agent TCP thread.
static void DispatchMessagesDebugAgentCallback(Environment* env) {
  // TODO(indutny): move async handle to environment
  uv_async_send(&dispatch_debug_messages_async);
}


// Called from the main thread.
static void DispatchDebugMessagesAsyncCallback(uv_async_t* handle) {
  if (debugger_running == false) {
    fprintf(stderr, "Starting debugger agent.\n");

    Environment* env = Environment::GetCurrent(node_isolate);
    Context::Scope context_scope(env->context());

//    StartDebug(env, false);
//    EnableDebug(env);
  }
  Isolate::Scope isolate_scope(node_isolate);
  v8::Debug::ProcessDebugMessages();
}


//###

//===============================================================================================

typedef struct args_
{
  int    argc;
  char **argv;

  args_() { argc = 0; argv = NULL; }
  args_(int n = 0, char** a = NULL) : argc(n), argv(a) {}
}
args_t;

//===============================================================================================

#ifdef USE_NODESTART_EXAMPLE

pthread_t worker;

void *jsThread(void *ptr)
{
  if(ptr)
  {
    args_t *args = (args_t *) ptr;

    node::Start(args->argc, args->argv);
  }

  printf("jsThread() - EXITING...\n");

  return NULL;
}
#endif

//===============================================================================================

#ifdef USE_BASIC_EXAMPLE
Handle<ObjectTemplate>  globaltemplate;
Handle<Object>          processref;
Persistent<Context>     contextref;
#endif

args_t *s_gArgs;

int main(int argc, char** argv)
{
  args_t aa(argc, argv);
  s_gArgs = &aa;


#ifdef USE_BASIC_EXAMPLE
  // Create a new Isolate and make it the current one.
  Isolate* isolate = Isolate::New();
  Isolate::Scope isolate_scope(isolate);

  // Create a stack-allocated handle scope.
  HandleScope handle_scope(isolate);

  // Create a new context.
  Local<Context> context = Context::New(isolate);

  // Enter the context for compiling and running the hello world script.
  Context::Scope context_scope(context);

  // Create a string containing the JavaScript source code.
  Local<String> source = String::NewFromUtf8(isolate, "'Hello' + ', World!'");

  // Compile the source code.
  Local<Script> script = Script::Compile(source);

  // Run the script to get the result.
  Local<Value> result = script->Run();

  // Convert the result to an UTF8 string and print it.
  String::Utf8Value utf8(result);
  printf("%s\n", *utf8);
  return 0;
#endif

#ifdef USE_NODESTART_EXAMPLE
#if 0
  printf("STARTING via *OTHER* THREAD... argc = %d\n", argc);

  if(pthread_create(&worker, NULL, jsThread, (void *) &aa))
  {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }

  getchar();

#else

  node::Start(argc, argv);

#endif
#else


 // node_isolate = Isolate::New();

  rtNode myNode;

  rtNodeContextRef ctx = myNode.createContext();

  node_isolate = ctx->mIsolate;
  myNode.init(s_gArgs->argc, s_gArgs->argv);

  // - - - - - - - - - - - - - - - - - - - - - - - - - -

//  if(argc > 1)
//  {
//    printf("\nRun file... %s\n\n", argv[1]);
//    ctx->run_file(argv[1]);
//  }

  ctx->run("");

  printf("Run Done..\n");

  // - - - - - - - - - - - - - - - - - - - - - - - - - -

  printf("\n Term...");
  myNode.term();
  printf("Done\n");

  return 0;
#endif

  return 0;
}

//===============================================================================================

rtNodeContext::rtNodeContext() :
  mIsolate(NULL), mRefCount(0)
{
  // Create a new Isolate and make it the current one.
  mIsolate = Isolate::New();

  Isolate::Scope isolate_scope(mIsolate);

  // Create a stack-allocated handle scope.
  HandleScope handle_scope(mIsolate);

  // Create a new context.
  mContext.Reset(mIsolate, Context::New(mIsolate));
}

rtNodeContext::~rtNodeContext()
{
  mContext.Reset();

  mIsolate->Dispose();
}

void rtNodeContext::addObject(std::string const& name, rtObjectRef const& obj)
{

}


inline bool file_exists(const std::string& name)
{
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}

rtObjectRef rtNodeContext::run_file(std::string file)
{
  if(file_exists(file) == false)
  {
    return rtObjectRef(0);  // ERROR
  }

  std::ifstream       js_file(file.c_str());
  std::stringstream   script;

  script << js_file.rdbuf(); // slurp up file

  return run(script.str());
}


rtObjectRef rtNodeContext::run(std::string js)
{
  int exec_argc;
  const char** exec_argv;
//  Init(&s_gArgs->argc, const_cast<const char**>(s_gArgs->argv), &exec_argc, &exec_argv);

  {
    Locker         locker(mIsolate);
    Isolate::Scope isolate_scope(mIsolate);
    HandleScope    handle_scope(mIsolate);    // Create a stack-allocated handle scope.

    // Enter the context for compiling and running ... Persistent > Local
    v8::Local<v8::Context> local_context(v8::Local<v8::Context>::New(mIsolate, mContext));

    //*******************

    Environment* env = CreateEnvironment(
          mIsolate,
          uv_default_loop(),
          local_context,
          s_gArgs->argc,
          s_gArgs->argv,
          exec_argc,
          exec_argv);

    Context::Scope context_scope(local_context);

    LoadEnvironment(env);

    //*******************

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

    //*******************
  }//scope

  getchar();

  return  rtObjectRef(0);
}

//===============================================================================================

rtNode::rtNode() :
  mPlatform(NULL), mPxNodeExtension(NULL)
{

}



void rtNode::init(int argc, char** argv)
{
  // Hack around with the argv pointer. Used for process.title = "blah".
  argv = uv_setup_args(argc, argv);

  // Initialize V8.
  // V8::InitializeICU();
  // V8::InitializeExternalStartupData(s_gArgs->argv[0]);

//   mPlatform = v8::platform::CreateDefaultPlatform();
//   V8::InitializePlatform(mPlatform);

  int exec_argc;
  const char** exec_argv;
//  Init(&argc, const_cast<const char**>(argv), &exec_argc, &exec_argv);

//##

  printf("\n  ####   1");

  // Initialize prog_start_time to get relative uptime.
  prog_start_time = static_cast<double>(uv_now(uv_default_loop()));

  printf("\n  ####   2");

  // Make inherited handles noninheritable.
  uv_disable_stdio_inheritance();

  printf("\n  ####   3");

  // init async debug messages dispatching
  // FIXME(bnoordhuis) Should be per-isolate or per-context, not global.
  uv_async_init(uv_default_loop(),
                &dispatch_debug_messages_async,
                DispatchDebugMessagesAsyncCallback);

  printf("\n  ####   3");

  uv_unref(reinterpret_cast<uv_handle_t*>(&dispatch_debug_messages_async));
//##
  printf("\n  ####   4");

  V8::SetArrayBufferAllocator(&ArrayBufferAllocator::the_singleton);

  printf("\n  ####   5");

  V8::Initialize();

  printf("\n  ####   6");

  //  printf(" RegisterExtension ... \n");

  // Register PX bindings code...
//  mPxNodeExtension = new Extension(PX_NODE_EXTENSION, PX_NODE_EXTENSION_PATH);
//  RegisterExtension(mPxNodeExtension);


/*
  printf("\n EXTENSION >>   name: %s  ",       mPxNodeExtension->name());
  printf("\n EXTENSION >> length: %d ",  (int) mPxNodeExtension->source_length());
  printf("\n EXTENSION >> dcount: %d ",        mPxNodeExtension->dependency_count());

        int    dcount = mPxNodeExtension->dependency_count();
  const char** deps   = mPxNodeExtension->dependencies();

  for(int i =0; i < dcount; i++)
  {
    printf("\n EXTENSION >> dep: %s  ", deps[i]);
  }
*/

//  printf(" RegisterExtension ... Done\n");
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

  rtNodeContextRef ctxref = new rtNodeContext;

  return ctxref;
}

//===============================================================================================
//===============================================================================================
//===============================================================================================
//===============================================================================================
//===============================================================================================
//===============================================================================================

