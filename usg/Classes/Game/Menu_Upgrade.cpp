#include "Atlas_ImageInfo.h"
#include "EntityPlayer.h"
#include "GameRound.h"
#include "GameState.h"
#include "GuiButton.h"
#include "Mat3x2.h"
#include "Menu_InGame.h"
#include "Menu_Upgrade.h"
#include "Menu_UpgradeBought.h"
#include "MenuMgr.h"
#include "SoundEffectMgr.h"
#include "StringBuilder.h"
#include "TempRender.h"
#include "TextureAtlas.h"
#include "Textures.h"
#include "UsgResources.h"
#include "Util_ColorEx.h"
#include "World.h"

namespace GameMenu
{
	static bool MaxLevelReached(Game::UpgradeType upgrade);
	static bool CanBuy(Game::UpgradeType upgrade, int level);
	static float GetHue(Vec2F pos);
//	static SpriteColor MakeGrayScale(SpriteColor color);
	static const char* UpgradeType_To_Name(Game::UpgradeType upgrade);
	static int UpgradeType_To_Shape(Game::UpgradeType type);
	
	Menu_Upgrade::Menu_Upgrade()
	{
	}
	
	Menu_Upgrade::~Menu_Upgrade()
	{
	}
	
	void Menu_Upgrade::Init()
	{
		mDenyText = 0;
		mDenyAnim.Initialize(g_GameState->m_TimeTracker_Global, false);
		
		SetTransition(TransitionEffect_Slide, Vec2F(0.0f, (float)VIEW_SY), 0.2f);
		
		Button btn_Upgrade_Vulcan;
		Button btn_Upgrade_Beam;
		Button btn_Upgrade_Shock;
		Button btn_Upgrade_Special;
		Button btn_Upgrade_Border;
		Button btn_Cancel;
		
		float scaleX = 1.0f;
		float scaleY = 1.0f;

		float sx = 143.0f * scaleX;
		float sy = 98.0f * scaleY;
		float spacing = 1.0f * scaleX;
		float basePos = (VIEW_SX - sx * 3.0f - spacing * 2.0f) / 2.0f;
		float posIncrement = sx + spacing;
		float y1 = 50.0f * scaleY;
#if defined(PSP_UI)
		float y2 = 155.0f * scaleY;
#else
		float y2 = 160.0f * scaleY;
#endif
		float cancelSx = 67.0f * scaleX;
		float cancelSy = 67.0f * scaleY;
		
		float cancelX = basePos + posIncrement * 2.0f + (sx - cancelSx) / 2.0f;
		float cancelY = y2 + (sy - cancelSy) / 2.0f;
		
		btn_Upgrade_Vulcan.Setup_Custom(0, Vec2F(basePos + posIncrement * 0.0f, y1), Vec2F(sx, sy), Game::UpgradeType_Vulcan, CallBack(this, Handle_Upgrade), CallBack(this, Handle_RenderUpgrade));
		btn_Upgrade_Beam.Setup_Custom(0, Vec2F(basePos + posIncrement * 1.0f, y1), Vec2F(sx, sy), Game::UpgradeType_Beam, CallBack(this, Handle_Upgrade), CallBack(this, Handle_RenderUpgrade));
		btn_Upgrade_Shock.Setup_Custom(0, Vec2F(basePos + posIncrement * 2.0f, y1), Vec2F(sx, sy), Game::UpgradeType_Shock, CallBack(this, Handle_Upgrade), CallBack(this, Handle_RenderUpgrade));
		btn_Upgrade_Special.Setup_Custom(0, Vec2F(basePos + posIncrement * 0.0f, y2), Vec2F(sx, sy), Game::UpgradeType_Special, CallBack(this, Handle_Upgrade), CallBack(this, Handle_RenderUpgrade));
		btn_Upgrade_Border.Setup_Custom(0, Vec2F(basePos + posIncrement * 1.0f, y2), Vec2F(sx, sy), Game::UpgradeType_BorderPatrol, CallBack(this, Handle_Upgrade), CallBack(this, Handle_RenderUpgrade));
		btn_Cancel.Setup_Shape(0, Vec2F(cancelX, cancelY), g_GameState->GetShape(Resources::UPGRADE_CANCEL), 0, CallBack(this, Handle_GoBack));
		
		AddButton(btn_Upgrade_Vulcan);
		AddButton(btn_Upgrade_Beam);
		AddButton(btn_Upgrade_Shock);
		AddButton(btn_Upgrade_Special);
		AddButton(btn_Upgrade_Border);
		AddButton(btn_Cancel);

#if defined(PSP_UI)
		Translate((VIEW_SX-480)/2, (VIEW_SY-320)/2);
#endif
	}
	
