#include "FontMap.h"
#include "GameState.h"
#include "Graphics.h"
#include "Mat3x2.h"
#include "MenuMgr.h"
#include "SoundEffectMgr.h"
#include "StringBuilder.h"
#include "TempRender.h"
#include "Textures.h"
#include "Timer.h"
#include "TouchDLG.h"
#include "UsgResources.h"
#include "Util_ColorEx.h"
#include "View_UpgradeHD.h"

#define ITEM_COUNT 12 
#define ITEM_SIZE_CLOSED 60.0f
#define ITEM_SIZE_OPENED 150.0f
#define ITEM_OPEN_SPEED 5.0f
#define ITEM_CLOSE_SPEED 5.0f

#define SCROLL_FALLOFF 0.01f

typedef struct
{
	const char* Caption;
	const char* Description;
	int Type;
} Item;

const static Item sItemList[] =
{
	{ "Vulcan Upgrade", "The vulcan is a versatile weapon that may be deployed against a wide variety of enemies", 0 },
	{ "Laser Upgrade", "The laser is a highly effective weapon in situations where you can maximize its area damaging potential", 0 },
	{ "Missile Upgrade", "These smart missiles will hunt down enemies using their heat detecting sensors", 0 },
	{ "Shockwave Upgrade", "The experimental shockwave weapon disturbs time and space absorbing everything within its range and slowing down time", 0 },
	{ "Turret X2", "The deployable turret is an autonomous device that deals heavy damage", 0 },
	{ "Turret X5", "The deployable turret is an autonomous device that deals heavy damage", 0 },
	{ "Pacify", "Heal the world, and make it a better place using the pacifier effect, a phenomenon witnessed when bursts of high energy gamma radiation comes into cantact with dark matter", 0 },
	{ "Unlock Powder Gun", "Nicknamed the powder gun, the SG1044X is a high energy mass emitter dealing heavy damage", 0 },
	{ "Credit Magnet X2", "Use credit magnets to boost your upgrading ability", 0 }
};

const static int sItemListSize = sizeof(sItemList) / sizeof(Item);

template <uint32_t RESX, uint32_t RESY>
class Grid
{
public:
	float mScaleX;
	float mScaleY;
	
	void Initialize(float sizeX, float sizeY)
	{
		mScaleX = sizeX / RESX;
		mScaleY = sizeY / RESY;
	}
	
	void Render(float threshold)
	{	
		float vScale = 1.0f / threshold;
		
		for (uint32_t x = 0; x < RESX; ++x)
		{
			for (uint32_t y = 0; y < RESY; ++y)
			{
				const float scale = mField[x][y] * vScale;
				
				const float px = (x + 0.5f) * mScaleX;
				const float py = (y + 0.5f) * mScaleY;

				const Vec2F pos(px, py);
				const Vec2F size = Vec2F(mScaleX, mScaleY) * scale * 1.5f;
				
				RenderQuad(pos - size, pos + size, 0.2f, 0.05f, 0.4f, scale, g_GameState->GetTexture(Textures::COLOR_WHITE));
			}
		}
	}
	
	void Integrate_Begin()
	{
		memset(mField, 0, sizeof(mField));
	}
	
	void Integrate_End()
	{
		// nop
	}
	
	void Integrate_Circle(float posX, float posY, float radius)
	{	
		for (uint32_t x = 0; x < RESX; ++x)
		{
			for (uint32_t y = 0; y < RESY; ++y)
			{
				float dx = (x + 0.5f) * mScaleX - posX;
				float dy = (y + 0.5f) * mScaleY - posY;
				
				float v = 1.0f / ((dx * dx + dy * dy) / (radius * radius) + 1.0f);
				
				mField[x][y] += v;
			}
		}
	}
	
	float mField[RESX][RESY];
};

static Grid<32, 24> sGrid;

namespace Game
{
	View_UpgradeHD::View_UpgradeHD()
	{
		Initialize();
	}
	
	void View_UpgradeHD::Initialize()
	{
		TouchListener listener;
		listener.Setup(this, HandleTouchBegin, HandleTouchEnd, HandleTouchMove);
		g_GameState->m_TouchDLG->Register(USG::TOUCH_PRIO_UPGRADE, listener);
		
		sGrid.Initialize(VIEW_SX, VIEW_SY);
	}
	
