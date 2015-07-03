#include "rtDefs.h"
#include "rtLog.h"
#include "pxContext.h"


//#include <time.h> // JUNK


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


#define DFB_CHECK(x...)              \
{                                    \
   DFBResult err = x;                \
                                     \
   if (err != DFB_OK)                \
   {                                 \
      rtLogError( " DFB died... " ); \
      DirectFBErrorFatal( #x, err ); \
   }                                 \
}

//====================================================================================================================================================================================

//#warning TODO:  Use  pxContextSurfaceNativeDesc  within   pxFBOTexture ??

pxContextSurfaceNativeDesc  defaultContextSurface;
pxContextSurfaceNativeDesc* currentContextSurface = &defaultContextSurface;

pxContextFramebufferRef defaultFramebuffer(new pxContextFramebuffer());
pxContextFramebufferRef currentFramebuffer = defaultFramebuffer;

// TODO get rid of this global crap

static int gResW, gResH;
static pxMatrix4f gMatrix;
static float gAlpha = 1.0;

//====================================================================================================================================================================================


static IDirectFBSurface       *boundMask;
static IDirectFBSurface       *boundTexture;
static IDirectFBSurface       *boundFramebuffer;

void premultiply(float* d, const float* s)
{
   d[0] = s[0]*s[3];
   d[1] = s[1]*s[3];
   d[2] = s[2]*s[3];
   d[3] = s[3];
}

//====================================================================================================================================================================================

class pxFBOTexture : public pxTexture
{
public:
   pxFBOTexture() : mOffscreen(), mWidth(0), mHeight(0), mTexture(NULL)
   {
      mTextureType = PX_TEXTURE_FRAME_BUFFER;
   }

   ~pxFBOTexture()  { deleteTexture(); }

   void createTexture(int w, int h)
   {
      rtLogDebug("############# this: %p >>  %s  WxH: %d x %d \n", this, __PRETTY_FUNCTION__, w,h);

      if (mTexture)
      {
         deleteTexture();
      }

      if(w <= 0 || h <= 0)
      {
         //rtLogDebug("\nERROR:  %s(%d, %d) - INVALID",__PRETTY_FUNCTION__, w,h);
         return;
      }

      rtLogDebug("############# this: %p >>  %s(%d, %d) \n", this, __PRETTY_FUNCTION__, w, h);

      mWidth  = w;
      mHeight = h;

      mOffscreen.init(w,h);

      createSurface(mOffscreen); // surface is framebuffer

      DFB_CHECK(mTexture->Clear(mTexture, 0x00, 0x00, 0x00, 0x00 ) );  // TRANSPARENT

      //DFB_CHECK(mTexture->Clear(mTexture, 0x00, 0xFF, 0x00, 0xFF ) );  // DEBUG - GREEN !!!!

      //rtLogDebug("############# this: %p >>  %s - CLEAR_GREEN !!!!\n", this, __PRETTY_FUNCTION__);
   }

   pxError resizeTexture(int width, int height)
   {
      rtLogDebug("############# this: %p >>  %s  WxH: %d, %d\n", this, __PRETTY_FUNCTION__, width, height);

      if (mWidth != width || mHeight != height || !mTexture )
      {
         createTexture(width, height);
         return PX_OK;
      }

      // glActiveTexture(GL_TEXTURE3);
      // glBindTexture(GL_TEXTURE_2D, mTextureId);
      // glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
      //              width, height, GL_RGBA,
      //              GL_UNSIGNED_BYTE, NULL);

      // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      // glUniform1f(u_alphatexture, 1.0);

      boundFramebuffer = mTexture;

      return PX_OK;
   }

   virtual pxError deleteTexture()
   {
      rtLogDebug("############# this: (virtual) %p >>  %s  ENTER\n",mTexture, __PRETTY_FUNCTION__);

      if(mTexture)
      {
         // if(currentFramebuffer == this)
         // {
         //   // Is this sufficient ? or Dangerous ?
         //   currentFramebuffer = NULL;
         //   boundTexture = NULL;
         // }

         mTexture->Release(mTexture);
         mTexture = NULL;
      }

      return PX_OK;
   }

   virtual pxError prepareForRendering()
   {
	  //glBindFramebuffer(GL_FRAMEBUFFER, mFramebufferId);

      rtLogDebug("############# this: (virtual) >>  %s  ENTER\n", __PRETTY_FUNCTION__);

	  boundFramebuffer = mTexture;

      gResW = mWidth;
      gResH = mHeight;
    
      return PX_OK;
   }

  // TODO get rid of pxError
  // TODO get rid of bindTexture() and bindTextureAsMask()
   virtual pxError bindGLTexture(int tLoc)
   {
      (void) tLoc;

      if (mTexture == NULL)
      {
         rtLogDebug("############# this: %p >>  %s  ENTER\n", this,__PRETTY_FUNCTION__);
         return PX_NOTINITIALIZED;
      }

      boundTexture = mTexture;

      return PX_OK;
   }

   virtual pxError bindGLTextureAsMask(int mLoc)
   {
      (void) mLoc;

      if (!mTexture)
      {
         rtLogDebug("############# this: %p >>  %s  ENTER\n", this,__PRETTY_FUNCTION__);
         return PX_NOTINITIALIZED;
      }

      boundMask  = mTexture;

      return PX_OK;
   }

#if 1 // Do we need this?  maybe for some debugging use case??
  virtual pxError getOffscreen(pxOffscreen& o)
  {
    (void)o;
    // TODO
    return PX_FAIL;
  }
#endif

   virtual int width()   { return mWidth;  }
   virtual int height()  { return mHeight; }

   void setWidth(int w)  { dsc.width  = mWidth  = w; }
   void setHeight(int h) { dsc.height = mHeight = h; }

   void                    setSurface(IDirectFBSurface* s)     { mTexture  = s; };

   IDirectFBSurface*       getSurface()     { return mTexture; };
   DFBVertex*              getVetricies()   { return &v[0];   };
   DFBSurfaceDescription   getDescription() { return dsc;     };

   DFBVertex v[4];  //quad


private:
   pxOffscreen             mOffscreen;

   int                     mWidth;
   int                     mHeight;

   IDirectFBSurface       *mTexture;
   DFBSurfaceDescription   dsc;

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
      dsc.caps                  = DSCAPS_VIDEOONLY; //   DSCAPS_PREMULTIPLIED
      dsc.flags                 = DFBSurfaceDescriptionFlags(DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT | DSDESC_CAPS);
      dsc.pixelformat           = dfbPixelformat;

      DFB_CHECK(dfb->CreateSurface( dfb, &dsc, &mTexture ) );

      DFB_CHECK(mTexture->Blit(mTexture, tmp, NULL, 0,0)); // blit copy to surface

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      tmp->Release(tmp);

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      return PX_OK;
   }

};// CLASS - pxFBOTexture

