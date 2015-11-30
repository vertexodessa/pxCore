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



rtError getScene(int numArgs, const rtValue* args, rtValue* result, void* ctx); // fwd

//static void disposeNode(const FunctionCallbackInfo<Value>& args); //fwd

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

pxEventLoop eventLoop;

class testWindow: public pxViewWindow
{
public:

  void setScene(rtNodeContextRef ctx, pxScene2dRef s)
  {
    rtValue v = new rtFunctionCallback(getScene, s.getPtr());
    ctx->add("getScene", v);

    mScene = s;
  }

  void onCloseRequest()
  {
    // When someone clicks the close box no policy is predefined.
    // so we need to explicitly tell the event loop to exit
    eventLoop.exit();
  }
private:

  pxScene2dRef        mScene;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static testWindow win;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

rtError getScene(int numArgs, const rtValue* args, rtValue* result, void* ctx)
{
  // We don't use the arguments so just return the scene object referece

  if (result)
  {
    pxScene2dRef s = (pxScene2d*)ctx;
    *result = s; // return the scene reference
  }

  return RT_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//static void disposeNode(const FunctionCallbackInfo<Value>& args)
//{
//  printf("DEBUG:  disposeNode() ... ENTER \n");
//
//  if (args.Length() < 1)
//    return;
//
//  if (!args[0]->IsObject())
//    return;
//
//  Local<Object> obj = args[0]->ToObject();

//  rtObjectWrapper* wrapper = static_cast<rtObjectWrapper *>(obj->GetAlignedPointerFromInternalField(0));
//  if (wrapper)
//    wrapper->dispose();
//}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

args_t *s_gArgs;

int main(int argc, char** argv)
{
//  args_t aa(/*argc*/ 1, argv);   // HACK ... not sure why 'node' chokes on args of "rtNode start.js"

  args_t aa(argc, argv);
  s_gArgs = &aa;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // Setup node CONTEXT...
  //
  rtNode myNode;

  rtNodeContextRef ctx = myNode.createContext();

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

  win.setScene(ctx, scene);

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // RUN !
  //

  rtValue junk("Hello from add() World!");
  ctx->add("junk", junk);

  ctx->runScript("1+2");
  ctx->runScript("2+3");
  ctx->runScript("3+4");
  ctx->runScript("4+5");
  ctx->runScript("var blah = \"Hello World\"");

  ctx->runFile("start.js");

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//  use_debug_agent = true;
//  debug_wait_connect = true;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  eventLoop.run();

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  myNode.term();

  return 0;
}
