#include "Precompiled.h"
#include "Shape.h"
#include "VecRend_Bezier.h"

ShapeItem::ShapeItem()
{
	m_Name = "noname";

	m_RGB[0] = 255;
	m_RGB[1] = 255;
	m_RGB[2] = 255;
	m_RGB[3] = 255;
	
	m_Stroke = 1.5f;
	m_Hardness = 1.0f;
	
	m_LineColor = 255;
	m_FillColor = 31;
	m_LineOpacity = 1.0f;
	m_FillOpacity = 1.0f;
	
	m_Visible = true;
	m_Filled = true;
	m_Collision = true;
}

void ShapeItem::Finalize(int scale)
{
	BBoxI bbox;

	switch (m_Type)
	{
	case ShapeType_Poly:
		for (size_t i = 0; i < m_Poly.m_Poly.m_Points.size(); ++i)
		{
			const PointI& point = m_Poly.m_Poly.m_Points[i];

			bbox.Merge(point);
		}
		break;

	case ShapeType_Circle:
		{
			PointI min = PointI(m_Circle.x - m_Circle.r, m_Circle.y - m_Circle.r);
			PointI max = PointI(m_Circle.x + m_Circle.r, m_Circle.y + m_Circle.r);

			bbox.Merge(min);
			bbox.Merge(max);
		}
		break;

	case ShapeType_Curve:
		{
			// create points array

			size_t curveCount = m_Curve.m_Curve.m_Points.size();

			if (!m_Curve.m_Curve.m_IsClosed && curveCount > 0)
				curveCount--;

			m_Curve.Allocate(curveCount * 4);

			for (size_t i = 0; i < curveCount; ++i)
			{
				const CurvePoint& point1 = m_Curve.m_Curve.m_Points[i];
				const CurvePoint& point2 = m_Curve.m_Curve.m_Points[(i + 1) % m_Curve.m_Curve.m_Points.size()];

				PointF* temp = &m_Curve.m_Points[i * 4];

				temp[0].x = (float)point1.x;
				temp[0].y = (float)point1.y;
				temp[1].x = (float)point1.x + point1.tx2;
				temp[1].y = (float)point1.y + point1.ty2;
				temp[2].x = (float)point2.x + point2.tx1;
				temp[2].y = (float)point2.y + point2.ty1;
				temp[3].x = (float)point2.x;
				temp[3].y = (float)point2.y;
			}

			// sample points

			std::vector<PointF> points;

			VecRend_SampleBezier(&m_Curve.m_Points[0], m_Curve.m_Points.size() / 4, points);

			for (size_t i = 0; i < points.size(); ++i)
			{
				PointI point1 = PointI((int)floorf(points[i].x), (int)floorf(points[i].y));
				PointI point2 = PointI((int)ceilf(points[i].x), (int)ceilf(points[i].y));

				bbox.Merge(point1);
				bbox.Merge(point2);
			}
		}
		break;

	case ShapeType_Rectangle:
		break;
			
	case ShapeType_Tag:
		m_Stroke = 0;
		break;
		
	case ShapeType_Picture:
		{
			m_Stroke = 0;
			
			LOG_DBG("Finalize: ShapeType_Picture: ContentScale: %f", m_Picture.m_ContentScale);
			
			PointI min;
			min[0] = m_Picture.m_Location[0];
			min[1] = m_Picture.m_Location[1];
			PointI max = PointI(m_Picture.m_Location.x + m_Picture.m_Image.m_Sx * m_Picture.m_Scale / m_Picture.m_ContentScale, m_Picture.m_Location.y + m_Picture.m_Image.m_Sy * m_Picture.m_Scale / m_Picture.m_ContentScale);

			bbox.Merge(min);
			bbox.Merge(max);
		}
		break;

	case ShapeType_Undefined:
		throw ExceptionVA("unknown shape type");
		break;
	}

	m_BB = bbox;

	//int border = ceilf(m_Stroke * scale);
	//int border1 = ((m_Stroke * 2.0f - 1.0f) / 2.0f) * scale;
	
	//int border1 = ((m_Stroke * 2.0f) / 2.0f) * scale;
	//int border2 = ((m_Stroke * 2.0f) / 2.0f) * scale;

	int border1 = (int)ceilf(m_Stroke * scale);
	int border2 = (int)ceilf(m_Stroke * scale);

	bbox.m_Min.x -= border1;
	bbox.m_Min.y -= border1;
	bbox.m_Max.x += border2;
	bbox.m_Max.y += border2;

	m_BBWithStroke = bbox;
}

