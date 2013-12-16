#pragma once

#include "AngleController.h"
#include "SpriteGfx.h"
#include "Types.h"

/*
 
 THE THINGS WE WANT TO ACCOMPLISH:
 
 - wave intermezzos:
 	- todo
 - level intermezzos:
 	- todo
 - game over:
 	- todo
 - boss introduction:
 	- todo

 HOW WE WILL ACCOMPLISH:
 
 Grid properties:
 - global illumincation
 
 Grid cell properties:
 - density (# entities/cell)
 - local illumincation
 - color (hue)
 
 Grid cell blending
 - LERP local illuminication
 - Use highest density
 - LERP color
 
 */

namespace Game
{
	enum GridEffectType
	{
		GridEffectType_Colors,
		GridEffectType_Dark
	};
	
	class Grid_Effect
	{
	public:
		Grid_Effect();
		~Grid_Effect();
		void Initialize();
		void Setup(float sx, float sy, float cellSx, float cellSy);
		
		class Cell
		{
		public:
			Cell()
			{
				m_ObjectCount = 0;
				m_Position = 0.0f;
				m_Speed = 0.0f;
				m_ValueDestroyed = 0.0f;
			}
			
			int m_ObjectCount;
			// todo: hue, etc
			float m_Position;
			float m_Speed;
			Vec2F m_TexCoord;
			Vec2F m_Delta;
			float m_ValueDestroyed;
		};
		
		// --------------------
		// Effects
		// --------------------
		void Clear();
		void BaseHue_set(float hue);
		float BaseHue_get() const;
		void BaseBrightness_set(float brightness);
		float BaseBrightness_get() const;
		void Scroll(Vec2F delta);
		void BaseMultiplier_set(float multiplier);
		
		float m_BaseHue;
		float m_BaseBrightness;
		AngleController m_BaseHueController;
		Vec2F m_Scroll;
		float m_BaseMultiplier;
		float m_BaseMultiplierAnim;
		
		// --------------------
		// Animation
		// --------------------
		void Update(float dt);
		void Update_Water(float dt);
		void Update_Animation(float dt);
		
		inline void EffectType_set(GridEffectType type)
		{
#ifdef DEBUG
//			type = GridEffectType_Colors;
#endif
			
			m_EffectType = type;
		}
		
		GridEffectType m_EffectType;
		
		// --------------------
		// Drawing
		// --------------------
		void Render();
		
		// --------------------
		// Management
		// --------------------
		void ObjectAdd(const Vec2F& pos);
		void ObjectRemove(const Vec2F& pos);
		void ObjectUpdate(const Vec2F& oldPos, const Vec2F& newPos);

		// --------------------
		// Interaction
		// --------------------
		void Impulse(const Vec2F& pos, float impulse);
		
		// --------------------
		// Debugging
		// --------------------
		int DBG_GridSx_get() const;
		int DBG_GridSy_get() const;
		const class Cell* DBG_GetCell(int x, int y) const;
		float DBG_CellSx_get() const;
		float DBG_CellSy_get() const;
		
	private:
		void Allocate(int sx, int sy);
		
		inline void GetCellCoord(const Vec2F& pos, int* out_Coord) const
		{
			out_Coord[0] = (int)(pos[0] * m_CellSxRcp);
			out_Coord[1] = (int)(pos[1] * m_CellSyRcp);
		}
		
		int m_GridSx;
		int m_GridSy;
		float m_CellSx;
		float m_CellSy;
		float m_CellSxRcp;
		float m_CellSyRcp;
		Cell* m_Cells;
		Sprite m_Sprite;
	};
}
