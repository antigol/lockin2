QT += gui
QT += widgets
QT += multimedia

CONFIG += c++11

TARGET = lockin

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += src/main.cc \
    xygraph/xyscene.cc \
    xygraph/xyview.cc \
    src/fifo.cc \
    src/lockin_gui.cc \
    src/lockin.cc \
    src/audioutils.cc

HEADERS += \
    xygraph/xyscene.hh \
    xygraph/xyview.hh \
    src/fifo.hh \
    src/lockin_gui.hh \
    src/lockin.hh \
    src/audioutils.hh

FORMS += \
    src/lockin_gui.ui
