#pragma once

#include <string>
#include "IImageLoader.h"
#include "Image.h"
#include "VecTypes.h"

typedef Circle DrawCircle;

class DrawPoly
{
public:
	Poly m_Poly;
	Hull m_Hull;
	std::vector<Triangle> m_Triangles;
};

class DrawCurve
{
public:
	DrawCurve()
	{
	}

	~DrawCurve()
	{
	}

	void Allocate(int pointCount)
	{
		m_Points.clear();

		for (int i = 0; i < pointCount; ++i)
		{
			PointF point;

			m_Points.push_back(point);
		}
	}

	Curve m_Curve;
	std::vector<PointF> m_Points;
};

class DrawRectangle
{
public:
	PointF m_Location;
	PointF m_Size;
};

class DrawPicture
{
public:
	DrawPicture()
	{
		m_ContentScale = 1.0f;
		m_Scale = 1;
	}
	
	void Load(IImageLoader* loader)
	{
		loader->Load(m_Image, m_Path);
	}

	Image m_Image;
	PointI m_Location;
	std::string m_Path;
	float m_ContentScale;
	int m_Scale;
};

enum ShapeType
{
	ShapeType_Undefined,
	ShapeType_Poly,
	ShapeType_Circle,
	ShapeType_Curve,
	ShapeType_Rectangle,
	ShapeType_Tag,
	ShapeType_Picture
};

class ShapeItem
{
public:
	ShapeItem();

	void Finalize(int scale);
	void Offset(int x, int y);
	void Scale(int scale);

	ShapeType m_Type;
	DrawPoly m_Poly;
	DrawCircle m_Circle;
	DrawCurve m_Curve;
	DrawRectangle m_Rectangle;
	DrawPicture m_Picture;
	Tag m_Tag;

	BBoxI m_BB;
	BBoxI m_BBWithStroke;

	std::string m_Name;
	std::string m_Group;
	int m_RGB[4]; // todo
	float m_Stroke;
	float m_Hardness;
	int m_LineColor;
	int m_FillColor;
	float m_LineOpacity;
	float m_FillOpacity;
	bool m_Visible;
	bool m_Filled;
	bool m_Collision;
};

class Shape
{
public:
	Shape();
	
	void Finalize(int scale);

	void Offset(int x, int y);
	void Scale(int s);
	void Normalize(int scale);
	ShapeItem* FindItem(std::string name);
	Tag* FindTag(std::string name);
	std::vector<ShapeItem*> FindItemsByGroup(std::string group);

	std::vector<ShapeItem> m_Items;
	bool m_HasShadow;
	bool m_HasSprite;
	
	BBoxI m_BB;
	BBoxI m_BBWithStroke;
};
