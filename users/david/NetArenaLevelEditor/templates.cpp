#include "templates.h"

#include "ed.h"

#include <QFileDialog>
#include "QInputDialog"

#include "grid.h"




TemplateThumb::TemplateThumb()
{
}

TemplateThumb::~TemplateThumb()
{
}

void TemplateThumb::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
	ed.m_templateScene->GetCurrentFolder()->SetCurrentTemplate(m_template->m_name);

	ed.m_preview->setPixmap(*m_template->m_middle.m_pixmap);

	e->accept();
}



Template::Template() : m_mec(ed.m_mecPallette), m_col(ed.m_colPallette)
{
	m_name = "";
}

Template::~Template()
{
}

void Template::InitAsLevel()
{
	m_mec.CreateLayer(MAPX, MAPY);
	m_col.CreateLayer(MAPX, MAPY);
	m_front.CreateLayer(MAPX, MAPY);
	m_middle.CreateLayer(MAPX, MAPY);
	m_back.CreateLayer(MAPX, MAPY);
}

bool Template::CreateNewTemplate()
{
	m_mec.CreateLayer(MAPX, MAPY);
	m_col.CreateLayer(MAPX, MAPY);


	QString middle = QFileDialog::getOpenFileName(0,
		tr("Select main image"), "", tr("Image Files (*.png)"));

	if(middle == "")
		return false;

	QString front = QFileDialog::getOpenFileName(0,
		tr("Select front image"), "", tr("Image Files (*.png)"));
	QString back = QFileDialog::getOpenFileName(0,
		tr("Select background image"), "", tr("Image Files (*.png)"));

	QPixmap m(middle);
	QPixmap f(front);
	QPixmap b(back);

	!m.isNull() ? m_middle.m_pixmap = new QPixmap(m) : m_middle.m_pixmap = new QPixmap();
	!f.isNull() ? m_front.m_pixmap	= new QPixmap(f) : m_front.m_pixmap	= new QPixmap();
	!b.isNull() ? m_back.m_pixmap	= new QPixmap(b) : m_back.m_pixmap	= new QPixmap();

	QPixmap thumb = b.scaled(THUMBSIZE, THUMBSIZE);
	QPainter painter(&thumb);
	painter.drawPixmap(0, 0, THUMBSIZE, THUMBSIZE, m.scaled(THUMBSIZE, THUMBSIZE));
	painter.drawPixmap(0, 0, THUMBSIZE, THUMBSIZE, f.scaled(THUMBSIZE, THUMBSIZE));

	m_thumb = new QPixmap(thumb);

	bool ok;
	QString text = QInputDialog::getText((QWidget*)ed.GetSettingsWidget(), tr("Template Name Dialog"),
											tr("Template Name:"), QLineEdit::Normal,
											QDir::home().dirName(), &ok);
	if (ok && !text.isEmpty())
	{
		m_name = text;
		return true;
	}

	return false;
}

void Template::GetMaxXY(int& x, int& y)
{
	int x_max, y_max = 0;

	x_max = m_front.m_pixmap->size().width();
	y_max = m_front.m_pixmap->size().height();

	x_max < m_middle.m_pixmap->size().width()	? x_max = m_middle.m_pixmap->size().width()		: x_max = x_max;
	x_max < m_back.m_pixmap->size().width()		? x_max = m_back.m_pixmap->size().width()		: x_max = x_max;
	y_max < m_middle.m_pixmap->size().height()	? y_max = m_middle.m_pixmap->size().height()	: y_max = y_max;
	y_max < m_back.m_pixmap->size().height()	? y_max = m_back.m_pixmap->size().height()		: y_max = y_max;

	x_max = x_max/BLOCKSIZE;
	y_max = y_max/BLOCKSIZE;

	x = x_max;
	y = y_max;
}

#include "grid.h"
void Template::StampTo(int x, int y)
{
	int x_max, y_max;
	GetMaxXY(x_max, y_max);

	for(int i = 0; i < y_max; i++)
		for(int j = 0; j < x_max; j++)
		{
			if(i+y < MAPY && j+x < MAPX)
			{
				ed.m_level->m_mec.m_grid[i+y][j+x] = m_mec.m_grid[i][j];
				ed.m_level->m_col.m_grid[i+y][j+x] = m_col.m_grid[i][j];
			}
		}

	QPainter painter(ed.m_level->m_front.m_pixmap);
	painter.drawPixmap(x*BLOCKSIZE, y*BLOCKSIZE, *m_front.m_pixmap);
	QPainter painter2(ed.m_level->m_middle.m_pixmap);
	painter2.drawPixmap(x*BLOCKSIZE, y*BLOCKSIZE, *m_middle.m_pixmap);
	QPainter painter3(ed.m_level->m_back.m_pixmap);
	painter3.drawPixmap(x*BLOCKSIZE, y*BLOCKSIZE, *m_back.m_pixmap);

	UpdateLayers();

	ed.m_grid->update();
}

