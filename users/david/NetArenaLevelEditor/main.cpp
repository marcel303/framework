#include "main.h"

#include <QApplication>

#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QWidget>
#include <QMainWindow>


#include <QGraphicsSceneMouseEvent>
#include <QDebug>


#include <QMap>


#include <QSlider>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>

#include <QButtonGroup>

#define BASEX 26
#define BASEY 16

#define BLOCKSIZE 64

int MAPX = BASEX;
int MAPY = BASEY;

char selectedTile = ' ';

TilePallette* palletteTile;
EditorScene* sceneMech;
QGraphicsScene* scenePallette;
QMap<short, QPixmap*> pixmaps;


EditorScene* sceneArt;
QGraphicsScene* sceneArtPallette;
QMap<short, QPixmap*> pixmapsArt;


EditorScene* sceneCollission;
QGraphicsScene* sceneCollissionPallette;
QMap<short, QPixmap*> pixmapsCollission;

QMap<QString, QPixmap*> pixmapObjects;
QMap<QString, GameObject*> objectPallette;
QGraphicsScene* sceneObjectPallette;
QList<GameObject*> gameObjects;
QString ObjectPath; //the directory for all object textures
ObjectPropertyWindow* objectPropWindow = 0;

EditorView* view;
QGraphicsView* viewPallette;


EditorScene* templateScene;
Tile** m_templateTiles;

bool leftbuttonHeld = false;

enum EditorMode
{
	EM_Level,
	EM_Object,
	EM_Template,
} editorMode;




void SetSelectedTile(char selection)
{
    selectedTile = selection;
    if(palletteTile)
        palletteTile->SetSelectedBlock(selectedTile);
}

void AddAllToScene()
{
    for(int y = 0; y < MAPY; y++)
        for (int x = 0; x < MAPX; x++)
        {
            sceneMech->m_tiles[y][x].setOpacity(0.7);

            sceneCollission->m_tiles[y][x].setAcceptedMouseButtons(0);
            sceneCollission->m_tiles[y][x].setAcceptHoverEvents(false);
            sceneCollission->m_tiles[y][x].setOpacity(0.2);

            sceneArt->m_tiles[y][x].setAcceptedMouseButtons(0);
            sceneArt->m_tiles[y][x].setAcceptHoverEvents(false);
            sceneArt->m_tiles[y][x].setOpacity(0.2);

            sceneMech->addItem(&sceneCollission->m_tiles[y][x]);
            sceneMech->addItem(&sceneArt->m_tiles[y][x]);
        }
}



void SetEditMechScene()
{
    for(int y = 0; y < MAPY; y++)
        for (int x = 0; x < MAPX; x++)
        {
            sceneMech->m_tiles[y][x].setAcceptedMouseButtons(Qt::AllButtons);
            sceneMech->m_tiles[y][x].setAcceptHoverEvents(true);

            sceneCollission->m_tiles[y][x].setAcceptedMouseButtons(Qt::NoButton);
            sceneCollission->m_tiles[y][x].setAcceptHoverEvents(false);

            sceneArt->m_tiles[y][x].setAcceptedMouseButtons(Qt::NoButton);
            sceneArt->m_tiles[y][x].setAcceptHoverEvents(false);
        }

	foreach(GameObject* obj, gameObjects)
		obj->setAcceptedMouseButtons(Qt::NoButton);
}

void SetEditArtScene()
{
    for(int y = 0; y < MAPY; y++)
        for (int x = 0; x < MAPX; x++)
        {
            sceneMech->m_tiles[y][x].setAcceptedMouseButtons(Qt::NoButton);
            sceneMech->m_tiles[y][x].setAcceptHoverEvents(false);

            sceneCollission->m_tiles[y][x].setAcceptedMouseButtons(Qt::NoButton);
            sceneCollission->m_tiles[y][x].setAcceptHoverEvents(false);

            sceneArt->m_tiles[y][x].setAcceptedMouseButtons(Qt::AllButtons);
            sceneArt->m_tiles[y][x].setAcceptHoverEvents(true);
        }

	foreach(GameObject* obj, gameObjects)
		obj->setAcceptedMouseButtons(Qt::NoButton);
}

