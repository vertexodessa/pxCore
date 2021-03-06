
UNAME_S := $(shell uname -s)
HNAME_S := $(shell hostname)

ifeq ($(UNAME_S),Darwin)
all: pxscene pxscene.app
deploy: pxscene.dmg
else
all: pxscene
dfb: pxscene-dfb
libs: librtCore.so libpxscene.so
static-libs: librtCore.a libpxwayland.a
endif

HAVE_WESTEROS := $(shell test -f ../external/westeros/Makefile.ubuntu && echo 1)

ifneq ($(HAVE_WESTEROS),1)
HAVE_WESTEROS := $(shell pkg-config --exists westeros-compositor && echo 1)
endif

clean:
	rm -rf $(OBJDIR)
	rm -f pxscene
	rm -rf pxscene.app

ifeq ($(HNAME_S),raspberrypi )
PX_PLATFORM=PX_PLATFORM_GENERIC_EGL
PXLIBS=$(PXCOREDIR)/build/egl
TARGETINCLUDE= -I/opt/vc/include \
  -I/opt/vc/include/interface/vcos/pthreads \
  -I/opt/vc/include/interface/vmcs_host/linux
else
ifeq ($(UNAME_S),Darwin)
PX_PLATFORM=PX_PLATFORM_MAC
PXLIBS=$(PXCOREDIR)/build/mac
TARGETINCLUDE=
else
ifeq ($(PX_PLATFORM),DirectFB)
PX_PLATFORM=PX_PLATFORM_GENERIC_DFB
else
PX_PLATFORM=PX_PLATFORM_GLUT
PXLIBS=$(PXCOREDIR)/build/glut
TARGETINCLUDE=
endif
endif
endif


VPATH=linux
RT_SRCS_FULL=\
    utf8.c\
    ioapi_mem.c\
    unzip.c\
    ioapi.c\
    rtError.cpp \
    rtLog.cpp \
    rtFile.cpp \
    rtPathUtils.cpp \
    rtUrlUtils.cpp \
    rtString.cpp \
    rtValue.cpp \
    rtObject.cpp \
    jsCallback.cpp \
    rtFunctionWrapper.cpp \
    rtObjectWrapper.cpp \
    rtThreadPool.cpp \
    rtThreadTask.cpp \
    rtWrapperUtils.cpp \
    rtThreadQueue.cpp \
    rtLibrary.cpp \
    rtZip.cpp \
    linux/rtMutexNative.cpp \
    linux/rtThreadPoolNative.cpp

ifeq ($(HAVE_WESTEROS),1)
else
RT_SRCS_FULL += ../external/westeros-stub/westeros-stub.cpp
endif

PX_SRCS_FULL=\
    pxScene2d.cpp \
    pxResource.cpp \
    pxConstants.cpp \
    pxRectangle.cpp \
    pxFont.cpp \
    pxText.cpp \
    pxTextBox.cpp \
    pxImage.cpp \
    pxUtil.cpp \
    pxImage9.cpp \
    pxFileDownloader.cpp \
    pxInterpolators.cpp \
    pxMatrix4T.cpp \
    pxArchive.cpp

EXTDIR=../external
PXCOREDIR=../../..
PXHDRS=$(PXCOREDIR)/src
#PXLIBS=$(PXCOREDIR)/build/glut

NODEINC=$(EXTDIR)/libnode
NODEDIR=$(EXTDIR)/libnode

ZLIB=$(EXTDIR)/zlib/


SEARCH=\
  -Ilinux \
  -I$(EXTDIR)/jpg \
  -I$(EXTDIR)/curl/include \
  -I$(EXTDIR)/ft/include \
  -I$(EXTDIR)/png \
  -I$(PXHDRS)\
  -I$(ZLIB) \
  -I./ \
  -I$(NODEINC)/src \
  -I$(NODEINC)/deps/uv/include \
  -I$(NODEINC)/deps/v8 \
  -I$(NODEINC)/deps/debugger-agent/include \
  -I$(NODEINC)/deps/v8/include \
  -I$(NODEINC)/deps/cares/include \
  -I$(EXTDIR)/westeros/external/install/include \
  -I$(EXTDIR)/westeros-stub \
  -Irpc/
   $(TARGETINCLUDE)


