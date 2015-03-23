#include "EditorView.h"

#include <QInputDialog>

#include"gameobject.h"
#include "SettingsWidget.h"
#include "main.h"



void SwitchScene()
{
	SwitchSceneTo(sceneCounter+1);
}

void TemplatePallette()
{
	ed.objectPropWindow->m_w->hide();
	templateScene->m_listView->show();
}

void ObjectPallette()
{
	templateScene->m_listView->hide();
	ed.objectPropWindow->m_w->show();
}

EditorView::EditorView() : EditorViewBasic()
{
	newMapWindow = 0;

	QMenuBar* bar = new QMenuBar(this);

	QAction* newAct1 = new QAction(tr("&New"), this);
	newAct1->setShortcuts(QKeySequence::New);
	newAct1->setStatusTip(tr("Create a new file"));
	connect(newAct1, SIGNAL(triggered()), this, SLOT(New()));

	QAction* newAct2 = new QAction(tr("&Save"), this);
	newAct2->setShortcuts(QKeySequence::Save);
	newAct2->setStatusTip(tr("Save to file"));
	connect(newAct2, SIGNAL(triggered()), this, SLOT(Save()));

	QAction* newAct3 = new QAction(tr("&Load"), this);
	newAct3->setShortcuts(QKeySequence::Open);
	newAct3->setStatusTip(tr("Load from file"));
	connect(newAct3, SIGNAL(triggered()), this, SLOT(Load()));

	QAction* newAct5 = new QAction(tr("&SaveTemplate"), this);
	newAct5->setStatusTip(tr("Templatus Savus"));
	connect(newAct5, SIGNAL(triggered()), this, SLOT(SaveTemplate()));

	QAction* newAct6 = new QAction(tr("&ImportTemplate"), this);
	newAct6->setStatusTip(tr("Import Template Image"));
	connect(newAct6, SIGNAL(triggered()), this, SLOT(ImportTemplate()));

	QAction* newAct7 = new QAction(tr("&SwitchLevelTemplate"), this);
	newAct7->setStatusTip(tr("SwitchLevelOrTemplate"));
	connect(newAct7, SIGNAL(triggered()), this, SLOT(SwitchLevelTemplateEdit()));


	bar->addAction(newAct1);
	bar->addAction(newAct2);
	bar->addAction(newAct3);
	bar->addAction(newAct5);
	bar->addAction(newAct6);
	bar->addAction(newAct7);


	bar->showNormal();
}

EditorView::~EditorView()
{
}

void EditorView::Save()
{
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save to file"));
	SaveLevel(fileName);
}


void EditorView::SaveTemplate()
{
	if(templateScene->GetCurrentTemplate())
		templateScene->GetCurrentTemplate()->SaveTemplate();
}

void EditorView::Load()
{
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open file"));
	fileName.chop(4);
	LoadLevel(fileName);
}

void EditorView::New()
{
	CreateAndShowNewMapDialog();
}

void EditorView::SwitchToMech()
{
		SwitchSceneTo(SCENEMECH);
		sliderOpacMech->setValue(100);
		sliderOpacArt->setValue(0);
		sliderOpacColl->setValue(0);
		sliderOpacObject->setValue(0);
}

void EditorView::SwitchToArt()
{
		SwitchSceneTo(SCENEART);
		sliderOpacMech->setValue(20);
		sliderOpacArt->setValue(80);
		sliderOpacColl->setValue(0);
		sliderOpacObject->setValue(0);
}

void EditorView::SwitchToCollission()
{
		SwitchSceneTo(SCENECOLL);
		sliderOpacMech->setValue(0);
		sliderOpacArt->setValue(20);
		sliderOpacColl->setValue(80);
		sliderOpacObject->setValue(0);
}

void EditorView::SwitchToObject()
{
		SwitchSceneTo(SCENEOBJ);
		sliderOpacMech->setValue(20);
		sliderOpacArt->setValue(0);
		sliderOpacColl->setValue(0);
		sliderOpacObject->setValue(80);
}


