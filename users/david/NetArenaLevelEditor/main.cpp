#include "main.h"

#include "includeseditor.h"
#include "ed.h"

#include "gameobject.h"
#include "EditorView.h"

#include "SettingsWidget.h"


#define view view2 //hack

short selectedTile = ' ';



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

#include <QGridLayout>
QSplitter* hlayout;
QVBoxLayout* vlayout;
QGridLayout* grid;

QString ArtFolderPath;

bool placeTemplate = false;



void SetSelectedTile(char selection)
{
	placeTemplate = false;

    selectedTile = selection;
    if(palletteTile)
        palletteTile->SetSelectedBlock(selectedTile);
}

void AddAllToScene()
{
	ed.EditTemplates();
	for(int i = 0; i < 2; i++) //once for templates, once for level
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
		ed.EditLevels();
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

	if(sceneCounter > 4)
        sceneCounter = 0;

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
		view->setWindowTitle("Editing Level Objects");
		viewPallette->setWindowTitle("GameObjects");
		viewPallette->setScene(sceneObjectPallette);
		SetEditObjects();
		break;
	case SCENETEMPLATE:
		view->setWindowTitle("Editing with Templates!");
		viewPallette->setWindowTitle("Templates");
		//viewPallette->setScene(sceneObjectPallette);
		SetEditMechScene();
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

        p = new QPixmap;
        if(p->load(path+line))
        {

            p2 = new QPixmap(p->scaledToWidth(BLOCKSIZE));

            map[key2] = p2;
        }
        delete p;

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
		artMap[key] = line;
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
	sceneCounter = SCENEMECH;
    LoadPixmapsGeneric("BlockList.txt", pixmaps);
	sceneCounter = SCENEART;
	LoadPixmapsArt("ArtList.txt", pixmapsArt);
	sceneCounter = SCENECOLL;
    LoadPixmapsGeneric("CollissionList.txt", pixmapsCollission);
	sceneCounter = SCENEOBJ;
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
		if(x > PALLETTEROWSIZE)
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
		if(x > PALLETTEROWSIZE)
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
			short key = in.read(1)[0].toLatin1();
            s->m_tiles[y][x].SetSelectedBlock(key);
        }
        if(y < (MAPY -1))
				in.read(1);
    }
    file.close();
}

void LoadArt(QString filename, EditorScene* s)
{
	QFile file(filename+".txt");
	QFile fileIndex(filename+"Index.txt");

	fileIndex.open(QIODevice::ReadOnly | QIODevice::Text);


	QTextStream inIndex(&fileIndex);

	QMap<short, short> tempIndexToArtIndex;
	filename.chop(filename.split('/').last().size()); //get path

	while(!inIndex.atEnd())
	{
		QString artName = inIndex.readLine();
		QPixmap* p = new QPixmap();

		if(p->load(filename+artName))
		{
			tempIndexToArtIndex[tempIndexToArtIndex.size()] = pixmapsArt.size();
			pixmapsArt[pixmapsArt.size()] = p;
		}
	}

    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);

    int mx;
    in >> mx;
    int my;
    in >> my;

    int key;
    for(int y = 0; y < my; y++)
    {
        for (int x = 0; x < mx; x++)
        {
			in >> key;
            s->m_tiles[y][x].SetSelectedBlock(tempIndexToArtIndex[(short)key]);
        }
    }
    file.close();
}

void LoadLevel(QString filename)
{
    sceneCounter = 0;
    LoadGeneric(filename + ".txt", sceneMech);
    sceneCounter = 1;
	LoadArt(filename + "Art", sceneArt);
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
	QMap<short, short> artTranslation;
	QMap<short, QPixmap> art2;


	QFile fileArtIndex(filename+"Index.txt");

    fileArtIndex.open(QIODevice::WriteOnly | QIODevice::Truncate);
	QTextStream artLines(&fileArtIndex);

	for(int y = 0; y < MAPY; y++)
		for (int x = 0; x < MAPX; x++)
		{
			if(!artTranslation.contains(s->m_tiles[y][x].getBlock()))
			{
				artTranslation[s->m_tiles[y][x].getBlock()] = artTranslation.size()-1;
				s->m_tiles[y][x].pixmap().save(filename + "_" + QString::number(artTranslation.size()-1) + ".png", "PNG");

				artLines << (filename.split('/').last() + "_" + QString::number(artTranslation.size()-1) + ".png") << endl;
			}
		}
	fileArtIndex.close();

	QFile fileArt(filename+".txt");
    fileArt.open(QIODevice::WriteOnly | QIODevice::Truncate);
    QDataStream in(&fileArt);
    in.setByteOrder(QDataStream::LittleEndian);

    in << MAPX << MAPY;
	for(int y = 0; y < MAPY; y++)
		for (int x = 0; x < MAPX; x++)
        {
           in << (int)(artTranslation[s->m_tiles[y][x].getBlock()]);
        }

	qDebug() << "Done saving level.";

	fileArt.close();

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
	QDir dir;
	dir.mkpath(filename);

	QString name = filename + '/' + filename.split('/').last();

	SaveGeneric(name + ".txt", sceneMech);
    if(sceneCollission)
		SaveGeneric(name + "Collission.txt", sceneCollission);
    if(sceneArt)
		SaveArtFile(name + "Art", sceneArt);

	SaveObjects(name + "Objects.txt");
}


