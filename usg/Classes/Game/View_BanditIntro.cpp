#include "BanditEntity.h"
#include "BezierPath.h"
#include "EncouragementTexts.h"
#include "FontMap.h"
#include "GameRound.h"
#include "GameScore.h"
#include "GameState.h"
#include "Graphics.h"
#include "Grid_Effect.h"
#include "MenuMgr.h"
#include "StringBuilder.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"
#include "Util_ColorEx.h"
#include "View_BanditIntro.h"
#include "World.h"

#ifdef BBOS
#include "GameView_BBOS.h"
#endif

namespace Game
{
	static void RenderInfoField(const char* text, Vec2F position, Vec2F itemPosition, float alpha, int screenScale)
	{
		Vec2F pos = itemPosition;
		Vec2F size(104.0f, 21.0f);
		
		// render connector
				
		float tangentSize = 100.0f;
		Vec2F tan1(+tangentSize, 0.0f);
		Vec2F tan2(-tangentSize, 0.0f);
		
		RenderCurve(position, tan1, pos + Vec2F(3.0f, size[1] * 0.5f), tan2, 3.0f, SpriteColor_Make(210, 255, 0, (int)(alpha * 191.0f)), 20, Textures::BANDITVIEW_CURVE);
		
		// render textbox
		
		g_GameState->Render(g_GameState->GetShape(Resources::BANDITVIEW_THINGY), pos, 0.0f, SpriteColors::White);
		RenderText(pos, size, g_GameState->GetFont(Resources::FONT_CALIBRI_SMALL), SpriteColor_Make(255, 255, 255, 255), TextAlignment_Center, TextAlignment_Center, screenScale == 1, text);
	}
	
	class ItemThingy
	{
	public:
		ItemThingy();
		
		void Setup(const Bandits::Link* link, const char* text, Vec2F position);
		Vec2F Position_get();
		Vec2F Offset_get();
		float Alpha_get();
		
		void Update(float dt);
		void Render(int screenScale);
		
		enum State
		{
			State_Idle,
			State_SlideIn,
			State_SlideOut,
			State_Dead
		};
		
		void State_set(State state);
		void StartAnim(float time);
		inline float AnimProgress_get()
		{
			return 1.0f - mAnimTime * mAnimTimeRcp;
		}
		
		State mState;
		const Bandits::Link* mLink;
		const char* mText;
		Vec2F mPosition;
		Vec2F mTarget;
		float mAnimTime;
		float mAnimTimeRcp;
		
		friend class ItemThingyMgr;
	};
	
	ItemThingy::ItemThingy()
	{
		mState = State_Dead;
		mLink = 0;
		mText = 0;
		mAnimTime = 1.0f;
		mAnimTimeRcp = 1.0f;
	}
	
	void ItemThingy::Setup(const Bandits::Link* link, const char* text, Vec2F position)
	{
		Assert(link != 0);
		Assert(text != 0);
		
		mState = State_Dead;
		mLink = link;
		mText = text;
		mPosition = position;
		mTarget = position;
		
		State_set(State_SlideIn);
	}
	
	Vec2F ItemThingy::Position_get()
	{
		return mPosition + Offset_get();
	}
	
	Vec2F ItemThingy::Offset_get()
	{
		switch (mState)
		{
			case State_SlideIn:
				return Vec2F(150.0f * (1.0f - AnimProgress_get()), 0.0f);
			case State_SlideOut:
				return Vec2F(150.0f * AnimProgress_get(), 0.0f);
			default:
				return Vec2F(0.0f, 0.0f);
		}
	}
	
	float ItemThingy::Alpha_get()
	{
		switch (mState)
		{
			case State_SlideIn:
				return AnimProgress_get();
			case State_SlideOut:
				return 1.0f - AnimProgress_get();
			default:
				return 1.0f;
		}
	}
	
