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
class EditorView;
class QGraphicsView;
class Ed
{
public:
	static Ed& I()
	{
		static Ed e;
		return e;
	}

	EditorScene*& GetSceneArt();
	EditorScene*& GetSceneMech();
	EditorScene*& GetSceneCollission();

    EditorScene*& GetCurrentScene();


    EditorView*& GetView();
    QGraphicsView*& GetViewPallette();

	EditorMode& GetEditorMode(){return m_editorMode;}


    void EditTemplates();
    void EditLevels();


    int& GetSceneCounter();


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

    EditorView* m_view;
    QGraphicsView* m_viewPallette;

    int m_sceneCounter;

public:

	ObjectPropertyWindow* objectPropWindow;

	QString ObjectPath; //the directory for all object textures
};


