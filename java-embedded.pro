TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++2b

LIBS += -lfmt
DESTDIR = bin
TARGET = pico-java

SOURCES += \
        boards/gamebuino.cpp \
        boards/pico.cpp \
        boards/picosystem.cpp \
        classfile.cpp \
        main.cpp

HEADERS += \
    boards/gamebuino.h \
    boards/pico.h \
    boards/picosystem.h \
    classfile.h \
    globals.h \
    stb_image.h
