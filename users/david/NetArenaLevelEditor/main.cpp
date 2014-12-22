#include "main.h"

#include <QApplication>

#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QWidget>
#include <QMainWindow>


#include <QGraphicsSceneMouseEvent>
#include <QDebug>


#include <QMap>

int MAPX = 30;
int MAPY = 16;

char selectedTile = ' ';

TilePallette* palletteTile;
EditorScene* sceneMech;
QGraphicsScene* scenePallette;
QMap<short, QPixmap*> pixmaps;


EditorScene* sceneArt;
QGraphicsScene* sceneArtPallette;
QMap<short, QPixmap*> pixmapsArt;
Tile** tilesArt = 0;


EditorScene* sceneCollission;
QGraphicsScene* sceneCollissionPallette;
QMap<short, QPixmap*> pixmapsCollission;
Tile** tilesCollission = 0;


EditorView* view;
QGraphicsView* viewPallette;

bool leftbuttonHeld = false;




void SetSelectedTile(char selection)
{
    selectedTile = selection;
    if(palletteTile)
        palletteTile->SetSelectedBlock(selectedTile);
}



int sceneCounter = 0;
void SwitchScene()
{
    sceneCounter++;
    if(sceneCounter > 2)
        sceneCounter = 0;

    switch(sceneCounter)
    {
    case 0:
        view->setScene(sceneMech);
        viewPallette->setScene(scenePallette);
        SetSelectedTile(pixmaps.begin().key());
        view->setWindowTitle("Mechanical Layout");
        scenePallette->addItem(palletteTile);
        break;
    case 1:
        view->setScene(sceneArt);
        viewPallette->setScene(sceneArtPallette);
        SetSelectedTile(pixmapsArt.begin().key());
        view->setWindowTitle("Art Layout");
        sceneArtPallette->addItem(palletteTile);
        break;
    case 2:
        view->setScene(sceneCollission);
        viewPallette->setScene(sceneCollissionPallette);
        SetSelectedTile(pixmapsCollission.begin().key());
        view->setWindowTitle("Collission Layout");
        sceneCollissionPallette->addItem(palletteTile);
        break;
    default:
        break;
    }
}

void SwitchSceneTo(int s)
{
    while(sceneCounter != s)
        SwitchScene();
}

QGraphicsScene* getCurrentScene()
{
    switch (sceneCounter)
    {
    case 0:
        return sceneMech;
        break;
    case 1:
        return sceneArt;
        break;
    case 2:
        return sceneCollission;
        break;
    default:
        return 0;
        break;
    }
    return 0;
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
void LoadPixmapsGeneric(QString filename, QMap<short, QPixmap*>& map)
{
    QPixmap* p;
    QPixmap* p2;
    QFile file(filename);
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream in(&file);

    qDebug() << filename << " open = " << file.isOpen();

    QList<QString> list;
    while(!in.atEnd())
    {
        QString line = file.readLine();
        line.chop(1);
        list.push_back(line);
    }

    QString path = list.front();
    list.pop_front();

    while(!list.empty())
    {
        QString line = list.front();
        QChar key = line[line.size()-1];
        short key2 = key.toLatin1();
        line.chop(4);

        p = new QPixmap(path+line);
        p2 = new QPixmap(p->scaledToWidth(100));
        delete p;
        map[key2] = p2;

        list.pop_front();
    }
}

void LoadPixmapsArt(QString filename, QMap<short, QPixmap*>& map)
{
    QPixmap* p;
    QPixmap* p2;
    QFile file(filename);
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    QTextStream in(&file);

    qDebug() << filename << " open = " << file.isOpen();

    QList<QString> list;
    while(!in.atEnd())
    {
        QString line = file.readLine();
        list.push_back(line);
    }

    QString path = list.front();
    path.chop(1);
    list.pop_front();

    short key = 0;
    while(!list.empty())
    {
        QString line = list.front();
        line.chop(1); //chop endline

        p = new QPixmap(path+line);
        p2 = new QPixmap(p->scaledToWidth(100));
        delete p;
        map[key] = p2;
        key++;

        list.pop_front();
    }
}

void LoadPixmaps()
{
    LoadPixmapsGeneric("BlockList.txt", pixmaps);
    sceneCounter = 1;
    LoadPixmapsArt("ArtList.txt", pixmapsArt);
    sceneCounter =2;
    LoadPixmapsGeneric("CollissionList.txt", pixmapsCollission);
    sceneCounter = 0;
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
        t->setPos(x*100, y*100);

        x++;
    }

    palletteTile->SetSelectedBlock(map.begin().key());
    palletteTile->setPos(200, 000);
    s->addItem(palletteTile);
}

