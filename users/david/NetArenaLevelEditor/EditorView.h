#pragma once

#include "includeseditor.h"

class QPushButton;
class NewMapWindow : public QWidget
{
	Q_OBJECT
public:
	NewMapWindow();
	~NewMapWindow();

public slots:
	void NewMap();
	void CancelNewMap();


public:

	QPushButton* ok;
	QPushButton* cancel;

	QSlider* x;
	QSlider* y;

};

class EditorViewBasic : public QGraphicsView
{
public:
    EditorViewBasic ();
    virtual ~EditorViewBasic ();

    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *e);

    void zoomIn(int level = 1);
    void zoomOut(int level = 1);

    int zoomLevel;

    void UpdateMatrix();


#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent * e);
#endif


};

class QSlider;
class EditorView : public EditorViewBasic
{
    Q_OBJECT
public:
    EditorView();
    virtual ~EditorView();


    QSlider* sliderOpacMech;
    QSlider* sliderOpacArt;
    QSlider* sliderOpacColl;
    QSlider* sliderOpacObject;

	QWidget* newMapWindow;

public slots:
    void Save();
    void SaveTemplate();
    void Load();
    void New();
    void SwitchToMech();
    void SwitchToArt();
    void SwitchToCollission();
    void SwitchToObject();

    void SwitchToTemplateMode();
    void SwitchObjectTemplatePallette();

    void SetOpacityMech(int s);
    void SetOpacityArt(int s);
    void SetOpacityCollission(int s);
    void SetOpacityObject(int s);

	void AddNewFolder();
	void RemoveFolder();


    void ImportTemplate();

    void mousePressEvent(QMouseEvent * e);
    void mouseReleaseEvent(QMouseEvent * e);

private:
	void CreateAndShowNewMapDialog();

};
