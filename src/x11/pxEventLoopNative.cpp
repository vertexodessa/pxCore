// pxCore CopyRight 2005-2006 John Robinson
// Portable Framebuffer and Windowing Library
// pxEventLoopNative.cpp

#include "../pxEventLoop.h"


//#define USE_DFB_TEST


//================================================================================================================================================================================
//================================================================================================================================================================================

#ifdef USE_DFB_TEST

#include <time.h>

#include <directfb.h>


void testSoftBlit(int w, int h);
void testHardBlit(const char *filename, int w, int h);

void testWorker(int ms);


#define PIXEL_FMT DSPF_ABGR // DSPF_ABGR   // DSPF_ARGB       //DSPF_RGB24;//DSPF_ABGR; //DSPF_ABGR;


#define FLAGS_NULL   ((DFBSurfaceFlipFlags) 0)

static IDirectFB                  *dfb;
static IDirectFBSurface           *screen_surface;
static IDirectFBSurface           *image_surface;

static DFBRectangle rcScreen;
static DFBRectangle rcImage;

DFBResult              err;

static pthread_t work_thread;

static int iterations = 100; // default
static int pass = 0;

//================================================================================================================================================================================

#define DFB_CHECK(x...)                                   \
{                                                         \
  DFBResult err = x;                                      \
                                                          \
  if (err != DFB_OK)                                      \
  {                                                       \
    fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ );\
    DirectFBErrorFatal( #x, err );                        \
  }                                                       \
}

#define DFB_ERROR(err)                                    \
  fprintf(stderr, "%s:%d - %s\n", __FILE__, __LINE__,     \
  DirectFBErrorString(err));

#endif //USE_DFB_TEST

//================================================================================================================================================================================
//================================================================================================================================================================================


#ifdef PX_PLATFORM_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysymdef.h>
#endif //PX_PLATFORM_X11

#include "../pxOffscreen.h"

#if defined(ENABLE_GLUT)
  #include "pxWindowNativeGlut.h"
#elif defined(ENABLE_DFB)
  #include "pxWindowNativeDfb.h"
#else
  #include "pxWindowNative.h"
#endif

#include <string>

void pxEventLoop::run()
{
    // For now we delegate off to the x11 pxWindowNative class
    pxWindowNative::runEventLoop();
}

void pxEventLoop::runOnce()
{
  pxWindowNative::runEventLoopOnce();
}

void pxEventLoop::exit()
{
    // For now we delegate off to the x11 pxWindowNative class
    pxWindowNative::exitEventLoop();
}