void SetEditCollissionScene()
{
    for(int y = 0; y < MAPY; y++)
        for (int x = 0; x < MAPX; x++)
        {
            sceneMech->m_tiles[y][x].setAcceptedMouseButtons(Qt::NoButton);
            sceneMech->m_tiles[y][x].setAcceptHoverEvents(false);

            sceneCollission->m_tiles[y][x].setAcceptedMouseButtons(Qt::AllButtons);
            sceneCollission->m_tiles[y][x].setAcceptHoverEvents(true);

            sceneArt->m_tiles[y][x].setAcceptedMouseButtons(Qt::NoButton);
            sceneArt->m_tiles[y][x].setAcceptHoverEvents(false);
        }

	foreach(GameObject* obj, gameObjects)
		obj->setAcceptedMouseButtons(Qt::NoButton);
}

void SetEditObjects()
{
	for(int y = 0; y < MAPY; y++)
		for (int x = 0; x < MAPX; x++)
		{
			sceneMech->m_tiles[y][x].setAcceptedMouseButtons(Qt::AllButtons);
			sceneMech->m_tiles[y][x].setAcceptHoverEvents(false);

			sceneCollission->m_tiles[y][x].setAcceptedMouseButtons(Qt::NoButton);
			sceneCollission->m_tiles[y][x].setAcceptHoverEvents(false);

			sceneArt->m_tiles[y][x].setAcceptedMouseButtons(Qt::NoButton);
			sceneArt->m_tiles[y][x].setAcceptHoverEvents(false);
		}

	foreach(GameObject* obj, gameObjects)
		obj->setAcceptedMouseButtons(Qt::AllButtons);
}


void SetOpactyForLayer(EditorScene* s, qreal opac)
{
    for(int y = 0; y < MAPY; y++)
        for (int x = 0; x < MAPX; x++)
            s->m_tiles[y][x].setOpacity(opac);
}


int sceneCounter = 0;
void SwitchSceneTo(int s)
{
    if(s == sceneCounter)
        return;

    sceneCounter = s;

	if(sceneCounter > 3)
        sceneCounter = 0;

	editorMode = EM_Level;

    switch(sceneCounter)
    {
    case 0:
        viewPallette->setScene(scenePallette);
        SetSelectedTile(pixmaps.begin().key());
        view->setWindowTitle("Mechanical Layout");
        viewPallette->setWindowTitle("Mechanical Layout");
        scenePallette->addItem(palletteTile);
        SetEditMechScene();
        break;
    case 1:
        viewPallette->setScene(sceneArtPallette);
        SetSelectedTile(pixmapsArt.begin().key());
        view->setWindowTitle("Art Layout");
        viewPallette->setWindowTitle("Art Pallette");
        sceneArtPallette->addItem(palletteTile);
        SetEditArtScene();
        break;
    case 2:
        viewPallette->setScene(sceneCollissionPallette);
        SetSelectedTile(pixmapsCollission.begin().key());
        view->setWindowTitle("Collission Layout");
        viewPallette->setWindowTitle("Collission Pallette");
        sceneCollissionPallette->addItem(palletteTile);
        SetEditCollissionScene();
        break;
	case 3:
        editorMode = EM_Object;
        view->setWindowTitle("Editing Level Objects");
		viewPallette->setWindowTitle("GameObjects");
		viewPallette->setScene(sceneObjectPallette);
		SetEditObjects();
		break;
    default:
        break;
    }
}

void SwitchScene()
{
    SwitchSceneTo(sceneCounter+1);
}

QMap<short, QPixmap*>& getCurrentPixmap()
{
    switch (sceneCounter)
    {
    case 0:
        return pixmaps;
        break;
    case 1:
        return pixmapsArt;
        break;
    case 2:
        return pixmapsCollission;
        break;
    default:
        return pixmaps;
        break;
    }
    return pixmaps;
}

#include <QFile>
QList<QString> GetLinesFromConfigFile(QString filename)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream in(&file);

    qDebug() << filename << " open = " << file.isOpen();

    QList<QString> list;
    while(!in.atEnd())
    {
        QString line = in.readLine();
        list.push_back(line);
    }

    return list;
}

void LoadPixmapsGeneric(QString filename, QMap<short, QPixmap*>& map)
{
    QPixmap* p;
    QPixmap* p2;

    QList<QString> list = GetLinesFromConfigFile(filename);

    QString path = list.front();
    list.pop_front();

    while(!list.empty())
    {
        QString line = list.front();
        QChar key = line[line.size()-1];
        short key2 = key.toLatin1();
        line.chop(4);

        p = new QPixmap(path+line);
        p2 = new QPixmap(p->scaledToWidth(BLOCKSIZE));
        delete p;
        map[key2] = p2;

        list.pop_front();
    }
}

