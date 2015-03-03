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


#define BASEX 30//52
#define BASEY 16//32

#define BLOCKSIZE 64//32

#define ZOOMSPEED 12

#define SCENEMECH 0
#define SCENEART 1
#define SCENECOLL 2
#define SCENEOBJ 3


QList<QString> GetLinesFromConfigFile(QString filename);
QPixmap* GetObjectPixmap(QString texture);
