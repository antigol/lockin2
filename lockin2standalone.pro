QT += gui
QT += multimedia

TARGET = lockin2

SOURCES += main.cpp

LIBS += -llockin2 -lxygraph

## lancer la commande 'sudo ldconfig' pour résoudre le problème d'execution

