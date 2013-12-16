#pragma once

#include "ColList.h"
#include "FixedSizeString.h"
#include "ISoundPlayer.h"
#include "SpriteGfx.h"
#include "Types.h"

/*
 
 Game Help System
 
 - Used by GameRound to display help text on early tutorial levels
 
 - Used at intro screen to display help messages 
 
 */
namespace Game
{
	namespace HelpColors
	{
		const static SpriteColor EnemyCaption = SpriteColor_Make(191, 227, 255, 255);
		const static SpriteColor Enemy = SpriteColor_Make(227, 227, 227, 255);
		const static SpriteColor PowerupCaption = SpriteColor_Make(255, 255, 191, 255);
		const static SpriteColor Powerup = SpriteColor_Make(227, 227, 227, 255);
		const static SpriteColor MaxiCaption = SpriteColor_Make(255, 63, 227, 255);
		const static SpriteColor BadSectorCaption = SpriteColor_Make(255, 0, 0, 255);
	};
	
	class HelpMessage
	{
	public:
		HelpMessage();
		void Initialize(const char* text, SpriteColor color);
		
		void Update(float dt);
		void Render(Vec2F position, Vec2F size, float alpha);
		
	private:
		FixedSizeString<64> mText;
		SpriteColor mColor;
	};
	
	class GameHelp
	{
	public:
		GameHelp();
		void Setup(Vec2F pos, Vec2F size);
		
		void Update(float dt);
		void Render();
		
		void WriteBegin(float life, const char* caption, SpriteColor captionColor);
		void WriteEnd();
		void WriteLine(SpriteColor color, const char* text, ...);
		
	private:
		float mLife;
		float mLifeRcp;
		const char* mCaption;
		SpriteColor mCaptionColor;
		Col::List<HelpMessage> mLines;
		bool mWriteActive;
		RectF mRect;
	};
	
	class HelpState
	{
	public:
		HelpState();
		~HelpState();
		
		void GameBegin(bool playTutorial);
		void Update(float dt);
		void Render();
		
		enum State
		{
			State_Undefined = 1 << 0,
			State_HitMove = 1 << 1,
			State_HitFire = 1 << 2,
			State_HitSpecial = 1 << 3,
			State_HitUpgrade = 1 << 4,
			State_HitWeaponSwitch = 1 << 5
		};
		
		void DoComplete(State state);
		bool IsComplete(State state) const;
		bool IsCompleteOrActive(State state) const;
		bool IsActive(State state) const;
		bool IsDone_get() const;
		
	private:
		void NextAction();
		void PlayVoiceOver(int id);
		void StopVoiceOver();
		
		bool m_Done;
		int m_StateTodo; // currently active states
		int m_StateDone; // completed states
		int m_ActionIdx;
		bool m_AudioWait;
		bool m_StateWait;
		float m_TimeWait;
		bool m_FadeWait;
		float m_Volume;
		bool m_EnemyWait;

		// background music volume fade
		float m_Fade;
		float m_FadeBegin;
		float m_FadeEnd;
		float m_FadeTime;
		float m_FadeTimeRcp;

		ISoundPlayer* m_VoiceOver;
	};
}