//====================================================================================================================================================================================

class pxTextureOffscreen : public pxTexture
{
public:
   pxTextureOffscreen() : mOffscreen(), mInitialized(false),
                          mTextureUploaded(false)
   {
      mTextureType = PX_TEXTURE_OFFSCREEN;
   }

   pxTextureOffscreen(pxOffscreen& o) : mOffscreen(), mInitialized(false),
                                        mTextureUploaded(false)
   {
      mTextureType = PX_TEXTURE_OFFSCREEN;

      createTexture(o);
   }

  ~pxTextureOffscreen() { deleteTexture(); };

   void createTexture(pxOffscreen& o)
   {
      mOffscreen.init(o.width(), o.height());

      // Flip the image data here so we match GL FBO layout
//      mOffscreen.setUpsideDown(true);
      o.blit(mOffscreen);

      // premultiply
      for (int y = 0; y < mOffscreen.height(); y++)
      {
         pxPixel* d = mOffscreen.scanline(y);
         pxPixel* de = d + mOffscreen.width();
         while (d < de)
         {
            d->r = (d->r * d->a)/255;
            d->g = (d->g * d->a)/255;
            d->b = (d->b * d->a)/255;
            d++;
         }
      }

      createSurface(o);

//JUNK
//     if(mTexture)
//     {
//        boundTexture = mTexture;

//        mTexture->Dump(texture,
//                      "/home/hfitzpatrick/projects/xre2/image_dumps",
//                      "image_");
//     }
//JUNK

      mInitialized = true;
   }

