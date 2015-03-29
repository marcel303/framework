#pragma once


#include <QApplication>

#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QWidget>
#include <QMainWindow>
#include <QGraphicsSceneMouseEvent>
#include <QDebug>
#include <QMap>
#include <QSlider>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QCheckBox>
#include <QLabel>
#include <QButtonGroup>
#include <QMenuBar>
#include <QDir>
#include <QFileDialog>
#include <QKeyEvent>
#include <QtMath>
#include <QColorDialog>
#include <QTextEdit>
#include <QPushButton>

#include "ed.h"


#define BASEX 64//60//52 96 64
#define BASEY 36//32//32 54 36

#define BLOCKSIZE 30//32//32
#define THUMBSIZE 60

#define ZOOMSPEED 12

#define SCENEMECH 0
#define SCENEART 1
#define SCENECOLL 2
#define SCENEOBJ 3
#define SCENETEMPLATE 4


#define PALLETTEROWSIZE 3


#define ed Ed::I()

#define sceneArt ed.GetSceneArt()
#define sceneMech ed.GetSceneMech()
#define sceneCollision ed.GetSceneCollision()

#define templateScene ed.GetTemplateScene()

#define editorMode ed.GetEditorMode()

#define view2 ed.GetView() //hack
#define viewPallette ed.GetViewPallette()
#define sceneCounter ed.GetSceneCounter()

#define gameObjects ed.GetGameObjects()

#define MAPX ed.GetMapX()
#define MAPY ed.GetMapY()

#define leftbuttonHeld ed.m_leftbuttonHeld


#define settingsWidget ed.GetSettingsWidget()


QList<QString> GetLinesFromConfigFile(QString filename);
QPixmap* GetObjectPixmap(QString texture);

void SaveLevel(QString filename);
void LoadLevel(QString filename);
void SwitchSceneTo(int s);
void StampTemplate(int tilex, int tiley, EditorTemplate* ct, bool record = true);
void UnstampTemplate(int tilex, int tiley, EditorTemplate* ct, bool record = true);

void LoadPixmaps();

QString GetPath();
