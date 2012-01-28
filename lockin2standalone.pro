QT += gui
QT += multimedia

TARGET = lockin2

SOURCES += main.cpp

FORMS += \
    lockingui.ui

LIBS += -llockin2

## lancer la commande 'sudo ldconfig' pour résoudre le problème d'execution

