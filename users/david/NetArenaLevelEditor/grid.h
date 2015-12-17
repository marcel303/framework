#pragma once

#include "includeseditor.h"

#include <QMimeData>


class STile : public QGraphicsPixmapItem
{
public:
	STile();
	virtual ~STile();

	virtual void mousePressEvent ( QGraphicsSceneMouseEvent * e );
	virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * e );
	virtual void hoverEnterEvent ( QGraphicsSceneHoverEvent * e );

	void SetPos(int x, int y);


	int m_x;
	int m_y;
};

class QPixmap;
class GridLayer: public QGraphicsItem
{
public:

	GridLayer();
	virtual ~GridLayer();

	virtual QRectF boundingRect() const;
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);

	QImage* m_image;
	QPixmap* m_pixmap;

};



class Template;
class Grid : public QGraphicsScene
{
public:
	Grid();
	virtual ~Grid();

	void SetupLevelImages();
	void CreateGrid(int x, int y);
	virtual void CustomMouseEvent (QGraphicsSceneMouseEvent * e , STile *tile);


	void dragEnterEvent(QGraphicsSceneDragDropEvent *e);
	void dropEvent(QGraphicsSceneDragDropEvent *e);
	void dragMoveEvent(QGraphicsSceneDragDropEvent *e);

	void SetLayerFront(QImage *p);
	void SetCurrentTarget(Template* t);

	STile** m_tiles;

	int m_mapx;
	int m_mapy;

	GridLayer* m_front;
	GridLayer* m_middle;
	GridLayer* m_back;
	GridLayer* m_col;
	GridLayer* m_mec;

};


