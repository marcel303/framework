#include "Calc.h"
#include "GameState.h"
#include "Grid_Effect.h"
#include "Util_ColorEx.h"

#include "GameRound.h"

#define VERTEX_IDX(x, y) ((x) + (y) * m_GridSx)
#define INDEX_IDX(x, y, v) (((x) + (y) * (m_GridSx - 1)) * 2 * 3 + (v))
#define GRID_UPRES 1
#define GRID_OFFSET (-(GRID_UPRES-1)/2)

namespace Game
{
	Grid_Effect::Grid_Effect()
	{
		Initialize();
	}
	
	Grid_Effect::~Grid_Effect()
	{
		Allocate(0, 0);
	}
	
	void Grid_Effect::Initialize()
	{
		m_GridSx = 0;
		m_GridSy = 0;
		m_CellSx = 1.0f;
		m_CellSy = 1.0f;
		m_Cells = 0;
		m_BaseHue = 0.0f;
		m_BaseBrightness = 0.0f;
		m_BaseMultiplier = 1.0f;
		m_BaseMultiplierAnim = 1.0f;
		m_EffectType = GridEffectType_Colors;
	}
	
	void Grid_Effect::Setup(float sx, float sy, float cellSx, float cellSy)
	{
		cellSx /= GRID_UPRES;
		cellSy /= GRID_UPRES;
		
		const int gridSx = (int)Calc::DivideUp(sx, cellSx);
		const int gridSy = (int)Calc::DivideUp(sy, cellSy);
		
		m_CellSx = sx / gridSx;
		m_CellSy = sy / gridSy;
		m_CellSxRcp = gridSx / sx;
		m_CellSyRcp = gridSy / sy;
		
		Allocate(gridSx, gridSy);
		
//		BaseHue_set(0.66f);
		BaseHue_set(0.0f);
		BaseBrightness_set(0.5f);
		m_BaseHueController.Setup(m_BaseHue, m_BaseHue, Calc::mPI / 4.0f);
		
		Update(0.0f);
	}
	
	// --------------------
	// Effects
	// --------------------
	void Grid_Effect::Clear()
	{
		// todo
	}
	
	void Grid_Effect::BaseHue_set(float hue)
	{
		m_BaseHue = hue;
	}
	
	float Grid_Effect::BaseHue_get() const
	{
		return m_BaseHue;
	}
	
	void Grid_Effect::BaseBrightness_set(float brightness)
	{
		m_BaseBrightness = brightness;
	}
	
	float Grid_Effect::BaseBrightness_get() const
	{
		return m_BaseBrightness;
	}
	
	void Grid_Effect::Scroll(Vec2F delta)
	{
		m_Scroll += delta;
	}
	
	void Grid_Effect::BaseMultiplier_set(float multiplier)
	{
		m_BaseMultiplier = multiplier;
	}
	
	// --------------------
	// Animation
	// --------------------
	void Grid_Effect::Update(float dt)
	{
		const float kMinDT = 1.0f / 10.0f;
		
		if (dt > kMinDT)
			dt = kMinDT;
		
		Update_Water(dt);
		
		// update water animation
		
		Update_Animation(dt);
	}
	
