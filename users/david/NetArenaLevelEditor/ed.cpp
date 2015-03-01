#include "ed.h"

#include "includeseditor.h"

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