Tile::Tile()
{
	selectedBlock = ' ';

    this->setOpacity(1.0);

    setAcceptHoverEvents(true);
    setShapeMode( QGraphicsPixmapItem::BoundingRectShape );
}

Tile::~Tile()
{
}

void Tile::mousePressEvent ( QGraphicsSceneMouseEvent * e )
{
	sceneMech->CustomMouseEvent(e, this);
}

void Tile::mouseReleaseEvent ( QGraphicsSceneMouseEvent * e )
{
    e->accept();
}

void Tile::SetSelectedBlock(short block)
{
    if(getCurrentPixmap().contains(block))
	{
        setPixmap(*(getCurrentPixmap()[block]));

		selectedBlock = block;
	}
	else
		qDebug() << "trying to set tile to nonexistant block";
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
			m_tiles[y][x].SetSelectedBlock((sceneCounter == SCENEART)? 0 : ' ');
            addItem(&m_tiles[y][x]);
            m_tiles[y][x].setPos(x*BLOCKSIZE, y*BLOCKSIZE);
        }
}

void EditorScene::ResetLevel()
{
	for(int y = 0; y < m_mapy; y++)
		for (int x = 0; x < m_mapx; x++)
		{
			m_tiles[y][x].SetSelectedBlock(' ');
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

void EditorScene::CustomMouseEvent ( QGraphicsSceneMouseEvent * e, Tile* tile )
{
	if(sceneCounter != SCENEOBJ)
	{
		int tilex = (int)(e->scenePos().x()/float(BLOCKSIZE));
		int tiley = (int)(e->scenePos().y()/float(BLOCKSIZE));

		if(placeTemplate)
		{
			//Stamp template on level
			int temp = sceneCounter;
			EditorTemplate* ct = templateScene->GetCurrentTemplate();
			if(ct)
				foreach(EditorTemplate::TemplateTile* tt, ct->m_list)
				{
					if(tilex+tt->x < MAPX && tiley+tt->y < MAPY)
					{
						sceneCounter = SCENEMECH;
						sceneMech->m_tiles			[tiley+tt->y][tilex+tt->x].SetSelectedBlock(tt->blockMech);
						sceneCounter = SCENECOLL;
						sceneCollission->m_tiles	[tiley+tt->y][tilex+tt->x].SetSelectedBlock(tt->blockColl);
						sceneCounter = SCENEART;
						sceneArt->m_tiles			[tiley+tt->y][tilex+tt->x].SetSelectedBlock(tt->GetArtKey());

						if(editorMode == EM_Template)
						{
							EditorTemplate::TemplateTile* tt2 = ct->GetOrAddTemplateTile(tilex+tt->x, tiley+tt->y);
							tt2->blockMech = tt->blockMech;
							tt2->blockColl = tt->blockColl;
							tt2->blockArt = tt->blockArt;
						}
					}
				}
			sceneCounter = temp;

			e->accept();
		}
		else
		{
			if(e->buttons() == Qt::RightButton)
			{
				if(sceneCounter == SCENEART)
					tile->SetSelectedBlock(artMap.keys().first());
				else
					tile->SetSelectedBlock(' ');

				if(editorMode == EM_Template)
				{
					if(sceneCounter == SCENEMECH)
						templateScene->GetCurrentTemplate()->GetOrAddTemplateTile(tilex, tiley)->blockMech = ' ';
					if(sceneCounter == SCENECOLL)
						templateScene->GetCurrentTemplate()->GetOrAddTemplateTile(tilex, tiley)->blockColl = ' ';
					if(sceneCounter == SCENEART)
					{
						templateScene->GetCurrentTemplate()->GetOrAddTemplateTile(tilex, tiley)->blockArt = artMap.first();
						tile->SetSelectedBlock(artMap.keys().first());
					}
				}
			}
			else if (e->buttons() == Qt::LeftButton)
			{
				tile->SetSelectedBlock(selectedTile);
				if(editorMode == EM_Template)
				{
					if(sceneCounter == SCENEMECH)
						templateScene->GetCurrentTemplate()->GetOrAddTemplateTile(tilex, tiley)->blockMech = selectedTile;
					if(sceneCounter == SCENECOLL)
						templateScene->GetCurrentTemplate()->GetOrAddTemplateTile(tilex, tiley)->blockColl = selectedTile;
					if(sceneCounter == SCENEART)
					{
						templateScene->GetCurrentTemplate()->GetOrAddTemplateTile(tilex, tiley)->blockArt = artMap[selectedTile];
						tile->SetSelectedBlock(artMap.keys().first());
					}
				}
			}
		}
	}
	else //sceneCounter == SCENEOBJECT
	{
		GameObject* obj = new GameObject();
		obj->palletteTile = false;
		obj->Load(ed.objectPropWindow->GetCurrentGameObject()->toText());
		obj->SetPos(e->scenePos());

		AddGameObject(obj);

		ed.objectPropWindow->SetCurrentGameObject(obj);

		e->accept();
	}
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
			QFile file(dir.path() + "//" + dir.dirName() + "//" + filename);
			file.open(QIODevice::ReadOnly | QIODevice::Text);

			EditorTemplate* t = new EditorTemplate();
            t->LoadTemplate(dir.path() + "/" + filename);

			m_currentFolder->AddTemplate(t);
		}

		dir.cdUp();//back down
	}
}

