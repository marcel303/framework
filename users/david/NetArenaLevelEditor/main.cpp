#include "main.h"

#include "includeseditor.h"
#include "ed.h"

#include "gameobject.h"


int MAPX = BASEX;
int MAPY = BASEY;

char selectedTile = ' ';

#define ed Ed::I()

#define sceneArt ed.GetSceneArt()
#define sceneMech ed.GetSceneMech()
#define sceneCollission ed.GetSceneCollission()

#define editorMode ed.GetEditorMode()




TilePallette* palletteTile;
QGraphicsScene* scenePallette;
QMap<short, QPixmap*> pixmaps;

QGraphicsScene* sceneArtPallette;
QMap<short, QPixmap*> pixmapsArt;

QGraphicsScene* sceneCollissionPallette;
QMap<short, QPixmap*> pixmapsCollission;

QMap<QString, QPixmap*> pixmapObjects;
QMap<QString, GameObject*> objectPallette;
QGraphicsScene* sceneObjectPallette;
QList<GameObject*> gameObjects;


EditorView* view;
QGraphicsView* viewPallette;


TemplateScene* templateScene;
Tile** m_templateTiles;



QWidget* settingsWidget;

#include <QGridLayout>
QSplitter* hlayout;
QVBoxLayout* vlayout;
QGridLayout* grid;

bool leftbuttonHeld = false;





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
			sceneMech->m_tiles[y][x].setOpacity(1);

            sceneCollission->m_tiles[y][x].setAcceptedMouseButtons(0);
            sceneCollission->m_tiles[y][x].setAcceptHoverEvents(false);
			sceneCollission->m_tiles[y][x].setOpacity(0);

            sceneArt->m_tiles[y][x].setAcceptedMouseButtons(0);
            sceneArt->m_tiles[y][x].setAcceptHoverEvents(false);
			sceneArt->m_tiles[y][x].setOpacity(0);

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
    case SCENEMECH:
        viewPallette->setScene(scenePallette);
        SetSelectedTile(pixmaps.begin().key());
        view->setWindowTitle("Mechanical Layout");
        viewPallette->setWindowTitle("Mechanical Layout");
        scenePallette->addItem(palletteTile);
        SetEditMechScene();
        break;
    case SCENEART:
        viewPallette->setScene(sceneArtPallette);
        SetSelectedTile(pixmapsArt.begin().key());
        view->setWindowTitle("Art Layout");
        viewPallette->setWindowTitle("Art Pallette");
        sceneArtPallette->addItem(palletteTile);
        SetEditArtScene();
        break;
    case SCENECOLL:
        viewPallette->setScene(sceneCollissionPallette);
        SetSelectedTile(pixmapsCollission.begin().key());
        view->setWindowTitle("Collission Layout");
        viewPallette->setWindowTitle("Collission Pallette");
        sceneCollissionPallette->addItem(palletteTile);
        SetEditCollissionScene();
        break;
	case SCENEOBJ:
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
	case SCENEMECH:
        return pixmaps;
        break;
	case SCENEART:
        return pixmapsArt;
        break;
	case SCENECOLL:
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

    file.close();

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
		if(p->isNull())
		{
			qDebug() << "couldn't load artfile: " << line;
			delete p;
			list.pop_front();
			key++;
			continue;
		}
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
		QPixmap* p = new QPixmap(Ed::I().ObjectPath + texture);
        pixmapObjects[texture] = p;
    }
    return pixmapObjects[texture];
}

void AddGameObject(GameObject* obj)
{
    gameObjects.push_back(obj);
    sceneMech->addItem(obj);
}


