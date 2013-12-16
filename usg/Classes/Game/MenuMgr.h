#pragma once

#include "libiphone_forward.h"
#include "Menu.h"

namespace GameMenu
{
	class MenuMgr
	{
	public:
		MenuMgr();
		~MenuMgr();
		
		void Initialize();
		void Reset();
		
		Menu* ActiveMenu_get();
		void ActiveMenu_set(MenuType menu);
		void CursorEnabled_set(bool enabled);

		void Update(float dt);
		void Render();

		bool HandleTouchBegin(const TouchInfo& touchInfo);
		bool HandleEvent(const Event& e);
		
		Menu* Menu_get(MenuType menu);

		void HandleFocus(IGuiElement* oldElement, IGuiElement* newElement);
		
	private:
		static bool HandleTouchBegin(void* obj, const TouchInfo& touchInfo);
		static bool HandleTouchMove(void* obj, const TouchInfo& touchInfo);
		static bool HandleTouchEnd(void* obj, const TouchInfo& touchInfo);
		
		Menu* m_Menus[MenuType__End];
		
		MenuType m_ActiveMenu;

		bool m_CursorEnabled;
	};
}
