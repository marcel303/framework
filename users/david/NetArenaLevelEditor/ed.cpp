#include "ed.h"

#include "includeseditor.h"

#include "EditorView.h"
#include "gameobject.h"
#include "SettingsWidget.h"
#include "main.h"

QList<GameObject*> m_gameObjects; //hack

void Ed::Initialize()
{
	m_mapx = BASEX;
	m_mapy = BASEY;

	objectPropWindow = 0;

	m_leftbuttonHeld = false;

	m_sceneTemplatePallette = new QGraphicsScene();

    m_settingsWidget = new SettingsWidget();
}

EditorScene*& Ed::GetSceneArt()
{
	if(m_editorMode == EM_Template)
		return m_sceneArtTemplate;

	return m_sceneArt;
}
EditorScene*& Ed::GetSceneMech()
{
	if(m_editorMode == EM_Template)
		return m_sceneMechTemplate;

	return m_sceneMech;
}
EditorScene*& Ed::GetSceneCollission()
{
	if(m_editorMode == EM_Template)
		return m_sceneCollissionTemplate;

	return m_sceneCollission;
}

int &Ed::GetSceneCounter()
{
    return m_sceneCounter;
}

EditorScene*& Ed::GetCurrentScene()
{
    switch (m_sceneCounter)
    {
    case SCENEMECH:
        return GetSceneMech();
        break;
    case SCENEART:
        return GetSceneArt();
        break;
    case SCENECOLL:
        return GetSceneCollission();
        break;
    default:
        return GetSceneMech();
        break;
    }
}


EditorView*& Ed::GetView()
{
    return m_view;
}

QGraphicsView*& Ed::GetViewPallette()
{
    return m_viewPallette;
}

QList<GameObject *> &Ed::GetGameObjects()
{
	return m_gameObjects;
}

SettingsWidget* Ed::GetSettingsWidget()
{
    return m_settingsWidget;
}

void Ed::SetEditorMode(EditorMode e)
{
	m_editorMode = e;

    switch(e)
    {
    case EM_Level:
            qDebug() << "Setting EditorMode to: Level";
        break;
    case EM_Object:
            qDebug() << "Setting EditorMode to: Object";
        break;
    case EM_Template:
            qDebug() << "Setting EditorMode to: Template";
        break;
    }


}

void Ed::EditTemplates()
{
	SetEditorMode(EM_Template);

    m_view->setScene((QGraphicsScene*)(GetCurrentScene()));
}

void Ed::EditLevels()
{
	SetEditorMode(EM_Level);

    m_view->setScene((QGraphicsScene*)(GetCurrentScene()));
}


void CreateNewMapHelper(int x, int y)
{
	sceneMech->setSceneRect(-MAPX*BLOCKSIZE,-MAPY*BLOCKSIZE, MAPX*3*BLOCKSIZE,MAPY*3*BLOCKSIZE); //-MAPX*2*BLOCKSIZE,-MAPY*2*BLOCKSIZE,MAPX*4*BLOCKSIZE,MAPY*4*BLOCKSIZE);

	sceneMech->CreateLevel(x, y);
	sceneArt->CreateLevel(x, y);
	sceneCollission->CreateLevel(x, y);
	sceneMech->AddGrid();
}

void Ed::CreateNewMap(int x, int y)
{
	EditLevels();


	m_mapx = x;
	m_mapy = y;

	LoadPixmaps();

	m_gameObjects.clear();

	CreateNewMapHelper(x, y);
	EditTemplates();
	CreateNewMapHelper(x, y);
	EditLevels();

	m_view->scale((float)MAPY/(float)MAPX,(float)MAPY/(float)MAPX);
}
