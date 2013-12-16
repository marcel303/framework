#include "GameSettings.h"
#include "GameState.h"
#include "Mat3x2.h"
#include "MenuMgr.h"
#include "SoundEffectMgr.h"
#include "TempRender.h"
#include "TouchDLG.h"
#include "TouchInfo.h"
#include "UsgResources.h"
#include "Util_ColorEx.h"
#include "View_Credits.h"
#include "World.h"

#if defined(BBOS) || 1
	#define MINICREDITS 1
#else
	#define MINICREDITS 0
#endif

#if defined(BBOS)
	#include <bps/navigator.h>
	#define SHOWURL 0
#endif

#define MIN_SCROLL_POSITION (VIEW_SY + 10.0f)

#define SPACING_CAT 60.0f
#define SPACING_NAME 28.0f

namespace Game
{
	struct CreditLine
	{
		int type;
		const char* text;
	};
	
	static CreditLine lines[] =
	{
		{ 0, "Production" },
		{ 1, "Grannies Games" },
		{ 0, "Tech Programming" },
		{ 1, "Marcel Smit" },
		{ 1, "David van Spengen" },
		{ 0, "Game Design" },
		{ 1, "Marcel Smit" },
		{ 1, "David van Spengen" },
		{ 0, "Music" },
		{ 1, "www.stockmusic.net" },
		{ 1, "www.imphenzia.com"},
		{ 0, "Sound Effects" },
		{ 1, "David van Spengen" },
		{ 0, "Fonts" },
		{ 1, "Dan Zadorozny" },
		{ 1, "Iconian Fonts" },
		{ 0, "Game Testing" },
		{ 1, "Joost Meulenkamp" },
		{ 1, "Alexander Immerzeel" },
		{ 1, "Johan Boer" },
		{ 1, "Jan Martijn Metselaar" },
		{ 1, "Alex Bongers" },
		{ 1, "Laurens Schmitz" },
		{ 1, "Many thanks to all testers!" },
		{ 0, "Tester Extraordinaire" },
		{ 1, "Nigel Ignacio" },
		{ 0, "Snake and kamikaze art" },
		{ 1, "Alexander Immerzeel" },
		{ 0, "Special Thanks" },
		{ 1, "Winitu Consulting for" },
		{ 1, "letting us use" },
		{ 1, "their office space.." },
		{ 0, "visit us at" },
		{ 1, "www.grannies-games.com!" },
	};
	
	View_Credits::View_Credits()
	{
	}
	
	void View_Credits::Initialize()
	{
		m_CreditsCount = sizeof(lines) / sizeof(CreditLine);
		m_CreditsSize = 0.0f;
		for (int i = 0; i < m_CreditsCount; ++i)
			m_CreditsSize += CalcSpacing(i);
		
		m_ScrollPosition = 0.0f;
		m_TextVisibility = 0.f;
		
		TouchListener listener;
		listener.Setup(this, HandleTouchBegin, HandleTouchEnd, HandleTouchMove);
		g_GameState->m_TouchDLG->Register(USG::TOUCH_PRIO_CREDITS, listener);
	}
	
	// --------------------
	// View
	// --------------------
	
	class Springy
	{
	public:
		Springy()
			: mTarget(1.f)
			, mValue(1.f)
			, mRigidity(0.f)
			, mSpeed(0.f)
			, mDamping(0.f)
			, mTimer(0.f)
		{
		}

		void Initialize(float target, float rigidity, float damping)
		{
			mTarget = target;
			mValue = target;
			mRigidity = rigidity;
			mDamping = damping;
			mTimer = 0.f;
		}

		void Update(float dt)
		{
			float delta = mTarget - mValue;
			float force = delta * mRigidity;
			mSpeed += force * dt;
			mSpeed *= powf(mDamping, dt);
			mValue += mSpeed * dt;

			mTimer -= dt;
			if (mTimer < 0.f)
				mTimer = 0.f;
		}

		void Target_set(float target)
		{
			mTarget = target;
		}

		void Value_set(float value)
		{
			mValue = value;
		}

		float Value_get()
		{
			return mValue;
		}

		void Timer_set(float value)
		{
			mTimer = value;
		}

		float Timer_get()
		{
			return mTimer;
		}

		float mTarget;
		float mValue;
		float mRigidity;
		float mSpeed;
		float mDamping;
		float mTimer;
	};

	static Springy sSpringyScale;
	static Springy sSpringyX;
	static Springy sSpringyY;
	static Springy sSpringyRot;
	static Springy sSpringyScale2;