#CXXFLAGS=-Wall -Wextra -g $(SEARCH) -DRT_PLATFORM_LINUX -D$(PX_PLATFORM) -DENABLE_RT_NODE
CXXFLAGS2=-fPIC -Wall -Wextra -g $(SEARCH) -DRT_PLATFORM_LINUX -D$(PX_PLATFORM) -DENABLE_RT_NODE -DRUNINMAIN

ifdef PXVERSION
CXXFLAGS=$(CXXFLAGS2) -DPX_SCENE_VERSION=$(PXVERSION)
else
CXXFLAGS=$(CXXFLAGS2)
endif


#CXXFLAGS=  $(SEARCH) -DENABLE_GLUT -DRT_PLATFORM_LINUX -DPX_PLATFORM_GLUT
#CXXFLAGS=-Wall -Wextra -g $(SEARCH) -DENABLE_GLUT -DRT_PLATFORM_LINUX -DPX_PLATFORM_GLUT


LDPNG    = -L$(EXTDIR)/png/.libs -lpng16
ifeq ($(UNAME_S),Darwin)
LDJPG    = $(EXTDIR)/jpg/.libs/libjpeg.a
else
LDJPG    = -L$(EXTDIR)/jpg/.libs -ljpeg
endif
LDFT     = -L$(EXTDIR)/ft/objs/.libs -lfreetype
LDCURL   = -L$(EXTDIR)/curl/lib/.libs -lcurl
LDZLIB   = -L$(EXTDIR)/zlib -lz

LDNODE   = -L$(NODEDIR)/out/Release/obj.target
LDNODEV8 = -L$(NODEDIR)/out/Release

#LDNODE   = -L$(NODEDIR)/out/Debug/obj.target
#LDNODEV8 = -L$(NODEDIR)/out/Debug

LDWESTEROS = -L$(EXTDIR)/westeros/external/install/lib -lwesteros_compositor


LDEXT   = $(LDPNG) $(LDJPG) $(LDFT) $(LDCURL) $(LDNODE) $(LDNODEV8) $(LDZLIB)
ifeq ($(HAVE_WESTEROS),1)
LDEXT += $(LDWESTEROS)
endif
ifeq ($(UNAME_S),Linux)
LDEXT += -lX11
endif

ifeq ($(UNAME_S),Darwin)
LDFLAGS = -L$(PXLIBS) -lnode -lpxCore -pthread $(LDEXT) -ldl -lv8_libplatform -framework OpenGL -framework Cocoa -framework Foundation
else
LDFLAGS = -L$(PXLIBS) -lnode -lpxCore -pthread -lglut -lGLEW -lGL $(LDEXT) -ldl -lrt -lv8_libplatform
endif

CXXFLAGS_FULL = $(CXXFLAGS) -fpermissive
CLAGS_FULL = $(CFLAGS)
LDFLAGS_FULL = $(LDFLAGS)

OBJDIR=obj

# rt core begin
RTCORE_SRCS=\
  ../utf8.c \
  ../rtString.cpp \
  ../rtLog.cpp \
  ../rtValue.cpp \
  ../rtObject.cpp \
  ../rtError.cpp \
  ../ioapi_mem.c

RTCORE_OBJS =$(patsubst ../%.cpp, %.o        , $(notdir $(RTCORE_SRCS)))
RTCORE_OBJS:=$(patsubst    %.cpp, %.o        , $(notdir $(RTCORE_OBJS)))
RTCORE_OBJS:=$(patsubst   %.c, %.o        , $(notdir $(RTCORE_OBJS)))
RTCORE_OBJS:=$(patsubst        %, $(OBJDIR)/%, $(RTCORE_OBJS))
# rt core end

