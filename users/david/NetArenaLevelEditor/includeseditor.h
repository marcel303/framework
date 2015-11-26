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


#define VERSION 1

#define BASEX 64
#define BASEY 36

#define BLOCKSIZE 30//32//32
#define THUMBSIZE 60

#define ZOOMSPEED 12

#define PALLETTEROWSIZE 3

#define ed Ed::I()

#define view2 ed.GetView() //hack

#define viewPallette ed.GetViewPallette()


#define MAPX ed.GetMapX()
#define MAPY ed.GetMapY()

#define leftbuttonHeld ed.m_leftbuttonHeld

#define settingsWidget ed.GetSettingsWidget()

QList<QString> GetLinesFromConfigFile(QString filename);

void SaveLevel(QString filename);
void LoadLevel(QString filename);

void LoadPixmaps();

QString GetPath();
