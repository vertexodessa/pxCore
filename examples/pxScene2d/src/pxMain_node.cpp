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

#include <pthread.h>

using namespace v8;
using namespace node;



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


rtError getScene(int numArgs, const rtValue* args, rtValue* result, void* ctx); // fwd

//static void disposeNode(const FunctionCallbackInfo<Value>& args); //fwd

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

pxEventLoop eventLoop;

class testWindow: public pxViewWindow
{
public:

  std::string debug_name;

//  virtual void onAnimationTimer()
//  {
//    printf("\nDEBUG: onAnimationTimer()) - debug_name = %s  \n", debug_name.c_str());

//    pxViewWindow::onAnimationTimer();
//  }

  virtual void onKeyDown(uint32_t keycode, uint32_t flags)
  {
     printf("\nDEBUG: %s()) - debug_name = %s  \n",__FUNCTION__, debug_name.c_str());

     pxViewWindow::onKeyDown(keycode, flags);
  }

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

static testWindow win1;
static testWindow win2;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

rtError getScene(int numArgs, const rtValue* args, rtValue* result, void* ctx)
{
  // We don't use the arguments so just return the scene object reference
  if (result)
  {
    pxScene2dRef s = (pxScene2d*)ctx;

    printf("\n\n #############\n #############  getScene() >>  s = %p\n #############\n\n", (void *) s);

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


void sleep(unsigned int mseconds)
{
    clock_t goal = mseconds + clock();
    while (goal > clock());
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define USE_CONTEXT_1
#define USE_WINDOW_1

#define USE_CONTEXT_2
#define USE_WINDOW_2

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

#ifdef USE_CONTEXT_1

  printf("\n### Node 1A"); // ##############################

  rtNode node1;

  printf("\n### Node 1B"); // ##############################

  rtNodeContextRef ctx1 = node1.createContext();

  printf("\n### Context 1A"); // ##############################

  node1.init(s_gArgs->argc, s_gArgs->argv);

  printf("\n### Context 1B"); // ##############################
#endif

#ifdef USE_CONTEXT_2

//  printf("\n### Node 2A"); // ##############################

//  rtNode node2;

//  printf("\n### Node 2B"); // ##############################

  rtNodeContextRef ctx2 = node1.createContext();

//  printf("\n### Context 2A"); // ##############################

//  node2.init(s_gArgs->argc, s_gArgs->argv);

  printf("\n### Context 2B"); // ##############################
#endif

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // Setup WINDOW and SCENE
  //
#ifdef USE_WINDOW_1

//  printf("\n### Window 1A"); // ##############################

  pxScene2dRef scene1 = new pxScene2d;

  win1.init(810, 10, 800, 600);

//  printf("\n### Window 1B\n"); // ##############################

  win1.setTitle(">> Window 1 <<");
  win1.setVisibility(true);
  win1.setView(scene1);
  win1.setAnimationFPS(60);

  win1.debug_name = "WindowOne";

  win1.setScene(ctx1, scene1);

  scene1->init();

  printf("\n### Window 1C"); // ##############################

#endif

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // Setup WINDOW and SCENE
  //
#ifdef USE_WINDOW_2

//  printf("\n### Window 2A"); // ##############################

  pxScene2dRef scene2 = new pxScene2d;

  win2.init(10, 10, 750, 550);

  win2.setTitle(">> Window 2 <<");
  win2.setVisibility(true);
  win2.setView(scene2);
  win2.setAnimationFPS(60);

//  printf("\n### Window 2B\n"); // ##############################

  win2.debug_name = "WindowTwo";

  win2.setScene(ctx2, scene2);

  scene2->init();

  printf("\n### Window 2C"); // ##############################

#endif

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  //
  // RUN !
  //

#ifdef USE_CONTEXT_1

//  printf("\n### Context 1A"); // ##############################

//  rtValue junk1("Hello from add() World! - Context 1");
//  ctx1->add("junk", junk1);

//  ctx1->runScript("1+2");

//  printf("\n### Context 1B"); // ##############################

#endif

#ifdef USE_WINDOW_1
  printf("\n### Window Run 1A"); // ##############################

//  ctx1->runFile("start.js");
  ctx1->runFile("fancyp.js");

  printf("\n### Window Run 1B"); // ##############################
#endif

#ifdef USE_CONTEXT_2

//  printf("\n### Context 2A"); // ##############################

//  rtValue junk2("Hello from add() World! - Context 2");
//  ctx2->add("junk", junk2);

 // ctx2->runScript("99+11");

//  printf("\n### Context 2B"); // ##############################

#endif

#ifdef USE_WINDOW_2
//  printf("\n### Window Run 2A"); // ##############################

  ctx2->runFile("start.js");
//  ctx2->runFile("fancyp.js");

  printf("\n### Window Run 2B"); // ##############################
#endif

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//  use_debug_agent = true;
//  debug_wait_connect = true;

  printf("\n### eventLoop 1A"); // ##############################
  fflush(stdout);

  printf("\n### eventLoop 2A"); // ##############################

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if defined(USE_WINDOW_1) || defined(USE_WINDOW_2)
  eventLoop.run();
#endif
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// TODO - term() crashes !!!... fix it !

#ifdef USE_CONTEXT_1
//  node1.term();
#endif

#ifdef USE_CONTEXT_2
//  node2.term();
#endif

  return 0;
}

