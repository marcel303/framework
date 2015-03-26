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

	bg = 0;
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
EditorScene*& Ed::GetSceneCollision()
{
	if(m_editorMode == EM_Template)
		return m_sceneCollisionTemplate;

	return m_sceneCollision;
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
		return GetSceneCollision();
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
    case EM_Template:
            qDebug() << "Setting EditorMode to: Template";
        break;
    }


}

void Ed::EditTemplates()
{
	SetEditorMode(EM_Template);

	m_view->setScene((QGraphicsScene*)(sceneMech));
    GetCurrentScene()->setBackgroundBrush(Qt::green);
}

void Ed::EditLevels()
{
	SetEditorMode(EM_Level);

	m_view->setScene((QGraphicsScene*)(sceneMech));
}

void CreateNewMapHelper(int x, int y)
{
	sceneMech->setSceneRect(-MAPX*BLOCKSIZE,-MAPY*BLOCKSIZE, MAPX*3*BLOCKSIZE,MAPY*3*BLOCKSIZE); //-MAPX*2*BLOCKSIZE,-MAPY*2*BLOCKSIZE,MAPX*4*BLOCKSIZE,MAPY*4*BLOCKSIZE);

	sceneCounter = SCENEMECH;
	sceneMech->CreateLevel(x, y);
	sceneCounter = SCENEART;
	sceneArt->CreateLevel(x, y);
	sceneCounter = SCENECOLL;
	sceneCollision->CreateLevel(x, y);
	sceneCounter = SCENEMECH;
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