void LoadPixmapsArt(QString filename, QMap<short, QPixmap*>& map)
{
    QPixmap* p;
    QPixmap* p2;

    QList<QString> list = GetLinesFromConfigFile(filename);

    QString path = list.front();
    list.pop_front();

    short key = 0;
    while(!list.empty())
    {
        QString line = list.front();

        p = new QPixmap(path+line);
        p2 = new QPixmap(p->scaledToWidth(BLOCKSIZE));
        delete p;
        map[key] = p2;
        key++;

        list.pop_front();
    }
}

GameObject* LoadGameObject(QMap<QString, QString>& map)
{
    GameObject* obj = new GameObject();
    obj->Load(map);

    return obj;
}

QPixmap* GetObjectPixmap(QString texture)
{
    if(!pixmapObjects.contains(texture))
    {
        QPixmap* p = new QPixmap(ObjectPath + texture);
        pixmapObjects[texture] = p;
    }
    return pixmapObjects[texture];
}

void AddGameObject(GameObject* obj)
{
    gameObjects.push_back(obj);
    sceneMech->addItem(obj);
}


void LoadObjects(QString filename, bool templates)
{
    QList<QString> list = GetLinesFromConfigFile(filename);

    ObjectPath = list.front();
    list.pop_front();

	QMap<QString, QString> map;
    while(!list.empty())
    {
        QString line = list.front();
        QStringList pair = line.split(":");

        if(pair.first() == "object" && !map.isEmpty())
        {
            LoadGameObject(map);
            map.clear();
        }
        map[pair.first()] = pair.last();

        list.pop_front();
    }
    GameObject* obj;
    if(!map.isEmpty())
        obj = LoadGameObject(map);

    if(templates) //fill the object pallette
	{

		QPixmap* p = new QPixmap();
		*p = obj->pixmap().scaled(BLOCKSIZE, BLOCKSIZE);
		obj->setPixmap(*p);
        objectPallette[obj->type] = obj;
	}
    else
        AddGameObject(obj);

}

void LoadPixmaps()
{
    LoadPixmapsGeneric("BlockList.txt", pixmaps);
    sceneCounter = 1;
    LoadPixmapsArt("ArtList.txt", pixmapsArt);
    sceneCounter =2;
    LoadPixmapsGeneric("CollissionList.txt", pixmapsCollission);
    sceneCounter = 0;
    LoadObjects("Objects.txt", true);
}


void LoadPalletteGeneric(QGraphicsScene* s, QMap<short, QPixmap*>& map)
{
    TilePallette* t;
    QMap<short, QPixmap*>::iterator i;

    int y = 1;
    int x = 0;
    for(i = map.begin(); i != map.end(); ++i)
    {
        if(x > 3)
        {
            y++;
            x = 0;
        }

        t = new TilePallette();
        t->SetSelectedBlock(i.key());
        s->addItem(t);
        t->setPos(x*BLOCKSIZE, y*BLOCKSIZE);

        x++;
    }

    palletteTile->SetSelectedBlock(map.begin().key());
    palletteTile->setPos(200, 000);
    s->addItem(palletteTile);
}

void LoadObjectsPallette()
{
	int x = 0;
	int y = 0;
    foreach(GameObject* obj, objectPallette.values())
    {
		if(x > 3)
		{
			y++;
			x = 0;
		}

		obj->setPos(x*BLOCKSIZE, y*BLOCKSIZE);
		sceneObjectPallette->addItem(obj);

		x++;
    }
}

void LoadPallette()
{
    sceneCounter = 1;
    LoadPalletteGeneric(sceneArtPallette, pixmapsArt);
    sceneCounter = 2;
    LoadPalletteGeneric(sceneCollissionPallette, pixmapsCollission);
    sceneCounter =0;
    LoadPalletteGeneric(scenePallette, pixmaps);

	LoadObjectsPallette();
}




void LoadGeneric(QString filename, EditorScene* s)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly | QIODevice::Text);



    QTextStream in(&file);

    for(int y = 0; y < MAPY; y++)
    {
        for (int x = 0; x < MAPX; x++)
        {
            char key = in.read(1)[0].toLatin1();
            s->m_tiles[y][x].SetSelectedBlock(key);
        }
        if(y < (MAPY -1))
                in.read(1);
    }
    file.close();
}

