QT += gui
QT += multimedia

TARGET = lockin2

HEADERS += \
    lockin2.hpp \
    lockin2gui.hpp \
    qfifo.hpp \
    audioutils.hpp

SOURCES += \
    lockin2.cpp \
    main.cpp \
    lockin2gui.cpp \
    qfifo.cpp \
    audioutils.cpp

FORMS += \
    lockingui.ui
