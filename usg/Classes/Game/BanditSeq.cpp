#include "Bandit.h"
#include "BanditEntity.h"
#include "BanditSeq.h"
#include "GameState.h"
#include "ParticleGenerator.h"
#include "SoundEffectMgr.h"
#include "TempRender.h"
#include "TextureAtlas.h"
#include "Textures.h"
#include "UsgResources.h"
#include "World.h"

#define MOVE_SPEED_DEATH 100.0f

namespace Bandits
{
	BanditSeq::BanditSeq()
	{
		mBandit = 0;
	}
	
	void BanditSeq::Initialize(EntityBandit* bandit)
	{
		mBandit = bandit;
		
		mDestruction.Initialize(bandit);
	}
	
	void BanditSeq::Update(float dt)
	{
		mDestruction.Update(dt);
	}
	
	void BanditSeq::Render_Back()
	{
		mDestruction.Render_Back();
	}
	
	void BanditSeq::Render_Front()
	{
		mDestruction.Render_Front();
	}
	
	bool BanditSeq::IsRunning_get() const
	{
		return mDestruction.IsRunning_get();
	}
	
	void BanditSeq::StartDestruction()
	{
		mDestruction.Start();
	}
	
	//
	
	BanditSeq::Sweep::Sweep()
	{
		mIsActive = false;
	}
	
	void BanditSeq::Sweep::Start(float angle, float breadth, float speed)
	{
		mIsActive = true;
		mAngle = angle;
		mBreadth = breadth;
		mSpeed = speed;
	}
	
	void BanditSeq::Sweep::Stop()
	{
		mIsActive = false;
	}
	
	void BanditSeq::Sweep::Update(float dt)
	{
		if (!mIsActive)
			return;
		
		mAngle += mSpeed * dt;
	}
	
	void BanditSeq::Sweep::Render(Vec2F pos)
	{
		if (!mIsActive)
			return;
		
		SpriteColor color = SpriteColor_MakeF(1.0f, 1.0f, 1.0f, 0.7f);
		
		RenderSweep(pos, mAngle - mBreadth * 0.5f, mAngle + mBreadth * 0.5f, 5000.0f, color);
	}
	
	//
	
	void BanditSeq::Destruction::Initialize(EntityBandit* bandit)
	{
		mBandit = bandit;
		
		mIsActive = false;
		
		mAnimSeq.Initialize(g_GameState->m_TimeTracker_World, CallBack(this, HandleEvent));
	}
	
	enum DsEvent
	{
		DsEvent_Pulse, // core receives an impulse, casting it aside
		DsEvent_Explosions1, // near core, x2
		DsEvent_Explosions2, // surrounding core, x5
		DsEvent_Explosions3, // outer shell
		DsEvent_StrokesActivate1, // activate stroke effect (some)
		DsEvent_StrokesActivate2, // activate stroke effect (all)
		DsEvent_Flash, // flash the screen
		DsEvent_StrokesDeactivate, // deactivate stroke effect
		DsEvent_FlamesSpawn, // spawn flame effect
		DsEvent_Die, // let bandit disappear
		DsEvent_End // end animation
	};
	
	const static Game::AnimKey sDestructionKeys[] =
	{
		{ 0.0f, DsEvent_Pulse },
		{ 0.5f, DsEvent_Explosions1 },
		{ 0.5f, DsEvent_Flash },
		{ 1.0f, DsEvent_StrokesActivate1 },
		{ 1.0f, DsEvent_Explosions2 },
		{ 1.0f, DsEvent_Flash },
		{ 1.5f, DsEvent_Explosions3 },
		{ 1.5f, DsEvent_Flash },
		{ 1.5f, DsEvent_StrokesActivate2 },
		{ 2.0f, DsEvent_Flash },
		{ 2.0f, DsEvent_StrokesDeactivate },
		{ 2.0f, DsEvent_Die },
		{ 2.0f, DsEvent_FlamesSpawn },
		{ 3.0f, DsEvent_End }
	};
	
