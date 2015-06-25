#include "main.h"

#include "includeseditor.h"
#include "ed.h"

#include "gameobject.h"
#include "EditorView.h"

#include "SettingsWidget.h"

#include <QImage>

#define view view2 //hack

short selectedTile = ' ';



TilePallette* palletteTile;
QGraphicsScene* scenePallette;
QMap<short, QPixmap*> pixmaps;

QGraphicsScene* sceneArtPallette;
QMap<short, QPixmap*> pixmapsArt;
QMap<short, QString> artMap;
QMap<QString, short> templateToArtMap;

QGraphicsScene* sceneCollisionPallette;
QMap<short, QPixmap*> pixmapsCollision;

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

				sceneCollision->m_tiles[y][x].setAcceptedMouseButtons(0);
				sceneCollision->m_tiles[y][x].setAcceptHoverEvents(false);
				sceneCollision->m_tiles[y][x].setOpacity(0);

				sceneArt->m_tiles[y][x].setAcceptedMouseButtons(0);
				sceneArt->m_tiles[y][x].setAcceptHoverEvents(false);
				sceneArt->m_tiles[y][x].setOpacity(0);

				sceneMech->addItem(&sceneCollision->m_tiles[y][x]);
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

			sceneCollision->m_tiles[y][x].setAcceptedMouseButtons(Qt::NoButton);
			sceneCollision->m_tiles[y][x].setAcceptHoverEvents(false);

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

			sceneCollision->m_tiles[y][x].setAcceptedMouseButtons(Qt::NoButton);
			sceneCollision->m_tiles[y][x].setAcceptHoverEvents(false);

            sceneArt->m_tiles[y][x].setAcceptedMouseButtons(Qt::AllButtons);
            sceneArt->m_tiles[y][x].setAcceptHoverEvents(true);
        }

	foreach(GameObject* obj, gameObjects)
		obj->setAcceptedMouseButtons(Qt::NoButton);
}

void SetEditCollisionScene()
{
    for(int y = 0; y < MAPY; y++)
        for (int x = 0; x < MAPX; x++)
        {
            sceneMech->m_tiles[y][x].setAcceptedMouseButtons(Qt::NoButton);
            sceneMech->m_tiles[y][x].setAcceptHoverEvents(false);

			sceneCollision->m_tiles[y][x].setAcceptedMouseButtons(Qt::AllButtons);
			sceneCollision->m_tiles[y][x].setAcceptHoverEvents(true);

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

			sceneCollision->m_tiles[y][x].setAcceptedMouseButtons(Qt::NoButton);
			sceneCollision->m_tiles[y][x].setAcceptHoverEvents(false);

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
		viewPallette->setScene(sceneCollisionPallette);
		SetSelectedTile(pixmapsCollision.begin().key());
		view->setWindowTitle("Collision Layout");
		viewPallette->setWindowTitle("Collision Pallette");
		sceneCollisionPallette->addItem(palletteTile);
		SetEditCollisionScene();
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
		SetEditArtScene();
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
		return pixmapsCollision;
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
	LoadPixmapsGeneric("CollisionList.txt", pixmapsCollision);
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
	LoadPalletteGeneric(sceneCollisionPallette, pixmapsCollision);
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

    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);
	in.setByteOrder(QDataStream::LittleEndian);

    QPixmap image(filename + "data.png");

	int key = 0;
	int hitcount = 0;

    for(int y = 0; y < MAPY; y++)
    {
        for (int x = 0; x < MAPX; x++)
        {
			s->m_tiles[y][x].SetSelectedBlock(0);
		}
	}

    in >> key; //version
    in >> key; //mapx
    in >> key; //mapy
    in >> key; //count

	while(!in.atEnd())
	{
		in >> key;
		QPixmap* p = new QPixmap(image.copy((hitcount*BLOCKSIZE)%1920, ((hitcount*BLOCKSIZE)/1920)*BLOCKSIZE, BLOCKSIZE, BLOCKSIZE));

		pixmapsArt[pixmapsArt.size()] = p;
		int tiley = key/MAPX;
		int tilex = key%MAPX;
		s->m_tiles[tiley][tilex].SetSelectedBlock((short)pixmapsArt.size()-1);

		hitcount++;
	}

    file.close();
}