	void ItemThingy::Update(float dt)
	{
		if (mState == State_Dead)
			return;
		
		const float v1 = powf(0.1f, dt);
		const float v2 = 1.0f - v1;
		
		if (mPosition[1] == 0.0f)
			mPosition = mTarget;
		else
			mPosition = mPosition * v1 + mTarget * v2;
		
		if (mAnimTime > 0.0f)
		{
			mAnimTime -= dt;
			
			if (mAnimTime <= 0.0f)
			{
				mAnimTime = 0.0f;
				
				switch (mState)
				{
					case State_Idle:
						break;
					case State_SlideIn:
						mState = State_Idle;
						break;
					case State_SlideOut:
						mState = State_Dead;
						break;
					case State_Dead:
						break;
				}
			}
		}
	}
	
	void ItemThingy::Render(int screenScale)
	{
		if (mState == State_Dead)
			return;
		
		Vec2F pos = Position_get();
		
		// todo: render shape @ position
		
		// todo: render text @ position
		
		Vec2F mid(VIEW_SX/2.0f, VIEW_SY/2.0f);
		
		RenderInfoField(mText, mLink->GlobalPosition_get() + mid, Position_get(), Alpha_get(), screenScale);
	}
	
	void ItemThingy::State_set(State state)
	{
		mState = state;
		
		switch (state)
		{
			case State_SlideIn:
				StartAnim(0.8f);
				break;
			case State_SlideOut:
				StartAnim(0.4f);
				break;
			default:
				break;
		}
	}
	
	void ItemThingy::StartAnim(float time)
	{
		mAnimTime = time;
		mAnimTimeRcp = 1.0f / time;
	}
	
	class ItemThingyMgr
	{
	public:
		ItemThingyMgr();

		ItemThingy& Allocate();
		void Clear();
		bool Contains(const Bandits::Link* link);
		ItemThingy* Find(const Bandits::Link* link);
		
		void Update(float dt);
		void Render(int screenScale);
		
	private:
		ItemThingy mPool[10];
		int mPoolIndex;
	};
	
#define POOL_SIZE ((int)(sizeof(mPool) / sizeof(ItemThingy)))
	
	ItemThingyMgr::ItemThingyMgr()
	{
		mPoolIndex = 0;
	}
	
	ItemThingy& ItemThingyMgr::Allocate()
	{
		mPoolIndex = (mPoolIndex + 1) % POOL_SIZE;
		
		return mPool[mPoolIndex];
	}
	
	void ItemThingyMgr::Clear()
	{
		for (int i = 0; i < POOL_SIZE; ++i)
			mPool[i].mState = ItemThingy::State_Dead;
	}
	
	bool ItemThingyMgr::Contains(const Bandits::Link* link)
	{
		for (int i = 0; i < POOL_SIZE; ++i)
			if (mPool[i].mState != ItemThingy::State_Dead && mPool[i].mLink == link)
				return true;
		
		return false;
	}
	
	ItemThingy* ItemThingyMgr::Find(const Bandits::Link* link)
	{
		for (int i = 0; i < POOL_SIZE; ++i)
			if (mPool[i].mState != ItemThingy::State_Dead && mPool[i].mLink == link)
				return &mPool[i];
		
		return 0;
	}
	
	void ItemThingyMgr::Update(float dt)
	{
		Vec2F pos(VIEW_SX - 120.0f, 40.0f);
		Vec2F size(4.0f, 20.0f);
		
		for (int i = 0; i < POOL_SIZE; ++i)
		{
			if (mPool[i].mState == ItemThingy::State_Dead)
				continue;
			
			mPool[i].mTarget = pos;
			
			mPool[i].Update(dt);
			
			pos += size;
		}
	}
	
	void ItemThingyMgr::Render(int screenScale)
	{
		for (int i = 0; i < POOL_SIZE; ++i)
			mPool[i].Render(screenScale);
	}
	
	static ItemThingyMgr gItemThingyMgr;
	
