#include "VecComposition.h"

ShapeLibItem::ShapeLibItem()
{
}

ShapeLib::ShapeLib()
{
}

const ShapeLibItem* ShapeLib::Find(std::string name) const
{
	for (size_t i = 0; i < m_Items.size(); ++i)
		if (m_Items[i].m_Name == name)
			return &m_Items[i];
	
	return 0;
}

Link::Link()
{
	m_AngleBase = 0.0f;
	m_Angle = 0.0f;
	m_AngleMin = 0.0f;
	m_AngleMax = 0.0f;
	m_AngleSpeed = 0.0f;
	m_LevelMin = 0;
	m_LevelMax = 1000;
	m_Flags = 0;
}

Composition::Composition()
{
}

const Link* Composition::Find(std::string name) const
{
	for (size_t i = 0; i < m_Links.size(); ++i)
		if (m_Links[i].m_Name == name)
			return &m_Links[i];
	
	return 0;
}
