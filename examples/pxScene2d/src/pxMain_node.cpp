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
  printf("DEBUG:  getScene() ... ENTER \n");

  setupScene(args);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void disposeNode(const FunctionCallbackInfo<Value>& args)
{
  printf("DEBUG:  disposeNode() ... ENTER \n");

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

#if 0
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // Add RECTANGLE to SCENE
  //
  rtObjectRef p;

  scene.sendReturns<rtObjectRef>("createRectangle", p);

  rtObjectRef root = scene.get<rtObjectRef>("root");

  p.set("x", 21);
  p.set("y", 20);
  p.set("w", 300);
  p.set("h", 30);
  p.set("fillColor",0xfeff00ff);
  p.set("lineColor",0xffffff80);
  p.set("lineWidth", 10);
  p.set("parent", root);

 // p.send("animateTo", "h", 600, 0.5, 0, 0);

// ctx->addObject("scene", scene);
   ctx->addObject("myRect", p);


#endif

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

 // Persistent<Object> javaScene;

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

// ctx->run(" var before = 'BEFORE: myRect.x = ' + myRect.x;  myRect.x += 300; var after = '... AFTER: myRect.x = '+ myRect.x;  before + after;");

// ctx->run("rtTest.js");
//  ctx->run("fancyp.js");
//  ctx->run("on_demand.js");
 // ctx->run("playmask.js");
  //  ctx->run("editor.js");
//  ctx->run("events.js");
  ctx->run("start.js");

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  eventLoop.run();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  printf("\n\nDEBUG: main() - Exiting...");

  myNode.term();


  return 0;
}