	//
	
	View_BanditIntro::View_BanditIntro(int screenScale)
	{
		m_ScreenScale = screenScale;
		m_MaxiPreview = 0;
	}

	View_BanditIntro::~View_BanditIntro()
	{
		delete m_MaxiPreview;
		m_MaxiPreview = 0;
	}
	
	void View_BanditIntro::Initialize()
	{
		m_BackDropTimer.Initialize(g_GameState->m_TimeTracker_Global, false);
		m_SlideTimer.Initialize(g_GameState->m_TimeTracker_Global, false);
		m_ProgressSavedTimer.Initialize(g_GameState->m_TimeTracker_Global, false);
		m_TouchToContinueTimer.Initialize(g_GameState->m_TimeTracker_Global, false);
		
		m_BackDrop = g_GameState->GetTexture(Textures::MENU_BANDIT_BACK);
		m_EncouragementText = 0;
		
		m_MaxiPreview = 0;
	}
	
	// --------------------
	// View
	// --------------------
	
	void View_BanditIntro::Update(float dt)
	{
		g_World->m_GridEffect->BaseHue_set(g_GameState->m_TimeTracker_Global->Time_get() / 50.0f);
		g_World->m_GridEffect->Impulse(Vec2F(Calc::Random(0.0f, (float)WORLD_SX), Calc::Random(0.0f, (float)WORLD_SY)), 0.1f);
		
		m_MaxiPreview->Update(0.00001f);
		
		UpdateBanditInfo();
		
		gItemThingyMgr.Update(dt);
	}
	
	void View_BanditIntro::Render()
	{
		RenderBackground();
		RenderPrimary();
	}
	
	int View_BanditIntro::RenderMask_get()
	{
		return RenderMask_Interface | RenderMask_Particles | RenderMask_WorldBackground;
	}
	
	float View_BanditIntro::FadeTime_get()
	{
		return 0.5f;
	}
	
	void View_BanditIntro::HandleFocus()
	{
		g_GameState->Interface_get()->ActiveMenu_set(GameMenu::MenuType_BanditIntro);
		
//		g_GameState->m_TouchDLG.Enable(TOUCH_PRIO_BANDITANNOUNCE);
		
		m_BackDropTimer.Start(AnimTimerMode_TimeBased, true, 0.5f, AnimTimerRepeat_None);
		m_SlideTimer.Start(AnimTimerMode_TimeBased, true, 0.5f, AnimTimerRepeat_None);
		m_ProgressSavedTimer.Start(AnimTimerMode_TimeBased, true, 2.0f, AnimTimerRepeat_None);
		m_TouchToContinueTimer.Start(AnimTimerMode_TimeBased, true, 0.8f, AnimTimerRepeat_Loop);
		
		GameText::GetEncouragementText(true, &m_EncouragementText);
/*		const char** encArray;
		int encArraySize;
		GameText::GetEncouragementTextArray(true, &encArray, encArraySize);
		int index = Calc::Random(encArraySize);
		m_EncouragementText = encArray[index];*/
		m_EncouragementTextSx = g_GameState->GetFont(Resources::FONT_CALIBRI_SMALL)->m_Font.MeasureText(m_EncouragementText);
		
		SetupMaxiPreview();
		
		gItemThingyMgr.Clear();
		
		m_IsActive = true;
	}
	
	void View_BanditIntro::HandleFocusLost()
	{
		m_IsActive = false;
		
		m_BackDropTimer.Start(AnimTimerMode_TimeBased, true, 0.3f, AnimTimerRepeat_None);
		m_SlideTimer.Start(AnimTimerMode_TimeBased, true, 0.3f, AnimTimerRepeat_None);
		
//		Game::g_World->m_PlayerController->TiltCalibrate();
		
//		g_GameState->m_TouchDLG.Disable(TOUCH_PRIO_BANDITANNOUNCE);
	}
	
