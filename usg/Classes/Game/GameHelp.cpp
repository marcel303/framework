#include "GameHelp.h"
#include "GameRound.h"
#include "GameState.h"
#include "Mat3x2.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"
#include "World.h"

#define HELP_VOICEOVER 0

#if HELP_VOICEOVER
	#include "SoundPlayer_OpenAL.h"
#endif

namespace Game
{
	enum ActionType
	{
		AT_AudioPlay,
		AT_AudioPlayWait,
		AT_AudioWait,
		AT_StateTodo,
		AT_StateWait,
		AT_TimeWait,
		AT_MusicFadeAndWait,
		AT_MusicDisable,
		AT_MusicEnable,
		AT_IntroEnemy,
		AT_EnemyWait,
		AT_IntroBoss
	};

	class Action
	{
	public:
		Action(ActionType type, int param1 = -1, int param2 = -1)
		{
			Desc = 0;
			Type = type;
			Param1 = param1;
			Param2 = param2;
		}

		Action(const char* desc, ActionType type, int param1 = -1, int param2 = -1)
		{
			Desc = desc;
			Type = type;
			Param1 = param1;
			Param2 = param2;
		}

		const char* Desc;
		ActionType Type;
		int Param1;
		int Param2;
	};

#if /*!defined(IPAD) && */!defined(WIN32) && !defined(MACOS)
	const static Action s_ActionList[] =
	{
		Action(AT_StateTodo, HelpState::State_HitMove),
		Action(AT_StateWait),
		Action(AT_StateTodo, HelpState::State_HitFire),
		Action(AT_StateWait),
		Action(AT_StateTodo, HelpState::State_HitSpecial),
		Action(AT_StateWait),
		Action(AT_StateTodo, HelpState::State_HitUpgrade),
		Action(AT_StateWait),
		Action(AT_StateTodo, HelpState::State_HitWeaponSwitch),
		Action(AT_StateWait),
		Action(AT_IntroEnemy, EntityClass_Mine),
		Action(AT_EnemyWait),
		Action(AT_TimeWait, 1.0f),
		Action(AT_IntroEnemy, EntityClass_EvilTriangle),
		Action(AT_EnemyWait),
		Action(AT_TimeWait, 1.0f),
		Action(AT_IntroEnemy, EntityClass_Kamikaze),
		Action(AT_EnemyWait),
		Action(AT_TimeWait, 1.0f),
		Action(AT_IntroEnemy, EntityClass_EvilTriangleBiggy),
		Action(AT_EnemyWait),
		Action(AT_TimeWait, 1.0f),
		Action(AT_IntroEnemy, EntityClass_EvilTriangleExtreme),
		Action(AT_EnemyWait),
		Action(AT_TimeWait, 1.0f),
		Action(AT_IntroEnemy, EntityClass_EvilSquare),
		Action(AT_EnemyWait),
		Action(AT_TimeWait, 1.0f),
		Action(AT_IntroEnemy, EntityClass_BorderPatrol),
		Action(AT_EnemyWait),
		Action(AT_TimeWait, 1.0f),
		Action(AT_IntroEnemy, EntityClass_EvilSquareBiggy),
		Action(AT_EnemyWait),
		Action(AT_TimeWait, 1.0f),
		Action(AT_IntroEnemy, EntityClass_Shield),
		Action(AT_EnemyWait),
		Action(AT_TimeWait, 1.0f),
		Action(AT_IntroBoss)
	};
#else
	const static Action s_ActionList[] =
	{
#if 0 // bgm fade test
		Action(AT_MusicFadeAndWait, 5000, 0),
		Action(AT_MusicFadeAndWait, 5000, 100),
		Action(AT_MusicFadeAndWait, 5000, 0),
		Action(AT_MusicFadeAndWait, 5000, 100),
#endif
#if 1
		Action("welcome", AT_AudioPlayWait, Resources::VO_TUT_01),
		Action(AT_TimeWait, 2000),
		Action("control methods", AT_AudioPlayWait, Resources::VO_TUT_02),
		Action(AT_TimeWait, 2000),
		Action("let's try moving around", AT_AudioPlayWait, Resources::VO_TUT_03),
		Action(AT_StateTodo, HelpState::State_HitMove),
		Action(AT_StateWait),
		Action(AT_TimeWait, 2000),
		Action("dual stick", AT_AudioPlayWait, Resources::VO_TUT_04),
		Action(AT_StateTodo, HelpState::State_HitFire),
		Action(AT_StateWait),
		Action(AT_TimeWait, 2000),
		Action("tilt control", AT_AudioPlayWait, Resources::VO_TUT_05),
		Action(AT_TimeWait, 2000),
		Action("control switch", AT_AudioPlayWait, Resources::VO_TUT_06),
		Action(AT_TimeWait, 2000),
		Action("control switch 2", AT_AudioPlayWait, Resources::VO_TUT_07),
		Action(AT_TimeWait, 2000),
		Action("look at weapons", AT_AudioPlayWait, Resources::VO_TUT_08),
		Action(AT_TimeWait, 2000),
		Action("vulcan", AT_AudioPlayWait, Resources::VO_TUT_09),
		Action(AT_TimeWait, 2000),
		Action("vulcan try out", AT_AudioPlayWait, Resources::VO_TUT_10),
		Action(AT_TimeWait, 2000),
		Action(AT_StateTodo, HelpState::State_HitFire),
		Action(AT_StateWait),
		Action("laser", AT_AudioPlayWait, Resources::VO_TUT_11),
		Action(AT_StateTodo, HelpState::State_HitWeaponSwitch),
		Action(AT_StateWait),
		Action(AT_TimeWait, 2000),
		Action("laser try out", AT_AudioPlayWait, Resources::VO_TUT_12),
		Action(AT_TimeWait, 2000),
		Action("upgrade intro", AT_AudioPlayWait, Resources::VO_TUT_14),
		Action(AT_StateTodo, HelpState::State_HitUpgrade),
		Action(AT_StateWait),
		Action("upgrade screen", AT_AudioPlayWait, Resources::VO_TUT_15),
		Action(AT_TimeWait, 2000),
		Action("enemies", AT_AudioPlayWait, Resources::VO_TUT_16),
		Action(AT_TimeWait, 2000),
#endif
#if 1
		Action(AT_IntroEnemy, EntityClass_Mine),
		Action("mine", AT_AudioPlayWait, Resources::VO_TUT_17),
		Action(AT_TimeWait, 2000),
		Action(AT_IntroEnemy, EntityClass_EvilTriangle),
		Action("triangle", AT_AudioPlayWait, Resources::VO_TUT_18),
		Action(AT_TimeWait, 2000),
		Action(AT_IntroEnemy, EntityClass_Kamikaze),
		Action("kamikaze", AT_AudioPlayWait, Resources::VO_TUT_19),
		Action(AT_TimeWait, 2000),
		Action(AT_IntroEnemy, EntityClass_EvilTriangleBiggy),
		Action("triangle clump", AT_AudioPlayWait, Resources::VO_TUT_20),
		Action(AT_TimeWait, 2000),
		Action(AT_IntroEnemy, EntityClass_EvilTriangleExtreme),
		Action("rage triangle", AT_AudioPlayWait, Resources::VO_TUT_21),
		Action(AT_TimeWait, 2000),
		Action(AT_IntroEnemy, EntityClass_EvilSquare),
		Action("square", AT_AudioPlayWait, Resources::VO_TUT_22),
		Action(AT_TimeWait, 2000),
		Action(AT_IntroEnemy, EntityClass_BorderPatrol),
		Action("border patrol", AT_AudioPlayWait, Resources::VO_TUT_23),
		Action(AT_TimeWait, 2000),
#endif
		Action(AT_IntroEnemy, EntityClass_EvilSquareBiggy),
		Action("square clump", AT_AudioPlayWait, Resources::VO_TUT_24),
		Action(AT_TimeWait, 2000),
		Action(AT_IntroEnemy, EntityClass_Shield),
		Action("shield", AT_AudioPlayWait, Resources::VO_TUT_25),
		Action(AT_TimeWait, 2000),
		Action(AT_IntroBoss),
		Action("boss", AT_AudioPlayWait, Resources::VO_TUT_26),
		Action(AT_TimeWait, 2000),
		Action("good luck", AT_AudioPlayWait, Resources::VO_TUT_27),
		Action(AT_AudioWait)
	};
#endif
	
