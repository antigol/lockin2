#-------------------------------------------------
#
# Project created by QtCreator 2012-01-27T23:39:01
#
#-------------------------------------------------

QT       += opengl multimedia

TARGET = lockin2
TEMPLATE = lib

DEFINES += LOCKIN2_LIBRARY

SOURCES += lockin2.cpp \
    qfifo.cpp \
    lockin2gui.cpp \
    audioutils.cpp

HEADERS += lockin2.hpp\
        lockin2_global.hpp \
    qfifo.hpp \
    lockin2gui.hpp \
    audioutils.hpp

symbian {
    MMP_RULES += EXPORTUNFROZEN
    TARGET.UID3 = 0xEF46DA07
    TARGET.CAPABILITY = 
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = lockin2.dll
    addFiles.path = !:/sys/bin
    DEPLOYMENT += addFiles
}

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/local/lib
        headers.files = lockin2.hpp lockin2gui.hpp
        headers.path = /usr/local/include/lockin2
    }
    INSTALLS += target headers
}

FORMS += \
    lockingui.ui




