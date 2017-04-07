QT += gui
QT += widgets
QT += multimedia

LIBS += -lasound

CONFIG += c++11

TARGET = lockin2

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += src/main.cc \
    xy/xyscene.cc \
    xy/xyview.cc \
    src/qfifo.cc \
    src/lockin2gui.cc \
    src/lockin2.cc \
    src/audioutils.cc

HEADERS += \
    xy/xyscene.hh \
    xy/xyview.hh \
    src/qfifo.hh \
    src/lockin2gui.hh \
    src/lockin2.hh \
    src/audioutils.hh

FORMS += \
    src/lockingui.ui

DISTFILES += \
    alsa/alsa.pro.user