void Template::Unstamp(int x, int y)
{
	int x_max, y_max;
	GetMaxXY(x_max, y_max);

	for(int i = 0; i < y_max; i++)
		for(int j = 0; j < x_max; j++)
		{
			if(i+y < MAPY && j+x < MAPX)
			{
				ed.m_level->m_mec.m_grid[i+y][j+x] = ' ';
				ed.m_level->m_col.m_grid[i+y][j+x] = ' ';
			}
		}

	QPixmap empty(x_max*BLOCKSIZE, y_max*BLOCKSIZE);
	empty.fill(QColor(Qt::transparent));

	QPainter painter(ed.m_level->m_front.m_pixmap);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	painter.drawPixmap(x*BLOCKSIZE, y*BLOCKSIZE, empty);

	QPainter painter2(ed.m_level->m_middle.m_pixmap);
	painter2.setCompositionMode(QPainter::CompositionMode_Source);
	painter2.drawPixmap(x*BLOCKSIZE, y*BLOCKSIZE, empty);

	QPainter painter3(ed.m_level->m_back.m_pixmap);
	painter3.setCompositionMode(QPainter::CompositionMode_Source);
	painter3.drawPixmap(x*BLOCKSIZE, y*BLOCKSIZE, empty);

	UpdateLayers();

	ed.m_grid->update();
}

void Template::Load(QString filename)
{
	filename.chop(5);
	m_front.m_pixmap = new QPixmap(filename + "front.png");
	m_back.m_pixmap = new QPixmap(filename + "back.png");
	m_middle.m_pixmap = new QPixmap(filename + "middle.png");

	QPixmap p(filename+"thumb.png");
	m_thumb = new QPixmap(p);

	m_mec.CreateLayer(MAPX, MAPY);
	m_col.CreateLayer(MAPX, MAPY);

	m_mec.LoadLayer(filename + "Mec");
	m_col.LoadLayer(filename + "Col");

	m_name = filename.split('\\').back();
}

void Template::Save(QString filename)
{
	m_front.m_pixmap->save(filename + "Front.png");
	m_back.m_pixmap->save(filename + "Back.png");
	m_middle.m_pixmap->save(filename + "Middle.png");
	m_thumb->save(filename + "Thumb.png");

	m_mec.SaveLayer(filename + "Mec");
	m_col.SaveLayer(filename + "Col");

	QFile file(filename + ".tmpl");
	file.open(QIODevice::WriteOnly);
	file.close();
}


void Template::Save()
{
	if(m_name != "")
		Save(ed.ArtFolderPath + "templates\\" + ed.m_templateScene->GetCurrentFolder()->m_folderName + "\\" + m_name);
	else
	{
		bool ok;
		QString text = QInputDialog::getText((QWidget*)ed.GetSettingsWidget(), tr("Template Name Dialog"),
												tr("Template Name:"), QLineEdit::Normal,
												QDir::home().dirName(), &ok);
		if (ok && !text.isEmpty())
		{
			m_name = text;

			Save(ed.ArtFolderPath + ed.m_templateScene->GetCurrentFolder()->m_folderName + "\\" + m_name);
		}
	}
}

void Template::UpdateLayers()
{
	m_mec.UpdateLayer();
	m_col.UpdateLayer();
}

void Template::LoadLevel(QString filename)
{
	m_front.LoadLayer(filename + "ArtFG");
	m_middle.LoadLayer(filename + "Art");
	m_back.LoadLayer(filename + "ArtBG");

	m_mec.LoadLayer(filename + "Mec");
	m_col.LoadLayer(filename + "Col");
}

void Template::SaveLevel(QString filename)
{
	m_front.SaveLayer(filename + "ArtFG");
	m_middle.SaveLayer(filename + "Art");
	m_back.SaveLayer(filename + "ArtBG");

	m_mec.SaveLayer(filename + "Mec");
	m_col.SaveLayer(filename + "Col");

	QPixmap thumb = m_back.m_pixmap->scaled(THUMBSIZE, THUMBSIZE);
	QPainter painter(&thumb);
	painter.drawPixmap(0, 0, THUMBSIZE, THUMBSIZE, m_middle.m_pixmap->scaled(THUMBSIZE, THUMBSIZE));
	painter.drawPixmap(0, 0, THUMBSIZE, THUMBSIZE, m_front.m_pixmap->scaled(THUMBSIZE, THUMBSIZE));

	thumb.save(filename + "Thu.png");
}



#include <QListView>
#include <QStringListModel>
TemplateScene::TemplateScene()
{
	m_listView = new QListView();
	m_model = new QStringListModel();

	UpdateList();

	connect(m_listView, SIGNAL(clicked(QModelIndex)), this, SLOT(folderListClicked(QModelIndex)));
}