TemplateScene::~TemplateScene()
{
}

void TemplateScene::folderListClicked(const QModelIndex &index)
{
	QString name = (m_model->data(index, Qt::DisplayRole)).toString();

    SetCurrentFolder(name);
}

void TemplateScene::folderNameChanged(const QString& name)
{
	QString tname = m_currentFolder->m_folderName;
	m_currentFolder->SetFolderName(name);

	m_folderMap.remove(tname);
	m_folderMap[name] = m_currentFolder;
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



        viewPallette->setScene(m_currentFolder->m_scene);
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

	m_currentTemplate = 0;
}

TemplateFolder::~TemplateFolder()
{
	delete m_scene;
}

void TemplateFolder::SetFolderName(QString name)
{
	m_folderName = name;
}

void TemplateFolder::LoadTileIntoScene()
{
	foreach(QGraphicsItem* i, m_scene->items())
		m_scene->removeItem(i);

	int x = 0;
	int y = 1;
	foreach(EditorTemplate* t, m_templateMap.values())
	{
		if(x > PALLETTEROWSIZE)
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
	{
		placeTemplate = true;
		m_currentTemplate = m_templateMap.first();
	}
	else
	{
		m_currentTemplate = 0;
		placeTemplate = false;
	}


}

void TemplateFolder::SetCurrentTemplate(QString name)
{
	if(m_templateMap.contains(name))
	{
		m_currentTemplate = m_templateMap[name];
        m_currentTemplate->LoadTemplateIntoScene();
		placeTemplate = true;
	}
	else
		SetCurrentTemplate();
}


void TemplateFolder::AddTemplate(EditorTemplate* t)
{
	m_templateMap[t->m_name] = t;

    LoadTileIntoScene();
}

EditorTemplate* TemplateFolder::GetCurrentTemplate()
{
	return m_currentTemplate;
}


QString GetPath()
{
    return ed.ArtFolderPath + "templates\\" + templateScene->GetCurrentFolderName() + "\\";
}

EditorTemplate::TemplateTile* EditorTemplate::GetOrAddTemplateTile(int x, int y)
{
	TemplateTile* tt = GetTemplateTile(x, y);
	if(tt == 0)
	{
		tt = new TemplateTile();
		tt->x = x;
		tt->y = y;

		tt->blockArt = artMap.first();

		m_list.append(tt);
	}

	return tt;
}

EditorTemplate::TemplateTile* EditorTemplate::GetTemplateTile(int x, int y)
{
	foreach(TemplateTile* tt, m_list)
		if(tt->x == x && tt->y == y)
			return tt;

    return 0;

}

void EditorTemplate::RemoveTemplateTile(int x, int y)
{
	foreach(TemplateTile* tt, m_list)
		if(tt->x == x && tt->y == y)
            m_list.removeOne(tt);
}

EditorTemplate::EditorTemplate()
{
    setShapeMode( QGraphicsPixmapItem::BoundingRectShape );

}

EditorTemplate::~EditorTemplate()
{
}

void EditorTemplate::LoadTemplateIntoScene()
{
    EditorMode orig = editorMode;

    ed.SetEditorMode(EM_Template);
	int s = sceneCounter;

	sceneMech->ResetLevel();
	sceneCollission->ResetLevel();
	sceneArt->ResetLevel();

	foreach(TemplateTile* t, m_list)
	{
		sceneCounter = SCENEMECH;
		sceneMech->m_tiles[t->y][t->x].SetSelectedBlock(t->blockMech);
		sceneCounter = SCENECOLL;
		sceneCollission->m_tiles[t->y][t->x].SetSelectedBlock(t->blockColl);
		sceneCounter = SCENEART;
		sceneArt->m_tiles[t->y][t->x].SetSelectedBlock(templateToArtMap[t->blockArt]);
	}

	sceneCounter = s;
    ed.SetEditorMode(orig);
}

void EditorTemplate::LoadTemplate(QString filename)
{
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Could not open template file: " << filename.split("/").last();
        return;
    }

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
		TemplateTile* tt = new TemplateTile();
		in >> tt->x;
		in >> tt->y;
		in >> tt->blockMech;
		in >> tt->blockColl;
		in >> tt->blockArt;

        m_list.append(tt);

		if(!templateToArtMap.contains(tt->blockArt))
        {
            QPixmap* part = new QPixmap;
			if(!part->load(GetPath() + tt->blockArt))
				qDebug() << "failed to load artfile: " << tt->blockArt << " for template: " << m_name;

			short key = (short)pixmapsArt.size();
			artMap[key] = tt->blockArt;
			templateToArtMap[tt->blockArt] = key;
            pixmapsArt[key] = part;
        }
    }

    file.close();
}

