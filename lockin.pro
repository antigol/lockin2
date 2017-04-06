QT += gui
QT += multimedia
QT += widgets

TARGET = lockin2

SOURCES += src/main.cpp \
    xy/xyscene.cc \
    xy/xyview.cc \
    src/qfifo.cpp \
    src/lockin2gui.cpp \
    src/lockin2.cpp \
    src/audioutils.cpp

HEADERS += \
    xy/xyscene.hh \
    xy/xyview.hh \
    src/qfifo.hpp \
    src/lockin2gui.hpp \
    src/lockin2.hpp \
    src/audioutils.hpp

FORMS += \
    src/lockingui.ui