	const static int s_ActionListSize = sizeof(s_ActionList) / sizeof(Action);
	const char* s_ActionDesc = 0;

	//

	HelpMessage::HelpMessage()
	{
	}
	
	void HelpMessage::Initialize(const char* text, SpriteColor color)
	{
		mText = text;
		mColor = color;
	}
	
	void HelpMessage::Render(Vec2F position, Vec2F size, float alpha)
	{
		mColor.v[3] = (int)(alpha * 255.0f);
		
		Mat3x2 mat;
		Mat3x2 matS;
		Mat3x2 matT;
		
		matT.MakeTranslation(position + size * 0.5f);
		matS.MakeScaling(0.8f, 0.8f);
		mat = matT * matS;
//		RenderText(position, size, g_GameState->GetFont(Resources::FONT_USUZI_SMALL), mColor, TextAlignment_Center, TextAlignment_Center, mText.c_str());
		RenderText(mat, g_GameState->GetFont(Resources::FONT_USUZI_SMALL), mColor, TextAlignment_Center, TextAlignment_Center, mText.c_str());
	}

	GameHelp::GameHelp()
	{
		mWriteActive = false;
		mLife = 0.0f;
		mLifeRcp = 1.0f;
	}
	
	void GameHelp::Setup(Vec2F pos, Vec2F size)
	{
		mRect.Setup(pos, size);
	}
	