void LoadObjects(QString filename, bool pallette)
{
    QList<QString> list = GetLinesFromConfigFile(filename);

    if(!list.empty())
    {
		Ed::I().ObjectPath = list.front();
        list.pop_front();
    }

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
    GameObject* obj = 0;
    if(!map.isEmpty())
        obj = LoadGameObject(map);

    if(pallette && obj) //fill the object pallette
	{

		QPixmap* p = new QPixmap();
		*p = obj->pixmap().scaled(BLOCKSIZE, BLOCKSIZE);
		obj->setPixmap(*p);
        objectPallette[obj->type] = obj;
	}
	else if(obj)
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

void LoadArt(QString filename, EditorScene* s)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly | QIODevice::Text);

    QDataStream in(&file);

    int mx;
    in >> mx;
    int my;
    in >> my;

    short key;
    for(int y = 0; y < my; y++)
    {
        for (int x = 0; x < mx; x++)
        {
            in >> key;
            s->m_tiles[y][x].SetSelectedBlock(key);
        }
    }
    file.close();

}

void LoadLevel(QString filename)
{
    sceneCounter = 0;
    LoadGeneric(filename + ".txt", sceneMech);
    sceneCounter = 1;
    LoadArt(filename + "Art.txt", sceneArt);
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

    QTextStream out(&file);

	foreach(GameObject* obj, gameObjects)
	{
		QString a = obj->toText();
        out << a;
	}
	file.close();
}

void SaveTemplateToFile(QString filename, int tx, int ty)
{
    QFile file("templates.txt");
    file.open(QIODevice::WriteOnly | QIODevice::Append);

    QTextStream text(&file);

    {
        QFile templateFile(filename + ".tmpl");
        templateFile.open(QIODevice::WriteOnly);

        QDataStream out(&templateFile);

		out << filename;
        out << tx << ty;

        for(int y = 0; y < ty; y++)
            for (int x = 0; x < tx; x++)
            {
                out << (quint16 )(sceneMech->m_tiles[y][x].getBlock());
                out << (quint16 )(sceneArt->m_tiles[y][x].getBlock());
                out << (quint16 )(sceneCollission->m_tiles[y][x].getBlock());
            }

        templateFile.close();

        text << filename << ".tmpl" << endl;

        qDebug() << "Template" << filename << " saved.";
    }
    file.close();
}

void LoadTemplates(QString filename = "templates.txt")
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly);

    if(!file.isOpen())
    qDebug() << filename << " could not be opened. Templates not loaded.";

    QTextStream in(&file);

    while(!in.atEnd())
    {
        QString templateName = in.readLine();
        EditorTemplate* t = new EditorTemplate();
        t->LoadTemplate(templateName);

        templateScene->AddTemplate(t);
    }
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

void CreateNewMap(int x, int y)
{
    MAPX = x;
    MAPY = y;


	sceneMech->setSceneRect(-MAPX*2*BLOCKSIZE,-MAPY*2*BLOCKSIZE,MAPX*4*BLOCKSIZE,MAPY*4*BLOCKSIZE);

    LoadPixmaps();

    //scene->CreateTiles();
    sceneMech->CreateLevel(x, y);
    sceneArt->CreateLevel(x, y);
    sceneCollission->CreateLevel(x, y);

    gameObjects.clear();

    view->scale((float)MAPY/(float)MAPX,(float)MAPY/(float)MAPX);


    sceneMech->AddGrid();
}


QWidget* newMapWindow = 0;


void NewMapWindow::NewMap()
{
    if (newMapWindow)
    {
        newMapWindow->hide();


        CreateNewMap(x->value(), y->value());
    }
}

void NewMapWindow::CancelNewMap()
{
    if (newMapWindow)
        newMapWindow->hide();
}

