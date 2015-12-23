#pragma once

#include "includeseditor.h"

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

public slots:
    void Save();
	void SaveAs();
    void Load();
	void New();

    void mousePressEvent(QMouseEvent * e);
	void mouseReleaseEvent(QMouseEvent * e);

private:

	bool flip;
    QString m_filename;

};
