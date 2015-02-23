#-------------------------------------------------
#
# Project created by QtCreator 2014-12-10T15:02:49
#
#-------------------------------------------------

QT       += core
QT       += gui
QT       += widgets
QT       += opengl

TARGET = NetArenaLevelEditor
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    gameobject.cpp \
    loaders.cpp

HEADERS += \
    main.h \
    gameobject.h \
    includeseditor.h