   virtual pxError deleteTexture()
   {
      rtLogInfo("pxTextureOffscreen::deleteTexture()");

      if(mTexture)
      {
         // if(currentFramebuffer == this)
         // {
         //   // Is this sufficient ? or Dangerous ?
         //   currentFramebuffer = NULL;
            boundTexture = NULL;
         // }

         mTexture->Release(mTexture);
         mTexture = NULL;
      }

      mInitialized = false;
      return PX_OK;
   }

   virtual pxError bindGLTexture(int tLoc)
   {
      (void) tLoc;

      if (!mInitialized)
      {
         return PX_NOTINITIALIZED;
      }

      // TODO would be nice to do the upload in createTexture but right now it's getting called on wrong thread
//      if (!mTextureUploaded)
//      {
//         createSurface(mOffscreen); // JUNK
//         mTextureUploaded = true;
//      }

      if (!mTexture)
      {
         rtLogDebug("############# this: %p >>  %s  ENTER\n", this,__PRETTY_FUNCTION__);
         return PX_NOTINITIALIZED;
      }

      boundTexture = mTexture;

      return PX_OK;
   }

  virtual pxError bindGLTextureAsMask(int mLoc)
   {
      (void) mLoc;

      if (!mInitialized)
      {
         return PX_NOTINITIALIZED;
      }

      if (!mTexture)
      {
         rtLogDebug("############# this: %p >>  %s  ENTER\n", this,__PRETTY_FUNCTION__);
         return PX_NOTINITIALIZED;
      }

      boundMask = mTexture;

      return PX_OK;
   }

  virtual pxError getOffscreen(pxOffscreen& o)
  {
    if (!mInitialized)
    {
      return PX_NOTINITIALIZED;
    }

    o.init(mOffscreen.width(), mOffscreen.height());
    mOffscreen.blit(o);

    return PX_OK;
  }

  virtual int width()  { return mOffscreen.width();  }
  virtual int height() { return mOffscreen.height(); }

private:
  pxOffscreen mOffscreen;
  bool        mInitialized;

  bool        mTextureUploaded;

  IDirectFBSurface       *mTexture;
  DFBSurfaceDescription   dsc;

private:

   pxError createSurface(pxOffscreen& o)
   {
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
      dsc.caps                  = DSCAPS_VIDEOONLY; //   DSCAPS_PREMULTIPLIED
      dsc.flags                 = DFBSurfaceDescriptionFlags(DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT | DSDESC_CAPS);
      dsc.pixelformat           = dfbPixelformat;

      DFB_CHECK(dfb->CreateSurface( dfb, &dsc, &mTexture ) );

      DFB_CHECK(mTexture->Blit(mTexture, tmp, NULL, 0,0)); // blit copy to surface

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      tmp->Release(tmp);

      // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

      return PX_OK;
   }

}; // CLASS - pxTextureOffscreen

//====================================================================================================================================================================================

class pxTextureAlpha : public pxTexture
{
public:
   pxTextureAlpha() :  mDrawWidth(0.0),  mDrawHeight(0.0),
                      mImageWidth(0.0), mImageHeight(0.0),
                      mTexture(NULL), mBuffer(NULL), mInitialized(false)
   {
      mTextureType = PX_TEXTURE_ALPHA;
   }

   pxTextureAlpha(float w, float h, float iw, float ih, void* buffer)
      : mDrawWidth(w),    mDrawHeight(h),
        mImageWidth(iw), mImageHeight(ih),
        mTexture(NULL), mBuffer(NULL), mInitialized(false)
   {
      mTextureType = PX_TEXTURE_ALPHA;

      // copy the pixels
      int bitmapSize = ih * iw;
      mBuffer = malloc(bitmapSize);

#if 1
      memcpy(mBuffer, buffer, bitmapSize);
#else
      // TODO consider iw,ih as ints rather than floats...
      int32_t bw = (int32_t)iw;
      int32_t bh = (int32_t)ih;

      // Flip here so that we match FBO layout...
      for (int32_t i = 0; i < bh; i++)
      {
        uint8_t *s = (uint8_t*)buffer+(bw*i);
        uint8_t *d = (uint8_t*)mBuffer+(bw*(bh-i-1));
        uint8_t *de = d+bw;
        while(d<de)
          *d++ = *s++;
      }
#endif

      // TODO Moved this to bindTexture because of more pain from JS thread calls
      createTexture(w, h, iw, ih);

      //JUNK
//       if(mTexture)
//       {
//          boundTexture = mTexture;

//          mTexture->Dump(mTexture,
//                        "/home/hfitzpatrick/projects/xre2/image_dumps",
//                        "image_alpha_");
//       }
      //JUNK
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

      if (mTexture != NULL)
      {
         deleteTexture();
      }

      if(iw == 0 || ih == 0)
      {
         return;
      }

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

      DFB_CHECK (dfb->CreateSurface( dfb, &dsc, &mTexture));

      mInitialized = true;
   }

