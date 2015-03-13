#pragma once


#include <QGraphicsScene>
#include <QGLWidget>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>

#include <QListView>

#include <QPair>
#include <QList>

#include <QMap>

class Tile : public QGraphicsPixmapItem
{
public:
    Tile();
    virtual ~Tile();

    virtual void mousePressEvent ( QGraphicsSceneMouseEvent * e );
    virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * e );
    virtual void hoverEnterEvent ( QGraphicsSceneHoverEvent * e );

    virtual void SetSelectedBlock(short block);

    short getBlock(){return selectedBlock;}

protected:
    short selectedBlock;
};


class TilePallette : public Tile
{
public:
    TilePallette ();
    virtual ~TilePallette ();

    virtual void mousePressEvent ( QGraphicsSceneMouseEvent * e );
	virtual void hoverEnterEvent ( QGraphicsSceneHoverEvent * e ){}
};

class EditorScene : public QGraphicsScene
{
public:
    EditorScene();
    virtual ~EditorScene ();

    virtual void CreateLevel(int x, int y);
    virtual void InitializeLevel();
    void AddGrid();

    virtual void DeleteTiles();
    virtual void CreateTiles();

	virtual void CustomMouseEvent (QGraphicsSceneMouseEvent * e , Tile *tile);

	Tile** m_tiles;

    int m_mapx;
    int m_mapy;
};







class EditorTemplate : public QGraphicsPixmapItem
{
public:

    EditorTemplate();
    ~EditorTemplate();

	virtual void mousePressEvent ( QGraphicsSceneMouseEvent * e );

    class TemplateTile
    {
    public:

        int x;
        int y;

        short blockMech;
        short blockColl;
		QString blockArt;

        short GetArtKey();
        QPixmap* GetPixmap();

        bool operator==(const TemplateTile& right){return (x == right.x) && (y == right.y);}
    };


    TemplateTile* GetTemplateTile(int x, int y);
    void RemoveTemplateTile(int x, int y);

    void LoadTemplateIntoScene();

    QList<TemplateTile> m_list;

    QString m_name;

	void LoadTemplate(QString filename);
    void SaveTemplate();

	int ImportFromImage(QString filename);
	void ConvertToPalletteTexture();
};

class TemplateFolder
{
public:

	TemplateFolder();
	~TemplateFolder();

	void AddTemplate(EditorTemplate* t);
	void SaveTemplate();

    void LoadTileIntoScene();

	EditorTemplate* GetCurrentTemplate();
	void SetCurrentTemplate();
	void SetCurrentTemplate(QString name);

	void SetFolderName(QString name);





	EditorTemplate* m_currentTemplate;

	QMap<QString, EditorTemplate*> m_templateMap;
	QString m_folderName;

	QGraphicsScene* m_scene;
};


#include <QStringListModel>
class TemplateScene: public QWidget
{
	Q_OBJECT
public:
    TemplateScene();
    virtual ~TemplateScene();

	void Initialize();

    EditorTemplate* GetCurrentTemplate();
	void SetCurrentTemplate(QString name);

	void SetCurrentFolder(QString name);
	QString GetCurrentFolderName(){return m_currentFolder->m_folderName;}

    void AddTemplate(EditorTemplate* t);
	void AddFolder(QString foldername);
	void RemoveFolder(QString foldername);

	void UpdateList();

	QStringList m_nameList;
	QStringListModel* m_model;
	QListView* m_listView;
	TemplateFolder* m_currentFolder;

	QMap<QString, TemplateFolder*> m_folderMap;

public slots:
	void folderListClicked(const QModelIndex &index);
    void folderNameChanged(const QString& name);

};