NewMapWindow::NewMapWindow()
{
    x = new QSlider();
    y = new QSlider();

    x->setSingleStep(BASEX);
    y->setSingleStep(BASEY);

    x->setSliderPosition(BASEX);
    y->setSliderPosition(BASEY);

    x->setRange(BASEX, BASEX*10);
    y->setRange(BASEY, BASEY*10);

    ok = new QPushButton();
    cancel = new QPushButton();

    ok->setText("Create Map");
    cancel->setText("Cancel");

    connect(ok, SIGNAL(clicked()), this, SLOT(NewMap()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(CancelNewMap()));


    QVBoxLayout* b = new QVBoxLayout;
    b->addWidget(x);
    b->addWidget(y);
    b->addWidget(ok);
    b->addWidget(cancel);

    setLayout(b);
}

NewMapWindow::~NewMapWindow()
{
}

void CreateAndShowNewMapDialog()
{
    if(!newMapWindow)
        newMapWindow = new NewMapWindow();

    newMapWindow->show();
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
	return;
    for (int x = 0; x <= m_mapx*BLOCKSIZE; x+=BLOCKSIZE)
    {
        addLine(x, 0, x, m_mapy*BLOCKSIZE);
    }
    for(int y = 0; y <= m_mapy*BLOCKSIZE; y+= BLOCKSIZE)
    {
        addLine(0, y, m_mapx*BLOCKSIZE, y);
    }
}

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
			obj->Load(ed.objectPropWindow->GetCurrentGameObject()->toText());
			obj->SetPos(e->scenePos());

			AddGameObject(obj);

			ed.objectPropWindow->SetCurrentGameObject(obj);

			e->accept();
			break;
		}
		case EM_Template:
		{
            int tilex = (int)(e->scenePos().x()/float(BLOCKSIZE));
            int tiley = (int)(e->scenePos().y()/float(BLOCKSIZE));

            //Stamp template on level
            EditorTemplate* t = templateScene->GetCurrentTemplate();
            int temp = sceneCounter;
            foreach(EditorTemplate::TemplateTile tt, t->m_list)
            {
                SwitchSceneTo(SCENEMECH);
                sceneMech->m_tiles[tiley+tt.y][tilex+tt.x].SetSelectedBlock(tt.blockMech);
                SwitchSceneTo(SCENEART);
                sceneCollission->m_tiles[tiley+tt.y][tilex+tt.x].SetSelectedBlock(tt.blockColl);
                SwitchSceneTo(SCENECOLL);
                sceneArt->m_tiles[tiley+tt.y][tilex+tt.x].SetSelectedBlock(tt.blockArt);
            }
            SwitchSceneTo(temp);

			e->accept();

			break;
		}
		default:
			e->ignore();
			break;
	}
}


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

    QAction* newAct5 = new QAction(tr("&SaveTemplate"), this);
    newAct5->setStatusTip(tr("Templatus Savus"));
    connect(newAct5, SIGNAL(triggered()), this, SLOT(SaveTemplate()));

	QAction* newAct6 = new QAction(tr("&ImportTemplate"), this);
	newAct6->setStatusTip(tr("Import Template Image"));
	connect(newAct6, SIGNAL(triggered()), this, SLOT(ImportTemplate()));

    bar->addAction(newAct1);
    bar->addAction(newAct2);
    bar->addAction(newAct3);
    bar->addAction(newAct4);
    bar->addAction(newAct5);
	bar->addAction(newAct6);

    bar->showNormal();
}

EditorView::~EditorView()
{
}

void EditorView::Save()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save to file"));
    SaveLevel(fileName);
}

#include <QInputDialog>
void EditorView::SaveTemplate()
{
    int x = QInputDialog::getInt(this, "Template X size", "X:");
    int y = QInputDialog::getInt(this, "Template Y size", "Y:");
	//QString fileName = QFileDialog::getSaveFileName(this, tr("Save to file"));

	templateScene->GetCurrentTemplate()->SaveTemplate(x, y);
}

