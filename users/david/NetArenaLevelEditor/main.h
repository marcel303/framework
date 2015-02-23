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

class TileProperties
{
public:
   TileProperties();
   ~TileProperties();


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

    virtual void CustomMouseEvent ( QGraphicsSceneMouseEvent * e );

    Tile** m_tiles;
	TileProperties** m_properties;

    int m_mapx;
    int m_mapy;
};



class EditorViewBasic : public QGraphicsView
{
public:
    EditorViewBasic ();
    virtual ~EditorViewBasic ();

    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *e);

    void zoomIn(int level = 1);
    void zoomOut(int level = 1);

    int zoomLevel;

    void UpdateMatrix();


#ifndef QT_NO_WHEELEVENT
    void wheelEvent(QWheelEvent * e);
#endif


};

class QSlider;
class EditorView : public EditorViewBasic
{
    Q_OBJECT
public:
    EditorView();
    virtual ~EditorView();


    QSlider* sliderOpacMech;
    QSlider* sliderOpacArt;
    QSlider* sliderOpacColl;
	QSlider* sliderOpacObject;

public slots:
    void Save();
    void SaveTemplate();
    void Load();
    void New();
	void SwitchToMech();
	void SwitchToArt();
	void SwitchToCollission();
	void SwitchToObject();

    void SwitchToTemplateMode();

    void SetOpacityMech(int s);
    void SetOpacityArt(int s);
    void SetOpacityCollission(int s);
	void SetOpacityObject(int s);


    void SwitchToBigMap();

	void ImportTemplate();

	void mousePressEvent(QMouseEvent * e);
	void mouseReleaseEvent(QMouseEvent * e);

};





class QPushButton;
class NewMapWindow : public QWidget
{
    Q_OBJECT
public:
    NewMapWindow();
    ~NewMapWindow();

public slots:
    void NewMap();
    void CancelNewMap();


public:

    QPushButton* ok;
    QPushButton* cancel;

    QSlider* x;
    QSlider* y;

};


class EditorTemplate
{
public:

    EditorTemplate();
    ~EditorTemplate();

    struct TemplateTile
    {
        int x;
        int y;

        short blockMech;
        short blockArt;
        short blockColl;
    };

    QList<TemplateTile> m_list;

    QString m_name;

	void LoadTemplate(QString filename);
	void SaveTemplate(int tx, int ty);

	int ImportFromImage(QString filename);
	void ConvertToPalletteTexture();

};

#include <QStringListModel>

class TemplateScene: public QWidget
{
	Q_OBJECT
public:
    TemplateScene();
    virtual ~TemplateScene();

    EditorTemplate* GetCurrentTemplate();

    void AddTemplate(EditorTemplate* t);

	void UpdateList();

	QMap<QString, EditorTemplate*> m_templateMap;
	QStringList m_nameList;
	QStringListModel* m_model;
	QListView* m_listView;
    EditorTemplate* m_currentTemplate;

public slots:
	void templateListClicked(const QModelIndex &index);
	void templateNameChanged(const QString& name);

};