void LoadLevel(QString filename)
{
    sceneCounter = 0;
    LoadGeneric(filename + ".txt", sceneMech);
    sceneCounter = 1;
    LoadGeneric(filename + "Art.txt", sceneArt);
    sceneCounter = 2;
    LoadGeneric(filename + "Collission.txt", sceneCollission);
    sceneCounter = 0;
    LoadObjects(filename, false);
}

void SaveGeneric(QString filename, EditorScene* s)
{
    QFile file(filename);
    file.open(QIODevice::WriteOnly | QIODevice::Text);

    QTextStream in(&file);

    for(int y = 0; y < MAPY; y++)
    {
        for (int x = 0; x < MAPX; x++)
        {
            in << (char)(s->m_tiles[y][x].getBlock());
        }
        if(y < (MAPY -1))
                in << endl;
    }
    file.close();
}

void SaveArtFile(QString filename, EditorScene* s)
{
    QFile file(filename);
    file.open(QIODevice::WriteOnly);


    QDataStream in(&file);

    in << MAPX << MAPY;
    for(int y = 0; y < MAPY; y++)
        for (int x = 0; x < MAPX; x++)
            in << (quint16 )(s->m_tiles[y][x].getBlock());
    file.close();
}

void SaveObjects(QString filename)
{
	QFile file(filename);
	file.open(QIODevice::WriteOnly);

	QTextStream in(&file);

	foreach(GameObject* obj, gameObjects)
	{
		QString a = obj->toText();
		in << a;
	}
	file.close();
}


void SaveLevel(QString filename)
{
    SaveGeneric(filename + ".txt", sceneMech);
    if(sceneCollission)
        SaveGeneric(filename + "Collission.txt", sceneCollission);
    if(sceneArt)
        SaveArtFile(filename + "Art.txt", sceneArt);

	SaveObjects(filename + "Objects.txt");
}

void SwitchMap(int x, int y)
{
    MAPX = x;
    MAPY = y;


    sceneMech->setSceneRect(0,0,MAPX*BLOCKSIZE,MAPY*BLOCKSIZE);

    LoadPixmaps();

    //scene->CreateTiles();
    sceneMech->CreateLevel(x, y);
    sceneArt->CreateLevel(x, y);
    sceneCollission->CreateLevel(x, y);

    view->scale((float)MAPY/(float)MAPX,(float)MAPY/(float)MAPX);
}





void CreateSettingsWidget();
void CreateObjectPropertyWindow();
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

	editorMode = EM_Level;

    view = new EditorView();
    sceneMech = new EditorScene();
    sceneArt = new EditorScene();
	sceneCollission = new EditorScene();

    view->setScene(sceneMech);
    view->showMaximized();



    SwitchMap(MAPX, MAPY);



    palletteTile = new TilePallette();

    SetSelectedTile('x');


	objectPropWindow = new ObjectPropertyWindow();
	objectPropWindow->CreateObjectPropertyWindow();

    viewPallette = new EditorViewBasic;
    scenePallette = new QGraphicsScene;
    sceneArtPallette = new QGraphicsScene;
    sceneCollissionPallette = new QGraphicsScene;
	sceneObjectPallette = new QGraphicsScene();

    viewPallette->setScene(scenePallette);
    viewPallette->showNormal();
    viewPallette->resize(430, 600);

    scenePallette->setSceneRect(0,0,4*BLOCKSIZE,10*BLOCKSIZE);
    sceneArtPallette->setSceneRect(0,0,4*BLOCKSIZE,10*BLOCKSIZE);
    sceneCollissionPallette->setSceneRect(0,0,4*BLOCKSIZE,10*BLOCKSIZE);

    LoadPallette();

    viewPallette->show();

    AddAllToScene();
    sceneMech->AddGrid();

    CreateSettingsWidget();



    return a.exec();
}




void LoadImageAndCreateArtTiles()
{
}

Tile::Tile()
{
    SetSelectedBlock(' ');

    this->setOpacity(1.0);

    setAcceptHoverEvents(true);
}

Tile::~Tile()
{
}