void EditorView::Load()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open file"));
    fileName.chop(4);
    LoadLevel(fileName);
}
void EditorView::New()
{
    CreateAndShowNewMapDialog();
}
void EditorView::SwitchToMech()
{
		SwitchSceneTo(SCENEMECH);
		sliderOpacMech->setValue(100);
		sliderOpacArt->setValue(0);
		sliderOpacColl->setValue(0);
		sliderOpacObject->setValue(0);
}
void EditorView::SwitchToArt()
{
		SwitchSceneTo(SCENEART);
		sliderOpacMech->setValue(20);
		sliderOpacArt->setValue(80);
		sliderOpacColl->setValue(0);
		sliderOpacObject->setValue(0);
}
void EditorView::SwitchToCollission()
{
		SwitchSceneTo(SCENECOLL);
		sliderOpacMech->setValue(0);
		sliderOpacArt->setValue(20);
		sliderOpacColl->setValue(80);
		sliderOpacObject->setValue(0);
}

void EditorView::SwitchToObject()
{
		SwitchSceneTo(3);
		sliderOpacMech->setValue(20);
		sliderOpacArt->setValue(0);
		sliderOpacColl->setValue(0);
		sliderOpacObject->setValue(80);
}

void EditorView::SwitchToBigMap()
{
    if(MAPX == BASEX)
        CreateNewMap(2*BASEX, 2*BASEY);
    else
        CreateNewMap(BASEX, BASEY);
}