	// --------------------
	// Boss preview
	// --------------------
	
	void View_BanditIntro::SetupMaxiPreview()
	{
		delete m_MaxiPreview;
		m_MaxiPreview = 0;
		
		Res* res = g_GameState->m_ResMgr.Get(g_GameState->m_GameRound->Classic_GetMaxiBossType());
		int level = g_GameState->m_GameRound->Classic_Level_get();
		Bandits::EntityBandit* bandit = new Bandits::EntityBandit();
		bandit->Initialize();
		bandit->IsAlive_set(XTRUE);
		bandit->Setup(res, level, Calc::mPI2, 0);
		bandit->DoPosition_set(Vec2F(0.0f, 0.0f));
//		bandit->Update(0.00001f);
		
		m_MaxiPreview = bandit;
	}
	
	// --------------------
	// Animation
	// --------------------
	
	float View_BanditIntro::AnimationOffset_get()
	{
		if (m_IsActive)
			return m_SlideTimer.Progress_get();
		else
			return 1.0f - m_SlideTimer.Progress_get();
	}
	
	// --------------------
	// Logic
	// --------------------
	
	void View_BanditIntro::UpdateBanditInfo()
	{
		Bandits::Link** links = m_MaxiPreview->Bandit_get()->LinkArray_get();
		int linkCount = m_MaxiPreview->Bandit_get()->LinkArraySize_get();
		
		float angle1;
		float angle2;
		
		GetScannerAngles(angle1, angle2);
		
		const PlaneF clip1 = PlaneF::FromPoints(Vec2F(0.0f, 0.0f), Vec2F::FromAngle(angle1 + Calc::mPI2));
		const PlaneF clip2 = PlaneF::FromPoints(Vec2F(0.0f, 0.0f), Vec2F::FromAngle(angle2 + Calc::mPI2));
		
		for (int i = 0; i < linkCount; ++i)
		{
			const Bandits::Link* link = links[i];
			
			if (!link->IsAlive_get())
				continue;
			
			const Vec2F pos = link->GlobalPosition_get();
			
			bool visible = true;
			
			if (clip1.Distance(pos) < 0.0f)
				visible = false;
			if (clip2.Distance(pos) > 0.0f)
				visible = false;
			
			ItemThingy* thingy = gItemThingyMgr.Find(link);
			
			if (thingy)
			{
				if (!visible && thingy->mState == ItemThingy::State_Idle)
					thingy->State_set(ItemThingy::State_SlideOut);
				else
					continue;
			}
			else
			{
				if (!visible)
					continue;
			
				const char* text = 0;
				
				if (link->LinkType_get() == Bandits::LinkType_Weapon)
				{
					Bandits::Weapon type = link->WeaponType_get();
					
					switch (type)
					{
						case Bandits::Weapon_Beam:
							text = "laser beam";
							break;
						case Bandits::Weapon_BlueSpray:
							text = "spray cannon";
							break;
						case Bandits::Weapon_Missile:
							text = "missiles";
							break;
						case Bandits::Weapon_PurpleSpray:
							text = "poisonous gas";
							break;
						case Bandits::Weapon_Vulcan:
							text = "vulcan cannon";
							break;
						case Bandits::Weapon_Undefined:
							break;
					}
				}
			
				if (text)
				{
					ItemThingy& item = gItemThingyMgr.Allocate();
					
					item.Setup(link, text, Vec2F());
				}
			}
		}
	}
	
	// --------------------
	// Drawing
	// --------------------
	
	void View_BanditIntro::RenderBackground()
	{
		float a;
		
		if (m_IsActive)
			a = 1.0f - m_BackDropTimer.Progress_get();
		else
			a = m_BackDropTimer.Progress_get();
		
		a *= 0.9f;
		
		RenderRect(Vec2F(0.0f, 0.0f), Vec2F((float)VIEW_SX, (float)VIEW_SY), 0.0f, 0.0f, 0.0f, a, m_BackDrop);
	}
	