	void BanditSeq::Destruction::Start()
	{
		mIsActive = true;
		
		mAnimSeq.Start(sDestructionKeys, sizeof(sDestructionKeys) / sizeof(Game::AnimKey));
	}
	
	void BanditSeq::Destruction::Stop()
	{
		mIsActive = false;
		
		mAnimSeq.Stop();
	}
	
	void BanditSeq::Destruction::Update(float dt)
	{
		if (mIsActive == false)
			return;
		
		for (int i = 0; i < STROKE_COUNT; ++i)
			mStrokes[i].Update(dt);
		
		mAnimSeq.Update();
	}
	
	void BanditSeq::Destruction::Render_Back()
	{
		if (mIsActive == false)
			return;
		
		for (int i = 0; i < STROKE_COUNT; ++i)
			mStrokes[i].Render(mBandit->Position_get());
	}
	
	void BanditSeq::Destruction::Render_Front()
	{
		if (mIsActive == false)
			return;
		
		mFlashAnim.Render();
	}
	
	bool BanditSeq::Destruction::IsRunning_get() const
	{
		return mIsActive;
	}
	
	const static Vec2F explosionSize(64.0f, 64.0f);
	
	static void SpawnExplosion(const Vec2F& pos)
	{
#if 0
		Game::SpriteAnim anim;
		anim.Setup(g_GameState->m_ResMgr.Get(Resources::EXPLOSION_01), 4, 4, 0, 15, 16, &g_GameState->m_TimeTracker_World);
		anim.Start();
		g_GameState->m_SpriteEffectMgr.Add(anim, pos - explosionSize * 0.5f, explosionSize, SpriteColors::Black);
#else
		ParticleGenerator::GenerateRandomExplosion(g_GameState->m_ParticleEffect, pos, 200.0f, 250.0f, 1.3f, 15, g_GameState->GetTexture(Textures::EXPLOSION_LINE)->m_Info, 1.0f, 0);
#endif
	}
	
	static void SpawnMegaExplosion(const Vec2F& pos)
	{
		const AtlasImageMap* image = g_GameState->GetTexture(Textures::EXPLOSION_LINE);
		const float life = 1.0f;
		const float speed = 50.0f;
		const SpriteColor color = SpriteColor_Make(255, 255, 0, 255);
		
		for (int i = 0; i < 10; ++i)
		{
			float angle = Calc::Random(0.0f, Calc::m2PI);
			
			Particle& p = g_GameState->m_ParticleEffect.Allocate(image->m_Info, 0, 0);
			
			Vec2F dir = Vec2F::FromAngle(angle);
							
			Particle_Default_Setup(
				&p,
				pos[0],
				pos[1], life, 10.0f, 4.0f, angle, speed);
			
			p.m_Color = color;
		}
	}
	