	void GameHelp::Update(float dt)
	{
		mLife -= dt;
		
		if (mLife <= 0.0f)
			mLines.Clear();
	}
	
	void GameHelp::Render()
	{
		if (mLines.Count_get() == 0)
			return;
		
		float t = mLife * mLifeRcp;
		
		float alpha = Calc::Saturate(sinf(t * Calc::mPI) / sinf(Calc::DegToRad(30.0f)));
		
		RenderRect(mRect.m_Position, mRect.m_Size, 0.0f, 0.0f, 0.0f, 0.4f * alpha, g_GameState->GetTexture(Textures::COLOR_WHITE));
		
		if (mCaption)
		{
			mCaptionColor.v[3] = (int)(alpha * 255.0f);
			// usuzi
			RenderText(mRect.m_Position - Vec2F(5.0f, 0.0f), Vec2F(), g_GameState->GetFont(Resources::FONT_USUZI_SMALL), mCaptionColor, TextAlignment_Left, TextAlignment_Center, true, mCaption);
			
			g_GameState->Render(g_GameState->GetShape(Resources::GAMEHELP_ICON), mRect.m_Position + mRect.m_Size - Vec2F(17.0f, 17.0f), 0.0f, mCaptionColor);
		}
		
		Vec2F pos = mRect.m_Position;
		Vec2F size(mRect.m_Size[0], 20.0f);
		pos[1] += (mRect.m_Size[1] - size[1] * mLines.Count_get()) * 0.5f;
		
		for (Col::ListNode<HelpMessage>* node = mLines.m_Head; node; node = node->m_Next)
		{
			node->m_Object.Render(pos, size, alpha);
			pos[1] += size[1];
		}
	}
	
	void GameHelp::WriteBegin(float life, const char* caption, SpriteColor captionColor)
	{
		mWriteActive = true;
		mLife = life;
		mLifeRcp = 1.0f / life;
		mCaption = caption;
		mCaptionColor = captionColor;
		
		mLines.Clear();
	}
	
	void GameHelp::WriteEnd()
	{
		mWriteActive = false;
	}
	
	void GameHelp::WriteLine(SpriteColor color, const char* text, ...)
	{
		Assert(mWriteActive);
		
		VA_SPRINTF(text, temp, text);
		
		HelpMessage message;
		
		message.Initialize(temp, color);
		
		mLines.AddTail(message);
	}
	
	//
	