void ShapeItem::Offset(int x, int y)
{
	switch (m_Type)
	{
	case ShapeType_Poly:
		for (size_t i = 0; i < m_Poly.m_Poly.m_Points.size(); ++i)
		{
			m_Poly.m_Poly.m_Points[i].x += x;
			m_Poly.m_Poly.m_Points[i].y += y;
		}
		break;

	case ShapeType_Circle:
		{
			m_Circle.x += x;
			m_Circle.y += y;
		}
		break;

	case ShapeType_Curve:
		for (size_t i = 0; i < m_Curve.m_Curve.m_Points.size(); ++i)
		{
			m_Curve.m_Curve.m_Points[i].x += x;
			m_Curve.m_Curve.m_Points[i].y += y;
		}
		break;
			
	case ShapeType_Rectangle:
		{
			m_Rectangle.m_Location.x += x;
			m_Rectangle.m_Location.y += y;
		}
		break;

	case ShapeType_Picture:
		{
			m_Picture.m_Location.x += x;
			m_Picture.m_Location.y += y;
		}
		break;

	case ShapeType_Tag:
		{
			m_Tag.x += x;
			m_Tag.y += y;
		}
		break;

	case ShapeType_Undefined:
		throw ExceptionVA("unknown shape type");
		break;
	}
}

void ShapeItem::Scale(int scale)
{
	switch (m_Type)
	{
	case ShapeType_Poly:
		for (size_t i = 0; i < m_Poly.m_Poly.m_Points.size(); ++i)
		{
			m_Poly.m_Poly.m_Points[i].x *= scale;
			m_Poly.m_Poly.m_Points[i].y *= scale;
		}
		break;

	case ShapeType_Circle:
		{
			m_Circle.x *= scale;
			m_Circle.y *= scale;
			m_Circle.r *= scale;
		}
		break;

	case ShapeType_Curve:
		for (size_t i = 0; i < m_Curve.m_Curve.m_Points.size(); ++i)
		{
			m_Curve.m_Curve.m_Points[i].x *= scale;
			m_Curve.m_Curve.m_Points[i].y *= scale;
			m_Curve.m_Curve.m_Points[i].tx1 *= scale;
			m_Curve.m_Curve.m_Points[i].ty1 *= scale;
			m_Curve.m_Curve.m_Points[i].tx2 *= scale;
			m_Curve.m_Curve.m_Points[i].ty2 *= scale;
		}
		break;

	case ShapeType_Rectangle:
		{
			m_Rectangle.m_Location.x *= scale;
			m_Rectangle.m_Location.y *= scale;
			m_Rectangle.m_Size.x *= scale;
			m_Rectangle.m_Size.y *= scale;
		}
		break;
			
	case ShapeType_Picture:
		{
			m_Picture.m_Location.x *= scale;
			m_Picture.m_Location.y *= scale;
			m_Picture.m_Scale *= scale;
		}
		break;
		
	case ShapeType_Tag:
		{
			m_Tag.x *= scale;
			m_Tag.y *= scale;
		}
		break;
		
	case ShapeType_Undefined:
		{
		}
		break;
	}
}

Shape::Shape()
{
	m_HasShadow = true;
	m_HasSprite = false;
}

void Shape::Finalize(int scale)
{
	m_BB = BBoxI();
	m_BBWithStroke = BBoxI();

	for (size_t i = 0; i < m_Items.size(); ++i)
	{
		m_Items[i].Finalize(scale);

		if (!m_Items[i].m_BB.IsEmpty_get())
		{
			m_BB.Merge(m_Items[i].m_BB);
			m_BBWithStroke.Merge(m_Items[i].m_BBWithStroke);
		}
	}
}

void Shape::Offset(int x, int y)
{
	for (size_t i = 0; i < m_Items.size(); ++i)
		m_Items[i].Offset(x, y);
}

void Shape::Scale(int s)
{
	for (size_t i = 0; i < m_Items.size(); ++i)
		m_Items[i].Scale(s);
}

void Shape::Normalize(int scale)
{
	Finalize(scale);

	Offset(
		-m_BBWithStroke.m_Min.x,
		-m_BBWithStroke.m_Min.y);

	Finalize(0);
}

ShapeItem* Shape::FindItem(std::string name)
{
	ShapeItem* result = 0;

	for (size_t i = 0; i < m_Items.size(); ++i)
	{
		if (m_Items[i].m_Name != name)
			continue;

		result = &m_Items[i];
	}

	return result;
}

Tag* Shape::FindTag(std::string name)
{
	ShapeItem* item = FindItem(name);

	if (!item)
		return 0;
	if (item->m_Type != ShapeType_Tag)
		return 0;

	return &item->m_Tag;
}

std::vector<ShapeItem*> Shape::FindItemsByGroup(std::string group)
{
	std::vector<ShapeItem*> result;

	for (size_t i = 0; i < m_Items.size(); ++i)
	{
		if (m_Items[i].m_Group != group)
			continue;

		result.push_back(&m_Items[i]);
	}

	return result;
}