	void Menu_Upgrade::HandleFocus()
	{
		Menu::HandleFocus();

		g_GameState->m_SoundEffects->Play(Resources::SOUND_UPGRADEMENU_OPEN, 0);
	}
	
	void Menu_Upgrade::HandleBack()
	{
		Handle_GoBack(this, 0);
	}

	bool Menu_Upgrade::Render()
	{
		if (!Menu::Render())
			return false;
		
		float a = 0.0f;
		
		if (mDenyAnim.IsRunning_get())
		{
			float t = mDenyAnim.Progress_get();
			
			a = sinf(t * Calc::mPI);
			
			float a1 = a * 0.5f;
			float a2 = a;
			float r = sinf(t * Calc::mPI);
			
			Vec2F size((float)VIEW_SX, 30.0f);
			Vec2F pos = Vec2F(0.0f, (VIEW_SY - size[1]) / 2.0f);
			
			// background
			RenderRect(Vec2F(), Vec2F((float)VIEW_SX, (float)VIEW_SY), 0.0f, 0.0f, 0.0f, a1, g_GameState->GetTexture(Textures::MENU_COLOR_WHITE));
			
			// bar
			RenderRect(pos, size, r, 0.0f, 0.0f, a2, g_GameState->GetTexture(Textures::MENU_COLOR_WHITE));
			
			// text
			RenderText(pos, size, g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE), SpriteColor_Make(200, 200, 200, (int)(a2 * 255.0f)), TextAlignment_Center, TextAlignment_Center, true, mDenyText);
		}

		{
#if defined(PSP_UI)
			const float y = VIEW_SY - 33.0f;
#else
			const float y = VIEW_SY - 35.0f;
#endif

			Menu_InGame::Render_Upgrade(Vec2F(25.0f, y), 1.f);
			const float progress = Game::g_World->m_Player->Credits_get() / 1000.0f;
			StringBuilder<32> sb;
			sb.AppendFormat("%.1f%%", progress * 100.0f);
			RenderText(Vec2F(125.0f, y + 3.0f), Vec2F(0.0f, 18.0f), g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColor_BlendF(SpriteColor_Make(255, 0, 127, 255), SpriteColor_Make(255, 255, 227, 255), a), TextAlignment_Left, TextAlignment_Center, true, sb.ToString());
		}
		