# pxscene so begin
PXSCENE_LIB_SRCS=\
           rtFile.cpp \
           rtPathUtils.cpp \
           rtThreadPool.cpp \
           rtThreadTask.cpp \
           rtThreadQueue.cpp \
           rtLibrary.cpp \
           rtZip.cpp \
	   pxScene2d.cpp \
           pxResource.cpp \
           pxConstants.cpp \
	   pxRectangle.cpp \
           pxText.cpp \
           pxTextBox.cpp \
           pxImage.cpp \
           pxUtil.cpp \
           pxImage9.cpp \
           pxFileDownloader.cpp \
           pxInterpolators.cpp \
	   pxMatrix4T.cpp \
           pxArchive.cpp \
           pxFont.cpp \
           rtNode.cpp \
           rtUrlUtils.cpp \
           linux/rtMutexNative.cpp \
           linux/rtThreadPoolNative.cpp \
           jsbindings/rtWrapperUtils.cpp \
           jsbindings/rtFunctionWrapper.cpp \
           jsbindings/rtObjectWrapper.cpp \
           jsbindings/jsCallback.cpp

ifneq ($(PX_PLATFORM),PX_PLATFORM_GENERIC_DFB)
PXSCENE_LIB_SRCS += pxContextGL.cpp pxWayland.cpp
else
PXSCENE_LIB_SRCS += pxContextDFB.cpp
endif


ifeq ($(HAVE_WESTEROS),1)
else
PXSCENE_LIB_SRCS += ../external/westeros-stub/westeros-stub.cpp
endif

PXSCENE_LIB_OBJS =$(patsubst ../%.cpp, %.o        , $(notdir $(PXSCENE_LIB_SRCS)))
PXSCENE_LIB_OBJS:=$(patsubst    %.cpp, %.o        , $(notdir $(PXSCENE_LIB_OBJS)))
PXSCENE_LIB_OBJS:=$(patsubst   %.c, %.o        , $(notdir $(PXSCENE_LIB_OBJS)))
PXSCENE_LIB_OBJS:=$(patsubst        %, $(OBJDIR)/%, $(PXSCENE_LIB_OBJS))

# pxWayland so begin
PXWAYLAND_LIB_SRCS=\
           rtFile.cpp \
           rtPathUtils.cpp \
           rtThreadPool.cpp \
           rtThreadTask.cpp \
           rtThreadQueue.cpp \
           rtLibrary.cpp \
           rtZip.cpp \
           pxScene2d.cpp \
           pxResource.cpp \
           pxConstants.cpp \
           pxRectangle.cpp \
           pxText.cpp \
           pxTextBox.cpp \
           pxImage.cpp \
           pxUtil.cpp \
           pxImage9.cpp \
           pxFileDownloader.cpp \
           pxInterpolators.cpp \
           pxMatrix4T.cpp \
           pxArchive.cpp \
           pxFont.cpp \
           rtUrlUtils.cpp \
           linux/rtMutexNative.cpp \
           linux/rtThreadPoolNative.cpp \
           pxContextGL.cpp \
           pxWayland.cpp

PXWAYLAND_LIB_OBJS =$(patsubst ../%.cpp, %.o        , $(notdir $(PXWAYLAND_LIB_SRCS)))
PXWAYLAND_LIB_OBJS:=$(patsubst    %.cpp, %.o        , $(notdir $(PXWAYLAND_LIB_OBJS)))
PXWAYLAND_LIB_OBJS:=$(patsubst   %.c, %.o        , $(notdir $(PXWAYLAND_LIB_OBJS)))
PXWAYLAND_LIB_OBJS:=$(patsubst        %, $(OBJDIR)/%, $(PXWAYLAND_LIB_OBJS))

