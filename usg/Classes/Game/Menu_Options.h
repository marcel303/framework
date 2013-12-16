#pragma once

#include "Menu.h"
#include "TriggerTimerEx.h"

namespace GameMenu
{
	class Menu_Options : public Menu
	{
	public:
		Menu_Options();
		virtual ~Menu_Options();
		
		virtual void Init();
		virtual void HandleFocus();
		virtual void HandleBack();
		
	private:
		static void Handle_ToggleSound(void* obj, void* arg);
		static void Handle_ToggleMusic(void* obj, void* arg);
		static void Handle_ToggleAlternateControls(void* obj, void* arg);
		static void Handle_ToggleScreenFlip(void* obj, void* arg);
		static void Handle_VolumeChange(void* obj, void* arg);
		static void Handle_Advanced(void* obj, void* arg);
		static void Handle_Credits(void* obj, void* arg);
	public:
		static void Handle_Accept(void* obj, void* arg);
		static void Handle_Dismiss(void* obj, void* arg);
	private:
//		static void Render_Volume(void* obj, void* arg);
		
		TriggerTimerG m_SoundVolumePreviewTimer;
	};
}
