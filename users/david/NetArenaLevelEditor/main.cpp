#include "main.h"

#include "includeseditor.h"
#include "ed.h"

#include "gameobject.h"
#include "EditorView.h"


#define view view2 //hack

char selectedTile = ' ';



TilePallette* palletteTile;
QGraphicsScene* scenePallette;
QMap<short, QPixmap*> pixmaps;

QGraphicsScene* sceneArtPallette;
QMap<short, QPixmap*> pixmapsArt;
QMap<short, QString> artMap;
QMap<QString, short> templateToArtMap;

QGraphicsScene* sceneCollissionPallette;
QMap<short, QPixmap*> pixmapsCollission;

QMap<QString, QPixmap*> pixmapObjects;
QMap<QString, GameObject*> objectPallette;
QGraphicsScene* sceneObjectPallette;



QWidget* settingsWidget;

#include <QGridLayout>
QSplitter* hlayout;
QVBoxLayout* vlayout;
QGridLayout* grid;


QString ArtFolderPath;







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
	ed.ArtFolderPath = path;

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
		for (int x = 0; x < MAPX-4; x++)
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



void SaveLevel(QString filename)
{
    SaveGeneric(filename + ".txt", sceneMech);
    if(sceneCollission)
        SaveGeneric(filename + "Collission.txt", sceneCollission);

    if(sceneArt)
        SaveArtFile(filename + "Art.txt", sceneArt);
    //TODO: save templates
    //TODO: save art index file
    //TODO: save art level file

	SaveObjects(filename + "Objects.txt");
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
                sceneCounter = SCENEMECH;
                sceneMech->m_tiles[tiley+tt.y][tilex+tt.x].SetSelectedBlock(tt.blockMech);
                sceneCounter = SCENECOLL;
                sceneCollission->m_tiles[tiley+tt.y][tilex+tt.x].SetSelectedBlock(tt.blockColl);
                sceneCounter = SCENEART;
                sceneArt->m_tiles[tiley+tt.y][tilex+tt.x].SetSelectedBlock(tt.GetArtKey());
            }
            sceneCounter = temp;

			e->accept();

			break;
		}
		default:
			e->ignore();
			break;
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

	connect(m_listView, SIGNAL(clicked(QModelIndex)), this, SLOT(folderListClicked(QModelIndex)));
	connect(m_listView, SIGNAL(objectNameChanged(QString)), this, SLOT(folderNameChanged(QString)));

}

void TemplateScene::Initialize()
{
	QDir dir(ed.ArtFolderPath + "templates");

	dir.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
	QStringList dirs = dir.entryList();
	dirs.count();

	foreach(QString d, dirs)
	{
		dir.cd(d);

		dir.setFilter(QDir::AllEntries);
		QStringList filters;
		filters << "*.tmpl";
		dir.setNameFilters(filters);

		QStringList templates = dir.entryList();

		//if(templates.count())
		{
			AddFolder(d);
            SetCurrentFolder(d);
		}

		foreach(QString filename, templates)
		{
			QFile file(dir.path() + filename);
			file.open(QIODevice::ReadOnly | QIODevice::Text);

			EditorTemplate* t = new EditorTemplate();
			t->LoadTemplate(dir.path() + filename);

			m_currentFolder->AddTemplate(t);
		}

		dir.cdUp();//back down
	}


			//bool QDir::mkdir(const QString & dirName) const
			//bool QDir::rename(const QString & oldName, const QString & newName)
			//bool QDir::removeRecursively()
}

TemplateScene::~TemplateScene()
{
}

void TemplateScene::folderListClicked(const QModelIndex &index)
{
	QString name = (m_model->data(index, Qt::DisplayRole)).toString();

	m_currentFolder = m_folderMap[name];

	m_currentFolder->SetCurrentTemplate();
}

