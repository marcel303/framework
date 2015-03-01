#pragma once

#include <QString>

enum EditorMode
{
	EM_Level,
	EM_Object,
	EM_Template,
};


class EditorScene;
class ObjectPropertyWindow;
class Ed
{
public:
	static Ed& I()
	{
		static Ed e;
		return e;
	}


	//void SwitchToTemplateEditMode();
	//void SwitchToLevelEditMode();

	EditorScene*& GetSceneArt();
	EditorScene*& GetSceneMech();
	EditorScene*& GetSceneCollission();

	EditorMode& GetEditorMode(){return m_editorMode;}

private:
	Ed()
	{
		objectPropWindow = 0;
	}

	~Ed(){}


	EditorScene* m_sceneArt;
	EditorScene* m_sceneArtTemplate;

	EditorScene* m_sceneMech;
	EditorScene* m_sceneMechTemplate;

	EditorScene* m_sceneCollission;
	EditorScene* m_sceneCollissionTemplate;

	EditorMode m_editorMode;

public:

	ObjectPropertyWindow* objectPropWindow;

	QString ObjectPath; //the directory for all object textures
};


