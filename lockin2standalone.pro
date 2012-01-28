QT += gui
QT += multimedia

TARGET = lockin2

HEADERS += \
    lockin2.hpp \
    lockin2gui.hpp \
    qaudioutils.hpp \
    qfifo.hpp

SOURCES += \
    lockin2.cpp \
    main.cpp \
    lockin2gui.cpp \
    qaudioutils.cpp \
    qfifo.cpp

FORMS += \
    lockingui.ui
