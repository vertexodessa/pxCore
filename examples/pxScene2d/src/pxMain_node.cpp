// pxCore Copyright 2007-2015 John Robinson
// main.cpp

#ifdef WIN32
#include <windows.h>
#endif

#include <stdio.h>

#include "pxCore.h"
#include "pxEventLoop.h"
#include "pxWindow.h"
#include "pxScene2d.h"
#include "pxViewWindow.h"

#include "rtNode.h"
#include "jsbindings/rtObjectWrapper.h"
#include "jsbindings/rtFunctionWrapper.h"

#include "node.h"
#include "node_javascript.h"


using namespace v8;
using namespace node;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

pxEventLoop eventLoop;

class testWindow: public pxViewWindow
{
public:

  Local<Object> scene(Isolate* isolate) const
  {
    return node::PersistentToLocal(isolate, mJavaScene);
  }

  void setScene(Isolate *isolate, pxScene2dRef s)
  {
    mScene = s;
    mJavaScene.Reset(isolate, rtObjectWrapper::createFromObjectReference(isolate, mScene.getPtr()));
  }

  void onCloseRequest()
  {
    // When someone clicks the close box no policy is predefined.
    // so we need to explicitly tell the event loop to exit
    eventLoop.exit();
  }
private:

  Persistent<Object> mJavaScene;
  pxScene2dRef mScene;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static testWindow win;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setupScene(const FunctionCallbackInfo<Value>& args)
{
  EscapableHandleScope scope(args.GetIsolate());

  args.GetReturnValue().Set(scope.Escape(win.scene(args.GetIsolate())));
}

static void getScene(const FunctionCallbackInfo<Value>& args)
{
  setupScene(args);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void disposeNode(const FunctionCallbackInfo<Value>& args)
{
//  printf("DEBUG:  disposeNode() ... ENTER \n");

  if (args.Length() < 1)
    return;

  if (!args[0]->IsObject())
    return;

  Local<Object> obj = args[0]->ToObject();

  rtObjectWrapper* wrapper = static_cast<rtObjectWrapper *>(obj->GetAlignedPointerFromInternalField(0));
  if (wrapper)
    wrapper->dispose();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

args_t *s_gArgs;

int main(int argc, char** argv)
{
  args_t aa(argc, argv);
  s_gArgs = &aa;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // Setup node CONTEXT...
  //
  rtNode myNode;

  rtNodeContextRef ctx = myNode.createContext();

  node_isolate = ctx->mIsolate; // Must come first !!

  myNode.init(s_gArgs->argc, s_gArgs->argv);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // Setup WINDOW and SCENE
  //
  pxScene2dRef scene = new pxScene2d;

  int width  = 800;
  int height = 600;

  win.init(10, 10, width, height);

  win.setTitle("pxCore!");
  win.setVisibility(true);
  win.setView(scene);
  win.setAnimationFPS(60);

  scene->init();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  Isolate::Scope isolate_scope(ctx->mIsolate);
  HandleScope     handle_scope(ctx->mIsolate);    // Create a stack-allocated handle scope.

  // Get a Local context...
  Local<Context> local_context = node::PersistentToLocal<Context>(ctx->mIsolate, ctx->mContext );
  Context::Scope context_scope(local_context);

  Handle<Object>  global = local_context->Global();

  win.setScene(ctx->mIsolate, scene);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  global->Set(String::NewFromUtf8(ctx->mIsolate, "getScene"),
              FunctionTemplate::New(ctx->mIsolate, getScene)->GetFunction());

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  global->Set(String::NewFromUtf8(ctx->mIsolate, "dispose"),
              FunctionTemplate::New(ctx->mIsolate, disposeNode)->GetFunction());

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  global->Set(String::NewFromUtf8(ctx->mIsolate, "px"),
              String::NewFromUtf8(ctx->mIsolate, "dummy value"));

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  if(argc >= 2)
  {
//    printf("\n\n#### Running USER JavaScript... [ %s ]\n", argv[1]);
//    ctx->run("start.js");
    ctx->run(argv[1]);
  }
  else // Default to ...
  {
//    printf("\n\n#### Running DEFAULT JavaScript...\n");

    ctx->run("start.js");
  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  eventLoop.run();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  myNode.term();

  return 0;
}
