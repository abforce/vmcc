TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    jsonxx.cc \
    PracticalSocket.cpp \
    messageforger.cpp

HEADERS += \
    jsonxx.h \
    PracticalSocket.h \
    messageforger.h


unix:!macx: LIBS += -lpthread