	void BanditSeq::Destruction::HandleEvent(void* obj, void* arg)
	{
		Destruction* self = (Destruction*)obj;
		Game::AnimKey* key = (Game::AnimKey*)arg;
		
//		Vec2F explosionSize(64.0f, 64.0f);
		
		switch ((DsEvent)key->event)
		{
			case DsEvent_Pulse:
			{
				self->mBandit->Make_MoveRetreat(Game::g_Target, MOVE_SPEED_DEATH);
				break;
			}
			case DsEvent_Explosions1:
			{
				g_GameState->m_SoundEffects->Play(Resources::SOUND_EXPLODE_SOFT, 0);
				float distance = 20.0f;
				int n = 3;
				for (int i = 0; i < n; ++i)
				{
					// spawn explosion sprites
					Vec2F offset = Vec2F::FromAngle(i / (float)n * Calc::m2PI) * distance;
					SpawnExplosion(self->mBandit->Position_get() + offset);
				}
				break;
			}
			case DsEvent_Explosions2:
			{
				g_GameState->m_SoundEffects->Play(Resources::SOUND_EXPLODE_SOFT, 0);
				float distance = 40.0f;
				int n = 6;
				for (int i = 0; i < n; ++i)
				{
					Vec2F offset = Vec2F::FromAngle(i / (float)n * Calc::m2PI) * distance;
					// spawn explosion sprites
					SpawnExplosion(self->mBandit->Position_get() + offset);
				}
				break;
			}
			case DsEvent_Explosions3:
			{
				g_GameState->m_SoundEffects->Play(Resources::SOUND_EXPLODE_SOFT, 0);
				float distance = 30.0f;
				int n = 4;
				for (int i = 0; i < n; ++i)
				{
					Vec2F offset = Vec2F::FromAngle(i / (float)n * Calc::m2PI) * distance;
					// spawn explosion sprites
					SpawnExplosion(self->mBandit->Position_get() + offset);
				}
				break;
			}
			case DsEvent_Flash:
			{
				g_GameState->m_SoundEffects->Play(Resources::SOUND_BANDIT_CORE_DESTROY, 0);
				self->mFlashAnim.Start(15, SpriteColors::White);
				break;
			}
			case DsEvent_StrokesActivate1:
			{
				for (int i = 0; i < STROKE_COUNT / 2; ++i)
				{
					float angle = Calc::Random(Calc::m2PI);
					float breadth = Calc::DegToRad(3.0f);
					float speed = Calc::Random(Calc::m2PI / 1.0f, Calc::m2PI / 1.5f) * (Calc::Random() % 2 == 0 ? -1 : +1);
					self->mStrokes[i].Start(angle, breadth, speed);
				}
				break;
			}
			case DsEvent_StrokesActivate2:
			{
				for (int i = 0; i < STROKE_COUNT; ++i)
				{
					float angle = Calc::Random(Calc::m2PI);
					float breadth = Calc::DegToRad(3.0f);
					float speed = Calc::Random(Calc::m2PI / 1.0f, Calc::m2PI / 1.5f) * (Calc::Random() % 2 == 0 ? -1 : +1);
					self->mStrokes[i].Start(angle, breadth, speed);
				}
				break;
			}
			case DsEvent_StrokesDeactivate:
			{
				for (int i = 0; i < STROKE_COUNT; ++i)
					self->mStrokes[i].Stop();
				break;
			}
			case DsEvent_FlamesSpawn:
			{
				g_GameState->m_SoundEffects->Play(Resources::SOUND_EXPLODE_SOFT, 0);
#if 0
				int n = 30;
				for (int i = 0; i < n; ++i)
				{
					float distance = Calc::Random(200.0f);
					float angle = Calc::Random(Calc::m2PI);
					Vec2F offset = Vec2F::FromAngle(angle) * distance;
					Game::SpriteAnim anim;
					anim.Setup(g_GameState->m_ResMgr.Get(Resources::EXPLOSION_01), 4, 4, 0, 15, Calc::Random(14, 20), &g_GameState->m_TimeTracker_World);
					anim.Start();
					g_GameState->m_SpriteEffectMgr.Add(anim, self->mBandit->Position_get() - explosionSize / 2.0f + offset, explosionSize, SpriteColors::Black);
				}
#else
				int n = 20;
				for (int i = 0; i < n; ++i)
				{
					float distance = Calc::Random(200.0f);
					float angle = Calc::Random(Calc::m2PI);
					Vec2F offset = Vec2F::FromAngle(angle) * distance;
					SpawnMegaExplosion(self->mBandit->Position_get() + offset);
				}
#endif
				break;
			}
			case DsEvent_Die:
			{
				self->mBandit->Destroy();
				break;
			}
			case DsEvent_End:
			{
				self->Stop();
				break;
			}
			default:
#ifndef DEPLOYMENT
				throw ExceptionVA("not implemented");
#else
				break;
#endif
		}
	}
}
