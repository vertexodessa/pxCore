#ifndef RT_CORE_H
#define RT_CORE_H

#include "rtConfig.h"

//todo - add support for DFB
#include "pxContextDescGL.h"
//#include "pxContextDescDFB.h"

#if defined(RT_PLATFORM_LINUX)
#include "linux/rtConfigNative.h"
#elif defined(RT_PLATFORM_WINDOWS)
#include <Windows.h>
#include "win/rtConfigNative.h"
#else
#error "PX_PLATFORM NOT HANDLED"
#endif

#endif
