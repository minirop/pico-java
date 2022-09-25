TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CXXFLAGS += -std=c++2b

LIBS += -lfmt
DESTDIR = bin

SOURCES += \
        boards/pico.cpp \
        lineanalyser.cpp \
        main.cpp

HEADERS += \
    boards/pico.h \
    globals.h \
    lineanalyser.h
