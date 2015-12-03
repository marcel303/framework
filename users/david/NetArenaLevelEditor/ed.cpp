#include "ed.h"

#include "includeseditor.h"

#include "EditorView.h"
#include "SettingsWidget.h"
#include "pallettes.h"
#include "grid.h"
#include "templates.h"


#include "QRadioButton"

void Ed::Initialize()
{
	m_mapx = BASEX;
	m_mapy = BASEY;

	m_leftbuttonHeld = false;

	m_settingsWidget = new SettingsWidget();

	m_grid = new Grid();
	m_settingsWidget->Create();

	m_templateFolders = new TemplateScene();
	m_templateFolders->Initialize();
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





void Ed::CreateNewMap()
{

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
	}
	if(m_settingsWidget->m_coll->isChecked())
	{
		m_viewPallette->setScene(m_colPallette);
	}
	if(m_settingsWidget->m_temp->isChecked())
	{
		m_viewPallette->setScene(m_templateFolders->GetCurrentFolder()->m_pallette);
	}
	if(m_settingsWidget->m_obj->isChecked())
	{
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