	static RectF sLinkArea[1];

	static void DoRandomAction()
	{
		int what = rand() % 10;

		if (what == 0)
		{
			sSpringyX.Target_set(rand() % 50);
			sSpringyX.Timer_set(0.5f);
		}
		if (what == 1)
		{
			sSpringyY.Target_set(rand() % 20);
			sSpringyY.Timer_set(0.5f);
		}
		if (what >= 2 && what <= 5)
		{
			sSpringyRot.Target_set(Calc::Random(-0.2f, +0.5f));
			sSpringyRot.Timer_set(0.5f);
		}
		if (what >= 6 && what <= 9)
		{
			sSpringyScale2.Target_set(Calc::Random(0.5f, 1.3f));
			sSpringyRot.Timer_set(1.0f);
		}
	}

	void View_Credits::Update(float dt)
	{
		m_ScrollPosition -= dt * 30.0f;

		m_TextVisibility += dt * 1.5f;
		if (m_TextVisibility > 1.f)
			m_TextVisibility = 1.f;

		int doSomething = rand() % 500;

		if (doSomething == 0)
		{
			DoRandomAction();
		}

		sSpringyScale.Update(dt);
		sSpringyX.Update(dt);
		sSpringyY.Update(dt);
		sSpringyRot.Update(dt);
		sSpringyScale2.Update(dt);

		if (sSpringyX.Timer_get() == 0.f)
			sSpringyX.Target_set(0.f);
		if (sSpringyY.Timer_get() == 0.f)
			sSpringyY.Target_set(0.f);
		if (sSpringyRot.Timer_get() == 0.f)
			sSpringyRot.Target_set(0.f);
		if (sSpringyScale2.Timer_get() == 0.f)
			sSpringyScale2.Target_set(1.f);
	}

	static Mat3x2 sDrawMat;

	static float DrawTitle(float x, float y, float a, const char * text)
	{
		SpriteColor color = SpriteColor_MakeF(0.9f, 0.9f, 0.9f, a);
		float scale = 0.75f;

		Mat3x2 matS; matS.MakeScaling(scale, scale);
		Mat3x2 matT; matT.MakeTranslation(Vec2F(x, y));
		Mat3x2 mat = sDrawMat * matT * matS;

		RenderText(mat, g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE), color, TextAlignment_Left, TextAlignment_Top, text);