	// --------------------
	// View related
	// --------------------
	
	void View_UpgradeHD::Update(float dt)
	{
		if (mScrollActive == false)
		{
			mScrollPosition += mScrollSpeed * dt;
			mScrollSpeed *= powf(0.5f, dt);
			
			float totalSize = TotalItemSize_get(false) - VIEW_SY;
			
			if (-mScrollPosition < 0.0f)
			{
				mScrollSpeed *= powf(0.2f, dt);
				//mScrollSpeed = 0.0f;
				mScrollPosition += (0.0f-mScrollPosition) * (1.0f - powf(SCROLL_FALLOFF, dt));
			}
			if (-mScrollPosition > totalSize)
			{
				mScrollSpeed *= powf(0.2f, dt);
				//mScrollSpeed = 0.0f;
				mScrollPosition += (-mScrollPosition-totalSize) * (1.0f - powf(SCROLL_FALLOFF, dt));
			}
		}
		
		switch (mOpenState)
		{
			case OpenState_Closed:
				break;
			case OpenState_ClosedAnim:
				mOpenAnim = Calc::Max(0.0f, mOpenAnim - ITEM_CLOSE_SPEED * dt);
				if (mOpenAnim == 0.0f)
					mOpenState = OpenState_Closed;
				break;
			case OpenState_Open:
				break;
			case OpenState_OpenAnim:
				mOpenAnim = Calc::Min(1.0f, mOpenAnim + ITEM_OPEN_SPEED * dt);
				if (mOpenAnim == 1.0f)
					mOpenState = OpenState_Open;
				float pos = ItemPosW_get(mOpenIndex) + ItemSize_get(mOpenIndex) + mScrollPosition;
				if (pos > VIEW_SY)
					mScrollPosition -= pos - VIEW_SY;
				break;
		}
	}
	
