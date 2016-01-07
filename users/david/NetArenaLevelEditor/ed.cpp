#include "ed.h"

#include "includeseditor.h"

#include "EditorView.h"
#include "SettingsWidget.h"
#include "pallettes.h"
#include "grid.h"
#include "templates.h"

#include <QListView>
#include <QRadioButton>

PreviewItem::PreviewItem()
{
}

PreviewItem::~PreviewItem()
{
}

//QRectF PreviewItem::boundingRect() const
//{
//	return QRectF();
//}

void Ed::Initialize()
{
	m_view = new EditorView();
	m_viewPallette = new EditorViewBasic;

	m_level = 0;
	m_currentTarget = 0;

	ArtFolderPath = QDir::currentPath() + "\\EditorData\\";

	m_mapx = BASEX;
	m_mapy = BASEY;

	m_leftbuttonHeld = false;

	m_settingsWidget = new SettingsWidget();

	LoadPallettes();

	m_grid = new Grid();
	m_settingsWidget->Create();


	m_templateScene = new TemplateScene();
	m_templateScene->Initialize();

	m_settingsWidget->m_grid->addWidget(m_templateScene->m_listView, 1, 1);
}

void Ed::LoadPallettes()
{
	m_mecPallette = new BasePallette();
	m_mecPallette->LoadPallette("BlockList.txt");

	m_colPallette = new BasePallette();
	m_colPallette->LoadPallette("CollisionList.txt");
}

void Ed::SetMapXY(int x, int y)
{
	m_mapx = x;
	m_mapy = y;
}


EditorView* Ed::GetView()
{
    return m_view;
}

QGraphicsView* Ed::GetViewPallette()
{
    return m_viewPallette;
}

SettingsWidget* Ed::GetSettingsWidget()
{
    return m_settingsWidget;
}

void Ed::NewLevel()
{
    if(m_level)
    {
        delete m_level;
    }

	m_grid->CreateGrid(m_mapx, m_mapy);

    m_level = new Template();
    m_level->InitAsLevel();
	m_currentTarget = m_level;

	m_grid->SetCurrentTarget(m_level);


	m_preview = new PreviewItem();
	ed.m_grid->addItem(m_preview);

}

void Ed::SaveLevel(const QString& filename)
{
	QDir dir;
	dir.mkpath(filename);

	m_level->SaveLevel(filename + '/');

	SaveConfig(filename + '/');
}

void Ed::SaveConfig(const QString& filename)
{
	QFile file(filename + "cfg.txt");
	file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);

	QTextStream out(&file);


	out << "Version: " << 1 << endl;
	out << "MAPX: " << MAPX << endl;
	out << "MAPY: " << MAPY << endl;

	file.close();
}

void Ed::LoadConfig(const QString& filename)
{
	QList<QString> lines = GetLinesFromConfigFile(filename + "cfg.txt");

	QMap<QString, QString> splits = SplitLines(lines);

	//int version = splits["Version"].toInt();
	//switch on version if ever applicable
	//case version 1:
	{

		SetMapXY(splits["MAPX"].toInt(), splits["MAPY"].toInt());
		NewLevel();




	}

}

void Ed::LoadLevel(const QString &filename)
{
	LoadConfig(filename);

	m_level->LoadLevel(filename);
}

BasePallette* Ed::GetCurrentPallette()
{
	if(m_settingsWidget->m_mech->isChecked())
	{
		return m_mecPallette;
	}
	if(m_settingsWidget->m_coll->isChecked())
	{
		return m_colPallette;
	}
	if(m_settingsWidget->m_temp->isChecked())
	{
		return 0;
	}
	if(m_settingsWidget->m_obj->isChecked())
	{
		return 0;
	}
	return 0;
}

void Ed::SetCurrentPallette()
{
	if(m_settingsWidget->m_mech->isChecked())
	{
		m_viewPallette->setScene(m_mecPallette);
	}
	if(m_settingsWidget->m_coll->isChecked())
	{
		m_viewPallette->setScene(m_colPallette);
	}
	if(m_settingsWidget->m_temp->isChecked())
	{
		m_viewPallette->setScene(m_templateScene->m_currentFolder->m_pallette);
	}
	if(m_settingsWidget->m_obj->isChecked())
	{
        m_templateScene->m_listView->hide();
        m_settingsWidget->m_objectText->show();

		return;
	}

	m_settingsWidget->m_objectText->hide();
	m_templateScene->m_listView->show();
}

void Ed::UpdateTransparancy()
{
	m_grid->m_mec->setOpacity(m_settingsWidget->m_mechSlider->value()/100.0);
	m_grid->m_col->setOpacity(m_settingsWidget->m_collSlider->value()/100.0);
	m_grid->m_front->setOpacity(m_settingsWidget->m_foreSlider->value()/100.0);
	m_grid->m_middle->setOpacity(m_settingsWidget->m_middleSlider->value()/100.0);
	m_grid->m_back->setOpacity(m_settingsWidget->m_backSlider->value()/100.0);
}

#include <QFile>
QList<QString> Ed::GetLinesFromConfigFile(QString filename)
{
	QFile file(filename);
	file.open(QIODevice::ReadOnly | QIODevice::Text);

	QTextStream in(&file);

	qDebug() << filename << " open = " << file.isOpen();

	QList<QString> list;
	while(!in.atEnd())
	{
		QString line = in.readLine();
		list.push_back(line);
	}

	file.close();

	return list;
}

QMap<QString, QString> Ed::SplitLines(const QList<QString>& lines)
{
	QMap<QString, QString> splits;
	foreach(const QString& a, lines)
	{
		QStringList t = a.split(':');
		splits[t.first()] = t.last();
	}

	return splits;
}

void Ed::SetCurrentTemplate(Template* t)
{
	m_currentTarget = t;

	m_grid->SetCurrentTarget(t);
}

void Ed::ReturnToLevel()
{
	m_currentTarget = m_level;

	m_grid->SetCurrentTarget(m_level);
}

void Ed::UpdatePreview(int x, int y)
{
	m_preview->setPos(x, y);
	m_preview->setOpacity(0.4);
}
