QT += gui
QT += multimedia

TARGET = lockin2

HEADERS += \
    lockin2.hpp \
    lockin2gui.hpp \
    qaudioutils.hpp

SOURCES += \
    lockin2.cpp \
    main.cpp \
    lockin2gui.cpp \
    qaudioutils.cpp

FORMS += \
    lockingui.ui
