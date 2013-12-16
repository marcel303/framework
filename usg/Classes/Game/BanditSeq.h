#pragma once

#include "Anim_ScreenFlash.h"
#include "AnimSeq.h"

namespace Bandits
{
	class EntityBandit;
	
	// Bandit related animation sequences
	
	class BanditSeq
	{
	public:
		BanditSeq();
		void Initialize(EntityBandit* bandit);
		
		void Update(float dt);
		void Render_Back();
		void Render_Front();
		bool IsRunning_get() const;
		
		void StartDestruction();
		
	private:
		EntityBandit* mBandit;
		
	public:
		class Sweep
		{
		public:
			Sweep();
			void Start(float angle, float breadth, float speed);
			void Stop();
			void Update(float dt);
			void Render(Vec2F pos);
			
		private:
			bool mIsActive;
			float mAngle;
			float mBreadth;
			float mSpeed;
		};
		
	public:
		// --------------------
		// Bandit destruction
		// --------------------
		class Destruction
		{
		public:
			void Initialize(EntityBandit* bandit);
			void Start();
			void Stop();
			void Update(float dt);
			void Render_Back();
			void Render_Front();
			bool IsRunning_get() const;
			
		private:
			const static int STROKE_COUNT = 10;
			EntityBandit* mBandit;
			
			bool mIsActive;
			
			Game::AnimSeq mAnimSeq;
			Game::Anim_ScreenFlash mFlashAnim;
			Sweep mStrokes[STROKE_COUNT];
			
			static void HandleEvent(void* obj, void* arg);
		};
		
	private:
		Destruction mDestruction;
	};
}