	void Grid_Effect::Update_Water(float dt)
	{
		// clear grid borders
		
		for (int x = 0; x < m_GridSx; ++x)
		{
			int index1 = x;
			int index2 = x + (m_GridSy - 1) * m_GridSx;
			
			m_Cells[index1].m_Position = 0.0f;
			m_Cells[index1].m_Speed = 0.0f;
			
			m_Cells[index2].m_Position = 0.0f;
			m_Cells[index2].m_Speed = 0.0f;
		}
		
		for (int y = 0; y < m_GridSy; ++y)
		{
			int index1 = y * m_GridSx;
			int index2 = y * m_GridSx + m_GridSx - 1;
			
			m_Cells[index1].m_Position = 0.0f;
			m_Cells[index1].m_Speed = 0.0f;
			
			m_Cells[index2].m_Position = 0.0f;
			m_Cells[index2].m_Speed = 0.0f;
		}
		
		// update velocity based on spring system
		
		for (int y = 1; y < m_GridSy - 1; ++y)
		{
			for (int x = 1; x < m_GridSx - 1; ++x)
			{
				const int index = x + y * m_GridSx;
				
				float x1 = m_Cells[index - 1].m_Position;
				float x2 = m_Cells[index + 1].m_Position;
				float y1 = m_Cells[index - m_GridSx].m_Position;
				float y2 = m_Cells[index + m_GridSx].m_Position;
				
				m_Cells[index].m_Delta[0] = x2 - x1;
				m_Cells[index].m_Delta[1] = y2 - y1;

				float d = x1 + x2 + y1 + y2 - m_Cells[index].m_Position * 5.0f;
				
//				m_Cells[index].m_Speed += d * 10.0f * dt;
				m_Cells[index].m_Speed += d * 4.0f * dt;
			}
		}
	}
	
	void Grid_Effect::Update_Animation(float dt)
	{
		// update base hue
		
		m_BaseHueController.TargetAngle_set(m_BaseHue * Calc::m2PI);
		m_BaseHueController.Update(dt);
		
		const float hue = m_BaseHueController.Angle_get() / Calc::m2PI;
		
		//printf("base hue: %f vs %f\n", m_BaseHue, hue);
		
		// update base multiplier
		
		float baseDelta = m_BaseMultiplier - m_BaseMultiplierAnim;
		float baseStep = Calc::Sign(baseDelta) * 0.3f * dt;
		if (fabsf(baseStep) > fabsf(baseDelta))
			baseStep = baseDelta;
		m_BaseMultiplierAnim += baseStep;
		
		// update movement
		
		const float falloff = powf(0.9f, dt);
//		const float falloff2 = powf(0.8f, dt);
		const float falloff_Destroyed = powf(0.5f, dt);
		
		//const float amp = 0.05f;
		const float amp = 0.015f;
		
		int index = 0;
		
		const SpriteColor backColor = Calc::Color_FromHue_NoLUT(hue);
		
//		LOG(LogLevel_Debug, "hue: %f -> %d, %d, %d", hue, (int)backColor.v[0], (int)backColor.v[1], (int)backColor.v[2]);
		
#ifdef DEBUG
		if (m_EffectType == GridEffectType_Dark)
		{
			//Assert(m_BaseHue == g_GameState->m_GameRound->m_WaveInfo.maxiHue);
		}
#endif
		
//		const float destroyedScale = 1.0f / 100.0f;
		const float destroyedScale = 1.0f / 65.0f;
//		const float destroyedScale = 1.0f / 50.0f;
		
#define UPDATE_CELL \
	m_Cells[index].m_Speed *= falloff; \
	m_Cells[index].m_Position += m_Cells[index].m_Speed * dt; \
	m_Cells[index].m_ValueDestroyed *= falloff_Destroyed
	
		if (m_EffectType == GridEffectType_Colors)
		{
			const float base = m_BaseBrightness * m_BaseMultiplierAnim;
			
			for (int y = 0; y < m_GridSy; ++y)
			{
				for (int x = 0; x < m_GridSx; ++x)
				{
					UPDATE_CELL;
					
					const float v = m_Cells[index].m_Position * 2.0f;
					
					float c = base * .5f - v;
					
					if (c < 0.0f)
						c = 0.0f;
					if (c > 2.0f)
						c = 2.0f;
					
					SpriteColor destroyedColor = Calc::Color_FromHue(hue + m_Cells[index].m_ValueDestroyed * destroyedScale);
					
					if (c < 1.0f)
						m_Sprite.m_Vertices[index].m_Color = SpriteColor_BlendF(SpriteColors::Black, destroyedColor, c);
					else
						m_Sprite.m_Vertices[index].m_Color = SpriteColor_BlendF(destroyedColor, SpriteColors::White, c - 1.0f);
					
					m_Sprite.m_Vertices[index].m_TexCoord.u = m_Cells[index].m_TexCoord[0] + m_Cells[index].m_Delta[0] * amp;
					m_Sprite.m_Vertices[index].m_TexCoord.v = m_Cells[index].m_TexCoord[1] + m_Cells[index].m_Delta[1] * amp;
				
					index++;
				}
			}
		}
		else
		{
			const int base = (int)(63.0f * m_BaseMultiplierAnim);
			
			for (int y = 0; y < m_GridSy; ++y)
			{
				for (int x = 0; x < m_GridSx; ++x)
				{
					UPDATE_CELL;
					
//					int v = base + m_Cells[index].m_ObjectCount * 64;
					int v = base + m_Cells[index].m_ObjectCount * 50;
					
					v = v / 2;
					
					if (v > 255)
						v = 255;
					
					m_Sprite.m_Vertices[index].m_Color = SpriteColor_Modulate(backColor, SpriteColor_Make(v, v, v, 255));
					
//					m_Sprite.m_Vertices[index].m_TexCoord.u = m_Cells[index].m_TexCoord[0] + m_Scroll[0];
//					m_Sprite.m_Vertices[index].m_TexCoord.v = m_Cells[index].m_TexCoord[1] + m_Scroll[1];
					m_Sprite.m_Vertices[index].m_TexCoord.u = m_Cells[index].m_TexCoord[0];
					m_Sprite.m_Vertices[index].m_TexCoord.v = m_Cells[index].m_TexCoord[1];
				
					index++;
				}
			}
		}
	}
	
