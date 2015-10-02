// pxCore CopyRight 2007-2015 John Robinson
// main.cpp

#ifdef WIN32
#include <windows.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

#include "pxCore.h"
#include "pxEventLoop.h"
#include "pxWindow.h"
#include "pxViewWindow.h"

#include "rtLog.h"
#include "rtRefT.h"
#include "rtPathUtils.h"
#include "pxScene2d.h"

#include "testScene.h"

#include "pxFileDownloader.h"

//extern rtRefT<pxScene2d> scene;

#define LIB_NODE_MAIN

#ifdef LIB_NODE_MAIN
//===============================================================================================

#include "node.h"
#include "v8.h"

typedef struct args_
{
  int    argc;
  char **argv;

  args_() { argc = 0; argv = NULL; }
  args_(int n = 0, char** a = NULL) : argc(n), argv(a) {}
}
args_t;

//===============================================================================================

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

//===============================================================================================

int main(int argc, char* argv[])
{
  args_t aa(argc, argv);

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

  return 0;
}

#endif // LIB_NODE_MAIN
//===============================================================================================

pxEventLoop eventLoop;

class testWindow: public pxViewWindow
{
private:
  void onCloseRequest()
  {
    // When someone clicks the close box no policy is predefined.
    // so we need to explicitly tell the event loop to exit
    eventLoop.exit();
  }  
};

int pxMain()
{
#ifndef LIB_NODE_MAIN
  int width = 1280;
  int height = 720;

  testWindow win;
  win.init(10, 10, width, height);

  win.setTitle("pxCore!");
  win.setVisibility(true);
  win.setView(testScene());
  win.setAnimationFPS(60);

#ifndef __APPLE__
  testWindow win2;

  win2.init(110, 110, width, height);

  win2.setTitle("pxCore! 2");
  win2.setVisibility(true);
  win2.setView(testScene());
  win2.setAnimationFPS(60);
#endif


  eventLoop.run();
#endif

  return 0;
}

