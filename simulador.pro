#-------------------------------------------------
#
# Project created by QtCreator 2013-06-19T20:30:24
#
#-------------------------------------------------

QT       += core gui
QT       += xml
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = simulador
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h \
    event.h \
    link.h \
    node.h \
    nsgenerator.h \
    scheduler.h \
    model/scheduler.h \
    model/packet.h \
    model/node.h \
    model/namgenerator.h \
    model/link.h \
    model/event.h \
    usim.h

FORMS    += mainwindow.ui