	// --------------------
	// Drawing
	// --------------------
	void Grid_Effect::Render()
	{
		g_GameState->m_SpriteGfx->WriteSprite(m_Sprite);
	}
	
	// --------------------
	// Management
	// --------------------
	void Grid_Effect::ObjectAdd(const Vec2F& pos)
	{
		int midCell[2];
		
		GetCellCoord(pos, midCell);
		
		for (int x = 0; x < GRID_UPRES; ++x)
		{
			for (int y = 0; y < GRID_UPRES; ++y)
			{
				int cell[2] = { midCell[0] + GRID_OFFSET + x, midCell[1] + GRID_OFFSET + y };
				
				if (cell[0] < 0 || cell[0] >= m_GridSx)
					continue;
				if (cell[1] < 0 || cell[1] >= m_GridSy)
					continue;
				
				const int index = cell[0] + cell[1] * m_GridSx;
				
				m_Cells[index].m_ObjectCount++;
				//m_Cells[index].m_Speed += 0.1f;
			}
		}
	}
	
	void Grid_Effect::ObjectRemove(const Vec2F& pos)
	{
		int midCell[2];
		
		GetCellCoord(pos, midCell);
		
		for (int x = 0; x < GRID_UPRES; ++x)
		{
			for (int y = 0; y < GRID_UPRES; ++y)
			{
				int cell[2] = { midCell[0] + GRID_OFFSET + x, midCell[1] + GRID_OFFSET + y };
				
				if (cell[0] < 0 || cell[0] >= m_GridSx)
					continue;
				if (cell[1] < 0 || cell[1] >= m_GridSy)
					continue;
				
				const int index = cell[0] + cell[1] * m_GridSx;
				
				m_Cells[index].m_ObjectCount--;
				m_Cells[index].m_Speed -= 0.1f;
				
				m_Cells[index].m_ValueDestroyed += 1.0f;
				
				if (m_Cells[index].m_ObjectCount == 0)
				{
					//m_Cells[index].m_Speed -= 0.2f;
					//m_Cells[index].m_Position = -0.5f;
				}
			}
		}
	}
	