void LoadPallette()
{
    sceneCounter = 1;
    LoadPalletteGeneric(sceneArtPallette, pixmapsArt);
    sceneCounter = 2;
    LoadPalletteGeneric(sceneCollissionPallette, pixmapsCollission);
    sceneCounter =0;
    LoadPalletteGeneric(scenePallette, pixmaps);
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


void SaveLevel(QString filename)
{
    SaveGeneric(filename + ".txt", sceneMech);
    if(sceneCollission)
        SaveGeneric(filename + "Collission.txt", sceneCollission);
    if(sceneArt)
        SaveArtFile(filename + "Art.txt", sceneArt);
}

void SwitchMap(int x, int y)
{
    MAPX = x;
    MAPY = y;


    sceneMech->setSceneRect(0,0,MAPX*100,MAPY*100);

    LoadPixmaps();

    //scene->CreateTiles();
    sceneMech->CreateLevel(x, y);
    sceneArt->CreateLevel(x, y);
    sceneCollission->CreateLevel(x, y);

    view->scale((float)MAPY/(float)MAPX,(float)MAPY/(float)MAPX);
}






int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    view = new EditorView();
    sceneMech = new EditorScene();
    sceneArt = new EditorScene();
    sceneCollission = new EditorScene();

    view->setScene(sceneMech);
    view->showMaximized();



    SwitchMap(MAPX, MAPY);



    palletteTile = new TilePallette();

    SetSelectedTile('x');




    viewPallette = new EditorViewBasic;
    scenePallette = new QGraphicsScene;
    sceneArtPallette = new QGraphicsScene;
    sceneCollissionPallette = new QGraphicsScene;

    viewPallette->setScene(scenePallette);
    viewPallette->showNormal();
    viewPallette->resize(430, 600);

    scenePallette->setSceneRect(0,0,4*100,10*100);
    sceneArtPallette->setSceneRect(0,0,4*100,10*100);
    sceneCollissionPallette->setSceneRect(0,0,4*100,10*100);

    LoadPallette();

    viewPallette->show();

    return a.exec();
}


Tile::Tile()
{
    SetSelectedBlock(' ');

    this->setOpacity(100);

    setAcceptHoverEvents(true);
}

Tile::~Tile()
{
}

void Tile::mousePressEvent ( QGraphicsSceneMouseEvent * e )
{
    qDebug() << "setting tile to selectedBlock: " << selectedBlock;

    if(e->buttons() == Qt::RightButton)
        SetSelectedBlock(' ');
    else if (e->buttons() == Qt::LeftButton)
        SetSelectedBlock(selectedTile);
    else if (e->buttons() == Qt::MiddleButton)
        LoadLevel("save");
    else
        SaveLevel("save");
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
            m_tiles[y][x].setPos(x*100, y*100);
        }

    for (int x = 0; x <= m_mapx*100; x+=100)
    {
        addLine(x, 0, x, m_mapy*100);
    }
    for(int y = 0; y <= m_mapy*100; y+= 100)
    {
        addLine(0, y, m_mapx*100, y);
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

    QAction* newAct4 = new QAction(tr("&Mechanics"), this);
    newAct4->setStatusTip(tr("Switch to Mechanics screen"));
    connect(newAct4, SIGNAL(triggered()), this, SLOT(SwitchToMech()));

    QAction* newAct5 = new QAction(tr("&Art"), this);
    newAct5->setStatusTip(tr("Switch to Art screen"));
    connect(newAct5, SIGNAL(triggered()), this, SLOT(SwitchToArt()));

    QAction* newAct6 = new QAction(tr("&Collission"), this);
    newAct6->setStatusTip(tr("Switch to Collissions screen"));
    connect(newAct6, SIGNAL(triggered()), this, SLOT(SwitchToCollission()));


    QAction* newAct7 = new QAction(tr("&MapSize"), this);
    newAct7->setStatusTip(tr("Switch to Big Map(tm)"));
    connect(newAct7, SIGNAL(triggered()), this, SLOT(SwitchToBigMap()));


    bar->addAction(newAct1);
    bar->addAction(newAct2);
    bar->addAction(newAct3);
    bar->addAction(newAct4);
    bar->addAction(newAct5);
    bar->addAction(newAct6);
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
void EditorView::SwitchToMech()
{
    SwitchSceneTo(0);
}
void EditorView::SwitchToArt()
{
    SwitchSceneTo(1);
}
void EditorView::SwitchToCollission()
{
    SwitchSceneTo(2);
}

void EditorView::SwitchToBigMap()
{
    if(MAPX == 30)
        SwitchMap(60, 32);
    else
        SwitchMap(30, 16);


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
        v1->setScene(getCurrentScene());
        v1->showNormal();
        v1->scale((float)MAPY/(float)MAPX,(float)MAPY/(float)MAPX);
        e->accept();
    }

    if(e->key() == Qt::Key_Shift)
    {
        leftbuttonHeld = true;
        e->accept();
    }
}

void EditorViewBasic::keyReleaseEvent(QKeyEvent *e)
{
    if(e->key() == Qt::Key_Shift)
    {
        leftbuttonHeld = false;
        e->accept();
    }
}