void Tile::mousePressEvent ( QGraphicsSceneMouseEvent * e )
{
    qDebug() << "setting tile to selectedBlock: " << selectedBlock;

	if(editorMode == EM_Level)
    {
        if(e->buttons() == Qt::RightButton)
            SetSelectedBlock(' ');
        else if (e->buttons() == Qt::LeftButton)
            SetSelectedBlock(selectedTile);
        else if (e->buttons() == Qt::MiddleButton)
            LoadLevel("save");
        else
            SaveLevel("save");

        e->accept();
    }
    else
        sceneMech->CustomMouseEvent(e);
}

void Tile::mouseReleaseEvent ( QGraphicsSceneMouseEvent * e )
{
    e->accept();
}

void Tile::SetSelectedBlock(short block)
{
    if(getCurrentPixmap().contains(block))
        setPixmap(*(getCurrentPixmap()[block]));

    selectedBlock = block;
}

void Tile::hoverEnterEvent ( QGraphicsSceneHoverEvent * e )
{
    if(leftbuttonHeld)
            SetSelectedBlock(selectedTile);

    e->accept();
}







TilePallette::TilePallette() : Tile()
{
    selectedBlock = ' ';
}

TilePallette::~TilePallette()
{
}

void TilePallette::mousePressEvent ( QGraphicsSceneMouseEvent * e )
{
    SetSelectedTile(selectedBlock);

    e->accept();
}





EditorScene::EditorScene()
{
    m_tiles = 0;

    m_mapx = MAPX;
    m_mapy = MAPY;
}

EditorScene::~EditorScene ()
{
    DeleteTiles();
}

void EditorScene::DeleteTiles()
{
    if(m_tiles == 0)
        return;

    for(int i = 0; i < m_mapy; ++i)
        delete[] m_tiles[i];

    delete[] m_tiles;
}

void EditorScene::CreateTiles()
{
    m_tiles = new Tile*[m_mapy];
    for(int i = 0; i < m_mapy; ++i)
        m_tiles[i] = new Tile[m_mapx];
}



void EditorScene::CreateLevel(int x, int y)
{
    DeleteTiles();

    m_mapx = x;
    m_mapy = y;

    CreateTiles();

    InitializeLevel();
}

void EditorScene::InitializeLevel()
{
    for(int y = 0; y < m_mapy; y++)
        for (int x = 0; x < m_mapx; x++)
        {
            m_tiles[y][x].SetSelectedBlock(' ');
            addItem(&m_tiles[y][x]);
            m_tiles[y][x].setPos(x*BLOCKSIZE, y*BLOCKSIZE);
        }
}

void EditorScene::AddGrid()
{
    for (int x = 0; x <= m_mapx*BLOCKSIZE; x+=BLOCKSIZE)
    {
        addLine(x, 0, x, m_mapy*BLOCKSIZE);
    }
    for(int y = 0; y <= m_mapy*BLOCKSIZE; y+= BLOCKSIZE)
    {
        addLine(0, y, m_mapx*BLOCKSIZE, y);
    }
}

EditorTemplate templ;

void EditorScene::CustomMouseEvent ( QGraphicsSceneMouseEvent * e )
{
	switch(editorMode)
	{
		case EM_Level:
			e->ignore();
			break;
		case EM_Object:
		{
			GameObject* obj = new GameObject();
			obj->palletteTile = false;
			obj->Load(objectPropWindow->GetCurrentGameObject()->toText());

			obj->SetPos(e->scenePos());


			AddGameObject(obj);

            objectPropWindow->SetCurrentGameObject(obj);

			e->accept();
			break;
		}
		case EM_Template:
		{
            int tilex = (int)(e->scenePos().x()/float(BLOCKSIZE));
            int tiley = (int)(e->scenePos().y()/float(BLOCKSIZE));

			EditorTemplate::TemplateTile t;
			t.x         = tilex;
			t.y         = tiley;
			QString n = QString::number(t.x) + "_" + QString::number(t.y);
			t.blockMech = m_tiles[tiley][tilex].getBlock();
			t.blockArt  = sceneArt->m_tiles[tiley][tilex].getBlock();
			t.blockColl = sceneCollission->m_tiles[tiley][tilex].getBlock();
			if(templ.m_list.contains(n))
				templ.m_list.remove(n);
			else
				templ.m_list[n] = t;

			e->accept();

			break;
		}
		default:
			e->ignore();
			break;
	}
}


#include <QMenuBar>


