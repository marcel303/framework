#include "ed.h"

#include "includeseditor.h"

#include "EditorView.h"

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



void Ed::EditTemplates()
{
    m_editorMode = EM_Template;

    m_view->setScene((QGraphicsScene*)(GetCurrentScene()));
}

void Ed::EditLevels()
{
    m_editorMode = EM_Level;

    m_view->setScene((QGraphicsScene*)(GetCurrentScene()));
}
