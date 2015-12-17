#pragma once

#include "includeseditor.h"
#include "layers.h"

class Template : public QGraphicsScene
{
public:
	Template();
	~Template();

	void Save(const QString& filename);
	void Load(const QString &filename);

	void InitAsLevel();
	bool CreateNewTemplate();

	void StampTo(int x, int y);
	void Unstamp(int x, int y);
	void GetMaxXY(int& x, int& y);

	void UpdateLayers();

	Template* GetMirror();


	ArtLayer m_front;
	ArtLayer m_back;
	ArtLayer m_middle;
	MechLayer m_mec;
	MechLayer m_col;

	QString m_name;
};


class TemplateFolder
{
public:

	TemplateFolder();
	~TemplateFolder();

	void SetFolderName(const QString &name);
	void LoadTileIntoScene();
	void SetCurrentTemplate();
	void SetCurrentTemplate(const QString& name);
	void AddTemplate(Template* t);
	void SelectNextTemplate();
	void SelectPreviousTemplate();

	Template* GetCurrentTemplate();

	QString m_folderName;
	Template* m_currentTemplate;
	QMap<QString, Template*> m_templateMap;

	QGraphicsScene* m_pallette;
};

#include <QStringList>
class QStringListModel;
class QListView;
class TemplateScene : public QWidget
{
	Q_OBJECT
public:

	TemplateScene();
	~TemplateScene();

	void Initialize();

	void folderListClicked(const QModelIndex &index);
	void folderNameChanged(const QString& name);
	void AddFolder(QString foldername);
	void RemoveFolder(QString foldername);
	void AddTemplate(Template* t);
	void UpdateList();
	void SetCurrentFolder(QString name);
	void SetCurrentTemplate(QString name);

	Template* GetCurrentTemplate();
	TemplateFolder* GetCurrentFolder();

	TemplateFolder* m_currentFolder;
	QStringList m_nameList;

	QMap<QString, TemplateFolder*> m_folderMap;

	QListView* m_listView;
	QStringListModel* m_model;
};

