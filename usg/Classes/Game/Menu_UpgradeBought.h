#pragma once

#include "FixedSizeString.h"
#include "Menu.h"

namespace GameMenu
{
	class Menu_UpgradeBought : public Menu
	{
	public:
		Menu_UpgradeBought();
		
		virtual void Init();
		virtual bool Render();
		
		void Setup(int captionImage, const char* captionText, int previewImage, Vec2F previewPos, const char* footerText);
		
	private:
		int m_CaptionImage;
		FixedSizeString<256> m_CaptionText;
		int m_PreviewImage;
		Vec2F m_PreviewPos;
		FixedSizeString<256> m_FooterText;
		
		static void Handle_Ok(void* obj, void* arg);
		static void Render_Text(void* obj, void* arg);
	};
}