		return true;
	}
	
	void Menu_Upgrade::Handle_Upgrade(void* obj, void* arg)
	{
#ifdef DEBUG
		if (Game::g_World && Game::g_World->m_Player)
			Game::g_World->m_Player->HandleReward(Game::Reward(Game::RewardType_ScoreCustom, 1000));
#endif
		
		Menu_Upgrade* self = (Menu_Upgrade*)obj;
		Button* button = (Button*)arg;
		
		Game::EntityPlayer* player = Game::g_World->m_Player;
		
		Game::UpgradeType upgrade = (Game::UpgradeType)button->m_Info;
		int level = player->GetUpgradeLevel(upgrade);
		
		if (MaxLevelReached(upgrade))
		{
			self->mDenyText = "SOLD OUT!";
			self->mDenyAnim.Start(AnimTimerMode_TimeBased, false, 1.0f, AnimTimerRepeat_None);
			return;
		}

		if (!CanBuy(upgrade, level))
		{
			self->mDenyText = "NOT YET AVAILABLE";
			self->mDenyAnim.Start(AnimTimerMode_TimeBased, false, 1.0f, AnimTimerRepeat_None);
			return;
		}
		
		int cost = g_GameState->m_GameRound->Classic_GetUpgradeCost(upgrade, level);
		
		player->Credits_Decrease(cost);
		
		player->HandleUpgrade(upgrade);
		
		g_GameState->m_SoundEffects->Play(Resources::SOUND_UPGRADEMENU_BUY, 0);
		
#if 1
		Menu_UpgradeBought* menu = (Menu_UpgradeBought*)g_GameState->m_MenuMgr->Menu_get(MenuType_UpgradeBought);
		
		switch (upgrade)
		{
		//void Setup(int captionImage, const char* captionText, int previewImage, const char* footerText);
			case Game::UpgradeType_Beam:
				menu->Setup(Resources::POWERUP_POWER_BEAM,
							"LASER UPGRADED",
							Textures::UPGRADEBOUGHT_LASER,
							Vec2F(178.0f, 138.0f),
							"fire power increased");
				break;
			case Game::UpgradeType_Shock:
				menu->Setup(Resources::POWERUP_POWER_SHOCK,
							"SHOCKWAVE UPGRADED",
							Textures::UPGRADEBOUGHT_SHOCK,
							Vec2F(120.0f, 114.0f),
							Game::g_World->m_Player->GetUpgradeLevel(upgrade) == 1 ? "shockwave unlocked!" : "cooldown decreased");
				break;
			case Game::UpgradeType_Special:
				menu->Setup(Resources::POWERUP_SPECIAL_MAX,
							"SPECIAL ATTACK",
							Textures::UPGRADEBOUGHT_SPECIAL,
							Vec2F(110.0f, 120.0f),
							"more missiles!");
				break;
			case Game::UpgradeType_Vulcan:
				menu->Setup(Resources::POWERUP_POWER_VULCAN,
							"VULCAN UPGRADED",
							Textures::UPGRADEBOUGHT_VULCAN,
							Vec2F(180.0f, 120.0f),
							"fire power increased");
				break;
			case Game::UpgradeType_BorderPatrol:
				menu->Setup(Resources::POWERUP_PATROL_RESET,
							"PATROL ENEMY RESET",
							Textures::UPGRADEBOUGHT_PATROL,
							Vec2F(157.0f, 113.0f),
							"less border patrol enemies");
				break;
			case Game::UpgradeType__Count:
				break;
		}
		
		g_GameState->m_MenuMgr->ActiveMenu_set(MenuType_UpgradeBought);
#else
		g_GameState->ActiveView_set(View_InGame);
#endif
	}
	
	void Menu_Upgrade::Handle_GoBack(void* obj, void* arg)
	{
		g_GameState->ActiveView_set(View_InGame);
	}
	
	void Menu_Upgrade::Handle_RenderUpgrade(void* obj, void* arg)
	{
		Button* button = (Button*)arg;
		
		Game::UpgradeType upgrade = (Game::UpgradeType)button->m_Info;
		
		Vec2F pos = button->Position_get();
		
		Vec2F backPos(0.0f, 0.0f);
		Vec2F overlayPos(5.0f, 5.0f);
		Vec2F upgradePos = backPos + Vec2F(16.0f, 86.0f);
		Vec2F upgradeSpace(15.0f, 0.0f);
		Vec2F upgradeSpace2(6.0f, 0.0f);
		
		const AtlasImageMap* imageWhite = g_GameState->GetTexture(Textures::MENU_COLOR_WHITE);
		const AtlasImageMap* imageOverlay = g_GameState->GetTexture(Textures::UPGRADE_OVERLAY);
		
		Vec2F overlaySize = Vec2F((float)imageOverlay->m_Info->m_ImageSize[0], (float)imageOverlay->m_Info->m_ImageSize[1]);
		
		Game::EntityPlayer* player = Game::g_World->m_Player;
		
		RenderRect(pos + backPos, 1.0f, 1.0f, 1.0f, g_GameState->GetTexture(Textures::UPGRADE_BACK));
		
		// todo: render upgrade level
		// todo: use grayscale background/color if not enough money
		// todo: draw upgrade name
		// todo: draw upgrade cost
		
		int level = player->GetUpgradeLevel(upgrade);
		int maxLevel = player->GetMaxUpgradelevel(upgrade);
//		bool maxLevelReached = MaxLevelReached(upgrade);
		
		if (maxLevel <= 5)
		{
			for (int i = 0; i < level; ++i)
				g_GameState->Render(g_GameState->GetShape(Resources::UPGRADE_STATE1), pos + upgradePos + upgradeSpace * (float)i, 0.0f, SpriteColors::White);
			for (int i = level; i < maxLevel; ++i)
				g_GameState->Render(g_GameState->GetShape(Resources::UPGRADE_STATE0), pos + upgradePos + upgradeSpace * (float)i, 0.0f, SpriteColors::White);
		}
		else
		{
			for (int i = 0; i < level; ++i)
				g_GameState->Render(g_GameState->GetShape(Resources::UPGRADE_STATE1_SM), pos + upgradePos + upgradeSpace2 * (float)i, 0.0f, SpriteColors::White);
			for (int i = level; i < maxLevel; ++i)
				g_GameState->Render(g_GameState->GetShape(Resources::UPGRADE_STATE0_SM), pos + upgradePos + upgradeSpace2 * (float)i, 0.0f, SpriteColors::White);
		}
		
		g_GameState->DataSetActivate(imageWhite->m_TextureAtlas->m_DataSetId);
		SpriteGfx& gfx = *g_GameState->m_SpriteGfx;
		gfx.Reserve(4, 6);
		gfx.WriteBegin();
		float u = imageWhite->m_Info->m_TexCoord[0];
		float v = imageWhite->m_Info->m_TexCoord[1];
		Vec2F p11 = pos + overlayPos + (overlaySize ^ Vec2F(0.0f, 0.0f));
		Vec2F p21 = pos + overlayPos + (overlaySize ^ Vec2F(1.0f, 0.0f));
		Vec2F p12 = pos + overlayPos + (overlaySize ^ Vec2F(0.0f, 1.0f));
		Vec2F p22 = pos + overlayPos + (overlaySize ^ Vec2F(1.0f, 1.0f));
		SpriteColor c11 = Calc::Color_FromHue(GetHue(p11));
		SpriteColor c21 = Calc::Color_FromHue(GetHue(p21));
		SpriteColor c12 = Calc::Color_FromHue(GetHue(p12));
		SpriteColor c22 = Calc::Color_FromHue(GetHue(p22));
		/*if (!canBuy)
		{
			c11 = SpriteColor_BlendF(c11, MakeGrayScale(c11), 0.75f);
			c21 = SpriteColor_BlendF(c21, MakeGrayScale(c21), 0.75f);
			c12 = SpriteColor_BlendF(c12, MakeGrayScale(c12), 0.75f);
			c22 = SpriteColor_BlendF(c22, MakeGrayScale(c22), 0.75f);
		}*/
		gfx.WriteVertex(p11[0], p11[1], c11.rgba, u, v);
		gfx.WriteVertex(p21[0], p21[1], c21.rgba, u, v);
		gfx.WriteVertex(p12[0], p12[1], c12.rgba, u, v);
		gfx.WriteVertex(p22[0], p22[1], c22.rgba, u, v);
		gfx.WriteIndex3(0, 3, 2);
		gfx.WriteIndex3(0, 1, 3);
		gfx.WriteEnd();
		
		RenderRect(pos + overlayPos, overlaySize, 1.0f, 1.0f, 1.0f, imageOverlay);
		
//		Game::g_World->m_Player->Credits_Increase(1);

		g_GameState->Render(g_GameState->GetShape(Resources::UPGRADEVIEW_POWERUP_BACK), pos + overlayPos + overlaySize / 2.0f, 0.0f, SpriteColors::White);
		g_GameState->Render(g_GameState->GetShape(UpgradeType_To_Shape(upgrade)), pos + overlayPos + overlaySize / 2.0f, 0.0f, SpriteColors::White);
		
//		Vec2F textBorder(3.0f, -1.0f);
		Vec2F textBorder(3.0f, 1.0f);
		RectF textRect;
		
		textRect.Setup(pos + overlayPos + textBorder, overlaySize - textBorder * 2.0f);
		
		const FontMap* font = g_GameState->GetFont(Resources::FONT_LGS);
		
		RenderText(textRect.m_Position, textRect.m_Size, font, SpriteColors::White, TextAlignment_Left, TextAlignment_Bottom, true, UpgradeType_To_Name(upgrade));
		
		Mat3x2 matR;
		Mat3x2 matT;
		Mat3x2 mat;

		matR.MakeRotation(Calc::DegToRad(-10.0f));
		//matT.MakeTranslation(pos + overlayPos + overlaySize + Vec2F(-30.0f, 5.0f));
		matT.MakeTranslation(pos + overlayPos + overlaySize + Vec2F(-30.0f, 2.0f));
		mat = matT.MultiplyWith(matR);
		StringBuilder<32> sb;
		sb.AppendFormat("%d/%d", level, maxLevel);
		RenderText(mat, g_GameState->GetFont(Resources::FONT_LGS), SpriteColors::White, sb.ToString());
	}
	
	static bool MaxLevelReached(Game::UpgradeType upgrade)
	{
		int level = Game::g_World->m_Player->GetUpgradeLevel(upgrade);
		int maxLevel = Game::g_World->m_Player->GetMaxUpgradelevel(upgrade);
		
		return level >= maxLevel;
	}
	
	static bool CanBuy(Game::UpgradeType upgrade, int level)
	{	
		int cost = g_GameState->m_GameRound->Classic_GetUpgradeCost(upgrade, level);
		
		int credits = Game::g_World->m_Player->Credits_get();
		
		return credits >= cost;
	}
	
	static float GetHue(Vec2F pos)
	{
		const Vec2F huePoint1(0.0f, (float)VIEW_SY);
		const Vec2F huePoint2((float)VIEW_SX, (float)VIEW_SY - (float)VIEW_SX);
		const PlaneF huePlane = PlaneF::FromPoints(huePoint1, huePoint2);
//		const float hue2 = Calc::DegToRad(195.0f + 180.0f) / Calc::m2PI;
		const float hue2 = Calc::DegToRad(220.0f + 180.0f) / Calc::m2PI;
		const float hue1 = Calc::DegToRad(40.0f + 180.0f) / Calc::m2PI;
		const float hueDistance = (huePoint2 - huePoint1).Length_get();
		const float distance = huePlane.Distance(pos) / hueDistance;
		
		return hue1 + (hue2 - hue1) * distance;
	}
	