	HelpState::HelpState()
	{
		m_Done = false;
		m_StateTodo = 0;
		m_StateDone = 0;
		m_ActionIdx = -1;
		m_AudioWait = false;
		m_StateWait = false;
		m_TimeWait = 0.0f;
		m_FadeWait = false;
		m_EnemyWait = false;
		m_VoiceOver = 0;

		// background music volume fade
		m_Fade = 1.0f;
		m_FadeBegin = 1.0f;
		m_FadeEnd = 1.0f;
		m_FadeTime = 0.0f;
		m_FadeTimeRcp = 1.0f;
	}
	
	HelpState::~HelpState()
	{
		StopVoiceOver();
	}

	void HelpState::GameBegin(bool playTutorial)
	{
		StopVoiceOver();
		
		if (playTutorial)
		{
			m_Done = false;
			m_StateTodo = 0;
			m_StateDone = 0;
			m_ActionIdx = -1;
			NextAction();
		}
		else
		{
			m_Done = true;
			m_StateTodo = 0x00000000;
			m_StateDone = 0xFFFFFFFF;
		}
	}
	
	void HelpState::Update(float dt)
	{
		if (m_Done == false)
		{
			bool ready = true;

			if (m_VoiceOver)
			{
				m_VoiceOver->Update();

				if (m_VoiceOver->HasFinished_get() == false)
					g_GameState->m_SoundPlayer->Volume_set(g_GameState->m_GameSettings->m_MusicVolume * 0.25f);
				else
				{
					g_GameState->m_SoundPlayer->Volume_set(g_GameState->m_GameSettings->m_MusicVolume);
					StopVoiceOver();
				}
			}

			if (m_AudioWait)
			{
				if (m_VoiceOver)
					ready &= m_VoiceOver->HasFinished_get();
			}

			if (m_StateWait)
			{
				ready &= m_StateTodo == 0;
			}

			if (m_FadeTime > 0.0f)
			{
				m_FadeTime -= dt;

				if (m_FadeTime < 0.0f)
					m_FadeTime = 0.0f;

				float t1 = m_FadeTime * m_FadeTimeRcp;
				float t2 = 1.0f - t1;

				m_Fade = m_FadeBegin * t1 + m_FadeEnd * t2;

				float volume = m_Fade * g_GameState->m_GameSettings->m_MusicVolume;

				g_GameState->m_SoundPlayer->Volume_set(volume);

				if (m_FadeWait)
				{
					ready &= m_FadeTime <= 0.0f;
				}
			}

			if (m_TimeWait > 0.0f)
			{
				m_TimeWait -= dt;

				ready &= m_TimeWait <= 0.0f;
			}
			
			if (m_EnemyWait)
			{
				LOG_INF("alive: %d", g_World->AliveEnemiesCount_get());
 				ready &= g_World->AliveEnemiesCount_get() == 0 && g_GameState->m_GameRound->EnemyWaveMgrIsEmpty_get();
			}

			if (ready)
			{
				m_AudioWait = false;
				m_StateWait = false;
				m_TimeWait = 0.0f;
				m_FadeWait = false;
				m_EnemyWait = false;

				if (IsDone_get() == false)
					NextAction();
			}
		}
		else
		{
			Assert(m_StateTodo == 0);
		}
	}

	void HelpState::Render()
	{
		if (IsDone_get() == false)
		{
#ifndef DEPLOYMENT
			if (s_ActionDesc != 0)
			{
				RenderText(Vec2F(0.0f, 0.0f), Vec2F(float(VIEW_SX), float(VIEW_SY)), g_GameState->GetFont(Resources::FONT_CALIBRI_SMALL), SpriteColors::White, TextAlignment_Center, TextAlignment_Center, true, s_ActionDesc);
			}
#endif
		}
	}

	void HelpState::DoComplete(State state)
	{
		if (m_Done == false)
		{
			if ((m_StateTodo & state) != 0)
			{
				m_StateTodo &= ~state;
				m_StateDone |= state;
			}
		}
	}
	
	bool HelpState::IsComplete(State state) const
	{
		if (m_Done)
			return true;
		else
			return (m_StateDone & state) != 0;
	}
	
