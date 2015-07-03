{
  "targets": [
    {
      "target_name": "px",
      "sources": [ 

        "jsCallback.cpp",
        "rtWrapperInit.cpp",
        "rtWrapperUtils.cpp",
        "rtFunctionWrapper.cpp",
        "rtObjectWrapper.cpp",

        "../rtError.cpp",
        "../rtObject.cpp",
        "../rtLog.cpp",
        "../rtString.cpp",
        "../rtValue.cpp",
        "../rtFile.cpp",
        "../rtThreadPool.cpp",
        "../rtThreadTask.cpp",
        "../rtLibrary.cpp",
        "../linux/rtMutexNative.cpp",
        "../linux/rtThreadPoolNative.cpp",
        "../utf8.c",

        "../pxContextDFB.cpp",
        "../pxImage.cpp",
        "../pxImage9.cpp",
        "../pxScene2d.cpp",
        "../pxRectangle.cpp",
        "../pxText.cpp",
        "../pxUtil.cpp",
        "../pxInterpolators.cpp",
        "../pxFileDownloader.cpp",
        "../pxTextureCacheObject.cpp",
        "../pxMatrix4T.cpp",
        "../pxTransform.cpp",
       ],

      "include_dirs" : [
        "../",
        "../../external/png",
        "../../external/ft/include",
        "../../external/curl/include",
        "../../external/jpg",
        "/usr/local/include/directfb",
        "../../../../src"
      ],

      "libraries": [
        "-L/usr/local/lib",
        "-L../../../external/ft/objs/.libs/",
        "-L../../../external/png/.libs",
        "-L../../../external/jpg/.libs",
        "-L../../../external/curl/lib/.libs/",
        "../../../../../build/x11/libpxCore.a",
        "-lX11",
        "-lfreetype",
        "-lpng16",
        "-lcurl",
        "-ldl",
        "-ldirectfb"
#        "-lrt",
      ],

  "conditions": [
    ['OS=="mac"', {'libraries': [
#            "-framework GLUT",
#            "-framework OpenGL",
            "../../../external/jpg/.libs/libjpeg.a",
            ]}],
    ['OS!="mac"', {'libraries': [
            "-ldirectfb",
#            "-lglut",
#            "-lGL",
#            "-lGLEW",
            "-ljpeg",
            ]}],
            ],

      "defines": [
        "RT_PLATFORM_LINUX",
        "PX_PLATFORM_GENERIC_DFB"
        "ENABLE_DFB",
        "RUNINMAIN"
      ],

      'cflags!': [
        "-Wno-unused-parameter"
      ],

      "cflags": [
        "-DPX_PLATFORM_GENERIC_DFB",
        "-DENABLE_DFB",
        "-Wno-attributes",
        "-Wall",
        "-Wextra"
      ]
    }
  ]
}
