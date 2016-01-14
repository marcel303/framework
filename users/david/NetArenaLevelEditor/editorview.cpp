#include "EditorView.h"

#include <QInputDialog>

#include "SettingsWidget.h"
#include "templates.h"

#include <QPixmap>


EditorView::EditorView() : EditorViewBasic()
{
	flip = true;

	QMenuBar* bar = new QMenuBar(this);

    m_filename = "";

	QAction* newAct1 = new QAction(tr("&New"), this);
	newAct1->setShortcuts(QKeySequence::New);
	newAct1->setStatusTip(tr("Create a new file"));
	connect(newAct1, SIGNAL(triggered()), this, SLOT(New()));

	QAction* newAct2 = new QAction(tr("&Save"), this);
	newAct2->setShortcuts(QKeySequence::Save);
    newAct2->setStatusTip(tr("Save"));
	connect(newAct2, SIGNAL(triggered()), this, SLOT(Save()));

    QAction* newAct3 = new QAction(tr("&SaveAs"), this);
    newAct3->setShortcuts(QKeySequence::SaveAs);
    newAct3->setStatusTip(tr("Save to file"));
    connect(newAct3, SIGNAL(triggered()), this, SLOT(SaveAs()));

    QAction* newAct4 = new QAction(tr("&Load"), this);
    newAct4->setShortcuts(QKeySequence::Open);
    newAct4->setStatusTip(tr("Load from file"));
    connect(newAct4, SIGNAL(triggered()), this, SLOT(Load()));

	bar->addAction(newAct1);
	bar->addAction(newAct2);
	bar->addAction(newAct3);
    bar->addAction(newAct4);


	bar->showNormal();

	UpdateMatrix();
}

EditorView::~EditorView()
{
}

void EditorView::Save()
{
    if(m_filename != "")
		ed.SaveLevel(m_filename);
    else
    {
        m_filename = QFileDialog::getSaveFileName(this, tr("Save to file"));

        if(m_filename != "")
			ed.SaveLevel(m_filename);
    }

}

void EditorView::SaveAs()
{
    QString temp = m_filename;
    m_filename = QFileDialog::getSaveFileName(this, tr("Save to file"));

    if(m_filename != "")
		ed.SaveLevel(m_filename);
    else
        m_filename = temp;
}


void EditorView::Load()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open file"));
    if(fileName != "")
    {
        fileName.chop(7);
        m_filename = fileName;

		ed.LoadLevel(m_filename);
    }
}

void EditorView::New()
{
    m_filename = "";

	bool ok;
	QString text = QInputDialog::getText((QWidget*)ed.GetSettingsWidget(), tr("Map Name"),
											tr("Map Name:"), QLineEdit::Normal,
											QDir::home().dirName(), &ok);
	if (ok && !text.isEmpty())
	{
		m_filename = text;
	}

	int x = QInputDialog::getInt((QWidget*)ed.GetSettingsWidget(), tr("Map Width"),
									 tr("Map Size X(64-128):"), 64, 64, 128, 1, &ok);
	if (!ok)
	{
	}

	int y = QInputDialog::getInt((QWidget*)ed.GetSettingsWidget(), tr("Map Height"),
									 tr("Map Size Y(36-72):"), 36, 36, 72, 1, &ok);
	if (!ok)
	{
	}

	ed.SetMapXY(x, y);
	ed.NewLevel();
}


#ifndef QT_NO_WHEELEVENT
void EditorViewBasic::wheelEvent(QWheelEvent* e)
{
	{
		if (e->delta() > 0)
			zoomIn(ZOOMSPEED);
		else
			zoomOut(ZOOMSPEED);
		e->accept();
	}
}
#endif

void EditorViewBasic::zoomIn(int level)
{
	zoomLevel += level;

	UpdateMatrix();
}

void EditorViewBasic::zoomOut(int level)
{
	zoomLevel -= level;

	if(zoomLevel < 0)
		zoomLevel = 0;

	UpdateMatrix();
}


void EditorViewBasic::UpdateMatrix()
{
	qreal scale = qPow(qreal(2), (zoomLevel - 250) / qreal(50));

	QMatrix matrix;
	matrix.scale(scale, scale);

	setMatrix(matrix);
}


EditorViewBasic::EditorViewBasic() : QGraphicsView()
{
	zoomLevel = 220;
}

EditorViewBasic::~EditorViewBasic()
{

}

void EditorViewBasic::keyPressEvent(QKeyEvent *e)
{
	if(e->key() == Qt::Key_Shift)
	{
		leftbuttonHeld = true;
		e->accept();
	}

	/*
	if(e->key() == Qt::Key_Z)
	{
		if(!ed.undoStack.empty() && editorMode == EM_Level)
		{
			EditorTemplate* et = ed.undoStack.pop();
			if(et)
			{
				StampTemplate(0,0,et, false);

				delete et;
			}
		}

		e->accept();
	}


	if(e->key() == Qt::Key_A)
		if(templateScene->GetCurrentFolder())
			templateScene->GetCurrentFolder()->SelectPreviousTemplate();

	if(e->key() == Qt::Key_D)
		if(templateScene->GetCurrentFolder())
			templateScene->GetCurrentFolder()->SelectNextTemplate();
    */
	if(e->key() == Qt::Key_S && !e->isAutoRepeat())
       if(ed.m_templateScene->GetCurrentFolder())
	   {
            Template* t = ed.m_templateScene->GetCurrentTemplate()->GetMirror();
            ed.m_templateScene->GetCurrentFolder()->SetTempTemplate(t);

            ed.m_preview->setPixmap(*t->m_middle.m_pixmap);

			e->accept();
		}

		e->accept();


}

void EditorViewBasic::keyReleaseEvent(QKeyEvent *e)
{
	if(e->key() == Qt::Key_Shift)
	{
		leftbuttonHeld = false;
		ed.m_level->UpdateLayers();
		e->accept();
	}
}

void EditorView :: mousePressEvent(QMouseEvent * e)
{
   if (e->button() == Qt::MidButton)
   {
	  QMouseEvent fake(e->type(), e->pos(), Qt::LeftButton, Qt::LeftButton, e->modifiers());
	  this->setInteractive(false);
	  this->setDragMode(QGraphicsView::ScrollHandDrag);
	  QGraphicsView::mousePressEvent(&fake);
   }
   else QGraphicsView::mousePressEvent(e);
}

void EditorView :: mouseReleaseEvent(QMouseEvent * e)
{
   if (e->button() == Qt::MidButton)
   {
	  QMouseEvent fake(e->type(), e->pos(), Qt::LeftButton, Qt::LeftButton, e->modifiers());
	  this->setInteractive(true);
	  this->setDragMode(QGraphicsView::NoDrag);
	  QGraphicsView::mouseReleaseEvent(&fake);
   }
   else QGraphicsView::mouseReleaseEvent(e);
}