PXSCENE_LIB_SEARCH=\
  -Ilinux \
  -I$(PXHDRS)\
  -I./ \
  -I$(NODEINC)/src \
  -I$(NODEINC)/deps/uv/include \
  -I$(NODEINC)/deps/v8 \
  -I$(NODEINC)/deps/debugger-agent/include \
  -I$(NODEINC)/deps/v8/include \
  -I$(NODEINC)/deps/cares/include \
  -Irpc \
  $(PXCORE_INCLUDES)

# pxScene and pxWayland so end

rtMutexNative.o: linux/rtMutexNative.cpp
	$(CXX) -o $(OBJDIR)/rtMutexNative.o -Wall $(CXXFLAGS_FULL) -c linux/rtMutexNative.cpp

rtThreadPoolNative.o: linux/rtThreadPoolNative.cpp
	$(CXX) -o $(OBJDIR)/rtThreadPoolNative.o -Wall $(CXXFLAGS_FULL) -c linux/rtThreadPoolNative.cpp

$(OBJDIR)/rtWrapperUtils.o: jsbindings/rtWrapperUtils.cpp
	$(CXX) -o $(OBJDIR)/rtWrapperUtils.o -Wall $(CXXFLAGS_FULL) -c jsbindings/rtWrapperUtils.cpp

$(OBJDIR)/rtFunctionWrapper.o: jsbindings/rtFunctionWrapper.cpp
	$(CXX) -o $(OBJDIR)/rtFunctionWrapper.o -Wall $(CXXFLAGS_FULL) -c jsbindings/rtFunctionWrapper.cpp

$(OBJDIR)/rtObjectWrapper.o: jsbindings/rtObjectWrapper.cpp
	$(CXX) -o $(OBJDIR)/rtObjectWrapper.o -Wall $(CXXFLAGS_FULL) -c jsbindings/rtObjectWrapper.cpp

$(OBJDIR)/jsCallback.o: jsbindings/jsCallback.cpp
	$(CXX) -o $(OBJDIR)/jsCallback.o -Wall $(CXXFLAGS_FULL) -c jsbindings/jsCallback.cpp

$(OBJDIR)/westeros-stub.o: $(EXTDIR)/westeros-stub/westeros-stub.cpp
	$(CXX) -o $(OBJDIR)/westeros-stub.o -Wall $(CXXFLAGS_FULL) -c $(EXTDIR)/westeros-stub/westeros-stub.cpp

SRCS_FULL_GL=$(PX_SRCS_FULL) $(RT_SRCS_FULL) pxContextGL.cpp pxWayland.cpp rtNode.cpp pxScene.cpp
SRCS_FULL_DFB=$(PX_SRCS_FULL) $(RT_SRCS_FULL) pxContextDFB.cpp rtNode.cpp pxScene.cpp

OBJS=$(patsubst %.cpp, $(OBJDIR)/%.o, $(notdir $(SRCS_FULL_GL)))
OBJS:=$(patsubst %.c, $(OBJDIR)/%.o, $(OBJS))
OBJS: $(SRCS_FULL_GL)

OBJS_DFB=$(patsubst %.cpp, $(OBJDIR)/%.o, $(notdir $(SRCS_FULL_DFB)))
OBJS_DFB:=$(patsubst %.c, $(OBJDIR)/%.o, $(OBJS_DFB))
OBJS_DFB: $(SRCS_FULL_DFB)

$(OBJDIR)/%.o : %.cpp
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	$(CXX) -c $(CXXFLAGS_FULL) $< -o $@

$(OBJDIR)/%.o : %.c
	@[ -d $(OBJDIR) ] || mkdir -p $(OBJDIR)
	$(CC) -c $(CXXFLAGS_FULL) $< -o $@


