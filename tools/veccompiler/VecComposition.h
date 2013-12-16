#pragma once

#include <string>
#include <vector>
#include "Stream.h"
#include "Types.h"

class ShapeLibItem
{
public:
	ShapeLibItem();
	
	std::string m_Name;
	std::string m_Path;
};

class ShapeLib
{
public:
	ShapeLib();

	const ShapeLibItem* Find(std::string name) const;
	
	std::vector<ShapeLibItem> m_Items;
};

class Link
{
public:
	Link();
	
	std::string m_Name;
	std::string m_ParentName;
	std::string m_ShapeName;
	
	Vec2F m_Location;
	float m_AngleBase;
	float m_Angle;
	float m_AngleMin;
	float m_AngleMax;
	float m_AngleSpeed;
	int m_LevelMin;
	int m_LevelMax;
	int m_Flags;
};

class Composition
{
public:
	Composition();
	
	const Link* Find(std::string name) const;
	
	std::vector<Link> m_Links;
	ShapeLib m_Library;
};
