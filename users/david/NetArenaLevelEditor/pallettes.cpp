#include "pallettes.h"

void LoadPixmapsGeneric(QString filename, QMap<char, QPixmap*>& map)
{
	QPixmap* p;
	QPixmap* p2;

	QList<QString> list = ed.GetLinesFromConfigFile(filename);

	QString path = list.front();
	list.pop_front();

	while(!list.empty())
	{
		QString line = list.front();
		QChar key = line[line.size()-1];
		char key2 = key.toLatin1();
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

PalletteTile::PalletteTile () : QGraphicsPixmapItem()
{
}

PalletteTile::~PalletteTile ()
{
}

void PalletteTile::mousePressEvent ( QGraphicsSceneMouseEvent * e )
{
	qDebug() << "setting id to " << m_id;
	m_base->m_selectedID = m_id;

	e->accept();
}

void PalletteTile::hoverEnterEvent ( QGraphicsSceneHoverEvent * e )
{
	e->accept();
}


BasePallette::BasePallette()
{
	m_selectedID = ' ';
}

BasePallette::~BasePallette()
{
}

void BasePallette::CreatePallette(QMap<char, QPixmap*>& map)
{
	this->clear();
	m_selectedID = ' ';

	PalletteTile* t;
	QMap<char, QPixmap*>::iterator i;

	int y = 1;
	int x = 0;
	for(i = map.begin(); i != map.end(); ++i)
	{
		if(x > PALLETTEROWSIZE)
		{
			y++;
			x = 0;
		}

		t = new PalletteTile();
		t->setPixmap(i.value()->scaled(BLOCKSIZE, BLOCKSIZE));

		addItem(t);
		t->setPos(x*BLOCKSIZE + 4, y*BLOCKSIZE + 4);

		t->m_id = i.key();
		t->m_base = this;

		x++;
	}
}

void BasePallette::LoadPallette(const QString &filename)
{
	m_palletteMap.clear();

	LoadPixmapsGeneric(filename, m_palletteMap);

	CreatePallette(m_palletteMap);
}

QPixmap* BasePallette::GetImage(unsigned int key)
{
	if(m_palletteMap.contains(key))
		return m_palletteMap[key];

	return 0;
}