void TemplateScene::folderNameChanged(const QString& name)
{
	QString tname = m_currentFolder->m_folderName;
	m_currentFolder->SetFolderName(name);

	m_folderMap.remove(tname);
	m_folderMap[name] = m_currentFolder;


	//save new definition?
}

void TemplateScene::AddFolder(QString foldername)
{
	TemplateFolder* f = new TemplateFolder;
	f->SetFolderName(foldername);

	m_folderMap[foldername] = f;

	UpdateList();
}

void TemplateScene::RemoveFolder(QString foldername)
{
	if(m_folderMap.contains(foldername))
	{

		m_folderMap.remove(foldername);

		UpdateList();
	}
	else
		qDebug() << "Couldn't find folder: " << foldername;
}


void TemplateScene::AddTemplate(EditorTemplate* t)
{
	m_currentFolder->AddTemplate(t);

	UpdateList();
}

void TemplateScene::UpdateList()
{
	m_nameList.clear();

	foreach(QString name, m_folderMap.keys())
	{
		m_nameList << name;
	}

	m_model->setStringList(m_nameList);

	m_listView->setModel(m_model);
}

void TemplateScene::SetCurrentFolder(QString name)
{
	if(m_folderMap.contains(name))
	{
		m_currentFolder = m_folderMap[name];
		m_currentFolder->SetCurrentTemplate(); //select top template of this folder.
	}
}

void TemplateScene::SetCurrentTemplate(QString name)
{
	m_currentFolder->SetCurrentTemplate(name);
}


EditorTemplate* TemplateScene::GetCurrentTemplate()
{
	return m_currentFolder->GetCurrentTemplate();
}




TemplateFolder::TemplateFolder()
{
	m_scene = new QGraphicsScene();
}

TemplateFolder::~TemplateFolder()
{
	delete m_scene;
}

void TemplateFolder::SetFolderName(QString name)
{
	m_folderName = name;
}

void TemplateFolder::LoadIntoScene()
{
	m_scene->clear();

	int x = 0;
	int y = 1;
	foreach(EditorTemplate* t, m_templateMap.values())
	{
		if(x > 3)
		{
			x = 0;
			y++;
		}

		m_scene->addItem(t);
		t->setPos(x*THUMBSIZE+2, y*THUMBSIZE+2);

		x++;
	}
}

void TemplateFolder::SetCurrentTemplate()
{
	if(m_templateMap.count())
		m_currentTemplate = m_templateMap.first();
}

void TemplateFolder::SetCurrentTemplate(QString name)
{
	if(m_templateMap.contains(name))
		m_currentTemplate = m_templateMap[name];
	else
		SetCurrentTemplate();
}


void TemplateFolder::AddTemplate(EditorTemplate* t)
{
	m_templateMap[t->m_name] = t;

	LoadIntoScene();
}

EditorTemplate* TemplateFolder::GetCurrentTemplate()
{
	return m_currentTemplate;
}


QString GetPath()
{
	return ed.ArtFolderPath + templateScene->GetCurrentFolderName() + "\\";
}

QPixmap GetPixmapForTemplateTile()
{

}

EditorTemplate::TemplateTile* EditorTemplate::GetTemplateTile(int x, int y)
{
    foreach(TemplateTile tt, m_list)
        if(tt.x == x && tt.y == y)
            return &tt;

    return 0;

}

void EditorTemplate::RemoveTemplateTile(int x, int y)
{
    foreach(TemplateTile tt, m_list)
        if(tt.x == x && tt.y == y)
            m_list.removeOne(tt);
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
	int count; in >> count;

	QString tmpname = GetPath() + m_name;

	QPixmap p;
	if(p.load(tmpname + "_tile.png"))
		setPixmap(p);
	else
	{
		qDebug() << "Failed to load template: " + m_name;
		return;
	}

    for(int i = 0; i < count; i++)
    {
        TemplateTile tt;
        in >> tt.x;
        in >> tt.y;
        in >> tt.blockMech;
        in >> tt.blockColl;
        in >> tt.blockArt;

        m_list.append(tt);

        if(!templateToArtMap.contains(tt.blockArt))
        {
            QPixmap* part = new QPixmap;
            if(!part->load(tt.blockArt))
                qDebug() << "failed to load artfile: " << tt.blockArt << " for template: " << m_name;

            short key = (short)artMap.size();
            artMap[key] = tt.blockArt;
            templateToArtMap[tt.blockArt] = key;
            pixmapsArt[key] = part;
        }
    }

    file.close();
}