	void Grid_Effect::ObjectUpdate(const Vec2F& oldPos, const Vec2F& newPos)
	{
		int oldCell[2];
		int newCell[2];
		
		GetCellCoord(oldPos, oldCell);
		GetCellCoord(newPos, newCell);
		
		if (oldCell[0] != newCell[0] || oldCell[1] != newCell[1])
		{
			ObjectRemove(oldPos);
			ObjectAdd(newPos);
		}
	}
	
	// --------------------
	// Interaction
	// --------------------
	
	void Grid_Effect::Impulse(const Vec2F& pos, float impulse)
	{
		int cell[2];
		
		GetCellCoord(pos, cell);
		
		if (cell[0] < 0 || cell[0] >= m_GridSx)
			return;
		if (cell[1] < 0 || cell[1] >= m_GridSy)
			return;
		
		const int index = cell[0] + cell[1] * m_GridSx;
		
		m_Cells[index].m_Speed += impulse;
	}
	
	void Grid_Effect::Allocate(int sx, int sy)
	{
		delete[] m_Cells;
		m_Cells = 0;
		m_GridSx = 0;
		m_GridSy = 0;
		m_Sprite.Allocate(0, 0);
		
		if (sx > 0 && sy > 0)
		{
			m_Cells = new Cell[sx * sy];
			m_GridSx = sx;
			m_GridSy = sy;
						
			// fill cells
			
			int vertexIdx = 0;
			
			for (int y = 0; y < sy; ++y)
			{
				for (int x = 0; x < sx; ++x)
				{
					m_Cells[vertexIdx].m_TexCoord[0] = x / (float)(sx - 1.0f);
					m_Cells[vertexIdx].m_TexCoord[1] = y / (float)(sy - 1.0f);
					
					vertexIdx++;
				}
			}
			
			// allocate sprite
			
			const int vertexCount = sx * sy;
			const int indexCount = (sx - 1) * (sy - 1) * 6;
			
			m_Sprite.Allocate(vertexCount, indexCount);
			
			// fill sprite
								
			const float scaleX = m_CellSx * m_GridSx / (sx - 1.f);
			const float scaleY = m_CellSy * m_GridSy / (sy - 1.f);
			
			vertexIdx = 0;
			
			for (int y = 0; y < sy; ++y)
			{
				for (int x = 0; x < sx; ++x)
				{
					m_Sprite.m_Vertices[vertexIdx].m_Coord.x = x * scaleX;
					m_Sprite.m_Vertices[vertexIdx].m_Coord.y = y * scaleY;
					m_Sprite.m_Vertices[vertexIdx].m_Color = SpriteColors::White;
					
					vertexIdx++;
				}
			}
			
			// fill index array
			
			int indexIdx = 0;
			
			for (int y = 0; y < sy - 1; ++y)
			{
				for (int x = 0; x < sx - 1; ++x)
				{
					const int index = x + y * sx;
					
					m_Sprite.m_Indices[indexIdx++] = index;
					m_Sprite.m_Indices[indexIdx++] = index + 1;
					m_Sprite.m_Indices[indexIdx++] = index + 1 + sx;
					
					m_Sprite.m_Indices[indexIdx++] = index;
					m_Sprite.m_Indices[indexIdx++] = index + 1 + sx;
					m_Sprite.m_Indices[indexIdx++] = index + sx;
				}
			}
		}
	}
	
	int Grid_Effect::DBG_GridSx_get() const
	{
		return m_GridSx;
	}
	
	int Grid_Effect::DBG_GridSy_get() const
	{
		return m_GridSy;
	}
	
	const Grid_Effect::Cell* Grid_Effect::DBG_GetCell(int x, int y) const
	{
		return m_Cells + x + y * m_GridSx;
	}
	
	float Grid_Effect::DBG_CellSx_get() const
	{
		return m_CellSx;
	}
	
	float Grid_Effect::DBG_CellSy_get() const
	{
		return m_CellSy;
	}
}