/*	static SpriteColor MakeGrayScale(SpriteColor color)
	{
		int c = (color.v[0] + color.v[1] + color.v[2]) / 3;
		
		return SpriteColor_Make(c, c, c, color.v[3]);
	}*/
	
	const char* UpgradeType_To_Name(Game::UpgradeType upgrade)
	{
		switch (upgrade)
		{
			case Game::UpgradeType_Beam:
				return "LASER";
			case Game::UpgradeType_Shock:
				return "SHOCKWAVE" ;
			case Game::UpgradeType_Special:
				return "SPECIAL";
			case Game::UpgradeType_Vulcan:
				return "VULCAN";
			case Game::UpgradeType_BorderPatrol:
				return "ESCALATION";
			default:
#ifndef DEPLOYMENT
				throw ExceptionVA("not implemented");
#else
				return "??";
#endif
		}
	}
	
	static int UpgradeType_To_Shape(Game::UpgradeType type)
	{
		if (g_GameState->DrawMode_get() == VectorShape::DrawMode_Silhouette)
			return Resources::POWERUP_CREDITS;
		
		switch (type)
		{
			case Game::UpgradeType_Beam:
				return Resources::POWERUP_POWER_BEAM;
			case Game::UpgradeType_Shock:
				return Resources::POWERUP_POWER_SHOCK;
			case Game::UpgradeType_Special:
				return Resources::POWERUP_SPECIAL_MAX;
			case Game::UpgradeType_Vulcan:
				return Resources::POWERUP_POWER_VULCAN;
			case Game::UpgradeType_BorderPatrol:
				return Resources::ENEMY_PATROL;
			default:
#ifndef DEPLOYMENT
				throw ExceptionVA("not implemented");
#else
				return Resources::PLAYER_SHIP;
#endif
		}
	}
}