void EditorView::SwitchToTemplateMode()
{
	QLayoutItem* l = 0;
	if(editorMode != EM_Template)
    {
		editorMode = EM_Template;

		viewPallette->hide();
		templateScene->m_listView->show();

        //template controls
    }
	else
    {
		editorMode = EM_Level;


		templateScene->m_listView->hide();
		viewPallette->show();

        if(!sceneCounter)
            SwitchScene();
        SwitchSceneTo(0);
    }

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

void EditorView::ImportTemplate()
{
	QString filename = QFileDialog::getOpenFileName(this, tr("Open Image for Template"));

	EditorTemplate* t = new EditorTemplate();
	if(t->ImportFromImage(filename))
	{
		filename.chop(4);
		QString f = filename.split('/').last();
		t->LoadTemplate(f + ".tmpl");
		templateScene->AddTemplate(t);
	}
	else
		delete t;
}


#ifndef QT_NO_WHEELEVENT
void EditorViewBasic::wheelEvent(QWheelEvent* e)
{
    {
        if (e->delta() > 0)
			zoomIn(ZOOMSPEED);
        else
			zoomOut(ZOOMSPEED);
        e->accept();
    }
}
#endif

void EditorViewBasic::zoomIn(int level)
{
    zoomLevel += level;

    UpdateMatrix();
}

void EditorViewBasic::zoomOut(int level)
{
    zoomLevel -= level;

    if(zoomLevel < 0)
        zoomLevel = 0;

    UpdateMatrix();
}


void EditorViewBasic::UpdateMatrix()
{
    qreal scale = qPow(qreal(2), (zoomLevel - 250) / qreal(50));

    QMatrix matrix;
    matrix.scale(scale, scale);

    setMatrix(matrix);
}


EditorViewBasic::EditorViewBasic() : QGraphicsView()
{
    zoomLevel = 220;
}

EditorViewBasic::~EditorViewBasic()
{

}

void EditorViewBasic::keyPressEvent(QKeyEvent *e)
{
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


void CreateSettingsWidget()
{
    settingsWidget = new QWidget();

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
			view, SLOT(SwitchToMech()));
    grid->addWidget(cb, 1, 1);
    bgroup->addButton(cb);

    cb = new QCheckBox();
    QObject::connect(cb, SIGNAL(stateChanged(int)),
			view, SLOT(SwitchToArt()));
    grid->addWidget(cb, 2, 1);
    bgroup->addButton(cb);

    cb = new QCheckBox();
    QObject::connect(cb, SIGNAL(stateChanged(int)),
			view, SLOT(SwitchToCollission()));
    grid->addWidget(cb, 3, 1);
    bgroup->addButton(cb);

	cb = new QCheckBox();
	QObject::connect(cb, SIGNAL(stateChanged(int)),
			view, SLOT(SwitchToObject()));
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

    settingsWidget->setLayout(grid);
    //w->show();
}




TemplateScene::TemplateScene()
{
	m_listView = new QListView();
	m_model = new QStringListModel();

	UpdateList();

	connect(m_listView, SIGNAL(clicked(QModelIndex)), this, SLOT(templateListClicked(QModelIndex)));
	connect(m_listView, SIGNAL(objectNameChanged(QString)), this, SLOT(templateNameChanged(QString)));

}

TemplateScene::~TemplateScene()
{
}

void TemplateScene::templateListClicked(const QModelIndex &index)
{
	QString name = (m_model->data(index, Qt::DisplayRole)).toString();

	m_currentTemplate = m_templateMap[name];
}

void TemplateScene::templateNameChanged(const QString& name)
{
	QString tname = m_currentTemplate->m_name;
	m_currentTemplate->m_name = name;

	m_templateMap.remove(tname);
	m_templateMap[name] = m_currentTemplate;


	//save new definition?
}


void TemplateScene::AddTemplate(EditorTemplate* t)
{
	m_templateMap[t->m_name] = t;

	UpdateList();
}

void TemplateScene::UpdateList()
{
	m_nameList.clear();

	foreach(QString name, m_templateMap.keys())
	{
		m_nameList << name;
	}

	m_model->setStringList(m_nameList);

	m_listView->setModel(m_model);
}

EditorTemplate* TemplateScene::GetCurrentTemplate()
{
	//m_listView.
	return m_currentTemplate;
}

EditorTemplate::EditorTemplate()
{
}

EditorTemplate::~EditorTemplate()
{
}

void EditorTemplate::LoadTemplate(QString filename)
{
    QFile file(filename);
    file.open(QIODevice::ReadOnly);

    QDataStream in(&file);

	in >> m_name;
    int x; in >> x;
    int y; in >> y;

    for(int j = 0; j < y; j++)
        for(int i = 0; i < x; i++)
        {
            TemplateTile tt;
            in >> tt.blockMech;
            in >> tt.blockArt;
            in >> tt.blockColl;

            tt.x = i;
            tt.y = j;

            m_list.append(tt);
        }

    file.close();
}

void EditorTemplate::SaveTemplate(int tx, int ty)
{
	QFile file("templates.txt");
	file.open(QIODevice::WriteOnly | QIODevice::Append);

	QTextStream text(&file);
	{
		QFile templateFile(m_name + ".tmpl");
		templateFile.open(QIODevice::WriteOnly);

		QDataStream out(&templateFile);

		out << m_name;
		out << tx << ty;

		for(int y = 0; y < ty; y++)
			for (int x = 0; x < tx; x++)
			{
				out << (quint16 )(sceneMech->m_tiles[y][x].getBlock());
				out << (quint16 )(sceneArt->m_tiles[y][x].getBlock());
				out << (quint16 )(sceneCollission->m_tiles[y][x].getBlock());
			}

		templateFile.close();

		text << m_name << ".tmpl" << endl;

		qDebug() << "Template" << m_name << " saved.";
	}
	file.close();
}

int EditorTemplate::ImportFromImage(QString filename)
{
	editorMode = EM_Template;

	QPixmap* img = new QPixmap(filename);

	QString name = filename.split('/').last(); //chops path
	name.chop(4); //chops .png

	QPixmap tile = img->scaled(BLOCKSIZE, BLOCKSIZE);
	QFile tileFile(name+"_tile.png");
	tileFile.open(QIODevice::WriteOnly);
	tile.save(&tileFile, "PNG");
	tileFile.close();

	if(img->size().width() < 1)
		return 0;

	if(img->size().height()/BLOCKSIZE > MAPY || img->size().width()/BLOCKSIZE > MAPX )
	{
		qDebug() << "Image too large for template";
		return 0;
	}

	QFile artList("ArtList.txt");
	artList.open(QIODevice::WriteOnly | QIODevice::Append);
	QTextStream in(&artList);
	for(int x = 0; x*BLOCKSIZE < img->size().width(); x++) //cut up the image into blocksized bits
	{
		for(int y = 0; y*BLOCKSIZE < img->size().height(); y++)
		{
			QPixmap *copy = new QPixmap (img->copy(x*BLOCKSIZE, y*BLOCKSIZE, BLOCKSIZE, BLOCKSIZE));

			QString tmpname = name + "_" + QString::number(x) + "_" + QString::number(y)  + ".png";
			QFile file(tmpname);
			file.open(QIODevice::WriteOnly);
			copy->save(&file, "PNG");
			file.close();

			in << tmpname << "\r\n";

			pixmapsArt[pixmapsArt.size()] = copy;

			sceneCounter = SCENEART;
			sceneArt->m_tiles[y][x].SetSelectedBlock(pixmapsArt.size()-1);
			sceneCounter = SCENEMECH;
			sceneMech->m_tiles[y][x].SetSelectedBlock(QString("x").toShort());
		}
	}
	artList.close();

	view->SwitchToArt();

	return 1;
}

void EditorView :: mousePressEvent(QMouseEvent * e)
{
   if (e->button() == Qt::MidButton)
   {
	  QMouseEvent fake(e->type(), e->pos(), Qt::LeftButton, Qt::LeftButton, e->modifiers());
	  this->setInteractive(false);
	  this->setDragMode(QGraphicsView::ScrollHandDrag);
	  QGraphicsView::mousePressEvent(&fake);
   }
   else QGraphicsView::mousePressEvent(e);
}

void EditorView :: mouseReleaseEvent(QMouseEvent * e)
{
   if (e->button() == Qt::MidButton)
   {
	  QMouseEvent fake(e->type(), e->pos(), Qt::LeftButton, Qt::LeftButton, e->modifiers());
	  this->setInteractive(true);
	  this->setDragMode(QGraphicsView::NoDrag);
	  QGraphicsView::mouseReleaseEvent(&fake);
   }
   else QGraphicsView::mouseReleaseEvent(e);
}


void InitializeScenesAndViews()
{
	editorMode = EM_Level;

	sceneMech = new EditorScene();
	sceneArt = new EditorScene();
	sceneCollission = new EditorScene();

	editorMode = EM_Template;

	sceneMech = new EditorScene();
	sceneArt = new EditorScene();
	sceneCollission = new EditorScene();

	templateScene = new TemplateScene();

	editorMode = EM_Level;

	view = new EditorView();

	viewPallette = new EditorViewBasic;
	viewPallette->setDragMode(QGraphicsView::ScrollHandDrag);
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

	palletteTile = new TilePallette();

	ed.objectPropWindow = new ObjectPropertyWindow();
	ed.objectPropWindow->CreateObjectPropertyWindow();
}


int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	grid = new QGridLayout();
	vlayout = new QVBoxLayout();
	hlayout = new QSplitter();

	QWidget rightside;

	editorMode = EM_Level;



	InitializeScenesAndViews();

	if(0)
	{
		view->setViewport(new QGLWidget());
		view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
	}

	view->setRenderHints(QPainter::Antialiasing);
	view->setScene(sceneMech);

	CreateNewMap(MAPX, MAPY);

	SetSelectedTile('x');

	LoadPallette();

	AddAllToScene();
	sceneMech->AddGrid();

	CreateSettingsWidget();

	rightside.setLayout(vlayout);
	view->setMinimumWidth(1200);
	hlayout->addWidget(view);

	hlayout->addWidget(&rightside);

	vlayout->addWidget(viewPallette);
	vlayout->addWidget(templateScene->m_listView);
	templateScene->m_listView->hide();
	vlayout->addWidget(settingsWidget);
	vlayout->addWidget(ed.objectPropWindow->m_w);

	hlayout->showMaximized();


	LoadTemplates();

	return a.exec();
}


