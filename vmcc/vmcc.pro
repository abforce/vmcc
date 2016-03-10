#-------------------------------------------------
#
# Project created by QtCreator 2015-04-08T11:50:05
#
#   Virtual Machine Controlling Client
#       Written by Ali Reza Barkhordari
#-------------------------------------------------

QT       += core gui network
CONFIG   += static

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = vmcc
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    vmvixhelper.cpp \
    uihelper.cpp \
    monitorpanel.cpp \
    socketpool.cpp \
    addsitedialog.cpp \
    vmvixhelperasync.cpp \
    jsonxx.cc \
    messageforger.cpp \
    graphvisualizer.cpp \
    logcatentry.cpp \
    logcatwidget.cpp \
    timelinevisualizer.cpp

HEADERS  += mainwindow.h \
    vmvixhelper.h \
    uihelper.h \
    monitorpanel.h \
    socketpool.h \
    addsitedialog.h \
    siteinfo.h \
    vmvixhelperasync.h \
    jsonxx.h \
    messageforger.h \
    graphvisualizer.h \
    logcatentry.h \
    logcatwidget.h \
    timelinevisualizer.h \
    qgraphicsarrowitem.h

FORMS    += mainwindow.ui \
    addsitedialog.ui \
    logcatwidget.ui \
    monitorpanel.ui

INCLUDEPATH += /usr/include/vmware-vix

unix:!macx: LIBS += -L$$PWD/../ -lvix

INCLUDEPATH += $$PWD/../
DEPENDPATH += $$PWD/../

unix:!macx: LIBS += -L$$PWD/../ -lgvmomi

INCLUDEPATH += $$PWD/../
DEPENDPATH += $$PWD/../