void EditorTemplate::SaveTemplate()
{
    QString path = GetPath();
	QDir dir(path);

	QFile templateFile(path + m_name + ".tmpl");

	templateFile.open(QIODevice::WriteOnly);

	QDataStream out(&templateFile);

    out << m_name;

    out << m_list.size();
	foreach(TemplateTile* t, m_list)
    {
		out << t->x;
		out << t->y;
		out << t->blockMech;
		out << t->blockColl;
		out << t->blockArt;

		if(!dir.entryList(QDir::Files).contains(t->blockArt))
			pixmapsArt[templateToArtMap[t->blockArt]]->save(path + t->blockArt, "PNG");
    }

	templateFile.close();

	//check if all artfiles are present in the template folder.
	//copy all unpresent artfiles into template folder.



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
			QString resourceName = m_name + "_" + QString::number(x) + "_" + QString::number(y)  + ".png";
            QFile file(tmpname);
			file.open(QIODevice::WriteOnly);
			copy->save(&file, "PNG");
			file.close();

			TemplateTile* tile = new TemplateTile();
			tile->x = x;
			tile->y = y;
			tile->blockArt = resourceName;

			m_list.append(tile);
		}
    }


	//template keeps track of own resource files in .tmpl
	//editor loads all template resource files and creates temp art list with mapping of short int to path/filename
	//on level save create level.artlist file with list of used path/filename resources and create level.art file with structure


    SaveTemplate();

	return 1;
}

EditorTemplate::TemplateTile::TemplateTile()
{
	x = -1;
	y = -1;

    blockMech = ' ';
	blockArt = "";
	blockColl = ' ';
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
	ed.SetEditorMode(EM_Level);

	sceneMech = new EditorScene();
	sceneArt = new EditorScene();
	sceneCollission = new EditorScene();

	ed.SetEditorMode(EM_Template);

	sceneMech = new EditorScene();
	sceneArt = new EditorScene();
	sceneCollission = new EditorScene();

	templateScene = new TemplateScene();

	ed.SetEditorMode(EM_Level);

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

	ed.SetEditorMode(EM_Level);



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

    ed.GetSettingsWidget()->Initialize();

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


    sceneCounter = 2;
    ed.GetSettingsWidget()->mech->setCheckState(Qt::Checked);

    view->ResetSliders();

	return a.exec();
}