		return 23.f;
	}

	static float DrawName(float x, float y, float a, const char * text)
	{
		float v = 0.75f;

		SpriteColor color = SpriteColor_MakeF(v, v, v, a);
		float scale = 0.7f;

		Mat3x2 matS; matS.MakeScaling(scale, scale);
		Mat3x2 matT; matT.MakeTranslation(Vec2F(x + 15.f, y));
		Mat3x2 mat = sDrawMat * matT * matS;

		RenderText(mat, g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE), color, TextAlignment_Left, TextAlignment_Top, text);

		return 20.f;
	}

	static float DrawSpacing()
	{
		return 15.f;
	}

	void View_Credits::Render()
	{
#if MINICREDITS
		{
			Mat3x2 transMat;
			Mat3x2 scaleMat;
			Mat3x2 transMat2;
			Mat3x2 rotMat;

			transMat.MakeTranslation(Vec2F(140.f, 90.f));

			const float scale = sSpringyScale.Value_get();
			scaleMat.MakeScaling(scale, scale);

			transMat2.MakeTranslation(Vec2F(sSpringyX.Value_get(), sSpringyY.Value_get()));
			Mat3x2 pivot2;
			pivot2.MakeTranslation(Vec2F(+200.f, +50.f));
			rotMat.MakeRotation(sSpringyRot.Value_get());
			Mat3x2 scale2Mat;
			const float scale2 = sSpringyScale2.Value_get();
			scale2Mat.MakeScaling(scale2, scale2);
			Mat3x2 pivot1;
			pivot1.MakeTranslation(Vec2F(-200.f, -50.f));

			sDrawMat = scaleMat * transMat * transMat2 * pivot2 * rotMat * scale2Mat * pivot1;

			g_GameState->Render(g_GameState->GetShape(Resources::MAINVIEW_LOGO), Vec2F(10.0f, 5.0f), 0.0f, SpriteColors::White);

			float x = 0.f;
			float y = 0.f;

			y += DrawTitle(x, y, m_TextVisibility, "Code & Design");
			y += DrawName(x, y, m_TextVisibility, "Marcel Smit");
			y += DrawName(x, y, m_TextVisibility, "David van Spengen");
			y += DrawSpacing();

			y += DrawTitle(x, y, m_TextVisibility, "Music");
			y += DrawName(x, y, m_TextVisibility, "Din Stalker");
			y += DrawSpacing();

			sDrawMat = transMat;

#if SHOWURL
			x -= 10.f;

			float y1 = y;
			y += DrawName(x, y, m_TextVisibility, "Music: insert name [link]");
			float y2 = y;
			float x1 = x;
			float x2 = x + VIEW_SX;
			Vec2F p1 = sDrawMat * Vec2F(x1, y1);
			Vec2F p2 = sDrawMat * Vec2F(x2, y2);
			sLinkArea[0].Setup(p1, p2-p1);
#endif

			x += 10.f;
		}

		return;
#endif

		//

		SpriteColor color_Name = SpriteColors::White;
		
		float y = m_ScrollPosition;
		
		for (int i = 0; i < m_CreditsCount; ++i)
		{
			SpriteColor color_Cat = SpriteColor_BlendF(SpriteColors::White, Calc::Color_FromHue(y / VIEW_SY), 0.75f);
													
			y += CalcSpacing(i);
			
			SpriteColor color;
			
			if (lines[i].type == 0)
				color = color_Cat;
			else
				color = color_Name;
			
			RenderText(Vec2F(0.0f, y), Vec2F((float)VIEW_SX, 0.0f), g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE), color, TextAlignment_Center, TextAlignment_Top, false, lines[i].text);
			
			if (i == m_CreditsCount - 1 && y < -50.0f)
				Reset();
		}
	}

	int View_Credits::RenderMask_get()
	{
		return RenderMask_Interface | RenderMask_Particles | RenderMask_WorldBackground;
	}
	
	void View_Credits::HandleFocus()
	{
		g_GameState->Interface_get()->ActiveMenu_set(GameMenu::MenuType_Credits);
		g_GameState->m_TouchDLG->Enable(USG::TOUCH_PRIO_CREDITS);
	
		Reset();
	}
	
	void View_Credits::HandleFocusLost()
	{
		g_GameState->m_TouchDLG->Disable(USG::TOUCH_PRIO_CREDITS);
	}
	
	void View_Credits::Show(View nextView)
	{
		m_NextView = nextView;
		
		g_GameState->ActiveView_set(::View_Credits);
	}
	
	// --------------------
	// Credits
	// --------------------
	float View_Credits::CalcSpacing(int index)
	{
		if (index > 0)
		{
			if (lines[index].type == 0)
				return SPACING_CAT;
			else
				return SPACING_NAME;
		}
		
		return 0.0f;
	}
	
	void View_Credits::Reset()
	{
		m_ScrollPosition = MIN_SCROLL_POSITION;
		m_TextVisibility = 0.f;

		sSpringyScale.Initialize(1.f, 10.f, 0.05f);
		sSpringyX.Initialize(0.f, 100.f, 0.01f);
		sSpringyY.Initialize(0.f, 100.f, 0.01f);
		sSpringyRot.Initialize(0.f, 200.f, 0.01f);
		sSpringyScale2.Initialize(1.f, 50.f, 0.02f);

		sSpringyRot.Value_set(-0.5f);
		sSpringyScale.Value_set(0.1f);
	}
	
	// --------------------
	// Touch related
	// --------------------
	bool View_Credits::HandleTouchBegin(void* obj, const TouchInfo& touchInfo)
	{
#if MINICREDITS
#ifdef BBOS
		if (SHOWURL && sLinkArea[0].IsInside(touchInfo.m_LocationView))
		{
			navigator_invoke("http://www.google.com", 0);
		}
		else
#endif
		{
			g_GameState->m_SoundEffects->Play(Resources::SOUND_ZOOM_IN, SfxFlag_MustFinish);

			DoRandomAction();
		}
#endif

		return true;
	}
	
	bool View_Credits::HandleTouchMove(void* obj, const TouchInfo& touchInfo)
	{
		View_Credits* self = (View_Credits*)obj;
		
		self->m_ScrollPosition += touchInfo.m_LocationDelta[1];
		
		if (self->m_ScrollPosition > MIN_SCROLL_POSITION)
			self->m_ScrollPosition = MIN_SCROLL_POSITION;
		
		g_World->SpawnZoomParticles(touchInfo.m_Location, 10);
		
		return true;
	}
	
	bool View_Credits::HandleTouchEnd(void* obj, const TouchInfo& touchInfo)
	{
		return true;
	}
}
