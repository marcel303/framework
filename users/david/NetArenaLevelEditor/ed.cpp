#include "ed.h"

#include "includeseditor.h"

#include "EditorView.h"
#include "SettingsWidget.h"
#include "pallettes.h"
#include "grid.h"
#include "templates.h"

#include <QListView>
#include <QRadioButton>

void Ed::Initialize()
{
	m_level = 0;
	m_currentTarget = 0;

	ArtFolderPath = "EditorData//";

	m_mapx = BASEX;
	m_mapy = BASEY;

	m_leftbuttonHeld = false;

	m_settingsWidget = new SettingsWidget();

    m_templateScene = new TemplateScene();
    m_templateScene->Initialize();

	m_grid = new Grid();
	m_settingsWidget->Create();

    m_settingsWidget->m_grid->addWidget(m_templateScene->m_listView, 1, 1);
	//m_templateScene->m_listView->hide();


}

void Ed::LoadPallettes()
{
	m_mecPallette = new BasePallette();
	m_mecPallette->LoadPallette("BlockList.txt");

	m_colPallette = new BasePallette();
	m_colPallette->LoadPallette("CollisionList.txt");
}




EditorView*& Ed::GetView()
{
    return m_view;
}

QGraphicsView*& Ed::GetViewPallette()
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

    m_level = new Template();
    m_level->InitAsLevel();
	m_currentTarget = m_level;

	m_grid->SetCurrentTarget(m_level);


}

void Ed::SaveLevel(const QString& filename)
{
	QDir dir;
	dir.mkpath(filename);

	//SaveGeneric(filename + '/' + "Mec.txt", sceneMech);
	//SaveGeneric(filename + '/' + "Col.txt", sceneCollision);
	//SaveArtFile(filename + '/' + "Art", sceneArt);
}

void Ed::LoadLevel(const QString &filename)
{
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

		m_settingsWidget->m_objectText->hide();
		m_templateScene->m_listView->show();
	}
	if(m_settingsWidget->m_coll->isChecked())
	{
		m_viewPallette->setScene(m_colPallette);

		m_settingsWidget->m_objectText->hide();
		m_templateScene->m_listView->show();
	}
	if(m_settingsWidget->m_temp->isChecked())
	{
        m_settingsWidget->m_objectText->hide();
        m_templateScene->m_listView->show();
	}
	if(m_settingsWidget->m_obj->isChecked())
	{
        m_templateScene->m_listView->hide();
        m_settingsWidget->m_objectText->show();
	}
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

