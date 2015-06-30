// pxCore CopyRight 2007-2015 John Robinson
// Portable Framebuffer and Windowing Library
// pxViewWindow.cpp

#include "pxViewWindow.h"

//#include "rtLog.h"

pxError pxViewWindow::view(pxViewRef& v)
{
    v = mView;
    return PX_OK;
}

pxError pxViewWindow::setView(pxIView* v)
{
    if (mView)
    {
        mView->removeListener(this);
        mView = NULL;
    }

    mView = v;

    if (v)
        v->addListener(this); 


    return PX_OK;
}

void pxViewWindow::onClose()
{
    setView(NULL);
}


void pxViewWindow::invalidateRect(pxRect* r)
{
    if (mView)
    {
#if 0
        pxRect b = mViewOffscreen.bounds();

        if (r)
        {
            b.intersect(*r);
            mView->onDraw(mViewOffscreen, &b);
        }
        else
#endif
        //mView->onDraw(mViewOffscreen, r);
		mView->onDraw(/*s*/);

        pxSurfaceNative s;
        beginNativeDrawing(s);
#if 1
        if (r)
        {
        //rtLog("invalidateRect %d %d \n", r->left(), r->top());
#if 1
            mViewOffscreen.blit(s, r->left(), r->top(),
                r->right()-r->left(), r->bottom()-r->top(),
                r->left(), r->top());
#else
            mViewOffscreen.blit(s);
#endif
        }
        else
#endif
        mViewOffscreen.blit(s);
        endNativeDrawing(s);
    }
}

void pxViewWindow::onSize(int32_t w, int32_t h)
{
    mViewOffscreen.init(w, h);
#if 0
    if (mView)
    {
        mView->onSize(w, h);
    }
#else
    invalidateRect(NULL);
#endif
}

void pxViewWindow::onMouseDown(int32_t x, int32_t y, uint32_t flags)
{
    if (mView)
    {
        mView->onMouseDown(x, y, flags);
    }
}

void pxViewWindow::onMouseUp(int32_t x, int32_t y, uint32_t flags)
{
    if (mView)
    {
        mView->onMouseUp(x, y, flags);
    }
}

void pxViewWindow::onMouseLeave()
{
    if (mView)
    {
        mView->onMouseLeave();
    }
}

void pxViewWindow::onMouseMove(int32_t x, int32_t y)
{
    if (mView)
    {
        mView->onMouseMove(x, y);
    }
}

void pxViewWindow::onKeyDown(uint32_t keycode, uint32_t flags)
{
    if (mView)
    {
        mView->onKeyDown(keycode, flags);
    }
}

void pxViewWindow::onKeyUp(uint32_t keycode, uint32_t flags)
{
    if (mView)
    {
        mView->onKeyUp(keycode, flags);
    }
}

void pxViewWindow::onChar(uint32_t codepoint)
{
  if (mView)
    mView->onChar(codepoint);
}

void pxViewWindow::onDraw(pxSurfaceNative s)
{
    if (mView)
    {
      //  mViewOffscreen.blit(s);
        mView->onDraw(/*s*/);
    }
}

