#include <math.h>
#include "CompositionCompiler.h"
#include "Exception.h"
#include "StringEx.h"

static float DegToRad(float angle)
{
	return angle / 180.0f * (float)M_PI;
}

static int Find(const std::vector<Link>& links, std::string name)
{
	for (int i = 0; i < (int)links.size(); ++i)
		if (links[i].m_Name == name)
			return i;
	
	return -1;
}

void CompositionCompiler::Compile(const Composition& composition, VRCC::CompiledComposition& out_Composition)
{
	// todo: create flat links representation
	
	//printf("link count: %d\n", (int)composition.m_Links.size());
	
	out_Composition.Allocate((int)composition.m_Links.size());
	
	for (size_t i = 0; i < composition.m_Links.size(); ++i)
	{
		const Link& link = composition.m_Links[i];
		VRCC::Composition_Link& temp = out_Composition.m_Links[i];
		
		const int index = (int)i;
		const int parentIndex = Find(composition.m_Links, link.m_ParentName);
		const ShapeLibItem* item = composition.m_Library.Find(link.m_ShapeName);
		
		if (parentIndex < 0 && link.m_Name != "root")
			throw ExceptionVA("parent does not exist: %s for %s", link.m_ParentName.c_str(), link.m_Name.c_str());
		if (item == 0 && link.m_ShapeName != "")
			throw ExceptionVA("shape not found in library: %s for %s", link.m_ShapeName.c_str(), link.m_Name.c_str());
		
		if (parentIndex < 0)
			out_Composition.m_RootIndex = index;
		else
		{
			//printf("adding child %d to parent %d\n", index, parentIndex);
			
			out_Composition.m_Links[parentIndex].m_ChildIndices.AddTail(index);
		}
			
		temp.m_Index = index;
		temp.m_ParentIndex = parentIndex;
		if (item)
			temp.m_ShapeName = item->m_Path;
		
		temp.m_Location = link.m_Location;
		temp.m_AngleBase = DegToRad(link.m_AngleBase);
		temp.m_Angle = DegToRad(link.m_Angle);
		temp.m_AngleMin = DegToRad(link.m_AngleMin);
		temp.m_AngleMax = DegToRad(link.m_AngleMax);
		temp.m_AngleSpeed = DegToRad(link.m_AngleSpeed);
		temp.m_LevelMin = link.m_LevelMin;
		temp.m_LevelMax = link.m_LevelMax;
		temp.m_Flags = link.m_Flags;
		
		// validate shape
		
		int weaponMask =
			VRCC::CompositionFlag_Weapon_Vulcan |
			VRCC::CompositionFlag_Weapon_Missile |
			VRCC::CompositionFlag_Weapon_Beam |
			VRCC::CompositionFlag_Weapon_BlueSpray |
			VRCC::CompositionFlag_Weapon_PurpleSpray;
		
		if (!(temp.m_Flags & weaponMask) && temp.m_ShapeName == String::Empty)
		{
			throw ExceptionVA("link without shape and not a weapon: %s", link.m_Name.c_str());
		}
	}

	// perform link validation

	for (int i = 0; i < out_Composition.m_LinkCount; ++i)
	{
		VRCC::Composition_Link& link = out_Composition.m_Links[i];

		if (link.m_LevelMax < link.m_LevelMin)
			throw ExceptionVA("max level < min level (%s)", composition.m_Links[i].m_Name.c_str());

		const int parentIndex = link.m_ParentIndex;//Find(composition.m_Links, link.m_ParentName);

		if (parentIndex >= 0)
		{
			//const Link& parent = composition.m_Links[parentIndex];
			const VRCC::Composition_Link& parent = out_Composition.m_Links[parentIndex];

//			if (link.m_LevelMin < parent.m_LevelMin)
//				throw ExceptionVA("min level < parent min level");
			if (link.m_LevelMin < parent.m_LevelMin)
				link.m_LevelMin = parent.m_LevelMin;
			if (link.m_LevelMax > parent.m_LevelMax)
				throw ExceptionVA("max level > parent max level");
		}
	}
}