	void View_UpgradeHD::Render()
	{
		g_GameState->m_SpriteGfx->Flush();
		
#if 0
		gGraphics.BlendModeSet(BlendMode_Normal_Opaque);
		g_GameState->DataSetSelect(DS_GLOBAL);
		gGraphics.TextureSet(g_GameState->m_ResMgr.Get(Resources::BACKGROUND_10));
		float uv0[2] = { 0.0f, 0.0f };
		float uv1[2] = { 1.0f, 1.0f };
		RenderGrid(Vec2F(0.0f, 0.0f), Vec2F(VIEW_SX, VIEW_SY), 1, 1, SpriteColors::White, uv0, uv1);
		//RenderRect(Vec2F(0.0f, 0.0f), Vec2F(VIEW_SX, VIEW_SY), 0.0f, 0.0f, 0.0f, 1.0f, g_GameState->GetTexture(Textures::COLOR_WHITE));
		g_GameState->m_SpriteGfx->Flush();
#endif
		
#if 0
		gGraphics.BlendModeSet(BlendMode_Additive);
		// fixme!
		sGrid.Integrate_Begin();
		for (uint32_t i = 0; i < 3; ++i)
		{
			float speed = (i + 1) / 3.0f;
			float x = VIEW_SX / 2.0f + 160.0f * sinf(g_TimerRT.Time_get() * speed);
			float y = VIEW_SY / 2.0f + 120.0f * cosf(g_TimerRT.Time_get() * speed);
			sGrid.Integrate_Circle(x, y, 100.0f);
		}
		sGrid.Integrate_End();
		sGrid.Render(3.0f);
		g_GameState->m_SpriteGfx->Flush();
#endif
		
		for (int i = 0; i < ITEM_COUNT; ++i)
		{
			const Item& item = sItemList[i % sItemListSize];
			
			float y1 = ItemPosV_get(i + 0);
			float y2 = ItemPosV_get(i + 1);
			float hues[2] = { -0.3f, -0.25f };
			float hue = hues[i % 2];
			SpriteColor color = Calc::Color_FromHue(hue);
			color.v[3] = 192;
			
			Vec2F pos(0.0f, y1);
			Vec2F size(VIEW_SX, y2 - y1);
			
			//gGraphics.BlendModeSet(BlendMode_Additive);
			gGraphics.BlendModeSet(BlendMode_Normal_Transparent);
			RenderRect(
				pos,
				size,
				color,
				g_GameState->GetTexture(Textures::COLOR_WHITE));
			g_GameState->m_SpriteGfx->Flush();
			
			gGraphics.BlendModeSet(BlendMode_Normal_Transparent);
			
			if (i == mOpenIndex)
			{
				RenderRect(
					pos,
					size,
					SpriteColor_MakeF(0.0f, 0.0f, 0.0f, 0.5f * mOpenAnim),
					g_GameState->GetTexture(Textures::GAMEOVER_HINT_BACK));
			}
			
			RenderText(pos + Vec2F(10.0f, 0.0f), Vec2F(VIEW_SX, ITEM_SIZE_CLOSED), g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE), SpriteColors::White, TextAlignment_Left, TextAlignment_Center, false, item.Caption);
			
			if (i == mOpenIndex)
			{
				if (mOpenAnim == 1.0f)
				{
					const float scale = 0.7f;
					const float sy = g_GameState->GetFont(Resources::FONT_USUZI_SMALL)->m_Font.m_Height * scale + 2.0f;
					const int maxSize = 40;
					float y = 0.0f;
					int index1 = 0;
					int index2 = 0;
					while (index1 != (int)strlen(item.Description))
					{
						for (int j = 0; j < maxSize; ++j)
						{
							if (item.Description[index1 + j] == 0)
							{
								index2 = index1 + j;
								break;
							}
							if (item.Description[index1 + j] == ' ')
								index2 = index1 + j;
						}
	
						Mat3x2 matT;
						matT.MakeTranslation(pos + Vec2F(15.0f, ITEM_SIZE_CLOSED + y));
						Mat3x2 matS;
						matS.MakeScaling(scale, scale);
						Mat3x2 mat = matT * matS;
						
						StringBuilder<50> sb;
						for (int j = index1; j < index2; ++j)
							sb.Append(item.Description[j]);
						
						RenderText(mat, g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColors::White, TextAlignment_Left, TextAlignment_Top, sb.ToString());
						
						index1 = index2;
						y += sy;
					}
				}
			}
			
			g_GameState->m_SpriteGfx->Flush();
		}
		
		gGraphics.BlendModeSet(BlendMode_Normal_Transparent);
		
		float scrollTotal = TotalItemSize_get(true);
		float scrollMargin = 5.0f;
		float scrollView = VIEW_SY - scrollMargin * 2.0f;
		
		if (scrollTotal > scrollView)
		{
			float scrollPosition = Calc::Mid(-mScrollPosition, 0.0f, scrollTotal);
			float scrollBarSize = scrollView / scrollTotal * scrollView;
			float scrollSpacing = scrollView - scrollBarSize;
			float scrollOffset = scrollMargin + scrollPosition / (scrollTotal - scrollView) * scrollSpacing;
			
			RenderRect(
				Vec2F(VIEW_SX - 15.0f, scrollOffset),
				Vec2F(10.0f, scrollBarSize),
				SpriteColor_MakeF(0.3f, 0.1f, 0.2f, 0.5f),
				g_GameState->GetTexture(Textures::COLOR_WHITE));
		}
		
