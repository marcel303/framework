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


class ObjectPropertyWindow;
class Ed
{
public:
	static Ed& I()
	{
		static Ed e;
		return e;
	}

private:
	Ed()
	{
		objectPropWindow = 0;
	}

	~Ed(){}

public:

	ObjectPropertyWindow* objectPropWindow;

	QString ObjectPath; //the directory for all object textures
};

QList<QString> GetLinesFromConfigFile(QString filename);
QPixmap* GetObjectPixmap(QString texture);