   virtual pxError deleteTexture()
   {
      rtLogDebug("############# this: (virtual) %p >>  %s  ENTER\n",mTexture, __PRETTY_FUNCTION__);

      if (mTexture != NULL)
      {
         mTexture->Release(mTexture);
         mTexture = NULL;
      }

      mInitialized = false;
      return PX_OK;
   }

   virtual pxError bindGLTexture(int tLoc)
   {
      (void) tLoc;

      // TODO Moved to here because of js threading issues
      if (!mInitialized) createTexture(mDrawWidth,mDrawHeight,mImageWidth,mImageHeight);
      if (!mInitialized)
      {
         //rtLogDebug("############# this: %p >>  %s  PX_NOTINITIALIZED\n", this, __PRETTY_FUNCTION__);

         return PX_NOTINITIALIZED;
      }

      boundTexture = mTexture;

      return PX_OK;
   }

   virtual pxError bindGLTextureAsMask(int mLoc)
   {
      (void) mLoc;

      if (!mInitialized)
      {
         rtLogDebug("############# this: %p >>  %s  PX_NOTINITIALIZED\n", this, __PRETTY_FUNCTION__);

         return PX_NOTINITIALIZED;
      }

      boundMask = mTexture;

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

   virtual int width()  { return mDrawWidth;  }
   virtual int height() { return mDrawHeight; }

   //DFBVertex*              getVetricies()   { return &v[0];   };
   //DFBVertex v[4];  //quad

   IDirectFBSurface*       getSurface()     { return mTexture; };
   DFBSurfaceDescription   getDescription() { return dsc;     };

private:
   float mDrawWidth;
   float mDrawHeight;

   float mImageWidth;
   float mImageHeight;

   IDirectFBSurface       *mTexture;
   DFBSurfaceDescription   dsc;

   void                   *mBuffer;
   bool                    mInitialized;

}; // CLASS - pxTextureAlpha

//====================================================================================================================================================================================


static void drawRect2(float x, float y, float w, float h, const float* c)
{
   const float verts[4][2] =
   {
      {  x,   y },
      {  x+w, y },
      {  x,   y+h },
      {  x+w, y+h }
   };

   (void) verts; // WARNING

   float colorPM[4];
   premultiply(colorPM,c);

   DFB_CHECK( boundTexture->SetColor( boundTexture, colorPM[0] * 255.0, // RGBA
                                                    colorPM[1] * 255.0,
                                                    colorPM[2] * 255.0,
                                                    colorPM[3] * 255.0));

   DFB_CHECK( boundTexture->FillRectangle( boundTexture, x, y, w, h));
}

static void drawRectOutline(float x, float y, float w, float h, float lw, const float* c)
{
   if(boundTexture == 0)
   {
      rtLogError("cannot drawRectOutline() on context surface because surface is NULL");
      return;
   }

   float colorPM[4];
   premultiply(colorPM, c);

   DFB_CHECK( boundTexture->SetColor( boundTexture, colorPM[0] * 255.0, // RGBA
                                                    colorPM[1] * 255.0,
                                                    colorPM[2] * 255.0,
                                                    colorPM[3] * 255.0));
   float half = lw/2;

   DFB_CHECK( boundTexture->FillRectangle( boundTexture, x-half, y-half,     lw + w, lw    ) ); // TOP
   DFB_CHECK( boundTexture->FillRectangle( boundTexture, x-half, y-half + h, lw + w, lw    ) ); // BOTTOM
   DFB_CHECK( boundTexture->FillRectangle( boundTexture, x-half, y-half,     lw,     lw + h) ); // LEFT
   DFB_CHECK( boundTexture->FillRectangle( boundTexture, w-half, y-half,     lw,     lw + h) ); // RIGHT

   //needsFlip = true;
}

static void drawImageTexture(float x, float y, float w, float h, pxTextureRef texture,
                             pxTextureRef mask, pxStretch xStretch, pxStretch yStretch, float* color)
{
   if (texture.getPtr() == NULL)
   {
      return;
   }

   float iw = texture->width();
   float ih = texture->height();

   if (iw <= 0 || ih <= 0)
   {
      rtLogError("ERROR: drawImageTexture() >>>  WxH: 0x0   BAD !");
      return;
   }

   if (xStretch == PX_NONE)  w = iw;
   if (yStretch == PX_NONE)  h = ih;

   const float verts[4][2] =
   {
     { x,     y },
     { x+w,   y },
     { x,   y+h },
     { x+w, y+h }
   };

   (void) verts; // WARNING

   float tw;
   switch(xStretch)
   {
      case PX_NONE:
      case PX_STRETCH: tw = 1.0;  break;
      case PX_REPEAT:  tw = w/iw; break;
   }

   float th;
   switch(yStretch)
   {
      case PX_NONE:
      case PX_STRETCH: th = 1.0;  break;
      case PX_REPEAT:  th = h/ih; break;
   }

   float firstTextureY  = th;
   float secondTextureY = 0;

   const float uv[4][2] =
   {
     { 0,  firstTextureY  },
     { tw, firstTextureY  },
     { 0,  secondTextureY },
     { tw, secondTextureY }
   };

#warning MOVE "Binding" to texture type drawing ... ?

// Is it HERE ?

   texture->bindGLTexture(0);

   if (mask.getPtr() != NULL) // && mask->getType() == PX_TEXTURE_ALPHA)
   {
      rtLogDebug("INFO: ###################  %s texture: %p WxH: %d x %d MASK MASK MASK\n",
                 __PRETTY_FUNCTION__, texture.getPtr(), texture->width(), texture->height());
      mask->bindGLTextureAsMask(0);
   }

   (void) uv; // WARNING

   float colorPM[4];
   premultiply(colorPM, color);


   if (mask.getPtr() == NULL && texture->getType() != PX_TEXTURE_ALPHA)
   {
      rtLogDebug("INFO: ################### ALPHA");

     // renderTexture = boundMask;

//     gTextureShader->draw(gResW,gResH,gMatrix.data(),gAlpha,4,verts,uv,texture,xStretch,yStretch);
   }
   else if (mask.getPtr() == NULL && texture->getType() == PX_TEXTURE_ALPHA)
   {
//     gATextureShader->draw(gResW,gResH,gMatrix.data(),gAlpha,4,verts,uv,texture,colorPM);
   }
   else if (mask.getPtr() != NULL)
   {
//     gTextureMaskedShader->draw(gResW,gResH,gMatrix.data(),gAlpha,4,verts,uv,texture,mask);
   }
   else
   {
     rtLogError("Unhandled case");
   }

   // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

   if (!currentFramebuffer)
   {
      rtLogFatal("FATAL:  the current render surface is NULL !");
      return;
   }

   DFBRectangle rcSrc;

   rcSrc.x = 0;
   rcSrc.y = 0;
   rcSrc.w = iw;//currentFramebuffer->width();
   rcSrc.h = ih;//currentFramebuffer->height();

   DFBRectangle rcDst;

   rcDst.x = x;
   rcDst.y = y;
   rcDst.w = w;
   rcDst.h = h;

   // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#if 0
   rtLogDebug("\n#############  dfbSurface: %p    boundTexture: %p", dfbSurface, boundTexture);
   rtLogDebug("\n#############  rcSrc: (%d,%d)   WxH: %d x %d ",rcSrc.x, rcSrc.y, rcSrc.w, rcSrc.h);
   rtLogDebug("\n#############  rcDst: (%d,%d)   WxH: %d x %d ",rcDst.x, rcDst.y, rcDst.w, rcDst.h);
#endif

#if 0
   // TEXTURED TRIANGLES...

   if(boundTexture != NULL)
   {
      pxFBOTexture *tex = (pxFBOTexture *) texture.getPtr();

      rtLogDebug("\nINFO: ###################  TextureTriangles()  !!!!!!!!!!!!!!!!");
      dfbSurface->TextureTriangles(dfbSurface, tex->getSurface(), tex->getVetricies(), NULL, 4, DTTF_STRIP);
   }
   else
   {
      rtLogDebug("\nINFO: ###################  TextureTriangles() fails");
   }
#else

   DFB_CHECK(boundFramebuffer->SetBlittingFlags(boundFramebuffer, (DFBSurfaceBlittingFlags) ( DSBLIT_BLEND_ALPHACHANNEL) ));

   if(texture->getType() == PX_TEXTURE_ALPHA)
   {
      boundTexture->Dump(boundTexture,
                     "/home/hfitzpatrick/projects/xre2/image_dumps",
                     "image_glphs_");

      DFB_CHECK(boundFramebuffer->SetBlittingFlags(boundFramebuffer, (DFBSurfaceBlittingFlags) (DSBLIT_COLORIZE | DSBLIT_BLEND_ALPHACHANNEL) ));

      DFB_CHECK(boundFramebuffer->SetColor( boundFramebuffer, colorPM[0] * 255.0, // RGBA
                                                  colorPM[1] * 255.0,
                                                  colorPM[2] * 255.0,
                                                  colorPM[3] * 255.0));
   }

   if(xStretch == PX_REPEAT || yStretch == PX_REPEAT)
   {
      DFB_CHECK(boundFramebuffer->TileBlit(boundFramebuffer, boundTexture,  /*&rcSrc*/ NULL, x , y));
   }
   else
   if(xStretch == PX_STRETCH || yStretch == PX_STRETCH)
   {
      DFB_CHECK(boundFramebuffer->StretchBlit(boundFramebuffer, boundTexture, &rcSrc, &rcDst));
   }
   else
   {
      DFB_CHECK(boundFramebuffer->Blit(boundFramebuffer, boundTexture, NULL, x,y));
   }

   needsFlip = true;

   if(color)
   {
      DFB_CHECK(boundFramebuffer->SetBlittingFlags(boundFramebuffer, (DFBSurfaceBlittingFlags) (DSBLIT_BLEND_ALPHACHANNEL) ));
   }

#endif
}

static void drawImage92(float x, float y, float w, float h,
                        float x1, float y1, float x2,float y2, pxTextureRef texture)
{
   if (texture.getPtr() == NULL)
     return;

   float ox1 = x;
   float ix1 = x+x1;
   float ix2 = x+w-x2;
   float ox2 = x+w;

   float oy1 = y;
   float iy1 = y+y1;
   float iy2 = y+h-y2;
   float oy2 = y+h;

   float w2 = texture->width();
   float h2 = texture->height();

   float ou1 = 0;
   float iu1 = x1/w2;
   float iu2 = (w2-x2)/w2;
   float ou2 = 1;

   float ov2 = 0;
   float iv2 = y1/h2;
   float iv1 = (h2-y2)/h2;
   float ov1 = 1;

#if 1 // sanitize values
   iu1 = pxClamp<float>(iu1, 0, 1);
   iu2 = pxClamp<float>(iu2, 0, 1);
   iv1 = pxClamp<float>(iv1, 0, 1);
   iv2 = pxClamp<float>(iv2, 0, 1);

   float tmin, tmax;

   tmin = pxMin<float>(iu1, iu2);
   tmax = pxMax<float>(iu1, iu2);
   iu1 = tmin;
   iu2 = tmax;

   tmin = pxMin<float>(iv1, iv2);
   tmax = pxMax<float>(iv1, iv2);
   iv1 = tmax;
   iv2 = tmin;

#endif

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

  const float uv[22][2] =
     {
       { ou1,ov1 },
       { iu1,ov1 },
       { ou1,iv1 },
       { iu1,iv1 },
       { ou1,iv2 },
       { iu1,iv2 },
       { ou1,ov2 },
       { iu1,ov2 },
       { iu2,ov2 },
       { iu1,iv2 },
       { iu2,iv2 },
       { iu1,iv1 },
       { iu2,iv1 },
       { iu1,ov1 },
       { iu2,ov1 },
       { ou2,ov1 },
       { iu2,iv1 },
       { ou2,iv1 },
       { iu2,iv2 },
       { ou2,iv2 },
       { iu2,ov2 },
       { ou2,ov2 }
     };

#warning TODO - draw the 9 slice !!

     (void) verts; // warning
     (void) uv;    // warning

//     gTextureShader->draw(gResW,gResH,gMatrix.data(),gAlpha,22,verts,uv,texture,PX_NONE,PX_NONE);
}

bool gContextInit = false;

void pxContext::init()
{
#if 0
  if (gContextInit)
    return;
  else
    gContextInit = true;
#endif

   boundTexture = dfbSurface;  // needed here.

   DFB_CHECK(dfbSurface->SetRenderOptions(dfbSurface, DSRO_MATRIX));

   rtLogSetLevel(RT_LOG_INFO); // LOG LEVEL
}

void pxContext::setSize(int w, int h)
{
   gResW = w;
   gResH = h;

   if (currentFramebuffer == defaultFramebuffer)
   {
      defaultContextSurface.width  = w;
      defaultContextSurface.height = h;
   }
}

void pxContext::clear(int /*w*/, int /*h*/)
{
   if(dfbSurface == 0)
   {
      rtLogError("cannot clear on context surface because surface is NULL");
      return;
   }

   //DFB_CHECK( dfbSurface->Clear( dfbSurface, 0x00, 0x00, 0x00, 0x00 )         ); // TRANSPARENT
   DFB_CHECK( dfbSurface->Clear( dfbSurface, 0x00, 0x00, 0xFF, 0xFF )       );  //  CLEAR_BLUE   << JUNK
}

void pxContext::setMatrix(pxMatrix4f& m)
{
   gMatrix.multiply(m);

   const float *mm = m.data();

   s32 matrix[9];

   // Convert to fixed point...s
   matrix[0] = (s32)(mm[0]  * 0x10000);
   matrix[1] = (s32)(mm[4]  * 0x10000);
   matrix[2] = (s32)(mm[12] * 0x10000);
   matrix[3] = (s32)(mm[1]  * 0x10000);
   matrix[4] = (s32)(mm[5]  * 0x10000);
   matrix[5] = (s32)(mm[13] * 0x10000);

   matrix[6] = 0x00000;
   matrix[7] = 0x00000;
   matrix[8] = 0x10000;

  // DFB_CHECK(dfbSurface->SetRenderOptions(dfbSurface, DSRO_MATRIX)); // JUNK

   DFB_CHECK(dfbSurface->SetMatrix( dfbSurface, matrix ));
}

void pxContext::setAlpha(float a)
{
  gAlpha *= a;
}

float pxContext::getAlpha()
{
   return gAlpha;
}

pxContextFramebufferRef pxContext::createFramebuffer(int width, int height)
{
   pxContextFramebuffer *fbo     = new pxContextFramebuffer();
   pxFBOTexture         *texture = new pxFBOTexture();

   texture->createTexture(width, height);

   fbo->setTexture(texture);

   return fbo;
}

pxError pxContext::updateFramebuffer(pxContextFramebufferRef fbo, int width, int height)
{
   if (fbo.getPtr() == NULL || fbo->getTexture().getPtr() == NULL)
   {
      return PX_FAIL;
   }

   return fbo->getTexture()->resizeTexture(width, height);
}

pxContextFramebufferRef pxContext::getCurrentFramebuffer()
{
   return currentFramebuffer;
}

pxError pxContext::setFramebuffer(pxContextFramebufferRef fbo)
{
   if (fbo.getPtr() == NULL || fbo->getTexture().getPtr() == NULL)
   {
      //glViewport ( 0, 0, defaultContextSurface.width, defaultContextSurface.height);

      gResW = defaultContextSurface.width;
      gResH = defaultContextSurface.height;

      // TODO probably need to save off the original FBO handle rather than assuming zero

boundFramebuffer = dfbSurface;//default
      currentFramebuffer = defaultFramebuffer;

      pxContextState contextState;
      currentFramebuffer->currentState(contextState);

      gAlpha  = contextState.alpha;
      gMatrix = contextState.matrix;

      return PX_OK;
   }

   currentFramebuffer = fbo;

   pxContextState contextState;
   currentFramebuffer->currentState(contextState);

   gAlpha  = contextState.alpha;
   gMatrix = contextState.matrix;

   return fbo->getTexture()->prepareForRendering();
}

#if 0
pxError pxContext::deleteContextSurface(pxTextureRef texture)
{
  if (texture.getPtr() == NULL)
  {
    return PX_FAIL;
  }
  return texture->deleteTexture();
}
#endif

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

