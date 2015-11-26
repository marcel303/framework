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
    loaders.cpp \
    ed.cpp \
    editorview.cpp \
    SettingsWidget.cpp \
    grid.cpp \
    layers.cpp \
    pallettes.cpp \
    templates.cpp

HEADERS += \
    gameobject.h \
    includeseditor.h \
    ed.h \
    EditorView.h \
    SettingsWidget.h \
    grid.h \
    layers.h \
    pallettes.h \
    templates.h
