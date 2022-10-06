TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++2b

LIBS += -lfmt
DESTDIR = bin

SOURCES += \
        boards/gamebuino.cpp \
        boards/pico.cpp \
        classfile.cpp \
        main.cpp

HEADERS += \
    boards/gamebuino.h \
    boards/pico.h \
    classfile.h \
    globals.h
