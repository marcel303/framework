#pragma once

#include "Menu.h"

namespace GameMenu
{
	class Menu_Pause : public Menu
	{
	public:
		Menu_Pause();
		virtual ~Menu_Pause();
		
		virtual void Init();
//		virtual void Update(float dt);
		virtual void HandleFocus();
		virtual void HandleBack();
		virtual bool HandlePause();
		
	private:
		static void Render_Calibrate(void* obj, void* arg);
		static void Handle_Calibrate(void* obj, void* arg);
		static void Handle_Resume(void* obj, void* arg);
		static void Handle_Options(void* obj, void* arg);
		static void Handle_Stop(void* obj, void* arg);
	};
}
