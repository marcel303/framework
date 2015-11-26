#pragma once

#include "includeseditor.h"



class LevelLayer
{
public:
		LevelLayer();
		virtual ~LevelLayer(){}

		virtual void CreateLayer(int x, int y) = 0;
		virtual void SetElement(int x, int y, bool update = true) = 0;
		virtual void DeleteElement(int x, int y, bool update = true) = 0;
		virtual void UpdateLayer() = 0;

		virtual void SaveLayer(QString filename) = 0;
		virtual void LoadLayer(QString filename) = 0;

		unsigned int** m_grid;

		int m_x;
		int m_y;

		QPixmap* m_pixmap;
};


class QPainter;
class MechLayer : public LevelLayer
{
public:
		MechLayer();
		virtual ~MechLayer();

		virtual void CreateLayer(int x, int y);
		virtual void SetElement(int x, int y, bool update = true);
		virtual void DeleteElement(int x, int y, bool update = true);
		virtual void UpdateLayer();

		virtual void SaveLayer(QString filename);
		virtual void LoadLayer(QString filename);
};

class ArtLayer: public LevelLayer
{
public:
		ArtLayer();
		virtual ~ArtLayer();

		virtual void CreateLayer(int x, int y);
		virtual void SetElement(int x, int y, bool update = true);
		virtual void DeleteElement(int x, int y, bool update = true);
		virtual void UpdateLayer();

		virtual void SaveLayer(QString filename);
		virtual void LoadLayer(QString filename);
};



class LayerPicker
{
public:

	LayerPicker();
	~LayerPicker();

	LevelLayer* GetCurrentLayer();





	QList<LevelLayer*> m_layers;
};


class CurrentSelection
{
public:

	CurrentSelection();
	~CurrentSelection();

};