void usage()
{
    fprintf(stderr, "\n\n");
    fprintf(stderr, "Usage: dfb_test -[s|h] <file> \n\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "         -n <number>                  Number of iterations\n");
    fprintf(stderr, "         -sw                          Perform S/W blit\n");
    fprintf(stderr, "         -hw                          Perform H/W blit\n");
    fprintf(stderr, "         -wk <time>                   Sleepy worker interval (ms)\n");
    fprintf(stderr, "         -screen <width> <height>     Use screen W x H resolution\n\n\n");

    exit(-1);
}

///////////////////////////////////////////
// Entry Point 


int main(int argc, char* argv[])
{
   (void) argc; (void) argv;

#ifndef USE_DFB_TEST

    pxMain();

#else
    int w = 1280, h = 720; // default
    int work_ms = 33;

    if(argc < 1)
    {
        usage(); exit(-1);
    }

    int argno = 1;

    bool testHW = false;
    bool testSW = false;
    bool testWK = false;

    char *filename = NULL;

    while(argno < argc)
    {
        std::string arg = argv[argno];

        fprintf(stderr, "\n Process >> %d:  %s", argno, arg.c_str());

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if(arg == "-n")
        {
            if(argno + 1 < argc) // have iterations ?
            {
                iterations = atoi(argv[++argno]);
            }
        }
        else
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if(arg == "-screen")
        {
            if(argno + 2 < argc) // have a dimensions ?
            {
                w = atoi(argv[++argno]);
                h = atoi(argv[++argno]);
            }
        }
        else
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if(arg == "-sw")
        {
            testSW = true;
            testHW = false;
        }
        else
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if(arg == "-hw")
        {
            testHW = true;
            testSW = false;

            if(argno + 1 > argc) // have a filename ?
            {
                fprintf(stderr,"\nERROR:  no filename...\n\n");
                usage();
            }

            filename = argv[++argno];
        }
        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
        if(arg == "-wk")
        {
            testWK = true;
            work_ms = atoi(argv[++argno]);
        }
        else

        // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

        argno++;

    }//WHILE

    if(testWK)
    {
        testWorker(work_ms);
    }

    if(testHW)
    {
        testHardBlit(filename, w, h);
    }
    else
    if(testSW)
    {
        testSoftBlit(w, h);
    }
    else
    {
        usage(); exit(-1);
    }
#endif

    return 0;
}


//================================================================================================================================================================================
//================================================================================================================================================================================

#ifdef USE_DFB_TEST

///////////////////////////////////////////

#define MILLISECOND_PER_SECOND         1e3
#define NANOSECONDS_PER_SECOND         1e9
#define NANOSECONDS_PER_MILLISECOND    1e6

double diff_ms(timespec start, timespec end)
{
    timespec temp;
    if ((end.tv_nsec - start.tv_nsec) < 0)
    {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    }
    else
    {
        temp.tv_sec  = end.tv_sec  - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }

    double usec = ((double) temp.tv_sec * MILLISECOND_PER_SECOND) +
                  ((double) temp.tv_nsec / NANOSECONDS_PER_MILLISECOND);

    return usec;
}

//================================================================================================================================================================================

bool LoadPngToSurface(IDirectFB *dfb, const char *filename, IDirectFBSurface **surface)
{
    if(filename == NULL)
    {
        fprintf(stderr, "\nFATAL - Invalid file name...");
        return false;
    }

    DFBSurfaceDescription  dsc;
    IDirectFBImageProvider *provider;

    DFB_CHECK (dfb->CreateImageProvider (dfb, filename,  &provider));
    DFB_CHECK (provider->GetSurfaceDescription(provider, &dsc));

    printf("INFO: Image WxH: %d x %d \n", dsc.width, dsc.height);

    rcImage.w = dsc.width;
    rcImage.h = dsc.height;

    DFB_CHECK (dfb->CreateSurface( dfb, &dsc, &image_surface ) );
    DFB_CHECK (provider->RenderTo(provider, image_surface, NULL));

    provider->Release (provider);

    return true;
}

//================================================================================================================================================================================

static int createDfbDisplay(int w, int h)
{
#ifdef USE_DFB_WINDOW
  // Get the primary layer
  if (dfb->GetDisplayLayer( dfb, DLID_PRIMARY, &layer ))
  {
    dfb->Release( dfb );
    return 0;
  }

  if (dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN ) ) // DFSCL_NORMAL   DFSCL_FULLSCREEN
  {
    dfb->Release( dfb );
    return 0;
  }

#else

  /* Request fullscreen mode. */
  dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );  /* Create an 8 bit palette surface. */

#endif

  DFBSurfaceDescription       dsc_screen;

  // Fill the surface description.
  dsc_screen.flags       = (DFBSurfaceDescriptionFlags) (DSDESC_CAPS    | DSDESC_PIXELFORMAT | DSDESC_WIDTH | DSDESC_HEIGHT);
  dsc_screen.caps        = (DFBSurfaceCapabilities)     (DSCAPS_PRIMARY | DSCAPS_FLIPPING);
  dsc_screen.pixelformat = PIXEL_FMT;

  dsc_screen.width  = w;
  dsc_screen.height = h;

  DFB_CHECK( dfb->CreateSurface( dfb, &dsc_screen, &screen_surface ) );

//  screen_surface->SetRenderOptions(screen_surface, DSRO_ANTIALIAS);

  DFB_CHECK( screen_surface->Clear( screen_surface, 0x00, 0x00, 0x00,  0xff ) );

  return 1;
}

//================================================================================================================================================================================
//================================================================================================================================================================================

void *work_func(void *ptr)
{
    struct timespec res;

    int ms = *((int *) ptr);

    res.tv_sec  = (ms / 1000);
    res.tv_nsec = (ms * 1000000) % 1000000000;

    printf("- Working... %d ms\n", ms);

    while(pass < iterations)
    {
       // printf("- Working... %d ms\n", ms);

        clock_nanosleep(CLOCK_MONOTONIC, 0, &res, NULL);

        for(int i = 0; i < 100000; i++)
        {
            int count = 0;

            count += i;

            float foo = (float) count / 100.0f;

            foo += count;
        }
    }//FOR

    return NULL;
}

//================================================================================================================================================================================