   if(boundTexture == NULL)
   {
      rtLogError("cannot drawRect() on context surface because boundTexture is NULL");
      return;
   }

   if(fillColor != NULL && fillColor[3] > 0.0) // with non-transparent color
   {
      // Draw FILL rectangle for smaller FILL areas
      float half = lineWidth/2;

      drawRect2(half, half, w-lineWidth, h-lineWidth, fillColor);
   }

   if(lineWidth > 0 && lineColor != NULL && lineColor[3] > 0.0) // with non-transparent color and non-zero stroke
   {
      drawRectOutline(0, 0, w, h, lineWidth, lineColor);
   }
}

void pxContext::drawImage9(float w, float h, float x1, float y1,
                           float x2, float y2, pxTextureRef texture)
{
   if (texture.getPtr() != NULL)
   {
      drawImage92(0, 0, w, h, x1, y1, x2, y2, texture);
   }
}

void pxContext::drawImage(float x, float y, float w, float h, pxTextureRef t, pxTextureRef mask,
                          pxStretch xStretch, pxStretch yStretch, float* color)
{
#ifndef USE_DRAW_IMAGE
   return;
#endif

   float black[4] = {0,0,0,1};
   drawImageTexture(x, y, w, h, t, mask, xStretch, yStretch, color ? color : black);
}