void LoadLevel(QString filename)
{
    sceneCounter = 0;
	LoadGeneric(filename + "Mec.txt", sceneMech);
    sceneCounter = 1;
	LoadArt(filename + "Art", sceneArt);
    sceneCounter = 2;
	LoadGeneric(filename + "Col.txt", sceneCollision);
    sceneCounter = 0;
	LoadObjects(filename + "Obj.txt", false);

	QPixmap p;
	if(p.load(filename+"Background.png") && (p.size().width() == 1920) && (p.size().height() == 1080))
	{
		ed.EditLevels();

		if(ed.bg)
			sceneMech->removeItem(ed.bg);

		ed.bg = sceneMech->addPixmap(p);
		ed.bg->setZValue(-10);
	}

}

void SaveGeneric(QString filename, EditorScene* s)
{
    QFile file(filename);
    file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);

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

bool testTransparentImage(QImage image)
{
	image.convertToFormat(QImage::Format_ARGB32);

	for (int x = 0 ; x < image.width(); x++)
	{
		for (int y = 0 ; y < image.height(); y++)
			if (qAlpha(image.pixel(x, y)) != 0)
				return false;
	}

	return true;
}

void SaveArtFile(QString filename, EditorScene* s)
{
	QFile fileArt(filename+".txt");
	fileArt.open(QIODevice::WriteOnly | QIODevice::Truncate);
	QDataStream out(&fileArt);
	out.setByteOrder(QDataStream::LittleEndian);


    out << VERSION;
    out << MAPX;
    out << MAPY;

	int count = 0;
	QList<int> hitlist;
	for(int y = 0; y < MAPY; y++)
	{
		for (int x = 0; x < MAPX; x++)
		{
				if(s->m_tiles[y][x].getBlock() != 0 && !testTransparentImage(s->m_tiles[y][x].pixmap().toImage()))
				{
					hitlist.append(count);
				}
				count++;
		}
	}

    out << hitlist.size();

	int count2 = 0;
	QImage artImage(1920, (((hitlist.size()*BLOCKSIZE)/1920)*BLOCKSIZE)+BLOCKSIZE, QImage::Format_ARGB32_Premultiplied);
	QPainter painter(&artImage);

	while(!hitlist.empty())
	{
		int key = hitlist.front();
		painter.drawPixmap((count2*BLOCKSIZE)%1920, ((count2*BLOCKSIZE)/1920)*BLOCKSIZE, s->m_tiles[key/MAPX][key%MAPX].pixmap());

        out << key;
		hitlist.pop_front();

		count2++;
	}

    artImage.save(filename + "data.png");
	fileArt.close();
}

void SaveObjects(QString filename)
{
	QFile file(filename);
    file.open(QIODevice::WriteOnly | QIODevice::Truncate);

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

	SaveGeneric(filename + '/' + "Mec.txt", sceneMech);
	if(sceneCollision)
		SaveGeneric(filename + '/' + "Col.txt", sceneCollision);
    if(sceneArt)
		SaveArtFile(filename + '/' + "Art", sceneArt);

	SaveObjects(filename + '/' + "Obj.txt");

	if(ed.bg)
		ed.bg->pixmap().save(filename + "/Background.png", "PNG");
}


Tile::Tile()
{
	selectedBlock = -1;

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
	QMap<short, QPixmap*>& currentMap = getCurrentPixmap();
	if(currentMap.contains(block))
	{
		if(selectedBlock != block)
			setPixmap(*(currentMap[block]));

		selectedBlock = block;
	}
	else
		qDebug() << "trying to set tile to nonexistant block";
}