void TemplateScene::Initialize()
{
	QDir dir(ed.ArtFolderPath + "templates");

	dir.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
	QStringList dirs = dir.entryList();
	dirs.count();

	foreach(QString d, dirs)
	{
		dir.cd(d);

		dir.setFilter(QDir::AllEntries);
		QStringList filters;
		filters << "*.tmpl";
		dir.setNameFilters(filters);

		QStringList templates = dir.entryList();

		//if(templates.count())
		{
			AddFolder(d);
			SetCurrentFolder(d);
		}

		foreach(QString filename, templates)
		{
			QFile file(dir.path() + "\\" + dir.dirName() + "\\" + filename);
			file.open(QIODevice::ReadOnly | QIODevice::Text);

			Template* t = new Template();
			t->Load(dir.path() + "\\" + filename);

			m_currentFolder->AddTemplate(t);
		}

		dir.cdUp();//back down
	}
}

TemplateScene::~TemplateScene()
{
}

void TemplateScene::folderListClicked(const QModelIndex &index)
{
	QString name = (m_model->data(index, Qt::DisplayRole)).toString();

	SetCurrentFolder(name);
}

void TemplateScene::AddFolder(QString foldername)
{
	TemplateFolder* f = new TemplateFolder;
	f->SetFolderName(foldername);

	m_folderMap[foldername] = f;

	UpdateList();
}


void TemplateScene::AddTemplate(Template* t)
{
	m_currentFolder->AddTemplate(t);

	UpdateList();
}

void TemplateScene::UpdateList()
{
	m_nameList.clear();

	foreach(QString name, m_folderMap.keys())
	{
		m_nameList << name;
	}

	m_model->setStringList(m_nameList);

	m_listView->setModel(m_model);
}

void TemplateScene::SetCurrentFolder(QString name)
{
	if(m_folderMap.contains(name))
	{
		m_currentFolder = m_folderMap[name];
		m_currentFolder->SetCurrentTemplate(); //select top template of this folder.

        //if template..
		viewPallette->setScene(m_currentFolder->m_pallette);
	}
}

void TemplateScene::SetCurrentTemplate(QString name)
{
	m_currentFolder->SetCurrentTemplate(name);
}

Template* TemplateScene::GetCurrentTemplate()
{
	return m_currentFolder->GetCurrentTemplate();
}







TemplateFolder* TemplateScene::GetCurrentFolder()
{
	return m_currentFolder;
}

TemplateFolder::TemplateFolder()
{
	m_pallette = new QGraphicsScene();

	m_currentTemplate = 0;
}

TemplateFolder::~TemplateFolder()
{
}

void TemplateFolder::SetFolderName(const QString &name)
{
	m_folderName = name;
}

void TemplateFolder::LoadTileIntoScene()
{
	m_pallette->clear();

	int x = 0;
	int y = 1;
	foreach(Template* t, m_templateMap.values())
	{
		if(x > PALLETTEROWSIZE)
		{
			x = 0;
			y++;
		}

		TemplateThumb* tt = new TemplateThumb;
		tt->setPixmap(*t->m_thumb);
		tt->m_template = t;
		m_pallette->addItem(tt);
		tt->setPos(x*THUMBSIZE+2, y*THUMBSIZE+2);

		x++;
	}
}

#include "SettingsWidget.h"
#include "grid.h"
void TemplateFolder::SetCurrentTemplate()
{
	if(m_templateMap.count())
	{

		m_currentTemplate = m_templateMap.first();

		if(settingsWidget->m_editT)
			ed.m_grid->SetCurrentTarget(m_currentTemplate);
	}
	else
	{
		m_currentTemplate = 0;
	}


}

void TemplateFolder::SetCurrentTemplate(const QString& name)
{
	if(m_templateMap.contains(name))
	{
		m_currentTemplate = m_templateMap[name];

		if(settingsWidget->m_editT)
			ed.m_grid->SetCurrentTarget(m_currentTemplate);
	}
	else
		SetCurrentTemplate();
}


void TemplateFolder::AddTemplate(Template* t)
{
	m_templateMap[t->m_name] = t;

	LoadTileIntoScene();
}

void TemplateFolder::SelectNextTemplate()
{
	if(m_currentTemplate)
	{
		if(m_templateMap.find(m_currentTemplate->m_name) != m_templateMap.end())
			SetCurrentTemplate((m_templateMap.find(m_currentTemplate->m_name)+1).key());
	}
}

void TemplateFolder::SelectPreviousTemplate()
{
	if(m_currentTemplate)
	{
		if((m_templateMap.find(m_currentTemplate->m_name)) != m_templateMap.begin())
			SetCurrentTemplate((m_templateMap.find(m_currentTemplate->m_name)-1).key());
	}
}

Template *TemplateFolder::GetCurrentTemplate()
{
	return m_currentTemplate;
}


QString GetPath()
{

	return ed.ArtFolderPath + "templates\\" + ed.m_templateScene->GetCurrentFolder()->m_folderName + "\\";
}


Template* Template::GetMirror()
{
	Template* t = new Template();

	//QPixmap* pixmap_reflect = new QPixmap(this->pixmap().transformed(QTransform().scale(-1, 1)));

	//?? profit!


	return t;
}
