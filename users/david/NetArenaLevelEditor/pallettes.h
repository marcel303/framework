#pragma once

#include "includeseditor.h"

class BasePallette : public QGraphicsScene
{
public:

	BasePallette();
	virtual ~BasePallette();

	void CreatePallette(QMap<char, QPixmap*>& map);

	int m_selectedID;

	QPixmap* GetImage(unsigned int key);
	void LoadPallette(const QString &filename);
	QMap<char, QPixmap*> m_palletteMap;
};

class PalletteTile : public QGraphicsPixmapItem
{
public:

	PalletteTile ();
	virtual ~PalletteTile ();

	virtual void mousePressEvent ( QGraphicsSceneMouseEvent * e );
	virtual void hoverEnterEvent ( QGraphicsSceneHoverEvent * e );

	int m_id;

	BasePallette* m_base;
};


class MechPallette : public BasePallette
{
};

class CollPallette : public BasePallette
{
};

class TemplatePallette: public BasePallette
{
};