	void View_BanditIntro::RenderPrimary()
	{
		float t = AnimationOffset_get();
		
		float offset = VIEW_SY * t;
		float bannerOffset = 200.0f * t;
		
		Vec2F offsetVector(0.0f, offset);
		
		// render warning banners
		
		float scroll = g_GameState->m_TimeTracker_Global->Time_get() * 40.0f;
		RenderBanner(Vec2F(0.0f, 80.0f - bannerOffset), -scroll);
		RenderBanner(Vec2F(0.0f, VIEW_SY + 120 + bannerOffset), scroll);
		
		// render text scroller
		
		if (m_IsActive)
		{
			RenderScroller(Vec2F(0.0f, 5.0f), -scroll);
		}
		
		if (m_IsActive && !m_SlideTimer.IsRunning_get())
		{
			// render bandit info
			
			RenderBanditInfo();
		}
		
		// render maxi preview
		
		RenderMaxiPreview();
		
		{
			StringBuilder<32> sb;
			sb.AppendFormat("level %d", g_GameState->m_GameRound->Classic_Level_get());
			RenderText(offsetVector + Vec2F(VIEW_SX/2.0f, VIEW_SY-120.0f), Vec2F(), g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE), SpriteColors::White, TextAlignment_Center, TextAlignment_Top, true, sb.ToString());
		}
		
		// render progress saved thingy
		
#if !defined(PSP_UI)
		if (g_GameState->m_GameRound->Modifier_Difficulty_get() != Difficulty_Custom)
			RenderText(Vec2F(0.0f, 25.0f), Vec2F((float)VIEW_SX, 20.0f), g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColor_MakeF(1.0f, 1.0f, 1.0f, m_ProgressSavedTimer.Progress_get()), TextAlignment_Left, TextAlignment_Top, true, "progress saved..");

//		if (g_GameState->m_GameSettings.m_ControllerType == ControllerType_Tilt)
//			RenderText(Vec2F(0.0f, 50.0f), Vec2F((float)VIEW_SX, 20.0f), g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColor_MakeF(1.0f, 1.0f, 1.0f, m_ProgressSavedTimer.Progress_get()), TextAlignment_Left, TextAlignment_Top, true, "tilt calibrated..");
#endif

		// render animated touch (press X) to continue text
		
#ifdef PSP_UI
		const char* text = "PRESS " PSPGLYPH_X " TO CONTINUE";
#elif defined(BBOS)
		const char* text;
		if (gGameView->m_GamepadIsEnabled)
			text = "PRESS [X] TO CONTINUE";
		else
			text = "TOUCH TO CONTINUE";
#else
		const char* text = "TOUCH TO CONTINUE";
#endif

		RenderText(offsetVector + Vec2F(8.0f, VIEW_SY-45.0f), Vec2F(0.0f, 0.0f), g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColor_MakeF(1.0f, 1.0f, 1.0f, m_TouchToContinueTimer.Progress_get()), TextAlignment_Left, TextAlignment_Bottom, true, text);
		
		// render mutation #
		
		{
			StringBuilder<32> sb;
			sb.AppendFormat("evolution: %03d", g_GameState->m_GameRound->Classic_Level_get());
			RenderText(offsetVector + Vec2F(8.0f, VIEW_SY-25.0f), Vec2F(0.0f, 0.0f), g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColors::White, TextAlignment_Left, TextAlignment_Bottom, true, sb.ToString());
		}
		
		// render score
		
		{
			StringBuilder<32> sb;
			sb.AppendFormat("score: %06d", g_GameState->m_Score->Score_get());
			RenderText(offsetVector + Vec2F(18.0f, VIEW_SY-13.0f), Vec2F(0.0f, 0.0f), g_GameState->GetFont(Resources::FONT_LGS), SpriteColors::White, TextAlignment_Left, TextAlignment_Bottom, true, sb.ToString());
		}
		