void EditorTemplate::SaveTemplate()
{

	QString path = ed.ArtFolderPath + templateScene->GetCurrentFolderName() + "\\";
	QFile templateFile(path + m_name + ".tmpl");

	templateFile.open(QIODevice::WriteOnly);

	QDataStream out(&templateFile);

    out << m_name;

    out << m_list.size();
    foreach(TemplateTile t, m_list)
    {
        out << t.x;
        out << t.y;
        out << t.blockMech;
        out << t.blockColl;
        out << t.blockArt;
    }

	templateFile.close();

	qDebug() << "Template" << m_name << " saved.";
}


int EditorTemplate::ImportFromImage(QString filename)
{
	QPixmap* img = new QPixmap(filename);

	QString name = filename.split('/').last(); //chops path
	name.chop(4); //chops .png

    m_name = name;

	QPixmap tile = img->scaled(THUMBSIZE, THUMBSIZE);

    QString path = GetPath();
	QFile tileFile(path + m_name +"_tile.png");

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

    for(int x = 0; x*BLOCKSIZE < img->size().width(); x++) //cut up the image into blocksized bits
	{
		for(int y = 0; y*BLOCKSIZE < img->size().height(); y++)
		{
			QPixmap *copy = new QPixmap (img->copy(x*BLOCKSIZE, y*BLOCKSIZE, BLOCKSIZE, BLOCKSIZE));

            QString tmpname = path + m_name + "_" + QString::number(x) + "_" + QString::number(y)  + ".png";
            QFile file(tmpname);
			file.open(QIODevice::WriteOnly);
			copy->save(&file, "PNG");
			file.close();

			TemplateTile tile;
			tile.x = x;
			tile.y = y;
			tile.blockMech = QString("x").toShort();
			tile.blockColl = QString("x").toShort();
            tile.blockArt = tmpname;

			m_list.append(tile);
		}
    }


	//template keeps track of own resource files in .tmpl
	//editor loads all template resource files and creates temp art list with mapping of short int to path/filename
	//on level save create level.artlist file with list of used path/filename resources and create level.art file with structure


    SaveTemplate();

    //view->SwitchToArt();

	return 1;
}

QPixmap* EditorTemplate::TemplateTile::GetPixmap()
{
    return pixmapsArt[GetArtKey()];
}

short EditorTemplate::TemplateTile::GetArtKey()
{
    return templateToArtMap[blockArt];
}

void EditorTemplate::mousePressEvent ( QGraphicsSceneMouseEvent * e )
{
	ed.GetTemplateScene()->SetCurrentTemplate(m_name);

	e->accept();
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

	view->setMinimumWidth(1200);
	viewPallette->setMinimumWidth(320);

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

	ed.CreateNewMap(MAPX, MAPY);

	SetSelectedTile('x');

	LoadPallette();

	AddAllToScene();
	sceneMech->AddGrid();

	CreateSettingsWidget();

	rightside.setLayout(vlayout);
	hlayout->addWidget(view);

	hlayout->addWidget(&rightside);

	vlayout->addWidget(viewPallette);
	vlayout->addWidget(settingsWidget);
	vlayout->addWidget(ed.objectPropWindow->m_w);
    vlayout->addWidget(templateScene->m_listView);
    templateScene->m_listView->hide();

    hlayout->showMaximized();

	templateScene->Initialize();

	return a.exec();
}