EditorView::EditorView() : EditorViewBasic()
{
    QMenuBar* bar = new QMenuBar(this);

    QAction* newAct1 = new QAction(tr("&New"), this);
    newAct1->setShortcuts(QKeySequence::New);
    newAct1->setStatusTip(tr("Create a new file"));
    connect(newAct1, SIGNAL(triggered()), this, SLOT(New()));

    QAction* newAct2 = new QAction(tr("&Save"), this);
    newAct2->setShortcuts(QKeySequence::Save);
    newAct2->setStatusTip(tr("Save to file"));
    connect(newAct2, SIGNAL(triggered()), this, SLOT(Save()));

    QAction* newAct3 = new QAction(tr("&Load"), this);
    newAct3->setShortcuts(QKeySequence::Open);
    newAct3->setStatusTip(tr("Load from file"));
    connect(newAct3, SIGNAL(triggered()), this, SLOT(Load()));

    QAction* newAct4 = new QAction(tr("&TemplateMode"), this);
    newAct4->setStatusTip(tr("Templaaaaate"));
    connect(newAct4, SIGNAL(triggered()), this, SLOT(SwitchToTemplateMode()));

    QAction* newAct7 = new QAction(tr("&MapSize"), this);
    newAct7->setStatusTip(tr("Switch to Big Map(tm)"));
    connect(newAct7, SIGNAL(triggered()), this, SLOT(SwitchToBigMap()));


    bar->addAction(newAct1);
    bar->addAction(newAct2);
    bar->addAction(newAct3);
    bar->addAction(newAct4);
    bar->addAction(newAct7);

    bar->showNormal();
}



#include <QDir>
#include <QFileDialog>

EditorView::~EditorView()
{
}

void EditorView::Save()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save to file"));
    SaveLevel(fileName);
}

void EditorView::Load()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open file"));
    fileName.chop(4);
    LoadLevel(fileName);
}
void EditorView::New()
{
    sceneMech->CreateLevel(MAPX, MAPY);
    sceneArt->CreateLevel(MAPX, MAPY);
    sceneCollission->CreateLevel(MAPX, MAPY);
}
void EditorView::SwitchToMech(int s)
{
    if(s)
    {
        SwitchSceneTo(0);
        sliderOpacMech->setValue(80);
        sliderOpacArt->setValue(20);
        sliderOpacColl->setValue(20);
		sliderOpacObject->setValue(20);
    }
}
void EditorView::SwitchToArt(int s)
{
    if(s)
    {
        SwitchSceneTo(1);
        sliderOpacMech->setValue(20);
        sliderOpacArt->setValue(80);
        sliderOpacColl->setValue(20);
		sliderOpacObject->setValue(20);
    }
}
void EditorView::SwitchToCollission(int s)
{
    if(s)
    {
        SwitchSceneTo(2);
        sliderOpacMech->setValue(20);
        sliderOpacArt->setValue(20);
        sliderOpacColl->setValue(80);
		sliderOpacObject->setValue(20);
    }
}

void EditorView::SwitchToObject(int s)
{
	if(s)
	{
		SwitchSceneTo(3);
		sliderOpacMech->setValue(20);
		sliderOpacArt->setValue(20);
		sliderOpacColl->setValue(20);
		sliderOpacObject->setValue(100);
	}
}

void EditorView::SwitchToBigMap()
{
    if(MAPX == BASEX)
        SwitchMap(2*BASEX, 2*BASEY);
    else
        SwitchMap(BASEX, BASEY);
}

void EditorView::SwitchToTemplateMode()
{
	if(editorMode != EM_Template)
		editorMode = EM_Template;
	else
		editorMode = EM_Level;
}

void EditorView::SetOpacityMech(int s)
{
   SetOpactyForLayer(sceneMech, s/100.0);
}

void EditorView::SetOpacityArt(int s)
{
   SetOpactyForLayer(sceneArt, s/100.0);
}
void EditorView::SetOpacityCollission(int s)
{
    SetOpactyForLayer(sceneCollission, s/100.0);
}

void EditorView::SetOpacityObject(int s)
{
    foreach(GameObject* obj, gameObjects)
	{
		obj->setOpacity(qreal(s) / 100.0);
	}
}




#include <QKeyEvent>

EditorViewBasic::EditorViewBasic() : QGraphicsView()
{
}

EditorViewBasic::~EditorViewBasic()
{

}

