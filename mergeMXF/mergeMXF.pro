#-------------------------------------------------
#
# Project created by QtCreator 2016-12-28T19:45:40
#
#-------------------------------------------------

QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = mergeMXF
TEMPLATE = app


SOURCES += main.cpp\
        wndmain.cpp \
    mxfmeta.cpp

HEADERS  += wndmain.h \
    mxfmeta.h

FORMS    += wndmain.ui
