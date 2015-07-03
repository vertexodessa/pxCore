#ifndef PX_CONTEXT_DESC_H
#define PX_CONTEXT_DESC_H

#include <directfb.h>

typedef struct _pxContextSurfaceNativeDesc
{
   _pxContextSurfaceNativeDesc() : framebuffer(0), texture(0),
      width(0), height(0),previousContextSurface(NULL) {}

   DFBSurfaceDescription dsc;

//   IDirectFBSurface      *surface;

   IDirectFBSurface      *framebuffer;
   IDirectFBSurface      *texture;

   int width;
   int height;
  _pxContextSurfaceNativeDesc* previousContextSurface;
}
pxContextSurfaceNativeDesc;

#endif //PX_CONTEXT_DESC_H
