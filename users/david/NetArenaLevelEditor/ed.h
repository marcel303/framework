#pragma once

#include <QString>

enum EditorMode
{
	EM_Level,
	EM_Template,
};

class EditorScene;
class TemplateScene;
class ObjectPropertyWindow;
class EditorView;
class QGraphicsView;
class GameObject;
class QGraphicsScene;
class SettingsWidget;
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
	QGraphicsScene* GetSceneTemplatePallette();


    EditorScene*& GetCurrentScene();

	TemplateScene*& GetTemplateScene(){return m_templateScene;}


    EditorView*& GetView();
    QGraphicsView*& GetViewPallette();

	QList<GameObject*>& GetGameObjects();

	EditorMode& GetEditorMode(){return m_editorMode;}

    SettingsWidget* GetSettingsWidget();


    void EditTemplates();
    void EditLevels();
	void EditObjects();


    int& GetSceneCounter();

	int GetMapX(){return m_mapx;}
	int GetMapY(){return m_mapy;}

	void CreateNewMap(int x, int y);



	void AddToArtList(QString filename);
	void RemoveFromArtList(QString filename);

	void ConvertArtListToLevelArtIndex();




private:
	Ed()
	{
		Initialize();
	}
	~Ed(){}

	void Initialize();


	EditorScene* m_sceneArt;
	EditorScene* m_sceneArtTemplate;

	EditorScene* m_sceneMech;
	EditorScene* m_sceneMechTemplate;

	EditorScene* m_sceneCollission;
	EditorScene* m_sceneCollissionTemplate;



	EditorMode m_editorMode;

    EditorView* m_view;
    QGraphicsView* m_viewPallette;

	QGraphicsScene* m_sceneTemplatePallette;


	TemplateScene* m_templateScene;


    SettingsWidget* m_settingsWidget;

    int m_sceneCounter;

	int m_mapx;
	int m_mapy;

public:

	void SetEditorMode(EditorMode e);

	bool m_leftbuttonHeld;

	ObjectPropertyWindow* objectPropWindow;

	QString ObjectPath; //the directory for all object textures
	QString ArtFolderPath; //the root directory for all art and template textures
};