	bool HelpState::IsCompleteOrActive(State state) const
	{
		if (m_Done)
			return true;
		else
			return ((m_StateTodo | m_StateDone) & state) != 0;
	}
	
	bool HelpState::IsActive(State state) const
	{
		if (m_Done)
			return false;
		else
		{
			int masked = m_StateTodo & state;
			
			return masked != 0;
		}
	}

	bool HelpState::IsDone_get() const
	{
		return m_Done;
	}

	void HelpState::NextAction()
	{
		Assert(m_Done == false);
		Assert(m_ActionIdx != s_ActionListSize);

		if (m_Done == false && m_ActionIdx < s_ActionListSize)
		{
			m_ActionIdx++;

			if (m_ActionIdx == 0)
				s_ActionDesc = "...begin...";

			if (m_ActionIdx != s_ActionListSize)
			{
				const Action& a = s_ActionList[m_ActionIdx];

				switch (a.Type)
				{
				case AT_AudioPlay:
					PlayVoiceOver(a.Param1);
					break;
				case AT_AudioWait:
					m_AudioWait = true;
					break;
				case AT_AudioPlayWait:
					PlayVoiceOver(a.Param1);
					m_AudioWait = true;
					break;
				case AT_StateTodo:
					m_StateTodo |= a.Param1;
					break;
				case AT_StateWait:
					m_StateWait = true;
					break;
				case AT_TimeWait:
					m_TimeWait = a.Param1 / 1000.0f;
					break;
				case AT_MusicFadeAndWait:
					m_FadeTime = a.Param1 / 1000.0f;
					m_FadeTimeRcp = 1.0f / m_FadeTime;
					m_FadeBegin = m_Fade;
					m_FadeEnd = a.Param2 / 100.0f;
					m_FadeWait = true;
					break;
				case AT_MusicDisable:
					g_GameState->m_SoundPlayer->IsEnabled_set(false);
					break;
				case AT_MusicEnable:
					g_GameState->m_SoundPlayer->IsEnabled_set(true);
					break;
				case AT_IntroEnemy:
					g_GameState->m_GameRound->ClassicLearn_IntroEnemy((EntityClass)a.Param1);
					break;
				case AT_EnemyWait:
					m_EnemyWait = true;
					break;
				case AT_IntroBoss:
					g_GameState->m_GameRound->ClassicLearn_IntroBoss();
					break;
				default:
#ifndef DEPLOYMENT
					throw ExceptionNA();
#else
					break;
#endif
				}

				if (a.Desc != 0)
					s_ActionDesc = a.Desc;
			}
			else
			{
				LOG_INF("game help: completed action list", 0);
				
				m_Done = true;
				
				Assert(m_VoiceOver == 0);
				StopVoiceOver();
				
				g_GameState->m_GameRound->Classic_HandleHelpComplete();
			
				LOG(LogLevel_Debug, "tutorial complete");
			}
		}
	}

	void HelpState::PlayVoiceOver(int id)
	{
		StopVoiceOver();

#if HELP_VOICEOVER
		m_VoiceOver = new SoundPlayer_OpenAL();
#endif

		if (m_VoiceOver)
		{
			m_VoiceOver->Initialize(true);
#if !defined(MACOS) || 1
			Res* res = g_GameState->m_ResMgr.Get(id);
			m_VoiceOver->Play(res, 0, 0, 0, false);
#else
			Res* res1 = g_GameState->m_ResMgr.Get(Resources::BGM_GAME2);
			Res* res2 = g_GameState->m_ResMgr.Get(Resources::BGM_GAME5);
			Res* res3 = g_GameState->m_ResMgr.Get(Resources::BGM_GAME6);
			Res* res4 = 0;//g_GameState->m_ResMgr.Get(Resources::BGM_GAME7);
			m_VoiceOver->Play(res1, res2, res3, res4, false);
#endif
		}
	}
	
	void HelpState::StopVoiceOver()
	{
		if (m_VoiceOver != 0)
		{
			m_VoiceOver->Shutdown();
			delete m_VoiceOver;
			m_VoiceOver = 0;
		}
	}
}
