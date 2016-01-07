
#include "grid.h"

#include "ed.h"
#include "layers.h"
#include "templates.h"


#include <QGraphicsItem>

Tile::Tile()
{
	this->setOpacity(1.0);

	setAcceptHoverEvents(true);
	setAcceptedMouseButtons(Qt::AllButtons);
	setShapeMode( QGraphicsPixmapItem::BoundingRectShape );

	m_x = 0;
	m_y = 0;
}

Tile::~Tile()
{
}

#include "SettingsWidget.h"
#include "QRadioButton"

#include "QGraphicsRectItem"

void Tile::mousePressEvent ( QGraphicsSceneMouseEvent * e )
{
	if(ed.GetSettingsWidget()->m_mech->isChecked())
	{
		if(e->button() == Qt::LeftButton)
			ed.m_currentTarget->m_mec.SetElement(m_x, m_y, true);
		if(e->button() == Qt::RightButton)
			ed.m_currentTarget->m_mec.DeleteElement(m_x, m_y, true);
	}
	if(ed.GetSettingsWidget()->m_coll->isChecked())
	{
		if(e->button() == Qt::LeftButton)
			ed.m_currentTarget->m_col.SetElement(m_x, m_y, true);
		if(e->button() == Qt::RightButton)
			ed.m_currentTarget->m_col.DeleteElement(m_x, m_y, true);
	}
	if(ed.GetSettingsWidget()->m_temp->isChecked())
	{
		if(e->button() == Qt::LeftButton)
			ed.m_templateScene->m_currentFolder->m_currentTemplate->StampTo(m_x, m_y);
		if(e->button() == Qt::RightButton)
			ed.m_templateScene->m_currentFolder->m_currentTemplate->Unstamp(m_x, m_y);
	}
	if(ed.GetSettingsWidget()->m_obj->isChecked())
	{
		if(e->button() == Qt::LeftButton)
		{
			QGraphicsRectItem* p = new QGraphicsRectItem(m_x*BLOCKSIZE, m_y*BLOCKSIZE,45, 45);

			p->setAcceptTouchEvents(true);

			ed.m_grid->addItem(p);
		}
	}



	e->accept();
}

void Tile::mouseReleaseEvent ( QGraphicsSceneMouseEvent * e )
{
	e->accept();
}

void Tile::hoverEnterEvent ( QGraphicsSceneHoverEvent * e )
{
	if(ed.m_leftbuttonHeld)
	{
		if(ed.GetSettingsWidget()->m_mech->isChecked())
		{
			ed.m_currentTarget->m_mec.SetElement(m_x, m_y, false);
		}
		if(ed.GetSettingsWidget()->m_coll->isChecked())
		{
			ed.m_currentTarget->m_col.SetElement(m_x, m_y, false);
		}
	}
	else if(ed.GetSettingsWidget()->m_temp->isChecked())
	{
		ed.UpdatePreview(m_x*BLOCKSIZE, m_y*BLOCKSIZE);
	}

	e->accept();
}

void Tile::SetPos(int x, int y)
{
	m_x = x;
	m_y = y;

	setPos(x * BLOCKSIZE, y * BLOCKSIZE);
}

GridLayer::GridLayer() : QGraphicsItem()
{
	m_image = 0;
	m_pixmap = 0;
}

GridLayer::~GridLayer()
{
}

QRectF GridLayer::boundingRect() const
{
	return QRectF(0, 0, MAPX*BLOCKSIZE, MAPY*BLOCKSIZE);
}

void GridLayer::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(widget);
	Q_UNUSED(option);

    if(m_pixmap)
		painter->drawPixmap(0, 0, *m_pixmap);
}


Grid::Grid()
{
	m_tiles = 0;
}

Grid::~Grid()
{
}

void Grid::SetupLevelImages()
{
	m_front = new GridLayer();
	addItem(m_front);
	m_front->setZValue(-1.0);
	m_front->setAcceptHoverEvents(false);
	m_front->setAcceptedMouseButtons(0);

	m_middle = new GridLayer();
	addItem(m_middle);
	m_middle->setZValue(-2.0);
	m_middle->setAcceptHoverEvents(false);
	m_middle->setAcceptedMouseButtons(0);

    //m_back = new GridLayer();
    //addItem(m_back);
    //m_back->setZValue(-3.0);
    //m_back->setAcceptHoverEvents(false);
    //m_back->setAcceptedMouseButtons(0);

	m_mec = new GridLayer();
	addItem(m_mec);
	m_mec->setZValue(-4.0);
	m_mec->setAcceptHoverEvents(false);
	m_mec->setAcceptedMouseButtons(0);

	m_col = new GridLayer();
	addItem(m_col);
	m_col->setZValue(-5.0);
	m_col->setAcceptHoverEvents(false);
	m_col->setAcceptedMouseButtons(0);
}

void Grid::CreateGrid(int x, int y)
{
	this->clear();

    m_x = x;
    m_y = y;

	SetupLevelImages();

	m_tiles = new Tile*[y];
	for(int i = 0; i < y; ++i)
		m_tiles[i] = new Tile[x];

	QPixmap* p = new QPixmap("EditorData\\tile.png");

	for(int y1 = 0; y1 < y; y1++)
	{
		for(int x1 = 0; x1 < x; x1++)
		{

			m_tiles[y1][x1].SetPos(x1, y1);
			m_tiles[y1][x1].setPixmap(*p);

			this->addItem(&m_tiles[y1][x1]);
		}
	}

	delete p;

    this->update();
}

void Grid::SetGrid(bool b)
{
    for(int y = 0; y < m_y; y++)
        for(int x = 0; x < m_x; x++)
            m_tiles[y][x].setOpacity((b? 100 : 0));

}

void Grid::SetCurrentTarget(Template* t)
{
	if(!t)
		return;

	m_front->m_pixmap = t->m_front.m_pixmap;
	m_middle->m_pixmap = t->m_middle.m_pixmap;
    //m_back->m_pixmap = t->m_back.m_pixmap;
	m_mec->m_pixmap = t->m_mec.m_pixmap;
	m_col->m_pixmap = t->m_col.m_pixmap;
}




