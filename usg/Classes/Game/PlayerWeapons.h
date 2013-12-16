#pragma once

namespace Game
{
	enum WeaponType
	{
		WeaponType_Undefined = -1,
		WeaponType_Vulcan = 0,
		WeaponType_Laser = 1,
		WeaponType_ZCOUNT = 2
	};
	 
	class WeaponSlot
	{
	public:
		WeaponSlot();
		void Initialize();
		
		void Setup(WeaponType type, int level);
		
		void IncreaseLevel();
		void DecreaseLevel();
		
		void Update(bool fire);
		bool IsReady_get() const;
		void Fire();
	private:
		void FireVulcan(int& bulletCount, Vec2F fireDirection_Forward, Vec2F fireDirection_Strafe, Vec2F fireDirection_Strafe2, const void* id);
		void FireBeam(SpriteColor color, const void* ignoreId);
	public:
		
		WeaponType Type_get() const;
		int Level_get() const;
		void Level_set(int value);
		int MaxLevel_get() const;
		bool IsActive_get() const;
		
	private:
		void ClampLevel();
		
		WeaponType m_Type;
		int m_MinLevel;
		int m_MaxLevel; // determined by weapon type
		int m_Level;
		int m_FireCount;
		int m_CoolDown[2];
	};
}
