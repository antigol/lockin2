include($$PWD/xygraph/xygraph.pri)

SOURCES += $$PWD/fifo.cc \
    $$PWD/lockin_gui.cc \
    $$PWD/lockin.cc

HEADERS += $$PWD/fifo.hh \
    $$PWD/lockin_gui.hh \
    $$PWD/lockin.hh

FORMS += $$PWD/lockin_gui.ui
