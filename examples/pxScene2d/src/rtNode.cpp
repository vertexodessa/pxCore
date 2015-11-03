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


//#define USE_BASIC_EXAMPLE
//#define USE_NODESTART_EXAMPLE
#define USE_MY_ALLOCATOR

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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

  rtNode myNode;

  rtNodeContextRef ctx = myNode.createContext();

  node_isolate = ctx->mIsolate; // Must come first !!

  myNode.init(s_gArgs->argc, s_gArgs->argv);
  printf("Init Done.\n");

  printf("Start Run...\n");
  ctx->run("");

  printf("Run Done...\n");

  printf("\n Term...");
  myNode.term();
  printf("Done\n");

  return 0;
#endif

  return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

  printf("run() >> Initialize()...\n");

  V8::Initialize();
  node_is_initialized = true;

  printf("run() >> Initialize()... DONE\n");

  {//scope

    Locker                locker(mIsolate);
    Isolate::Scope isolate_scope(mIsolate);
    HandleScope     handle_scope(mIsolate);    // Create a stack-allocated handle scope.

    // Enter the context for compiling and running ... Persistent > Local
    Local<Context> local_context = Context::New(mIsolate);

    printf("run() >> CreateEnvironment()...\n");

    Environment* env = CreateEnvironment(
          mIsolate,
          uv_default_loop(),
          local_context,
          s_gArgs->argc,
          s_gArgs->argv,
          exec_argc,
          exec_argv);

    printf("run() >> CreateEnvironment()... DONE\n");

    Context::Scope context_scope(local_context);

    printf("run() >> LoadEnvironment()... \n");

    LoadEnvironment(env);

    printf("run() >> LoadEnvironment()... DONE\n");

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

  getchar();

  return  rtObjectRef(0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

rtNode::rtNode() :
  mPlatform(NULL), mPxNodeExtension(NULL)
{

}

void rtNode::init(int argc, char** argv)
{
  int exec_argc;
  const char** exec_argv;

  Init(&argc, const_cast<const char**>(argv), &exec_argc, &exec_argv);

  // Hack around with the argv pointer. Used for process.title = "blah".
  argv = uv_setup_args(argc, argv);
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

