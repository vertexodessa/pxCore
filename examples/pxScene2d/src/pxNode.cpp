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

v8::Handle<v8::ObjectTemplate>  globaltemplate;
v8::Handle<v8::Object>          processref;
v8::Persistent<v8::Context>     contextref;


int main(int argc, char** argv)
{
  args_t aa(argc, argv);

#if 1
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
