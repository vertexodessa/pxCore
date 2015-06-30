#include "rtDefs.h"
#include "rtLog.h"
#include "pxContext.h"


#include <time.h> // JUNK


#include <directfb.h>

// Debug macros...
#define USE_DRAW_IMAGE
#define USE_DRAW_IMAGE_TEXTURE
#define USE_DRAW_RECT
#define USE_DRAW_DIAG_RECT
#define USE_DRAW_DIAG_LINE


extern IDirectFB                *dfb;
extern IDirectFBDisplayLayer    *dfbLayer;

extern DFBSurfaceDescription     dfbDescription;
extern IDirectFBSurface         *dfbSurface;
extern IDirectFBEventBuffer     *dfbBuffer;

extern DFBSurfacePixelFormat     dfbPixelformat;
extern bool needsFlip;

extern char* p2str(DFBSurfacePixelFormat fmt); // DEBUG


#define DFB_CHECK(x...)             \
{                                   \
  DFBResult err = x;                \
                                    \
  if (err != DFB_OK)                \
{                                   \
  rtLogError( " DFB died... " );    \
  DirectFBErrorFatal( #x, err );    \
  }                                 \
}


// /*pxContextSurfaceNativeDesc*/ pxTextureOffscreen  defaultContextSurface;
// /*pxContextSurfaceNativeDesc **/ pxTextureOffscreen  *currentContextSurface = &defaultContextSurface;

static IDirectFBSurface       *boundSurface;
static DFBSurfaceDescription  *boundDesc;
static bool                    useColor = false;

pxTextureRef defaultRenderSurface;
pxTextureRef currentRenderSurface = defaultRenderSurface;

struct point
{
  float x;
  float y;
  float s;
  float t;
};

//====================================================================================================================================================================================

class pxTextureOffscreen : public pxTexture    /// a.k.a. >> pxFBOTexture
{
  private:

    pxError createSurface(pxOffscreen& o)
    {
      mWidth  = o.width();
      mHeight = o.height();

      o.swizzleTo(dfbPixelformat == DSPF_ARGB ? PX_PIXEL_FMT_ARGB : PX_PIXEL_FMT_ABGR);  // Needed ?

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      dsc.width                 = o.width();
      dsc.height                = o.height();
      dsc.flags                 = DFBSurfaceDescriptionFlags(DSDESC_WIDTH | DSDESC_HEIGHT| DSDESC_PREALLOCATED | DSDESC_PIXELFORMAT);
      dsc.pixelformat           = dfbPixelformat;
      dsc.preallocated[0].data  = o.base();      // Buffer is your data
      dsc.preallocated[0].pitch = o.width() * 4;
      dsc.preallocated[1].data  = NULL;          // Not using a back buffer
      dsc.preallocated[1].pitch = 0;

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      IDirectFBSurface  *tmp;

      DFB_CHECK(dfb->CreateSurface( dfb, &dsc, &tmp ) );

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      dsc.width                 = o.width();
      dsc.height                = o.height();
      dsc.caps                  = DSCAPS_VIDEOONLY;//DSCAPS_VIDEOONLY;// DSCAPS_SYSTEMONLY;// DSCAPS_NONE;  //DSCAPS_VIDEOONLY   DSCAPS_PREMULTIPLIED
      dsc.flags                 = DFBSurfaceDescriptionFlags(DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT | DSDESC_CAPS);
      dsc.pixelformat           = dfbPixelformat;

      DFB_CHECK(dfb->CreateSurface( dfb, &dsc, &surface ) );

      DFB_CHECK(surface->Blit(surface, tmp, NULL, 0,0)); // blit copy to surface

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      tmp->Release(tmp);

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      return PX_OK;
    }

  public:
    pxTextureOffscreen() : mOffscreen(), surface(NULL)
    {
      mTextureType = PX_TEXTURE_OFFSCREEN;
    }

    pxTextureOffscreen(pxOffscreen& o) : mOffscreen(), surface(NULL)
    {
      mTextureType = PX_TEXTURE_OFFSCREEN;

      rtLogDebug("############# this: %p >>  %s - pxOffscreen \n",this, __PRETTY_FUNCTION__);

      createTexture(o);
    }

    ~pxTextureOffscreen()  { deleteTexture(); }


    void createTexture(pxOffscreen& o)
    {
      rtLogDebug("############# this: %p >>  %s(pxOffscreen o) \n", this, __PRETTY_FUNCTION__);

      mWidth  = o.width();
      mHeight = o.height();

      mOffscreen.init(o.width(), o.height());

      // Must retain the offscreen for the surface...
      memcpy( mOffscreen.base(), o.base(), mOffscreen.width()*mOffscreen.height()*4);

      createSurface(mOffscreen);
    }


    void createTexture(int w, int h)
    {
      rtLogDebug("############# this: %p >>  %s  WxH: %d x %d \n", this, __PRETTY_FUNCTION__, w,h);

      if(w <= 0 || h <= 0)
      {
        //rtLogDebug("\nERROR:  %s(%d, %d) - INVALID",__PRETTY_FUNCTION__, w,h);
        return;
      }

      if (surface)
      {
        deleteTexture();
      }

      rtLogDebug("############# this: %p >>  %s(%d, %d) \n", this, __PRETTY_FUNCTION__, w, h);

      mWidth  = w;
      mHeight = h;

      mOffscreen.init(w,h);

      createTexture(mOffscreen);

      //DFB_CHECK(surface->Clear(surface, 0xFF, 0x00, 0x00, 0xFF ) );  // DEBUG - CLEAR_RED !!!!
      //rtLogDebug("############# this: %p >>  %s - CLEAR_RED !!!!\n", this, __PRETTY_FUNCTION__);
   }


    pxError resizeTexture(int width, int height)
    {
      rtLogDebug("############# this: %p >>  %s  WxH: %d, %d\n", this, __PRETTY_FUNCTION__, width, height);

      if (mWidth != width || mHeight != height )
      {
        createTexture(width, height);
        return PX_OK;
      }

//      glActiveTexture(GL_TEXTURE3);
//      glBindTexture(GL_TEXTURE_2D, mTextureId);
//      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
//                   width, height, GL_RGBA,
//                   GL_UNSIGNED_BYTE, NULL);

//      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//      glUniform1f(u_alphatexture, 1.0);
      return PX_OK;
    }

    virtual pxError deleteTexture()
    {
      rtLogDebug("############# this: (virtual) %p >>  %s  ENTER\n",surface, __PRETTY_FUNCTION__);

      //return PX_OK;

      if(surface)
      {
        // if(currentRenderSurface == this)
        // {
        //   // Is this sufficient ? or Dangerous ?
        //   currentRenderSurface = NULL;
        //   boundSurface = NULL;
        // }

        surface->Release(surface);

        surface = NULL;
      }

      return PX_OK;
    }

    virtual pxError prepareForRendering()
    {
      rtLogDebug("############# this: (virtual) >>  %s  ENTER\n", __PRETTY_FUNCTION__);

      //      glBindFramebuffer(GL_FRAMEBUFFER, mFramebufferId);
      //      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
      //                             GL_TEXTURE_2D, mTextureId, 0);

      //      if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
      //      {
      //        if ((mWidth != 0) && (mHeight != 0))
      //        {
      //          rtLogWarn("error setting the render surface");
      //        }
      //        return PX_FAIL;
      //      }
      //      glActiveTexture(GL_TEXTURE3);
      //      glBindTexture(GL_TEXTURE_2D, mTextureId);
      //      glViewport ( 0, 0, mWidth, mHeight);
      //      glUniform2f(u_resolution, mWidth, mHeight);

      return PX_OK;
    }


    virtual pxError bindTexture()
    {
      if (!surface)
      {
        rtLogDebug("############# this: %p >>  %s  ENTER\n", this,__PRETTY_FUNCTION__);
        return PX_NOTINITIALIZED;
      }

      currentRenderSurface = this;
      boundSurface         = surface;
      boundDesc            = &dsc;

      useColor             = false;

      return PX_OK;
    }

    virtual pxError bindTextureAsMask()
    {
      //rtLogDebug("############# this: %p >>  %s  ENTER\n", this,__PRETTY_FUNCTION__);

      return PX_FAIL;
    }

    virtual pxError getOffscreen(pxOffscreen& o)
    {
      (void) o;
      rtLogDebug("############# this: %p >>  %s  ENTER\n", this, __PRETTY_FUNCTION__);

      //    if (!mInitialized)
      //    {
      //      return PX_NOTINITIALIZED;
      //    }
      //    o.init(mOffscreen.width(), mOffscreen.height());
      //    mOffscreen.blit(o);
      return PX_OK;
    }

    void setWidth(int w)  { dsc.width  = mWidth  = w; }
    void setHeight(int h) { dsc.height = mHeight = h; }

    virtual float width()  { return mWidth;  }
    virtual float height() { return mHeight; }

    void                    setSurface(IDirectFBSurface* s)     { surface  =s; };

    IDirectFBSurface*       getSurface()     { return surface; };
    DFBVertex*              getVetricies()   { return &v[0];   };
    DFBSurfaceDescription   getDescription() { return dsc;     };

    DFBVertex v[4];  //quad


  private:
    pxOffscreen             mOffscreen;

    int                   mWidth;
    int                   mHeight;

    IDirectFBSurface       *surface;
    DFBSurfaceDescription   dsc;
};// CLASS - pxTextureOffscreen

//====================================================================================================================================================================================

#warning TODO:  Use  pxContextSurfaceNativeDesc  within   pxTextureOffscreen ??
/*pxContextSurfaceNativeDesc*/ pxTextureOffscreen  defaultContextSurface;
/*pxContextSurfaceNativeDesc **/ pxTextureOffscreen  *currentContextSurface = &defaultContextSurface;

//====================================================================================================================================================================================

class pxTextureAlpha : public pxTexture
{
  public:
    pxTextureAlpha() : mDrawWidth(0.0), mDrawHeight (0.0), mImageWidth(0.0),
      mImageHeight(0.0), mBuffer(NULL), surface(NULL), mInitialized(false)
    {
      mTextureType = PX_TEXTURE_ALPHA;
    }

    pxTextureAlpha(float w, float h, float iw, float ih, void* buffer)
      : mDrawWidth(w), mDrawHeight (h), mImageWidth(iw),
        mImageHeight(ih), mBuffer(NULL), surface(NULL), mInitialized(false)
    {
      mTextureType = PX_TEXTURE_ALPHA;

      // copy the pixels
      int bitmapSize = ih * iw;

      mBuffer = malloc(bitmapSize);
      memcpy(mBuffer, buffer, bitmapSize);

      // TODO Moved this to bindTexture because of more pain from JS thread calls
      //    createTexture(w, h, iw, ih);
    }

    ~pxTextureAlpha()
    {
      if(mBuffer)
      {
        free(mBuffer);
      }
      deleteTexture();
    }

    void createTexture(float w, float h, float iw, float ih)
    {
      rtLogDebug("############# this: %p >>  %s  WxH: %.0f x %.0f \n", this, __PRETTY_FUNCTION__, w,h);

      if(w == 0 || h == 0)
      {
        return;
      }

      if(iw == 0 || ih == 0)
      {
        return;
      }

      if (mBuffer != NULL)
      {
        deleteTexture();
      }

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      mDrawWidth   = w;
      mDrawHeight  = h;
      mImageWidth  = iw;
      mImageHeight = ih;

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
      // Load DFB surface with TEXTURE

      dsc.width                 = iw;
      dsc.height                = ih;
      dsc.flags                 = (DFBSurfaceDescriptionFlags)(DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PREALLOCATED | DSDESC_PIXELFORMAT);// | DSDESC_CAPS);
//      dsc.caps                  = DSCAPS_VIDEOONLY;//DSCAPS_VIDEOONLY;// DSCAPS_SYSTEMONLY; //DSCAPS_NONE;  //DSCAPS_VIDEOONLY
      dsc.pixelformat           = DSPF_A8;      // ALPHA !!
      dsc.preallocated[0].data  = mBuffer;      // Buffer is your data
      dsc.preallocated[0].pitch = iw;
      dsc.preallocated[1].data  = NULL;         // Not using a back buffer
      dsc.preallocated[1].pitch = 0;

      DFB_CHECK (dfb->CreateSurface( dfb, &dsc, &surface));

      mInitialized = true;
    }

    virtual pxError deleteTexture()
    {
      rtLogDebug("############# this: (virtual) %p >>  %s  ENTER\n",surface, __PRETTY_FUNCTION__);

      if (surface)
      {
        surface->Release(surface);
      }
      mInitialized = false;
      return PX_OK;
    }

    virtual pxError bindTexture()
    {
      // TODO Moved to here because of js threading issues
      if (!mInitialized) createTexture(mDrawWidth,mDrawHeight,mImageWidth,mImageHeight);
      if (!mInitialized)
      {
        //rtLogDebug("############# this: %p >>  %s  PX_NOTINITIALIZED\n", this, __PRETTY_FUNCTION__);

        return PX_NOTINITIALIZED;
      }

      useColor = true;

      currentRenderSurface = this;
      boundSurface         = surface;
      boundDesc            = &dsc;

      return PX_OK;
    }

    virtual pxError bindTextureAsMask()
    {
      if (!mInitialized)
      {
        rtLogDebug("############# this: %p >>  %s  PX_NOTINITIALIZED\n", this, __PRETTY_FUNCTION__);

        return PX_NOTINITIALIZED;
      }

      currentRenderSurface = this;
      boundSurface         = surface;
      boundDesc            = &dsc;

      return PX_OK;
    }

    virtual pxError getOffscreen(pxOffscreen& o)
    {
      rtLogDebug("############# this: %p >>  %s  ENTER\n", this, __PRETTY_FUNCTION__);

      (void)o; // warning

      if (!mInitialized)
      {
        return PX_NOTINITIALIZED;
      }
      return PX_FAIL;
    }

        IDirectFBSurface*       getSurface()     { return surface; };
        DFBVertex*              getVetricies()   { return &v[0];   };
        DFBSurfaceDescription   getDescription() { return dsc;     };

        DFBVertex v[4];  //quad


  private:

    virtual float width()  { return mDrawWidth;  }
    virtual float height() { return mDrawHeight; }

    //  virtual float width()  { return dsc.width;  }
    //  virtual float height() { return dsc.height; }

  private:
    float mDrawWidth;
    float mDrawHeight;
    float mImageWidth;
    float mImageHeight;

    void                   *mBuffer;   // backing store
    IDirectFBSurface       *surface;

    DFBSurfaceDescription   dsc;

    bool        mInitialized;

}; // CLASS - pxTextureAlpha


//====================================================================================================================================================================================

static void draw9SliceRect(float x, float y, float w, float h, float x1, float y1, float x2, float y2)
{
  (void) x;  (void) y; (void) w; (void) h; (void) x1; (void) y1; (void) x2; (void) y2; // JUNK

#if 0
  float ox1 = x;
  float ix1 = x+x1;
  float ox2 = x+w;
  float ix2 = x+w-x2;
  float oy1 = y;
  float iy1 = y+y1;
  float oy2 = y+h;
  float iy2 = y+h-y2;

  const float verts[22][2] =
  {
    { ox1,oy1 },
    { ix1,oy1 },
    { ox1,iy1 },
    { ix1,iy1 },
    { ox1,iy2 },
    { ix1,iy2 },
    { ox1,oy2 },
    { ix1,oy2 },
    { ix2,oy2 },
    { ix1,iy2 },
    { ix2,iy2 },
    { ix1,iy1 },
    { ix2,iy1 },
    { ix1,oy1 },
    { ix2,oy1 },
    { ox2,oy1 },
    { ix2,iy1 },
    { ox2,iy1 },
    { ix2,iy2 },
    { ox2,iy2 },
    { ix2,oy2 },
    { ox2,oy2 }
  };

  const float colors[4][3] =
  {
    { 1, 0, 0 },
    { 0, 1, 0 },
    { 0, 1, 0 },
    { 0, 0, 1 }
  };
  const float uv[22][2] =
  {
    { 0, 0 },
    { 1, 0 },
    { 0, 1 },
    { 1, 1 }
  };
#endif


  //  {
  //    glUniform1f(u_alphatexture, 0.0);
  //    glVertexAttribPointer(attr_pos, 2, GL_FLOAT, GL_FALSE, 0, verts);
  //    glEnableVertexAttribArray(attr_pos);
  //    glDrawArrays(GL_TRIANGLE_STRIP, 0, 22);
  //    glDisableVertexAttribArray(attr_pos);
  //  }
}

// CALLED FROM >>> void pxContext::drawRect(float w, float h, float lineWidth, float* fillColor, float* lineColor)
//
static void drawRect2(float x, float y, float w, float h)
{
//  rtLogDebug(" %s  - STUB    xy: (%.0f, %.0f)  WxH: %d x %d \n",
//         __PRETTY_FUNCTION__, x, y, boundDesc->width, boundDesc->width);

  if(dfbSurface == 0)
  {
    rtLogError("cannot drawRect2() on context surface because surface is NULL");
    return;
  }

  DFB_CHECK (boundSurface->FillRectangle(dfbSurface, x, y, w, h));
}

// CALLED FROM >>> void pxContext::drawRect(float w, float h, float lineWidth, float* fillColor, float* lineColor)
//
static void drawRectOutline(float x, float y, float w, float h, float lw)
{
  if(dfbSurface == 0)
  {
    rtLogError("cannot drawRectOutline() on context surface because surface is NULL");
    return;
  }

  float half = lw/2;

  drawRect2(x-half, y-half,     lw + w, lw);      // TOP
  drawRect2(x-half, y-half + h, lw + w, lw);      // BOTTOM
  drawRect2(x-half, y-half,     lw,     lw + h);  // LEFT
  drawRect2(w-half, y-half,     lw,     lw + h);  // RIGHT

  //needsFlip = true;
}


// JUNK
// JUNK
// JUNK
// JUNK

class Timer
{
    timeval timer[2];

    struct timespec start_tm, end_tm;

  public:

    void start()
    {
        gettimeofday(&this->timer[0], NULL);
    }

    void stop()
    {
        gettimeofday(&this->timer[1], NULL);
    }

    long duration_ms() const // milliseconds
    {
        long secs( this->timer[1].tv_sec  - this->timer[0].tv_sec);
        long usecs(this->timer[1].tv_usec - this->timer[0].tv_usec);

        if(usecs < 0)
        {
            --secs;
            usecs += 1000000;
        }

        return static_cast<long>( (secs * 1000) + ((double) usecs / 1000.0 + 0.5));
    }
};

// JUNK
// JUNK
// JUNK
// JUNK

//static uint64_t sum = 0;
//static unsigned int blits = 0, fps = 0;
//static Timer timer;

static void drawImageTexture(float x, float y, float w, float h, pxTextureRef texture,
                             pxTextureRef mask, pxStretch xStretch, pxStretch yStretch, float* color)
{
//  printf("\n#############  >>  %s  - xy: (%f,%f)   ",__PRETTY_FUNCTION__,x,y);

#ifndef USE_DRAW_IMAGE_TEXTURE
  return;
#endif

  //rtLogDebug("\n#############  >>  %s  - xy: (%f,%f)   ",__PRETTY_FUNCTION__,x,y);

  if (texture.getPtr() == NULL)
  {
    rtLogDebug("\nINFO: ###################  nada\n");
    return;
  }

  float iw = texture->width();
  float ih = texture->height();

  if (iw <= 0 || ih <= 0)
  {
    //rtLogDebug("\nINFO: ###################  WxHh: 0x0   BAD\n");
    return;
  }

//  rtLogDebug("INFO: ###################  drawImageTexture() >> texture: %p   xy: (%f,%f)   WxH: %.0f x %.0f\n",
//         texture.getPtr(),x,y, texture->width(), texture->height());

  texture->bindTexture();

  if (mask.getPtr() != NULL)// && mask->getType() == PX_TEXTURE_ALPHA)
  {
    rtLogDebug("INFO: ###################  %s texture: %p WxH: %.0f x %.0f MASK MASK MASK\n", 
                   __PRETTY_FUNCTION__, texture.getPtr(), texture->width(), texture->height());
    mask->bindTexture();
  }

  if (xStretch == PX_NONE)  w = iw;
  if (yStretch == PX_NONE)  h = ih;

  float tw = 1.0;
  switch(xStretch)
  {
    case PX_NONE:
    case PX_STRETCH: tw = 1.0;  break;
    case PX_REPEAT:  tw = w/iw; break;
  }

  float th = 1.0;
  switch(yStretch)
  {
    case PX_NONE:
    case PX_STRETCH: th = 1.0;  break;
    case PX_REPEAT:  th = h/ih; break;
  }

  (void) tw; (void) th; // JUNK


  //  float firstTextureY  = 0;
  //  float secondTextureY = th;

  //  if (texture->getType() == PX_TEXTURE_FRAME_BUFFER)
  //  {
  //    //opengl renders to a framebuffer in reverse y coordinates than pxContext renders.
  //    //the texture y values need to be flipped
  //    firstTextureY  = th;
  //    secondTextureY = 0;
  //  }

  //  const float uv[4][2] =
  //  {
  //    { 0,  firstTextureY  },
  //    { tw, firstTextureY  },
  //    { 0,  secondTextureY },
  //    { tw, secondTextureY }
  //  };

  //  {
  //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  //    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  //    glUniform1f(u_alphatexture, 1.0);
  //    glVertexAttribPointer(attr_pos, 2, GL_FLOAT, GL_FALSE, 0, verts);
  //    glVertexAttribPointer(attr_uv, 2, GL_FLOAT, GL_FALSE, 0, uv);
  //    glEnableVertexAttribArray(attr_pos);
  //    glEnableVertexAttribArray(attr_uv);
  //    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  //    glDisableVertexAttribArray(attr_pos);
  //    glDisableVertexAttribArray(attr_uv);
  //  }

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  DFBRectangle rcSrc;

  if (!currentRenderSurface)
  {
    rtLogFatal("FATAL:  the current render surface is NULL !");
    return;
  }

  rcSrc.x = 0;
  rcSrc.y = 0;
  rcSrc.w = currentRenderSurface->width();
  rcSrc.h = currentRenderSurface->height();

  DFBRectangle rcDst;

  rcDst.x = x;
  rcDst.y = y;
  rcDst.w = w;
  rcDst.h = h;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if 0
  rtLogDebug("\n#############  dfbSurface: %p    boundSurface: %p", dfbSurface, boundSurface);
  rtLogDebug("\n#############  rcSrc: (%d,%d)   WxH: %d x %d ",rcSrc.x, rcSrc.y, rcSrc.w, rcSrc.h);
  rtLogDebug("\n#############  rcDst: (%d,%d)   WxH: %d x %d ",rcDst.x, rcDst.y, rcDst.w, rcDst.h);
#endif

#if 0
  // TEXTURED TRIANGLES...

  if(boundSurface != NULL)
  {
    pxTextureOffscreen *tex = (pxTextureOffscreen *) texture.getPtr();

    rtLogDebug("\nINFO: ###################  TextureTriangles()  !!!!!!!!!!!!!!!!");
    dfbSurface->TextureTriangles(dfbSurface, tex->getSurface(), tex->getVetricies(), NULL, 4, DTTF_STRIP);
  }
  else
  {
    rtLogDebug("\nINFO: ###################  TextureTriangles() fails");
  }
#else

  if(useColor)
  {
    DFB_CHECK(dfbSurface->SetBlittingFlags(dfbSurface, (DFBSurfaceBlittingFlags) (DSBLIT_COLORIZE | DSBLIT_BLEND_ALPHACHANNEL) ));

    DFB_CHECK(dfbSurface->SetColor( dfbSurface, color[0] * 255.0, // RGBA
                                                color[1] * 255.0,
                                                color[2] * 255.0,
                                                color[3] * 255.0));
  }

  if(xStretch == PX_REPEAT || yStretch == PX_REPEAT)
  {
    DFB_CHECK(dfbSurface->TileBlit(dfbSurface, boundSurface,  &rcDst, rcDst.w, rcDst.h));
  }
  else
  if(xStretch == PX_STRETCH || yStretch == PX_STRETCH)
  {
    DFB_CHECK(dfbSurface->StretchBlit(dfbSurface, boundSurface, &rcSrc, &rcDst));
  }
  else
  {
    DFB_CHECK(dfbSurface->Blit(dfbSurface, boundSurface, NULL, x,y));
  }

  needsFlip = true;

  if(color)
  {
    DFB_CHECK(dfbSurface->SetBlittingFlags(dfbSurface, (DFBSurfaceBlittingFlags) (DSBLIT_BLEND_ALPHACHANNEL) ));
  }

#endif
}



// CALLED FROM >>> void pxContext::drawImage9(float w, float h, float x1, float y1,
//                                            float x2, float y2, pxOffscreen& o)
//
static void drawImage92(float x, float y, float w, float h, float x1, float y1, float x2, float y2,
                         pxOffscreen& offscreen)
{
  (void) x;  (void) y; (void) w; (void) h; (void) x1; (void) y1; (void) x2; (void) y2; (void) offscreen; // JUNK

  //  glActiveTexture(GL_TEXTURE0);
  //  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
  //	       offscreen.width(), offscreen.height(), 0, GL_BGRA_EXT,
  //	       GL_UNSIGNED_BYTE, offscreen.base());

  //  glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
  //  glActiveTexture(GL_TEXTURE0);
  //  glUniform1i(u_texture, 0);

  //  float ox1 = x;
  //  float ix1 = x+x1;
  //  float ix2 = x+w-x2;
  //  float ox2 = x+w;

  //  float oy1 = y;
  //  float iy1 = y+y1;
  //  float iy2 = y+h-y2;
  //  float oy2 = y+h;



  //  float w2 = offscreen.width();
  //  float h2 = offscreen.height();

  //  float ou1 = 0;
  //  float iu1 = x1/w2;
  //  float iu2 = (w2-x2)/w2;
  //  float ou2 = 1;

  //  float ov1 = 0;
  //  float iv1 = y1/h2;
  //  float iv2 = (h2-y2)/h2;
  //  float ov2 = 1;

  //#if 1 // sanitize values
  //  iu1 = pxClamp<float>(iu1, 0, 1);
  //  iu2 = pxClamp<float>(iu2, 0, 1);
  //  iv1 = pxClamp<float>(iv1, 0, 1);
  //  iv2 = pxClamp<float>(iv2, 0, 1);

  //  float tmin, tmax;

  //  tmin = pxMin<float>(iu1, iu2);
  //  tmax = pxMax<float>(iu1, iu2);
  //  iu1 = tmin;
  //  iu2 = tmax;

  //  tmin = pxMin<float>(iv1, iv2);
  //  tmax = pxMax<float>(iv1, iv2);
  //  iv1 = tmin;
  //  iv2 = tmax;

  //#endif

  //  const float verts[22][2] =
  //  {
  //    { ox1,oy1 },
  //    { ix1,oy1 },
  //    { ox1,iy1 },
  //    { ix1,iy1 },
  //    { ox1,iy2 },
  //    { ix1,iy2 },
  //    { ox1,oy2 },
  //    { ix1,oy2 },
  //    { ix2,oy2 },
  //    { ix1,iy2 },
  //    { ix2,iy2 },
  //    { ix1,iy1 },
  //    { ix2,iy1 },
  //    { ix1,oy1 },
  //    { ix2,oy1 },
  //    { ox2,oy1 },
  //    { ix2,iy1 },
  //    { ox2,iy1 },
  //    { ix2,iy2 },
  //    { ox2,iy2 },
  //    { ix2,oy2 },
  //    { ox2,oy2 }
  //  };

  //  const float uv[22][2] =
  //  {
  //    { ou1,ov1 },
  //    { iu1,ov1 },
  //    { ou1,iv1 },
  //    { iu1,iv1 },
  //    { ou1,iv2 },
  //    { iu1,iv2 },
  //    { ou1,ov2 },
  //    { iu1,ov2 },
  //    { iu2,ov2 },
  //    { iu1,iv2 },
  //    { iu2,iv2 },
  //    { iu1,iv1 },
  //    { iu2,iv1 },
  //    { iu1,ov1 },
  //    { iu2,ov1 },
  //    { ou2,ov1 },
  //    { iu2,iv1 },
  //    { ou2,iv1 },
  //    { iu2,iv2 },
  //    { ou2,iv2 },
  //    { iu2,ov2 },
  //    { ou2,ov2 }
  //  };

  //  {
  //    glUniform1f(u_alphatexture, 1.0);
  //    glVertexAttribPointer(attr_pos, 2, GL_FLOAT, GL_FALSE, 0, verts);
  //    glVertexAttribPointer(attr_uv, 2, GL_FLOAT, GL_FALSE, 0, uv);
  //    glEnableVertexAttribArray(attr_pos);
  //    glEnableVertexAttribArray(attr_uv);

  //    glDrawArrays(GL_TRIANGLE_STRIP, 0, 22);

  //    glDisableVertexAttribArray(attr_pos);
  //    glDisableVertexAttribArray(attr_uv);
  //  }
}


void pxContext::init()
{
  boundSurface = dfbSurface;  // needed here.

  DFB_CHECK(dfbSurface->SetRenderOptions(dfbSurface, DSRO_MATRIX));

  rtLogSetLevel(RT_LOG_INFO); // LOG LEVEL

//  rtLogDebug("#############   dfbSurface: %p >>  %s ..  \n",dfbSurface, __PRETTY_FUNCTION__);
//  rtLogDebug("############# boundSurface: %p >>  %s ..  \n",boundSurface, __PRETTY_FUNCTION__);
}

void pxContext::setSize(int w, int h)
{
  rtLogDebug("############# this: %p >>  %s ..  WxH: %d x %d \n",this, __PRETTY_FUNCTION__,  w, h);

  //  glViewport(0, 0, (GLint)w, (GLint)h);
  //  glUniform2f(u_resolution, w, h);

  if (currentRenderSurface == defaultRenderSurface)
  {
    defaultContextSurface.setWidth(w);
    defaultContextSurface.setHeight(h);

    rtLogDebug("############# this: %p >>  %s .. defaultContextSurface ..  WxH: %d x %d \n",this, __PRETTY_FUNCTION__,  w, h);
  }
}


void pxContext::clear(int w, int h)
{
  (void) w; (void) h;

  if(dfbSurface == 0)
  {
    rtLogError("cannot clear on context surface because surface is NULL");
    return;
  }

  DFB_CHECK( dfbSurface->Clear( dfbSurface, 0x00, 0x00, 0x00, 0x00 )         ); // TRANSPARENT
  //DFB_CHECK( dfbSurface->Clear( dfbSurface, 0x00, 0x00, 0xFF, 0xFF )       );  //  CLEAR_BLUE   << JUNK
  //  DFB_CHECK( dfbSurface->Flip(  dfbSurface, NULL, (DFBSurfaceFlipFlags) 0) );
}

void pxContext::setMatrix(pxMatrix4f& mm)
{
  const float *m = mm.data();

  s32 matrix[9];

  // Convert to fixed point...s
  matrix[0] = (s32)(m[0]  * 0x10000);
  matrix[1] = (s32)(m[4]  * 0x10000);
  matrix[2] = (s32)(m[12] * 0x10000);
  matrix[3] = (s32)(m[1]  * 0x10000);
  matrix[4] = (s32)(m[5]  * 0x10000);
  matrix[5] = (s32)(m[13] * 0x10000);

  matrix[6] = 0x00000;
  matrix[7] = 0x00000;
  matrix[8] = 0x10000;

  //DFB_CHECK(dfbSurface->SetRenderOptions(dfbSurface, DSRO_MATRIX)); // JUNK

  DFB_CHECK(dfbSurface->SetMatrix( dfbSurface, matrix ));
}

void pxContext::setAlpha(float a)
{
  (void) a;

  // rtLogDebug("\n setAlpha()  - STUB");

  // DFB_CHECK( window->SetOpacity(window, a * 255.0) );
}

pxTextureRef pxContext::createContextSurface(int width, int height)
{
  pxTextureOffscreen* texture = new pxTextureOffscreen();
  texture->createTexture(width, height);

  rtLogDebug("############# this: %p >>  %s(%d,%d)  texture: %p \n",this, __PRETTY_FUNCTION__,  width, height, texture);

  return texture;
}

pxError pxContext::updateContextSurface(pxTextureRef texture, int width, int height)
{
  rtLogDebug("############# this: %p >>   %s()  - STUB\n", this, __PRETTY_FUNCTION__);

  if (texture.getPtr() == NULL)
  {
    return PX_FAIL;
  }
  
  return texture->resizeTexture(width, height);
}

pxTextureRef pxContext::getCurrentRenderSurface()
{
  rtLogDebug("############# this: %p >>  %s = %p\n", this, __PRETTY_FUNCTION__, currentRenderSurface.getPtr());

  return currentRenderSurface;
}

pxError pxContext::setRenderSurface(pxTextureRef texture)
{
  rtLogDebug("############# this: %p >>  %s  %p   \n", this, __PRETTY_FUNCTION__, texture.getPtr());

  if(texture.getPtr() == NULL)
  {
//    rtLogDebug("############# this: %p >>  %s  %p  at %d x %d\n",this, __PRETTY_FUNCTION__,
//           texture.getPtr(), defaultContextSurface.width(), defaultContextSurface.height());

    defaultContextSurface.createTexture(defaultContextSurface.width(), defaultContextSurface.height());

    currentRenderSurface = defaultRenderSurface;

    return PX_OK;
  }

  rtLogDebug("############# this: %p >>  %s - currentRenderSurface = texture (WxH: %.0f x %.0f)  \n",
         this, __PRETTY_FUNCTION__,  texture->width(), texture->height() );

  currentRenderSurface = texture;
  currentRenderSurface->bindTexture();

  return texture->prepareForRendering();
}


pxError pxContext::deleteContextSurface(pxTextureRef texture)
{
  rtLogDebug("############# this: %p >>  %s  - STUB\n", this, __PRETTY_FUNCTION__);

  if(texture.getPtr() == NULL)
  {
    rtLogError("cannot deleteContextSurface() on context surface because surface is NULL");
    return PX_FAIL;
  }

//    IDirectFBSurface *surface = contextSurface->surface; //alias

//    if(surface != 0)
//    {
//       surface->Release(surface);
//    }

  //  if (contextSurface->framebuffer != 0)
  //  {
  //    glDeleteFramebuffers(1, &contextSurface->framebuffer);
  //    contextSurface->framebuffer = 0;
  //  }
  //  if (contextSurface->texture != 0)
  //  {
  //    glDeleteTextures(1, &contextSurface->texture);
  //    contextSurface->texture = 0;
  //  }
  return PX_OK;
}

void pxContext::drawRect(float w, float h, float lineWidth, float* fillColor, float* lineColor)
{
#ifndef USE_DRAW_RECT
  return;
#endif

  if(fillColor == NULL && lineColor == NULL)
  {
    rtLogError("cannot drawRect() on context surface because colors are NULL");
    return;
  }

  if(boundSurface == NULL)
  {
    rtLogError("cannot drawRect() on context surface because boundSurface is NULL");
    return;
  }

  if(fillColor != NULL && fillColor[3] > 0.0) // with non-transparent color
  {
//    rtLogError("OK >> drawRect() on context surface  (%.1f, %.1f, %.1f,  %.1f) ",
//               fillColor[0],fillColor[1],fillColor[2], fillColor[3]);

    // Draw FILL rectangle for smaller FILL areas
    DFB_CHECK (boundSurface->SetColor(dfbSurface, fillColor[0] * 255.0, // RGBA
                                                  fillColor[1] * 255.0,
                                                  fillColor[2] * 255.0,
                                                  fillColor[3] * 255.0));
    float half = lineWidth/2;

    drawRect2(half, half, w-lineWidth, h-lineWidth);
  }

  if(lineColor != NULL && lineColor[3] > 0.0 && lineWidth > 0) // with non-transparent color and non-zero stroke
  {
//    rtLogError("OK >> drawRect() on context surface  (%.1f, %.1f, %.1f,  %.1f) ",
//               lineColor[0],lineColor[1],lineColor[2], lineColor[3]);

    DFB_CHECK (boundSurface->SetColor(dfbSurface, lineColor[0] * 255.0, // RGBA
                                                  lineColor[1] * 255.0,
                                                  lineColor[2] * 255.0,
                                                  lineColor[3] * 255.0));

    drawRectOutline(0, 0, w, h, lineWidth);
  }
}
  
void pxContext::drawImage9(float w, float h, float x1, float y1,
                           float x2, float y2, pxOffscreen& o)
{
  //if (texture.getPtr() != NULL)
  {
    drawImage92(0, 0, w, h, x1, y1, x2, y2, o);
  }
}

void pxContext::drawImage(float x, float y, float w, float h, pxTextureRef t, pxStretch xStretch, pxStretch yStretch)
{
   drawImage(x, y, w, h, t, pxTextureRef(), xStretch, yStretch);
}

void pxContext::drawImage(float x, float y, float w, float h, pxTextureRef t, pxTextureRef mask,
                          pxStretch xStretch, pxStretch yStretch, float* color)
{
#ifndef USE_DRAW_IMAGE
  return;
#endif

  // rtLogDebug("############# this: %p >>  %s  - STUBBY\n", this, __PRETTY_FUNCTION__);

  float black[4] = {0,0,0,1};
  drawImageTexture(x, y, w, h, t, mask, xStretch, yStretch, color ? color : black);
}

void pxContext::drawDiagRect(float x, float y, float w, float h, float* color)
{
#ifndef USE_DRAW_DIAG_RECT
  return;
#endif

  if (!mShowOutlines) return;

  if(boundSurface == NULL)
  {
    rtLogError("cannot drawDiagRect() on context surface because boundSurface is NULL");
    return;
  }

  if(w == 0.0 || h == 0.0)
  {
    rtLogError("cannot drawDiagLine() on context surface because boundSurface is NULL");
    return;
  }

  if(useColor)
  {
    DFB_CHECK (boundSurface->SetColor(dfbSurface, color[0] * 255.0, // RGBA
                                                  color[1] * 255.0,
                                                  color[2] * 255.0,
                                                  color[3] * 255.0));

    DFB_CHECK (boundSurface->DrawRectangle(dfbSurface, x, y, w, h));
  }
}

void pxContext::drawDiagLine(float x1, float y1, float x2, float y2, float* color)
{
#ifndef USE_DRAW_DIAG_LINE
  return;
#endif

  if (!mShowOutlines) return;

  if(boundSurface == NULL)
  {
    rtLogError("cannot drawDiagLine() on context surface because boundSurface is NULL");
    return;
  }

  DFB_CHECK (boundSurface->SetColor(dfbSurface, color[0] * 255.0, // RGBA
                                                color[1] * 255.0,
                                                color[2] * 255.0,
                                                color[3] * 255.0));

  DFB_CHECK (boundSurface->DrawLine(dfbSurface, x1,y1, x2,y2));
}

pxTextureRef pxContext::createTexture(float w, float h, float iw, float ih, void* buffer)
{
  rtLogDebug("############# this: %p >>  %s  WxH: %.0f x %.0f \n", this, __PRETTY_FUNCTION__, w,h);

  pxTextureAlpha* alphaTexture = new pxTextureAlpha(w,h,iw,ih,buffer);
  return alphaTexture;
}

pxTextureRef pxContext::createTexture(pxOffscreen& o)
{
  rtLogDebug("############# this: %p >>  %s  WxH: %d x %d \n", this, __PRETTY_FUNCTION__, o.width(), o.height());

  pxTextureOffscreen* offscreenTexture = new pxTextureOffscreen(o);
  return offscreenTexture;
}