void EditorViewBasic::keyPressEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_Space)
    {
        SwitchScene();
        e->accept();
    }

    if(e->key() == Qt::Key_P)
    {
        QGraphicsView* v1 = new QGraphicsView();
        v1->setScene(templateScene);
        v1->showNormal();
        v1->scale((float)MAPY/(float)MAPX,(float)MAPY/(float)MAPX);
        e->accept();
    }

    if(e->key() == Qt::Key_Shift)
    {
        leftbuttonHeld = true;
        e->accept();
    }

    if(e->key() == Qt::Key_I)
        AddAllToScene();
}

void EditorViewBasic::keyReleaseEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_Shift)
    {
        leftbuttonHeld = false;
        e->accept();
    }
}



void CreateSettingsWidget()
{
    QWidget* w = new QWidget();

    QGridLayout* grid = new QGridLayout();

    QLabel* label = new QLabel("Edit Layer");
    grid->addWidget(label, 0, 1);

    label = new QLabel("Mech");
    grid->addWidget(label, 1, 0);
    label = new QLabel("Art");
    grid->addWidget(label, 2, 0);
    label = new QLabel("Coll");
    grid->addWidget(label, 3, 0);
	label = new QLabel("Obj");
	grid->addWidget(label, 4, 0);


    QButtonGroup* bgroup = new QButtonGroup();
    QCheckBox* cb = new QCheckBox();
    QObject::connect(cb, SIGNAL(stateChanged(int)),
            view, SLOT(SwitchToMech(int)));
    grid->addWidget(cb, 1, 1);
    bgroup->addButton(cb);

    cb = new QCheckBox();
    QObject::connect(cb, SIGNAL(stateChanged(int)),
            view, SLOT(SwitchToArt(int)));
    grid->addWidget(cb, 2, 1);
    bgroup->addButton(cb);

    cb = new QCheckBox();
    QObject::connect(cb, SIGNAL(stateChanged(int)),
            view, SLOT(SwitchToCollission(int)));
    grid->addWidget(cb, 3, 1);
    bgroup->addButton(cb);

	cb = new QCheckBox();
	QObject::connect(cb, SIGNAL(stateChanged(int)),
			view, SLOT(SwitchToObject(int)));
	grid->addWidget(cb, 4, 1);
	bgroup->addButton(cb);

    bgroup->setExclusive(true);

    label = new QLabel("Transparency");
    grid->addWidget(label, 0, 2);

    view->sliderOpacMech = new QSlider(Qt::Horizontal);
    view->sliderOpacMech->setMinimum(0);
    view->sliderOpacMech->setMaximum(100);
    view->sliderOpacMech->setTickInterval(1);
    view->sliderOpacMech->setValue(100);

    view->sliderOpacArt = new QSlider(view->sliderOpacMech);
    view->sliderOpacArt->setOrientation(Qt::Horizontal);
    view->sliderOpacColl = new QSlider(view->sliderOpacMech);
    view->sliderOpacColl->setOrientation(Qt::Horizontal);
	view->sliderOpacObject = new QSlider(view->sliderOpacMech);
	view->sliderOpacObject->setOrientation(Qt::Horizontal);

    QObject::connect(view->sliderOpacMech, SIGNAL(valueChanged(int)),
            view, SLOT(SetOpacityMech(int)));
    QObject::connect(view->sliderOpacArt, SIGNAL(valueChanged(int)),
            view, SLOT(SetOpacityArt(int)));
    QObject::connect(view->sliderOpacColl, SIGNAL(valueChanged(int)),
            view, SLOT(SetOpacityCollission(int)));
	QObject::connect(view->sliderOpacObject, SIGNAL(valueChanged(int)),
			view, SLOT(SetOpacityObject(int)));

    grid->addWidget(view->sliderOpacMech, 1, 2);
    grid->addWidget(view->sliderOpacArt, 2, 2);
    grid->addWidget(view->sliderOpacColl, 3, 2);
	grid->addWidget(view->sliderOpacObject, 4, 2);

    w->setLayout(grid);
    w->show();
}


#include <QColorDialog>
#include <QTextEdit>
#include <QPushButton>

void ObjectPropertyWindow::CreateObjectPropertyWindow()
{
	currentObject = 0;

	m_w = new QWidget();

	QGridLayout* grid = new QGridLayout();

	text = new QTextEdit();
	grid->addWidget(text, 0, 0);

	QPushButton* b = new QPushButton;
	b->setText("Save");
    connect(b, SIGNAL(pressed()), this, SLOT(SaveToGameObject()));

	grid->addWidget(b, 1, 0);


    picker = new QColorDialog();
    picker->setOption(QColorDialog::ShowAlphaChannel);
    connect(picker, SIGNAL(currentColorChanged(const QColor&)), this, SLOT(SetColor()));

    grid->addWidget(picker, 2, 0);

	m_w->setLayout(grid);

	m_w->setWindowTitle("Object Properties");
	m_w->show();
}

