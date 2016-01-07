#pragma once

#include "QGraphicsRectItem"

class EditorObject : public QGraphicsRectItem //resizable super rectangle deluxe
{

	EditorObject();
	virtual ~EditorObject();


	virtual void mousePressEvent ( QGraphicsSceneMouseEvent * e );
	virtual void mouseReleaseEvent ( QGraphicsSceneMouseEvent * e );
	virtual void hoverEnterEvent ( QGraphicsSceneHoverEvent * e );

	virtual QRectF boundingRect() const;
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);

};