QList<QGraphicsItem*> previewList;
void displayPreview(int x, int y)
{
	if(!previewList.empty())
	{
		foreach(QGraphicsItem* i, previewList)
			ed.GetSceneMech()->removeItem(i);

		previewList.clear();
	}

    EditorTemplate* ct = ed.m_currentTemplate;
	if(ct && placeTemplate)
	{
		foreach(EditorTemplate::TemplateTile* tt, ct->m_list)
		{
			previewList.push_back(ed.GetSceneMech()->addPixmap(*tt->GetPixmap()));
			previewList.back()->setPos(x + tt->x*BLOCKSIZE, y + tt->y * BLOCKSIZE);
			previewList.back()->setOpacity(0.4);
		}
	}
}

#include <QGraphicsItem>
void Tile::hoverEnterEvent ( QGraphicsSceneHoverEvent * e )
{
    if(leftbuttonHeld)
            SetSelectedBlock(selectedTile);
	else
		displayPreview(pos().x(), pos().y());


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
			m_tiles[y][x].SetSelectedBlock(' ');
}

void StampTemplate(int tilex, int tiley, EditorTemplate* ct, bool record)
{
	//Stamp template on level
	if(record)
		ed.undoTemplate = new EditorTemplate();

	int temp = sceneCounter;
	if(ct)
		foreach(EditorTemplate::TemplateTile* tt, ct->m_list)
		{
			if(tilex+tt->x < MAPX && tiley+tt->y < MAPY)
			{
				if(record)
				{
					EditorTemplate::TemplateTile*	undoTemplateTile = ed.undoTemplate->GetOrAddTemplateTile(tilex+tt->x, tiley+tt->y);
					undoTemplateTile->blockMech = sceneMech->m_tiles			[tiley+tt->y][tilex+tt->x].getBlock();
					undoTemplateTile->blockColl = sceneCollision->m_tiles		[tiley+tt->y][tilex+tt->x].getBlock();
					undoTemplateTile->blockArt =  artMap[sceneArt->m_tiles		[tiley+tt->y][tilex+tt->x].getBlock()];
				}

				sceneCounter = SCENEMECH;
				sceneMech->m_tiles			[tiley+tt->y][tilex+tt->x].SetSelectedBlock(tt->blockMech);
				sceneCounter = SCENECOLL;
				sceneCollision->m_tiles	[tiley+tt->y][tilex+tt->x].SetSelectedBlock(tt->blockColl);
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
}

void UnstampTemplate(int tilex, int tiley, EditorTemplate* ct, bool record)
{
	if(record)
		ed.undoTemplate = new EditorTemplate();

	int temp = sceneCounter;
	if(ct)
		foreach(EditorTemplate::TemplateTile* tt, ct->m_list)
		{
			if(tilex+tt->x < MAPX && tiley+tt->y < MAPY)
			{
				if(record)
				{
					EditorTemplate::TemplateTile*	undoTemplateTile = ed.undoTemplate->GetOrAddTemplateTile(tilex+tt->x, tiley+tt->y);
					undoTemplateTile->blockMech = sceneMech->m_tiles			[tiley+tt->y][tilex+tt->x].getBlock();
					undoTemplateTile->blockColl = sceneCollision->m_tiles		[tiley+tt->y][tilex+tt->x].getBlock();
					undoTemplateTile->blockArt =  artMap[sceneArt->m_tiles		[tiley+tt->y][tilex+tt->x].getBlock()];
				}

				sceneCounter = SCENEMECH;
				sceneMech->m_tiles			[tiley+tt->y][tilex+tt->x].SetSelectedBlock(' ');
				sceneCounter = SCENECOLL;
				sceneCollision->m_tiles	[tiley+tt->y][tilex+tt->x].SetSelectedBlock(' ');
				sceneCounter = SCENEART;
				sceneArt->m_tiles			[tiley+tt->y][tilex+tt->x].SetSelectedBlock(artMap.firstKey());

				if(editorMode == EM_Template)
				{
					EditorTemplate::TemplateTile* tt2 = ct->GetOrAddTemplateTile(tilex+tt->x, tiley+tt->y);
					tt2->blockMech = ' ';
					tt2->blockColl = ' ';
					tt2->blockArt = artMap.first();
				}
			}
		}
	sceneCounter = temp;
}

bool mirror = true;
void EditorScene::CustomMouseEvent ( QGraphicsSceneMouseEvent * e, Tile* tile )
{
	if(sceneCounter != SCENEOBJ)
	{
		int tilex = (int)(e->scenePos().x()/float(BLOCKSIZE));
		int tiley = (int)(e->scenePos().y()/float(BLOCKSIZE));


		if(placeTemplate)
		{
			if(e->buttons() == Qt::LeftButton)
                StampTemplate(tilex, tiley, ed.m_currentTemplate);
			else if(e->buttons() == Qt::RightButton)
                UnstampTemplate(tilex, tiley, ed.m_currentTemplate);
			e->accept();
		}
		else
		{
			ed.undoStack.clear();
			if(e->buttons() == Qt::RightButton)
			{
				if(sceneCounter == SCENEART)
					tile->SetSelectedBlock(artMap.keys().first());
				else
					tile->SetSelectedBlock(' ');

				if(editorMode == EM_Template)
				{
					if(sceneCounter == SCENEMECH)
                        ed.m_currentTemplate->GetOrAddTemplateTile(tilex, tiley)->blockMech = ' ';
					if(sceneCounter == SCENECOLL)
                        ed.m_currentTemplate->GetOrAddTemplateTile(tilex, tiley)->blockColl = ' ';
					if(sceneCounter == SCENEART)
					{
                        ed.m_currentTemplate->GetOrAddTemplateTile(tilex, tiley)->blockArt = artMap.first();
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
                        ed.m_currentTemplate->GetOrAddTemplateTile(tilex, tiley)->blockMech = selectedTile;
					if(sceneCounter == SCENECOLL)
                        ed.m_currentTemplate->GetOrAddTemplateTile(tilex, tiley)->blockColl = selectedTile;
					if(sceneCounter == SCENEART)
					{
                        ed.m_currentTemplate->GetOrAddTemplateTile(tilex, tiley)->blockArt = artMap[selectedTile];
						if(!templateToArtMap.contains(artMap[selectedTile]))
							templateToArtMap[artMap[selectedTile]] = selectedTile;
						tile->SetSelectedBlock(selectedTile);
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

	if(ed.undoTemplate)
	{
		ed.undoStack.append(ed.undoTemplate);
		ed.undoTemplate = 0;
	}
}

EditorArtScene::EditorArtScene() : EditorScene()
{
}

EditorArtScene::~EditorArtScene ()
{
	DeleteTiles();
}

void EditorArtScene::ResetLevel()
{
	for(int y = 0; y < m_mapy; y++)
		for (int x = 0; x < m_mapx; x++)
		{
			m_tiles[y][x].SetSelectedBlock(0);
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

TemplateFolder* TemplateScene::GetCurrentFolder()
{
    return m_currentFolder;
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

        ed.m_currentTemplate = m_currentTemplate;
	}
	else
	{
		m_currentTemplate = 0;
        ed.m_currentTemplate = m_currentTemplate;

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

        ed.m_currentTemplate = m_currentTemplate;
	}
	else
		SetCurrentTemplate();
}


void TemplateFolder::AddTemplate(EditorTemplate* t)
{
	m_templateMap[t->m_name] = t;

    LoadTileIntoScene();
}

void TemplateFolder::SelectNextTemplate()
{
    if(m_currentTemplate)
    {
        if(m_templateMap.find(m_currentTemplate->m_name) != m_templateMap.end())
            SetCurrentTemplate((m_templateMap.find(m_currentTemplate->m_name)+1).key());
    }
}

void TemplateFolder::SelectPreviousTemplate()
{
    if(m_currentTemplate)
    {
        if((m_templateMap.find(m_currentTemplate->m_name)) != m_templateMap.begin())
            SetCurrentTemplate((m_templateMap.find(m_currentTemplate->m_name)-1).key());
    }
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

	sceneCounter = SCENEMECH;
	sceneMech->ResetLevel();
	sceneCounter = SCENECOLL;
	sceneCollision->ResetLevel();
	sceneCounter = SCENEART;
	sceneArt->ResetLevel();

	foreach(TemplateTile* t, m_list)
	{
		sceneCounter = SCENEMECH;
		sceneMech->m_tiles[t->y][t->x].SetSelectedBlock(t->blockMech);
		sceneCounter = SCENECOLL;
		sceneCollision->m_tiles[t->y][t->x].SetSelectedBlock(t->blockColl);
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

EditorTemplate::TemplateTile::TemplateTile(EditorTemplate::TemplateTile& t)
{
    x = t.x;
    y = t.y;

    blockMech = t.blockMech;
    blockArt = t.blockArt;
    blockColl = t.blockColl;
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


EditorTemplate* EditorTemplate::GetMirror()
{
    EditorTemplate* t = new EditorTemplate();

    QPixmap* pixmap_reflect = new QPixmap(this->pixmap().transformed(QTransform().scale(-1, 1)));

    t->setPixmap(*pixmap_reflect);
    //this->setPixmap(*pixmap_reflect);

    delete pixmap_reflect;

    int maxX = 0;
    foreach(TemplateTile* tt, m_list)                                                           //determine max x
    {
        TemplateTile* tt2 = new TemplateTile(*tt);
        t->m_list.append(tt2);

        if(tt2->x > maxX)
            maxX = tt2->x;
    }


    foreach(TemplateTile* tt, t->m_list)
    {
        tt->x = maxX -tt->x;                                                                    //x values become max x -x

        pixmap_reflect = new QPixmap(tt->GetPixmap()->transformed(QTransform().scale(-1, 1)));  //mirror all art tiles

        tt->blockArt.chop(4);
        tt->blockArt += "HMirror.png";


        if(!templateToArtMap.contains(tt->blockArt))                                            //add new mirrored tiles to artlist
        {
            //qDebug() << "adding " << tt->blockArt;
            short key = (short)pixmapsArt.size();
            artMap[key] = tt->blockArt;
            templateToArtMap[tt->blockArt] = key;
            pixmapsArt[key] = pixmap_reflect;
        }
    }

    return t;
}


void InitializeScenesAndViews()
{
	ed.SetEditorMode(EM_Level);

	sceneMech = new EditorScene();
	sceneArt = new EditorArtScene();
	sceneCollision = new EditorScene();

	ed.SetEditorMode(EM_Template);

	sceneMech = new EditorScene();
	sceneArt = new EditorArtScene();
	sceneCollision = new EditorScene();

	templateScene = new TemplateScene();

	ed.SetEditorMode(EM_Level);

	view = new EditorView();

	viewPallette = new EditorViewBasic;
	viewPallette->setDragMode(QGraphicsView::ScrollHandDrag);
	scenePallette = new QGraphicsScene;
	sceneArtPallette = new QGraphicsScene;
	sceneCollisionPallette = new QGraphicsScene;
	sceneObjectPallette = new QGraphicsScene();

	viewPallette->setScene(scenePallette);
	viewPallette->showNormal();
	viewPallette->resize(430, 600);

	view->setMinimumWidth(1200);
	viewPallette->setMinimumWidth(320);

	scenePallette->setSceneRect(0,0,4*BLOCKSIZE,10*BLOCKSIZE);
	sceneArtPallette->setSceneRect(0,0,4*BLOCKSIZE,10*BLOCKSIZE);
	sceneCollisionPallette->setSceneRect(0,0,4*BLOCKSIZE,10*BLOCKSIZE);

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

	sceneCounter = SCENECOLL; //incorrect so setting to mech will initiate change in pallette
	ed.GetSettingsWidget()->mech->setCheckState(Qt::Checked);

    view->ResetSliders();

	return a.exec();
}



void EditorScene::dragEnterEvent(QGraphicsSceneDragDropEvent *e)
{
	if (e->mimeData()->hasUrls())
		e->acceptProposedAction();
}

void EditorScene::dropEvent(QGraphicsSceneDragDropEvent *e)
{
	foreach (const QUrl &url, e->mimeData()->urls())
	{
		QString filename = url.toLocalFile();

		view->ImportTemplate(filename);
	}
}

void EditorScene::dragMoveEvent(QGraphicsSceneDragDropEvent *e)
{
	e->acceptProposedAction();
}

short Tile::getBlock()
{
	return selectedBlock;
}