void testWorker(int ms)
{
   static int time = ms;
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    if(time <= 0)
    {
        fprintf(stderr, "ERROR: Worker with invalid time... ignored !");
        return;
    }

    if( (work_thread == NULL) &&
        pthread_create(&work_thread, NULL, &work_func, (void *) &time))
    {
      fprintf(stderr, "Error creating thread\n");
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}

//================================================================================================================================================================================

void testHardBlit(const char *filename, int w, int h)
{
    if(filename == NULL)
    {
        usage(); exit(-1);
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    rcScreen.w = w;
    rcScreen.h = h;
    rcScreen.x = 0;
    rcScreen.y = 0;

//    rcImage.w = dsc.width;
//    rcImage.h = dsc.height;
//    rcImage.x = 0;//(rcScreen.w - rcImage.w) /2;
//    rcImage.y = 0;//(rcScreen.h - rcImage.h) /2;

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    DirectFBInit( NULL, NULL );

    DirectFBCreate( &dfb );

    if(dfb == NULL)
    {
      printf("\nERROR:  DirectFBCreate() failed.  DFB == NULL");
      exit(-1);
    }

    createDfbDisplay(w, h);

    LoadPngToSurface(dfb, filename, &image_surface);

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    timespec time1, time2;

    int dx = 1;

    if(rcImage.w != rcScreen.w || rcImage.h != rcScreen.h)
    {

        fprintf(stderr,"\nINFO:  HW Test @ %d x %d  - with IMG: %s (%d x %d) << STRETCH \n",
                rcScreen.w, rcScreen.h,filename, rcImage.w, rcImage.h);

        clock_gettime(CLOCK_MONOTONIC, &time1); //#####

        for (int i = 0; i < iterations; i++) // STRETCH
        {
            screen_surface->StretchBlit(screen_surface, image_surface, &rcImage, &rcScreen);
            screen_surface->Flip(screen_surface, NULL,  DSFLIP_WAITFORSYNC ); // DSFLIP_WAITFORSYNC   DFBSurfaceFlipFlags(0)

            rcScreen.x += 1;

            if(rcScreen.x > 30) rcScreen.x = 0;
        }//FOR

        clock_gettime(CLOCK_MONOTONIC, &time2); //#####
    }
    else
    {

        fprintf(stderr,"\nINFO:  HW Test @ %d x %d  - with IMG: %s (%d x %d) << BLIT\n",
                rcScreen.w, rcScreen.h,filename, rcImage.w, rcImage.h);

        clock_gettime(CLOCK_MONOTONIC, &time1); //#####

        for (pass = 0; pass < iterations; pass++)
        {
            screen_surface->Blit(screen_surface, image_surface, NULL, dx, 0);
            screen_surface->Flip(screen_surface, NULL,  DSFLIP_WAITFORSYNC ); // DSFLIP_WAITFORSYNC   DFBSurfaceFlipFlags(0)

            dx += 1;

            if(dx > 30) dx = 0;
        }//FOR

        clock_gettime(CLOCK_MONOTONIC, &time2); //#####
    }

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    double elapsed_ms = diff_ms(time1,time2);

    printf("\n Total Duration %f ms  for %d iterations.", elapsed_ms, iterations);
    printf("\n  Avg. Duration %f ms  for %d iterations. ", (elapsed_ms / (float) iterations), iterations);
    printf("\n  Avg. FPS      %f   \n\n", 1.0/( (elapsed_ms / (float) iterations) / 1000.0));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Clean up

    if(image_surface)
    {
      image_surface->Release(image_surface);
    }

    if(dfb)
    {
      dfb->Release(dfb);
    }
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
}

//================================================================================================================================================================================
//================================================================================================================================================================================

void testSoftBlit(int w, int h)
{
    fprintf(stderr,"\nINFO:  SW Test @ %d x %d\n", w, h);

    uint32_t *pOffscreen = (uint32_t *) malloc(w * h * sizeof(uint32_t));
    uint32_t *pOnscreen  = (uint32_t *) malloc(w * h * sizeof(uint32_t));

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    // Fill to RED
    for (int i = 0; i < h; i++) // Scan Lines
    {
        uint32_t *p  = &pOffscreen[i];
        uint32_t *pe = p + w;

        // per Scan Line
        while (p < pe)
        {
            *p++ = 0xFF0000FF; // RGBA
        }
    }//FOR
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    printf("\n#### Start - iterations = %d", iterations);

    timespec time1, time2;

    clock_gettime(CLOCK_MONOTONIC, &time1);

    for(pass = 0; pass < iterations; pass++)
    {
        for (int i = 0; i < h; i++) // Scan Lines
        {
            uint32_t *pSrc   = &pOffscreen[i * w];
            uint32_t *pSrc_e = pSrc + w;

            uint32_t *pDst   = &pOnscreen[i];

            // per Scan Line
            while (pSrc < pSrc_e)
            {
                *pDst++ = *pSrc++; // Copy
            }
        }//FOR
    }//FOR - iterations

    clock_gettime(CLOCK_MONOTONIC, &time2);
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    double elapsed_ms = diff_ms(time1,time2);

    printf("\n Total Duration %f ms  for %d iterations.", elapsed_ms, iterations);
    printf("\n  Avg. Duration %f ms  for %d iterations.", (elapsed_ms / (float) iterations), iterations);

    printf("\n######################### CPU TEST - End\n");

    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

    // Tidy up...
    free(pOffscreen);
    free(pOnscreen);
}

#endif //USE_DFB_TEST
//================================================================================================================================================================================
//================================================================================================================================================================================
