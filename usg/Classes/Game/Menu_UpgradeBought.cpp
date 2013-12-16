#include "Atlas_ImageInfo.h"
#include "GameSettings.h"
#include "GameState.h"
#include "GuiButton.h"
#include "Mat3x2.h"
#include "Menu_UpgradeBought.h"
#include "MenuRender.h"
#include "TempRender.h"
#include "TextureAtlas.h"
#include "UsgResources.h"

namespace GameMenu
{
	Menu_UpgradeBought::Menu_UpgradeBought()
	{
	}
	
	void Menu_UpgradeBought::Init()
	{
		SetTransition(TransitionEffect_Fade, Vec2F(0.0f, 100.0f), 0.3f);
		
		AddButton(Button::Make_Shape(0, Vec2F(VIEW_SX - 110.0f, VIEW_SY - 125.0f), g_GameState->GetShape(Resources::PAUSEVIEW_BUTTON_RESUME), 0, CallBack(this, Handle_Ok)));
		//AddButton(Button::Make_Custom(0, Vec2F(0.0f, 0.0f), Vec2F(0.0f, 0.0f), 0, CallBack(this, Handle_Ok), CallBack(this, Render_Empty)));
	}
	
	bool Menu_UpgradeBought::Render()
	{
		if (!Menu::Render())
			return false;
		
		// Caption
		
		g_GameState->Render(g_GameState->GetShape(m_CaptionImage), Vec2F(45.0f, 73.0f), 0.0f, SpriteColors::White);
		
		{
			Mat3x2 mat;
			Mat3x2 matT;
			Mat3x2 matS;
			matT.MakeTranslation(Vec2F(80.0f, 73.0f));
			matS.MakeScaling(0.6f, 0.8f);
			mat = matT * matS;
			
			RenderText(mat, g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE), SpriteColors::White, TextAlignment_Left, TextAlignment_Center, m_CaptionText.c_str());
		}
		
		// Preview
		
		RenderRect(m_PreviewPos, SpriteColors::White, g_GameState->GetTexture(m_PreviewImage));
		
		// Footer
		
		{
			Mat3x2 mat;
			Mat3x2 matT;
			Mat3x2 matS;
			matT.MakeTranslation(Vec2F(28.0f, VIEW_SY - 56.0f));
			matS.MakeScaling(0.6f, 0.8f);
			mat = matT * matS;
			
			RenderText(mat, g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColors::White, TextAlignment_Left, TextAlignment_Bottom, m_FooterText.c_str());
		}
		
		return true;
	}
	
	void Menu_UpgradeBought::Setup(int captionImage, const char* captionText, int previewImage, Vec2F previewPos, const char* footerText)
	{
		m_CaptionImage = captionImage;
		m_CaptionText = captionText;
		m_PreviewImage = previewImage;
		m_PreviewPos = previewPos;
		m_FooterText = footerText;

		if (VIEW_SX > 480)
		{
			PointI size = g_GameState->GetTexture(previewImage)->m_Info->m_Size;
			m_PreviewPos = Vec2F((VIEW_SX-size[0])/2.0f, (VIEW_SY-size[1])/2.0f);
		}
	}
	
	void Menu_UpgradeBought::Handle_Ok(void* obj, void* arg)
	{
		g_GameState->ActiveView_set(View_InGame);
	}
};