void ObjectPropertyWindow::SetColor()
{
    currentObject->q = picker->currentColor();
    UpdateObjectText();
}

void ObjectPropertyWindow::SetCurrentGameObject(GameObject* object)
{
	text->setText(object->toText());
	currentObject = object;
}

void ObjectPropertyWindow::UpdateObjectText()
{
    if(currentObject)
        text->setText(currentObject->toText());
}

GameObject* ObjectPropertyWindow::GetCurrentGameObject()
{
	return currentObject;
}

void ObjectPropertyWindow::SaveToGameObject()
{
	if(currentObject)
	{
		currentObject->Load(text->toPlainText());
        text->setText(currentObject->toText());
	}
}



GameObject::GameObject()
{
    x = -1;
    y = -1;

	type = "none";
	q = Qt::black;

    texture = "";

	speed = -1;

	palletteTile = false;

    setFlags(ItemIsMovable | ItemSendsGeometryChanges);
}

GameObject::~GameObject()
{
}

void GameObject::mousePressEvent ( QGraphicsSceneMouseEvent * e )
{
	objectPropWindow->SetCurrentGameObject(this);

    e->accept();
}

void GameObject::SetPos(QPointF p)
{
	setPos(p);
    //x = pos().x();
    //y = pos().y();

    //if(objectPropWindow && objectPropWindow->currentObject == this)
    //    objectPropWindow->UpdateObjectText();
}

typedef QPair<int, int> qp; //hack around compiler for foreach not being nice
void GameObject::Load(QString data)
{
	QList<QString> lines;

	QTextStream stream(&data);

	QList<QString> list;
	while(!stream.atEnd())
	{
		QString line = stream.readLine();
		list.push_back(line);
	}

	QMap<QString, QString> map;
	foreach(QString line, list)
	{
		QStringList list = line.split(":");
        map[*list.begin()] = list.back();
	}

    Load(map);
}

void GameObject::Load(QMap<QString, QString>& map)
{
    type = map["object"];

    if(map.contains("texture"))
        texture = map["texture"];

    setPixmap(*GetObjectPixmap(texture));

    if(map.contains("x"))
		x = map["x"].toInt();
    if(map.contains("y"))
		y = map["y"].toInt();

    setPos(x, y);

    if(map.contains("speed"))
        speed = map["speed"].toInt();


    if(map.contains("color"))
    {
        map["color"].push_front(map["color"][map["color"].size()-1]);
        map["color"].push_front(map["color"][map["color"].size()-2]);
        map["color"].push_front("#");
        map["color"].chop(2);

        q.setNamedColor(map["color"]);
    }
}

QString GameObject::toText()
{
    x = pos().x();
    y = pos().y();

	QString ret = "";
	if(type != "none")
	{ret += "object:" + type;						ret += "\n";}
    if(x >= 0)
	{ret += "x:" + QString::number(x);               ret += "\n";}
    if(y >= 0)
	{ret += "y:" + QString::number(y);               ret += "\n";}

	if(texture != "")
	{ret += "texture:" + texture; ret += "\n";}

	if(q != Qt::black)
	{
		ret += "color:";
		ret +=  QString::number(q.red(), 16);
		ret +=  QString::number(q.green(), 16);
		ret +=  QString::number(q.blue(), 16);
		ret +=  QString::number(q.alpha(), 16);		ret += "\n";
	}

	int i = 1;
	foreach(qp coord, path)
	{
		ret += "x" + QString::number(i) + ":" + QString::number(coord.first); ret += "\n";
		ret += "y" + QString::number(i) + ":" + QString::number(coord.second); ret += "\n";
		i++;
	}

	if(speed >= 0)
		ret += "speed:" + QString::number(speed);   ret += "\n";

	return ret;
}

QVariant GameObject::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if(change == ItemPositionChange)
    {

        //x = pos().x();
        //y = pos().y();

        if(objectPropWindow && objectPropWindow->currentObject == this)
            objectPropWindow->UpdateObjectText();
    }

    return QGraphicsPixmapItem::itemChange(change, value);
}
