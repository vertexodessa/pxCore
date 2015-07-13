TEMPLATE = subdirs

SUBDIRS = ./pxCore.pro ./examples/pxScene2d

pxScene2d.depends = pxCore

message("Support 'pxScene2d' for DFB ...")

unix {
    target.path = /usr/lib
    INSTALLS += target

    INCLUDEPATH += src
    INCLUDEPATH += ../../src
    INCLUDEPATH += ./external/jpg
    INCLUDEPATH += ./external/curl/include
    INCLUDEPATH += ./external/ft/include
    INCLUDEPATH += ./external/png
    INCLUDEPATH += /usr/local/include/directfb

    LIBS += -ldl -fPIC -lm
    LIBS += -L"./external/jpg/.libs" -ljpeg
    LIBS += -L"./external/png/.libs" -lpng16
    LIBS += -L"./external/ft/objs/.libs" -lfreetype
    LIBS += -L"./external/curl/lib/.libs" -lcurl

    LIBS += -L"/usr/local/lib" -ldirectfb -lpthread
  #  LIBS += -L"../../pxCore/build/x11" -lpxCore
}

message(" Here" $$PWD)

unix:!macx: LIBS += -L$$PWD/../../build-pxCore-Desktop-Debug -lpxCore


INCLUDEPATH += $$PWD/../../build-pxCore-Desktop-Debug
DEPENDPATH += $$PWD/../../build-pxCore-Desktop-Debug