# Enable these. There are already a bunch of things
# enabled by default. These are in addition to those.
SCAN_CHECKS=\
  alpha.core.BoolAssignment\
  alpha.core.CastSize\
  alpha.core.FixedAddr\
  alpha.core.SizeofPtr\
  alpha.core.TestAfterDivZero\
  alpha.cplusplus.NewDeleteLeaks\
  alpha.cplusplus.VirtualCall\
  alpha.security.ArrayBoundV2\
  alpha.security.MallocOverflow\
  alpha.security.ReturnPtrRange\
  alpha.unix.MallocWithAnnotations\
  alpha.unix.PthreadLock\
  alpha.unix.SimpleStream\
  alpha.unix.cstring.NotNullTerminated\
  alpha.unix.cstring.OutOfBounds\
  security.insecureAPI.strcpy

LLVM_CHECKS=$(patsubst %, -enable-checker %, $(SCAN_CHECKS))

ifeq ($(PX_PLATFORM),PX_PLATFORM_GENERIC_DFB)
PXSCENE_LIBS=-lpng15 -ldirectfb -ldirect -lfusion
PXSCENE_LIB_LINK_OPTIONS=$(PXSCENE_LIB_LINKING) -L../../../build/dfb -Wl,--whole-archive ../../../build/dfb/libpxCore.a
PXSCENE_LIB_DEFINES=-DPX_PLATFORM_GENERIC_DFB -DENABLE_DFB -DENABLE_DFB_GENERIC
else 
PXSCENE_LIBS=-lpng16 -lnxpl -lGLESv2 -lwesteros_compositor -Lrpc/ -lrtRemote -luuid
PXSCENE_LIB_LINK_OPTIONS=-L../../../build/egl -Wl,--whole-archive ../../../build/egl/libpxCore.a
PXSCENE_LIB_DEFINES=-DPX_PLATFORM_GENERIC_EGL -DENABLE_PX_WAYLAND_RPC -DENABLE_MAX_TEXTURE_SIZE
endif


analyze:
	$(MAKE) clean
	scan-build-3.5 $(LLVM_CHECKS) $(MAKE) pxscene

LINKLIBS := $(PXLIBS)/libpxCore.a
pxscene: $(OBJS) $(LINKLIBS)
	$(CXX) $(OBJS) $(LDFLAGS_FULL) -o pxscene

pxscene-dfb: CXXFLAGS_FULL = -fPIC -Wall -Wextra -g $(SEARCH) -I/usr/local/include/directfb -DRT_PLATFORM_LINUX -DPX_PLATFORM_X11 -DENABLE_RT_NODE -DRUNINMAIN -DENABLE_DFB 
pxscene-dfb: $(OBJS_DFB) $(PXCOREDIR)/build/dfb/libpxCore.a 
	$(CXX) $(OBJS_DFB) -lnode -lpxCore -pthread -L/usr/local/lib -ldirectfb $(LDEXT) -L$(PXCOREDIR)/build/dfb -lpxCore -ldl -lrt -lv8_libplatform -o pxscene

libpxscene.so:
	rm -rf $(OBJDIR)
libpxscene.so: CFLAGS_FULL = $(CFLAGS)
libpxscene.so: CXXFLAGS_FULL =-Wno-attributes -Wall -Wextra -fpermissive -fPIC $(PXSCENE_LIB_SEARCH) -g -DRT_PLATFORM_LINUX -DPX_NO_WINDOW $(PXSCENE_LIB_DEFINES) -DENABLE_RT_NODE -DRUNINMAIN -DBSTD_CPU_ENDIAN=BSTD_ENDIAN_LITTLE
libpxscene.so: LDFLAGS_FULL =$(PXSCENE_LIB_LINK_OPTIONS) -Wl,--no-whole-archive -lnode -pthread -ljpeg -lfreetype -lcurl $(PXSCENE_LIBS) $(LDNODE) $(LDNODEV8) -Lrpc/ -ldl -lrt -L./ -lrtCore -shared -lnexus -lnxclient -o libpxscene.so.1.0  
libpxscene.so: $(PXSCENE_LIB_OBJS)
	$(CXX) $(PXSCENE_LIB_OBJS) $(LDFLAGS_FULL)
	ln -sf libpxscene.so.1.0 libpxscene.so.1
	ln -sf libpxscene.so.1 libpxscene.so

