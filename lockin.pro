QT += gui
QT += widgets
QT += multimedia

CONFIG += c++11

TARGET = lockin

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/xygraph/xygraph.pri)
include($$PWD/lockin.pri)

SOURCES += main.cc
