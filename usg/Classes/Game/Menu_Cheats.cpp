#include "Bandit.h"
#include "EntityEnemy.h"
#include "EntityPlayer.h"
#include "GameRound.h"
#include "GameState.h"
#include "GuiButton.h"
#include "Menu_Cheats.h"
#include "MenuMgr.h"
#include "UsgResources.h"
#include "World.h"

namespace GameMenu
{
	Menu_Cheats::Menu_Cheats() : Menu()
	{
	}
	
	void Menu_Cheats::Init()
	{
		SetTransition(TransitionEffect_Slide, Vec2F(0.0f, 100.0f), 0.5f);
		
		Button btn_SpawnMiniBoss_Snake;
		Button btn_SpawnMiniBoss_Spinner;
		Button btn_SpawnMiniBoss_Magnet;
		Button btn_SpawnEnemies;
		Button btn_SpawnBandit_01;
		Button btn_SpawnBandit_02;
		Button btn_SpawnBandit_03;
		Button btn_SpawnBandit_04;
		Button btn_Cheat_Invincible;
		Button btn_Cheat_PowerUp;
		
		Button btn_SpawnFormation_Circle;
		Button btn_SpawnFormation_Cluster;
		Button btn_SpawnFormation_Line;
		Button btn_SpawnFormation_PlayerPos;
		Button btn_SpawnFormation_Prepared_Line;
		
		Button btn_Cheat_PlayerKill;
		
		Button btn_GoBack;
		
		float y;
		
		y = 0.0f;
		
		btn_SpawnMiniBoss_Snake.Setup_Text(0, Vec2F(10.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "MB:snake", Game::BossType_Snake, CallBack(this, Handle_SpawnMiniBoss));
		btn_SpawnMiniBoss_Spinner.Setup_Text(0, Vec2F(10.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "MB:spinner", Game::BossType_Spinner, CallBack(this, Handle_SpawnMiniBoss));
		btn_SpawnMiniBoss_Magnet.Setup_Text(0, Vec2F(10.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "MB:magnet", Game::BossType_Magnet, CallBack(this, Handle_SpawnMiniBoss));
		btn_SpawnBandit_01.Setup_Text(0, Vec2F(10.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "bandit:e", Resources::BANDIT_EASY, CallBack(this, Handle_SpawnBandit));
		btn_SpawnBandit_02.Setup_Text(0, Vec2F(10.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "bandit:h", Resources::BANDIT_HARD, CallBack(this, Handle_SpawnBandit));
		btn_SpawnBandit_04.Setup_Text(0, Vec2F(10.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "bandit:g", Resources::BANDIT_GUNSHIP, CallBack(this, Handle_SpawnBandit));
		btn_Cheat_Invincible.Setup_Text(0, Vec2F(10.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "cheat:inv", CheatType_Cheat_Invincibility, CallBack(this, Handle_Cheat));
		btn_Cheat_PowerUp.Setup_Text(0, Vec2F(10.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "cheat:powerup", CheatType_Cheat_PowerUp, CallBack(this, Handle_Cheat));
		
		y = 30.0f;
		
		btn_SpawnEnemies.Setup_Text(0, Vec2F(100.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "enemies", CheatType_SpawnEnemies, CallBack(this, Handle_Cheat));
		btn_SpawnFormation_Circle.Setup_Text(0, Vec2F(100.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "sf:circle", CheatType_Spawn_Circle, CallBack(this, Handle_Cheat));
		btn_SpawnFormation_Cluster.Setup_Text(0, Vec2F(100.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "sf:cluster", CheatType_Spawn_Cluster, CallBack(this, Handle_Cheat));
		btn_SpawnFormation_Line.Setup_Text(0, Vec2F(100.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "sf:line", CheatType_Spawn_Line, CallBack(this, Handle_Cheat));
		btn_SpawnFormation_PlayerPos.Setup_Text(0, Vec2F(100.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "sf:player", CheatType_Spawn_PlayerPos, CallBack(this, Handle_Cheat));
		btn_SpawnFormation_Prepared_Line.Setup_Text(0, Vec2F(100.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "sf:p:line", CheatType_Spawn_Prepared_Line, CallBack(this, Handle_Cheat));

		y = 30.0f;
		
		btn_Cheat_PlayerKill.Setup_Text(0, Vec2F(190.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "player:kill", CheatType_Player_Kill, CallBack(this, Handle_Cheat));
		AddButton(Button::Make_Text(0, Vec2F(190.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "player:credits", CheatType_Player_Credits, CallBack(this, Handle_Cheat)));
		AddButton(Button::Make_Text(0, Vec2F(190.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "player:w++", CheatType_Player_Upgrade, CallBack(this, Handle_Cheat)));
		AddButton(Button::Make_Text(0, Vec2F(190.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "round:level+", CheatType_Round_LevelUp, CallBack(this, Handle_Cheat)));
		AddButton(Button::Make_Text(0, Vec2F(190.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "round:level-", CheatType_Round_LevelDown, CallBack(this, Handle_Cheat)));
		AddButton(Button::Make_Text(0, Vec2F(190.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "render:bg", CheatType_Render_BG, CallBack(this, Handle_Cheat)));
		AddButton(Button::Make_Text(0, Vec2F(190.0f, y += 30.0f), Vec2F(50.0f, 20.0f), "render:game", CheatType_Render_GameOnly, CallBack(this, Handle_Cheat)));
		
		btn_GoBack.Setup_Shape(0, Vec2F(10.0f, 280.0f), g_GameState->GetShape(Resources::BUTTON_BACK), 0, CallBack(this, Handle_GoBack));
		
		AddButton(btn_SpawnMiniBoss_Snake);
		AddButton(btn_SpawnMiniBoss_Spinner);
		AddButton(btn_SpawnMiniBoss_Magnet);
		AddButton(btn_SpawnBandit_01);
		AddButton(btn_SpawnBandit_02);
		AddButton(btn_SpawnBandit_03);
		AddButton(btn_SpawnBandit_04);
		AddButton(btn_Cheat_Invincible);
		AddButton(btn_Cheat_PowerUp);
		
		AddButton(btn_SpawnEnemies);
		AddButton(btn_SpawnFormation_Circle);
		AddButton(btn_SpawnFormation_Cluster);
		AddButton(btn_SpawnFormation_Line);
		AddButton(btn_SpawnFormation_PlayerPos);
		AddButton(btn_SpawnFormation_Prepared_Line);
		
		AddButton(btn_Cheat_PlayerKill);
		
		AddButton(btn_GoBack);
	}
	
	void Menu_Cheats::Handle_SpawnMiniBoss(void* obj, void* arg)
	{
		Button* button = (Button*)arg;
		Game::BossType type = (Game::BossType)button->m_Info;
		Game::g_World->m_Bosses.Spawn(type, g_GameState->m_GameRound->Classic_Level_get());
	}
	
	void Menu_Cheats::Handle_Cheat(void* obj, void* arg)
	{
		Button* button = (Button*)arg;
		CheatType type = (CheatType)button->m_Info;
		
		float angle = Calc::Random(Calc::m2PI);
		Vec2F spawnP0 = Game::g_World->m_Player->Position_get() + Vec2F(Calc::Random_Scaled(50.0f), Calc::Random_Scaled(50.0f));
		Vec2F spawnP1 = spawnP0 - Vec2F::FromAngle(angle) * 150.0f;
		Vec2F spawnP2 = spawnP0 + Vec2F::FromAngle(angle) * 150.0f;
		
		switch (type)
		{
			case CheatType_SpawnEnemies:
				for (int i = Game::EntityClass__SmallFry_Begin + 1; i < Game::EntityClass__SmallFry_End; ++i)
				{
					Game::EntityClass entityType = (Game::EntityClass)i;
					
					Game::g_World->SpawnEnemy(entityType, Game::g_World->m_Player->Position_get(), Game::EnemySpawnMode_ZoomIn);
				}
				break;
			case CheatType_Cheat_Invincibility:
				Game::g_World->m_Player->Cheat_Invincibility();
				break;
			case CheatType_Cheat_PowerUp:
				Game::g_World->m_Player->Cheat_PowerUp();
				//Game::g_World->m_Player->HandlePowerup(Game::PowerupType_Fun_BeamFever);
				break;
			case CheatType_Spawn_Circle:
			{
				Game::EnemyWave wave;
				wave.MakeCircle(Game::EntityClass_EvilTriangle, spawnP0, 160.0f, 100.0f, 0.0f, Calc::m2PI, 20, 0.5f);
				g_GameState->m_GameRound->Classic_WaveAdd(wave);
				break;
			}
			case CheatType_Spawn_Cluster:
			{
				Game::EnemyWave wave;
				wave.MakeCluster(Game::EntityClass_EvilTriangle, spawnP0, 160.0f, 20, 0.5f);
				g_GameState->m_GameRound->Classic_WaveAdd(wave);
				break;
			}
			case CheatType_Spawn_Line:
			{
				Game::EnemyWave wave;
				wave.MakeLine(Game::EntityClass_EvilTriangle, spawnP1, spawnP2, 40.0f, 0.5f);
				g_GameState->m_GameRound->Classic_WaveAdd(wave);
				break;
			}
			case CheatType_Spawn_PlayerPos:
			{
				Game::EnemyWave wave;
				wave.MakePlayerPos(Game::EntityClass_EvilTriangle, 10, 0.5f);
				g_GameState->m_GameRound->Classic_WaveAdd(wave);
				break;
			}
			case CheatType_Spawn_Prepared_Line:
			{
				Game::EnemyWave wave;
				wave.MakePrepared_Line(Game::EntityClass_EvilTriangle, spawnP1 - spawnP0, spawnP2 - spawnP0, 8, 0.6f, false, true, true, true);
				g_GameState->m_GameRound->Classic_WaveAdd(wave);
				break;
			}
				
			case CheatType_SpawnMiniBoss:
			case CheatType_SpawnBandit:
				break;
			
			case CheatType_Player_Kill:
			{
				Game::g_World->m_Player->HandleDamage(Game::g_World->m_Player->Position_get(), Vec2F(0.0f, 0.0f), 1000000.0f, Game::DamageType_Instant);
				break;
			}
				
			case CheatType_Player_Credits:
			{
				Game::g_World->m_Player->Credits_Increase(1000000);
				break;
			}
				
			case CheatType_Player_Upgrade:
			{
				if (Game::g_World->m_Player->GetUpgradeLevel(Game::UpgradeType_Vulcan) < Game::g_World->m_Player->GetMaxUpgradelevel(Game::UpgradeType_Vulcan))
					Game::g_World->m_Player->HandleUpgrade(Game::UpgradeType_Vulcan);
				if (Game::g_World->m_Player->GetUpgradeLevel(Game::UpgradeType_Beam) < Game::g_World->m_Player->GetMaxUpgradelevel(Game::UpgradeType_Beam))
					Game::g_World->m_Player->HandleUpgrade(Game::UpgradeType_Beam);
				if (Game::g_World->m_Player->GetUpgradeLevel(Game::UpgradeType_Shock) < Game::g_World->m_Player->GetMaxUpgradelevel(Game::UpgradeType_Shock))
					Game::g_World->m_Player->HandleUpgrade(Game::UpgradeType_Shock);
				if (Game::g_World->m_Player->GetUpgradeLevel(Game::UpgradeType_Special) < Game::g_World->m_Player->GetMaxUpgradelevel(Game::UpgradeType_Special))
					Game::g_World->m_Player->HandleUpgrade(Game::UpgradeType_Special);
				break;
			}
				
			case CheatType_Round_LevelUp:
			{
				g_GameState->m_GameRound->DEBUG_LevelOffset(+1);
				break;
			}
				
			case CheatType_Round_LevelDown:
			{
				g_GameState->m_GameRound->DEBUG_LevelOffset(-1);
				break;
			}
				
			case CheatType_Render_BG:
			{
#ifndef DEPLOYMENT
				g_GameState->DBG_RenderMask = RenderMask_WorldBackground;
#endif
				break;
			}
				
			case CheatType_Render_GameOnly:
			{
#ifndef DEPLOYMENT
				g_GameState->DBG_RenderMask = RenderMask_Particles | RenderMask_WorldBackground | RenderMask_WorldPrimaryNonPlayer;
#endif
				break;
			}
				
			default:
				throw ExceptionVA("unknown cheat: %d", (int)type);
		}
	}
	
	void Menu_Cheats::Handle_SpawnBandit(void* obj, void* arg)
	{
		Button* button = (Button*)arg;
		Game::g_World->SpawnBandit(g_GameState->m_ResMgr.Get(button->m_Info), g_GameState->m_GameRound->Classic_Level_get(), 0);
	}
	
	void Menu_Cheats::Handle_GoBack(void* obj, void* arg)
	{
		g_GameState->Interface_get()->ActiveMenu_set(MenuType_InGame);
	}
}
