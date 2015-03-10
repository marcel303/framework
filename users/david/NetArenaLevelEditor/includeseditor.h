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


#define BASEX 30//52
#define BASEY 16//32

#define BLOCKSIZE 64//32
#define THUMBSIZE 64

#define ZOOMSPEED 12

#define SCENEMECH 0
#define SCENEART 1
#define SCENECOLL 2
#define SCENEOBJ 3
#define SCENETEMPLATE 4


#define ed Ed::I()

#define sceneArt ed.GetSceneArt()
#define sceneMech ed.GetSceneMech()
#define sceneCollission ed.GetSceneCollission()

#define templateScene ed.GetTemplateScene()

#define editorMode ed.GetEditorMode()

#define view2 ed.GetView() //hack
#define viewPallette ed.GetViewPallette()
#define sceneCounter ed.GetSceneCounter()

#define gameObjects ed.GetGameObjects()

#define MAPX ed.GetMapX()
#define MAPY ed.GetMapY()

#define leftbuttonHeld ed.m_leftbuttonHeld


QList<QString> GetLinesFromConfigFile(QString filename);
QPixmap* GetObjectPixmap(QString texture);

void SaveLevel(QString filename);
void LoadLevel(QString filename);
void SwitchSceneTo(int s);

void LoadPixmaps();
