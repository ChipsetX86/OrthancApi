include($$PWD/Global.pri)

QT       += network xml gui
CONFIG   += c++11 dll
TARGET    = $$qtLibraryTarget(orthancapi)
TEMPLATE  = lib
DEFINES  += COMMON_LIBRARY

HEADERS += \
    Api.h \
    CommonGlobal.h \
    Orthanc.h

SOURCES += \
    Api.cpp \
    Orthanc.cpp