void EditorView::SwitchToTemplates()
{
		SwitchSceneTo(SCENETEMPLATE);
		sliderOpacMech->setValue(20);
		sliderOpacArt->setValue(80);
		sliderOpacColl->setValue(0);
		sliderOpacObject->setValue(0);

		TemplatePallette();

}

void EditorView::ResetSliders()
{
    sliderOpacMech->setValue(50);
    sliderOpacArt->setValue(50);
    sliderOpacColl->setValue(0);
    sliderOpacObject->setValue(0);

    //SetOpactyForLayer(sceneMech, 50);
    //SetOpactyForLayer(sceneArt, 50);
    //SetOpactyForLayer(sceneCollission, 0);
    //SetOpacityObject(100);
}

bool flip = true;
void EditorView::SwitchLevelTemplateEdit()
{
	if(flip)
    {
		ed.EditTemplates();
        ResetSliders();
    }
	else
    {
		ed.EditLevels();
        ResetSliders();
    }

	flip = !flip;
}

void SetOpactyForLayer(EditorScene* s, qreal opac)
{
	for(int y = 0; y < MAPY; y++)
		for (int x = 0; x < MAPX; x++)
			s->m_tiles[y][x].setOpacity(opac);
}

void EditorView::SetOpacityMech(int s)
{
   SetOpactyForLayer(sceneMech, s/100.0);
}

void EditorView::SetOpacityArt(int s)
{
   SetOpactyForLayer(sceneArt, s/100.0);
}
void EditorView::SetOpacityCollission(int s)
{
	SetOpactyForLayer(sceneCollission, s/100.0);
}

void EditorView::SetOpacityObject(int s)
{
	foreach(GameObject* obj, gameObjects)
	{
		obj->setOpacity(qreal(s) / 100.0);
	}
}

void EditorView::ImportTemplate()
{
	ed.EditTemplates(); flip = false;

	QString filename = QFileDialog::getOpenFileName(this, tr("Open Image for Template"));

	EditorTemplate* t = new EditorTemplate();
	if(t->ImportFromImage(filename))
	{
		filename.chop(4);
		QString f = filename.split('/').last();

		delete t;
		t = new EditorTemplate();
		t->LoadTemplate(GetPath() + f + ".tmpl");

		templateScene->AddTemplate(t);
	}
	else
		delete t;
}

void EditorView::AddNewFolder()
{
	QString name = QInputDialog::getText(this, tr("Folder Name to add?"), "Folder Name");

	if(name != "")
		templateScene->AddFolder(name);
}

void EditorView::RemoveFolder()
{
	templateScene->RemoveFolder(templateScene->GetCurrentFolderName());
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
}

void EditorViewBasic::keyReleaseEvent(QKeyEvent *e)
{
	if(e->key() == Qt::Key_Shift)
	{
		leftbuttonHeld = false;
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





void EditorView::CreateAndShowNewMapDialog()
{
	if(!newMapWindow)
		newMapWindow = new NewMapWindow();

	newMapWindow->show();
}


void NewMapWindow::NewMap()
{
	hide();

	ed.CreateNewMap(x->value(), y->value());
}


void NewMapWindow::CancelNewMap()
{
	hide();
}

NewMapWindow::NewMapWindow()
{
	setMinimumWidth(140);

	x = new QSlider();
	y = new QSlider();

	x->setSingleStep(BASEX);
	y->setSingleStep(BASEY);

	x->setSliderPosition(BASEX);
	y->setSliderPosition(BASEY);

	x->setRange(BASEX, BASEX*10);
	y->setRange(BASEY, BASEY*10);

	ok = new QPushButton();
	cancel = new QPushButton();

	ok->setText("Create Map");
	cancel->setText("Cancel");

	connect(ok, SIGNAL(clicked()), this, SLOT(NewMap()));
	connect(cancel, SIGNAL(clicked()), this, SLOT(CancelNewMap()));


	QVBoxLayout* b = new QVBoxLayout;
	b->addWidget(x);
	b->addWidget(y);
	b->addWidget(ok);
	b->addWidget(cancel);

	setLayout(b);
}

NewMapWindow::~NewMapWindow()
{
}