void pxContext::drawDiagRect(float x, float y, float w, float h, float* color)
{
#ifndef USE_DRAW_DIAG_RECT
   return;
#endif

   if (!mShowOutlines) return;

   if(boundTexture == NULL)
   {
      rtLogError("cannot drawDiagRect() on context surface because boundTexture is NULL");
      return;
   }

   if(w == 0.0 || h == 0.0)
   {
      rtLogError("cannot drawDiagRect() - width/height cannot be Zero.");
      return;
   }

  const float verts[4][2] =
  {
    {  x,   y   },
    {  x+w, y   },
    {  x+w, y+h },
    {  x,   y+h },
   };
   (void) verts; // warning

   float colorPM[4];
   premultiply(colorPM, color);

   DFB_CHECK (boundTexture->SetColor(boundTexture, colorPM[0] * 255.0, // RGBA
                                                   colorPM[1] * 255.0,
                                                   colorPM[2] * 255.0,
                                                   colorPM[3] * 255.0));

   DFB_CHECK (boundTexture->DrawRectangle(boundTexture, x, y, w, h));
}

void pxContext::drawDiagLine(float x1, float y1, float x2, float y2, float* color)
{
#ifndef USE_DRAW_DIAG_LINE
   return;
#endif

   if (!mShowOutlines) return;

   if(boundTexture == NULL)
   {
      rtLogError("cannot drawDiagLine() on context surface because boundTexture is NULL");
      return;
   }

  const float verts[4][2] =
  {
    { x1, y1 },
    { x2, y2 },
   };
   (void) verts; // warning

   float colorPM[4];
   premultiply(colorPM,color);

   DFB_CHECK (boundTexture->SetColor(boundTexture, colorPM[0] * 255.0, // RGBA
                                                   colorPM[1] * 255.0,
                                                   colorPM[2] * 255.0,
                                                   colorPM[3] * 255.0));

   DFB_CHECK (boundTexture->DrawLine(boundTexture, x1,y1, x2,y2));
}

pxTextureRef pxContext::createTexture(pxOffscreen& o)
{
   pxTextureOffscreen* offscreenTexture = new pxTextureOffscreen(o);

   return offscreenTexture;
}

pxTextureRef pxContext::createTexture(float w, float h, float iw, float ih, void* buffer)
{
   pxTextureAlpha* alphaTexture = new pxTextureAlpha(w,h,iw,ih,buffer);

   return alphaTexture;
}

void pxContext::pushState()
{
   pxContextState contextState;

   contextState.matrix = gMatrix;
   contextState.alpha  = gAlpha;

   currentFramebuffer->pushState(contextState);
}

void pxContext::popState()
{
   pxContextState contextState;

   if (currentFramebuffer->popState(contextState) == PX_OK)
   {
      gAlpha  = contextState.alpha;
      gMatrix = contextState.matrix;
   }
}




#if 0

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
#endif //0
