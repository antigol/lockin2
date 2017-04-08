QT += gui
QT += widgets
QT += multimedia

CONFIG += c++11

TARGET = lockin

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += src/main.cc \
    xy/xyscene.cc \
    xy/xyview.cc \
    src/fifo.cc \
    src/lockin_gui.cc \
    src/lockin.cc \
    src/audioutils.cc

HEADERS += \
    xy/xyscene.hh \
    xy/xyview.hh \
    src/fifo.hh \
    src/lockin_gui.hh \
    src/lockin.hh \
    src/audioutils.hh

FORMS += \
    src/lockin_gui.ui
