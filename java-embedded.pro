TEMPLATE = app
CONFIG += console c++2a
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -lfmt
DESTDIR = bin

SOURCES += \
        boards/pico.cpp \
        lineanalyser.cpp \
        main.cpp

#QMAKE_POST_LINK += javac Pouet.java && mv Pouet.class bin

HEADERS += \
    boards/pico.h \
    globals.h \
    lineanalyser.h