		// render small text: waves, difficulty, encouragement text
		
		{
			SpriteColor smallColor = SpriteColor_Make(255, 255, 255, (int)(150.0f * (1.0f - m_SlideTimer.Progress_get())));
		
			StringBuilder<32> sb;
			sb.AppendFormat("waves: %d", g_GameState->m_GameRound->Classic_WaveCount_get());
			const float margin = 4.0f;
			RenderText(offsetVector + Vec2F(0.0f, VIEW_SY-60.0f), Vec2F(VIEW_SX-margin, 10.0f), g_GameState->GetFont(Resources::FONT_LGS), smallColor, TextAlignment_Right, TextAlignment_Top, true, sb.ToString());
			RenderText(offsetVector + Vec2F(0.0f, VIEW_SY-50.0f), Vec2F(VIEW_SX-margin, 10.0f), g_GameState->GetFont(Resources::FONT_LGS), smallColor, TextAlignment_Right, TextAlignment_Top, true, DifficultyToString());
		}
	}
	
	void View_BanditIntro::RenderMaxiPreview()
	{
		/*float offset = 0.0f;
		
		if (m_IsActive)
			offset = 640.0f * m_SlideTimer.Progress_get();
		else
			offset = -640.0f * (1.0f - m_SlideTimer.Progress_get());*/
		
		SpriteGfx& gfx = *g_GameState->m_SpriteGfx;
		gfx.Flush();

		gGraphics.BlendModeSet(BlendMode_Normal_Transparent_Add);
		gGraphics.MatrixPush(MatrixType_World);
		gGraphics.MatrixTranslate(MatrixType_World, VIEW_SX/2.0f, VIEW_SY/2.0f, 0.0f);
		
		float scale = 1.0f - AnimationOffset_get();
		
		gGraphics.MatrixScale(MatrixType_World, scale, scale, 1.0f);
		
		float angle = g_GameState->m_TimeTracker_Global->Time_get() * 5.0f;
//		glRotatef(angle, 0.0f, 0.0f, 1.0f);
		m_MaxiPreview->DoRotation_set(Calc::DegToRad(angle));
		m_MaxiPreview->Bandit_get()->RootLink_get()->Update(0.001f); // fixme, update transform
		
		m_MaxiPreview->Render();
//		m_MaxiPreview->Update(0.01f);
		
		gfx.Flush();
		
		gGraphics.MatrixPop(MatrixType_World);
		gGraphics.BlendModeSet(BlendMode_Normal_Transparent);
	}
	
	void View_BanditIntro::RenderBanner(Vec2F offset, float scroll)
	{
		float angle = -30.0f;
		Vec2F dir = Vec2F::FromAngle(angle);
		
		float sx = 1000.0f;
		float sy = 70.0f;
		
		SpriteGfx& gfx = *g_GameState->m_SpriteGfx;
		
		gfx.Flush();
		
		gGraphics.MatrixPush(MatrixType_World);
		//gGraphics.MatrixIdentity();
		Mat4x4 mat;
		mat.MakeIdentity();
		gGraphics.MatrixSet(MatrixType_World, mat);
		gGraphics.MatrixTranslate(MatrixType_World, offset[0], offset[1], 0.0f);
		gGraphics.MatrixRotate(MatrixType_World, Calc::DegToRad(angle), 0.0f, 0.0f, 1.0f);
		gGraphics.MatrixTranslate(MatrixType_World, -100.0f, 0.0f, 0.0f);
		
		float v = 0.1f;
		
		RenderRect(Vec2F(0.0f, 0.0f), Vec2F(sx, sy), v * 0.5f, v * 0.5f, v * 0.5f, 1.0f, g_GameState->GetTexture(Textures::MENU_COLOR_WHITE));
		RenderRect(Vec2F(0.0f, 0.0f - 3.0f), Vec2F(sx, 3.0f), v, v, v, 1.0f, g_GameState->GetTexture(Textures::MENU_COLOR_WHITE));
		RenderRect(Vec2F(0.0f, sy), Vec2F(sx, 3.0f), v, v, v, 1.0f, g_GameState->GetTexture(Textures::MENU_COLOR_WHITE));
		
		const FontMap* font = g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE);
		
		const char* text = "WARNING... EXTREME DANGER... ENEMY DETECTED... ";
		float tsx = font->m_Font.MeasureText(text);
		int n = (int)Calc::DivideUp(sx, tsx) + 1;
		
		scroll = fmodf(scroll, tsx) - tsx;
		
		float x = scroll;
		
		for (int i = 0; i < n; ++i)
		{
			RenderText(Vec2F(x, 0.0f), Vec2F(0.0f, sy), font, SpriteColor_Make(50, 50, 50, 255), TextAlignment_Left, TextAlignment_Center, false, text);
//			RenderText(Vec2F(x, 0.0f), Vec2F(0.0f, sy), font, SpriteColor_Make(255, 255, 0, 255), TextAlignment_Left, TextAlignment_Center, false, text);
			
			x += tsx;
		}
		
		gfx.Flush();
		
		gGraphics.MatrixPop(MatrixType_World);
	}
	
	void View_BanditIntro::RenderScroller(Vec2F pos, float scroll)
	{
		float sy = 20.0f;
		float spaceSx = m_EncouragementTextSx + 100.0f;
		
		RenderRect(pos, Vec2F((float)VIEW_SX, sy), 0.0f, 0.0f, 0.0f, 1.0f, g_GameState->GetTexture(Textures::MENU_COLOR_WHITE));
		
		scroll = fmodf(scroll, spaceSx) - spaceSx;
		
		pos[0] += scroll;
		
		while (pos[0] < VIEW_SX)
		{
			RenderText(pos, Vec2F(0.0f, sy), g_GameState->GetFont(Resources::FONT_CALIBRI_SMALL), SpriteColor_Make(255, 127, 63, 255), TextAlignment_Left, TextAlignment_Center, true,
					   m_EncouragementText);
			
			pos[0] += spaceSx;
		}
	}
	
	void View_BanditIntro::GetScannerAngles(float& oAngle1, float& oAngle2)
	{
		float angle = g_GameState->m_TimeTracker_Global->Time_get() * 360.0f / 10.0f;
		
		oAngle1 = Calc::DegToRad(angle);
		oAngle2 = Calc::DegToRad(angle + 90.0f);
	}
	
	void View_BanditIntro::RenderBanditInfo()
	{
		Vec2F mid(VIEW_SX/2.0f, VIEW_SY/2.0f);
		
		float angle1;
		float angle2;
		
		GetScannerAngles(angle1, angle2);
		
		// todo: render sweep
		
		//SpriteColor baseColor = Calc::Color_FromHSB(g_World->m_GridEffect.BaseHue_get(), 1.0f, 1.0f);
		
		for (float i = 1.0f; i < 4.0f; i += 0.8f)
		{
			//SpriteColor color = SpriteColor_Make(255, 0, 200 / i, 100);
			SpriteColor color = Calc::Color_FromHue(g_World->m_GridEffect->BaseHue_get() - 1.0f / 6.0f + i / 10.0f);
//			color.v[3] = 100;
			color.v[3] = (uint8_t)(127 / sqrtf(i));
//			color.v[0] /= i;
//			color.v[1] /= i;
//			color.v[2] /= i;
			
			RenderSweep2(mid, angle1 * i, angle2 * i, 90.0f + 40.0f * i, 130.0f + 30.0f * i, color, Textures::BANDITVIEW_SWEEP);
		}
		
		// render info buttons
		
		gItemThingyMgr.Render(m_ScreenScale);
	}
}