libpxwayland.so: 
	rm -rf $(OBJDIR)
libpxwayland.so: CFLAGS_FULL = $(CFLAGS)
libpxwayland.so: CXXFLAGS_FULL =-Wno-attributes -Wall -Wextra -fpermissive -fPIC $(PXSCENE_LIB_SEARCH) -g -DRT_PLATFORM_LINUX -DPX_NO_WINDOW $(PXSCENE_LIB_DEFINES) -DRUNINMAIN -DBSTD_CPU_ENDIAN=BSTD_ENDIAN_LITTLE
libpxwayland.so: LDFLAGS_FULL =$(PXSCENE_LIB_LINK_OPTIONS) -Wl,--no-whole-archive -pthread -ljpeg -lfreetype -lcurl $(PXSCENE_LIBS) -Lrpc/ -ldl -lrt -L./ -lrtCore -shared -lnexus -lnxclient -o libpxwayland.so.1.0
libpxwayland.so: $(PXWAYLAND_LIB_OBJS)
	$(CXX) $(PXWAYLAND_LIB_OBJS) $(LDFLAGS_FULL)
	ln -sf libpxwayland.so.1.0 libpxwayland.so.1
	ln -sf libpxwayland.so.1 libpxwayland.so

libpxwayland.a: CFLAGS_FULL = $(CFLAGS)
libpxwayland.a: CXXFLAGS_FULL =-Wno-attributes -Wall -Wextra -fpermissive -fPIC $(PXSCENE_LIB_SEARCH) -g -DRT_PLATFORM_LINUX -DPX_NO_WINDOW $(PXSCENE_LIB_DEFINES) -DRUNINMAIN -DBSTD_CPU_ENDIAN=BSTD_ENDIAN_LITTLE
libpxwayland.a: LDFLAGS_FULL =-ljpeg -lfreetype -lcurl -lpng16 -lnxpl -lGLESv2 -lwesteros_compositor -lnexus -lnxclient -lrt
libpxwayland.a: $(PXWAYLAND_LIB_OBJS)
	$(AR) rcs -o $@ $(PXWAYLAND_LIB_OBJS) $(patsubst -l%,$(PKG_CONFIG_SYSROOT_DIR)/usr/lib/lib%.so,$(LDFLAGS_FULL))

librtCore.so: CFLAGS_FULL = $(CFLAGS) -DRAPIDJSON_HAS_STDSTRING -Wall -Wextra -DRT_PLATFORM_LINUX -I../ -I. -fPIC
librtCore.so: CXXFLAGS_FULL = $(CXXFLAGS) -std=c++0x $(CFLAGS_FULL)
librtCore.so: LDFLAGS_FULL = -pthread -ldl -shared -o $@
librtCore.so: $(RTCORE_OBJS)
	$(CXX) $(RTCORE_OBJS) $(LDFLAGS_FULL)

librtCore.a: CFLAGS_FULL = $(CFLAGS) -DRAPIDJSON_HAS_STDSTRING -Wall -Wextra -DRT_PLATFORM_LINUX -I../ -I. -fPIC
librtCore.a: CXXFLAGS_FULL = $(CXXFLAGS) -std=c++0x $(CFLAGS_FULL)
librtCore.a: $(RTCORE_OBJS)
	$(AR) rcs -o $@ $(RTCORE_OBJS)

pxscene.app: pxscene
	rm -rf pxscene.app
	./mkapp.sh

pxscene.dmg:
ifndef PXVERSION
	$(error PXVERSION must be defined for the deploy target)
endif
	make clean
	make PX_SCENE_VERSION2=PXVERSION pxscene.app
	echo $(PXVERSION)
	./mkdeploy.sh $(PXVERSION)
