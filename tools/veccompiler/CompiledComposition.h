#pragma once

#include <string>
#include "ColList.h"
#include "Stream.h"
#include "Types.h"

namespace VRCC
{
	enum CompositionFlag
	{
		CompositionFlag_Draw_MirrorX = 1 << 0,
		CompositionFlag_Draw_MirrorY = 1 << 1,
		CompositionFlag_Behaviour_AngleSpin = 1 << 2,
		CompositionFlag_Animation_Thruster = 1 << 6,
		CompositionFlag_Defense_Shield = 1 << 8,
		CompositionFlag_Defense_Armour = 1 << 9,
		CompositionFlag_Weapon_Vulcan = 1 << 10,
		CompositionFlag_Weapon_Missile = 1 << 11,
		CompositionFlag_Weapon_Beam = 1 << 12,
		CompositionFlag_Weapon_BlueSpray = 1 << 13,
		CompositionFlag_Weapon_PurpleSpray = 1 << 14,
		CompositionFlag_Behaviour_AngleFollow = 2 << 16,
	};

	class Composition_Link
	{
	public:
		Composition_Link();
		
		int m_Index;
		int m_ParentIndex;
		Col::List<int> m_ChildIndices;
		Vec2F m_Location;
		float m_AngleBase;
		float m_Angle;
		float m_AngleMin;
		float m_AngleMax;
		float m_AngleSpeed;
		int m_LevelMin;
		int m_LevelMax;
		int m_Flags;
		std::string m_ShapeName;
	};
		
	class CompiledComposition
	{
	public:
		CompiledComposition();
		~CompiledComposition();
		
		void Load(Stream* stream);
		void Save(Stream* stream);
		
		void Allocate(int linkCount);
		
		int m_RootIndex;
		Composition_Link* m_Links;
		int m_LinkCount;
	};
}