		g_GameState->m_SpriteGfx->Flush();
	}
	
	int View_UpgradeHD::RenderMask_get()
	{
		return RenderMask_Interface | RenderMask_WorldBackground;
	}
	
	void View_UpgradeHD::HandleFocus()
	{
		g_GameState->Interface_get()->ActiveMenu_set(GameMenu::MenuType_Empty);
		
		g_GameState->m_TouchDLG->Enable(USG::TOUCH_PRIO_UPGRADE);
		
		// scrolling through the item list
		mScrollPosition = 0.0f;
		mScrollSpeed = 0.0f;
		mScrollActive = false;
		
		// opening item details
		mOpenIndex = -1;
		mOpenState = OpenState_Closed;
		mOpenAnim = 0.0f;
	}
	
	void View_UpgradeHD::HandleFocusLost()
	{
		g_GameState->m_TouchDLG->Disable(USG::TOUCH_PRIO_UPGRADE);
	}
	
	// --------------------
	// Touch related
	// --------------------
	
	bool View_UpgradeHD::HandleTouchBegin(void* obj, const TouchInfo& touchInfo)
	{
		View_UpgradeHD* self = (View_UpgradeHD*)obj;
		
		self->mScrollSpeed = 0.0f;
		
		return true;
	}
	
	bool View_UpgradeHD::HandleTouchMove(void* obj, const TouchInfo& touchInfo)
	{
		View_UpgradeHD* self = (View_UpgradeHD*)obj;
		
		self->mScrollActive = true;
		self->mScrollSpeed = touchInfo.m_LocationDelta[1] * 20.0f;
		
		if (self->mScrollActive)
		{
			self->mScrollPosition += touchInfo.m_LocationDelta[1];
		}
		
		return true;
	}
	
	bool View_UpgradeHD::HandleTouchEnd(void* obj, const TouchInfo& touchInfo)
	{
		View_UpgradeHD* self = (View_UpgradeHD*)obj;
		
		if (self->mScrollActive == false)
		{
			const int index = self->HitItem(touchInfo.m_LocationView);
			
			LOG_INF("item index: %d", index);
			
			if (index >= 0 && index < ITEM_COUNT)
			{
				if (index == self->mOpenIndex)
				{
					switch (self->mOpenState)
					{
						case OpenState_Closed:
							self->mOpenState = OpenState_OpenAnim;
							self->mOpenAnim = 0.0f;
							self->mOpenIndex = index;
							g_GameState->m_SoundEffects->Play(Resources::SOUND_ZOOM_OUT, 0);
							break;
						case OpenState_ClosedAnim:
							break;
						case OpenState_Open:
							self->mOpenState = OpenState_ClosedAnim;
							self->mOpenAnim = 1.0f;
							g_GameState->m_SoundEffects->Play(Resources::SOUND_ZOOM_OUT, 0);
							break;
						case OpenState_OpenAnim:
							break;
					}
				}
				else
				{
					self->mOpenState = OpenState_OpenAnim;
					self->mOpenAnim = 0.0f;
					self->mOpenIndex = index;
					g_GameState->m_SoundEffects->Play(Resources::SOUND_ZOOM_OUT, 0);
				}
			}
		}
		else
		{
			self->mScrollActive = false;
		}
		
		return true;
	}
	
	int View_UpgradeHD::HitItem(const Vec2F& pos)
	{
		for (int i = 0; i < ITEM_COUNT; ++i)
		{
			float y1 = ItemPosV_get(i + 0);
			float y2 = ItemPosV_get(i + 1);
			
			if (pos[1] >= y1 && pos[1] < y2)
				return i;
		}
		
		return -1;
	}
				
	float View_UpgradeHD::ItemPosW_get(int index)
	{
		float result = index * ITEM_SIZE_CLOSED;
		
		if (HasOpenItem_get() && index > mOpenIndex)
		{
			result += ItemSize_get(mOpenIndex) - ITEM_SIZE_CLOSED;
		}
		
		return result;
	}
				
	float View_UpgradeHD::ItemPosV_get(int index)
	{
		return ItemPosW_get(index) + mScrollPosition;
	}
	
	float View_UpgradeHD::ItemSize_get(int index)
	{
		if (index != mOpenIndex)
			return ITEM_SIZE_CLOSED;
		else
			return Calc::Lerp(ITEM_SIZE_CLOSED, ITEM_SIZE_OPENED, mOpenAnim);
	}
	
	float View_UpgradeHD::TotalItemSize_get(bool includeScroll)
	{
		float result = 0.0f;
		
		for (int i = 0; i < ITEM_COUNT; ++i)
		{
			result += ItemSize_get(i);
		}
		
		if (includeScroll)
		{
			float scrollPosition = -mScrollPosition;
			
			if (scrollPosition + VIEW_SY > result)
				result = scrollPosition + VIEW_SY;
			if (scrollPosition < 0.0f)
				result += -scrollPosition;
		}
		
		return result;
	}
	
	bool View_UpgradeHD::HasOpenItem_get()
	{
		return mOpenIndex != -1;
	}
}
