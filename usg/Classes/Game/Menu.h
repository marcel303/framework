#pragma once

#include "AnimTimer.h"
#include "BoundingBox2.h"
#include "CallBack.h"
#include "ColList.h"
#include "Forward.h"
#include "GuiElement.h"
#include "libgg_forward.h"
#include "libiphone_forward.h"
#include "MenuTypes.h"
#include "Types.h"

class VectorShape;

namespace GameMenu
{
	class Menu
	{
	public:
		Menu();
		virtual ~Menu();
		
		void Reset();
		void SetTransition(TransitionEffect effect, Vec2F vector, float duration);
		
		virtual void Init();
		
		virtual void Update(float dt);
		virtual bool Render();
		
		void Activate();
		void Deactivate();
		
		virtual void HandleFocus();
		
		void AddElement(IGuiElement* element);
		IGuiElement* FindElement(const char* name);
		bool TryFindElement(const char* name, IGuiElement** o_element);
		
		void AddButton(const Button& b);      // legacy
		template <typename T>
		T FindElementTyped(const char* name, GuiElementType type)
		{
			IGuiElement* element = FindElement(name);
			if (element->Type_get() != type)
				throw ExceptionVA("element not found: %s", name);
			return (T)element;
		}

		inline Button*        FindButton    (const char* name) { return FindElementTyped<Button*       >(name, GuiElementType_Button    ); }
		inline GuiCheckbox*   FindCheckbox  (const char* name) { return FindElementTyped<GuiCheckbox*  >(name, GuiElementType_CheckBox  ); }
		inline GuiSlider*     FindSlider    (const char* name) { return FindElementTyped<GuiSlider*    >(name, GuiElementType_Slider    ); }
		inline GuiListSlider* FindListSlider(const char* name) { return FindElementTyped<GuiListSlider*>(name, GuiElementType_ListSlider); }

		Col::ListNode<IGuiElement*>* FindNearestElement(int dx, int dy);
		Col::ListNode<IGuiElement*>* FindRelative(int direction);

		void MoveToDefault();
		void MoveSelection(int dx, int dy);
		void MoveNext();
		void MovePrev();

//		virtual XBOOL TouchTest(const Vec2F& location);
		bool HandleTouchBegin(const TouchInfo& touchInfo);
		bool HandleTouchMove(const TouchInfo& touchInfo);
		bool HandleTouchEnd(const TouchInfo& touchInfo);

		bool HandleEvent(const Event& e);
		virtual void HandleBack();
		virtual bool HandlePause();
		void HandleSelect();
		
	protected:
		// --------------------
		// State
		// --------------------
		void State_set(MenuState state);
		bool IsActive_get() const
		{
			return m_State == MenuState_ACTIVE || m_State == MenuState_ACTIVATE;
		}

		MenuState m_State;
		
		// --------------------
		// Elements
		// --------------------
		IGuiElement* Focus_get();
		void Focus_set(Col::ListNode<IGuiElement*>* focus);
		
		Col::List<IGuiElement*> m_Elements;
		Col::ListNode<IGuiElement*>* m_Focus;
		
		// --------------------
		// Transition
		// --------------------
		TransitionEffect m_TransitionEffect;
		Vec2F m_TransitionVector;
		float m_TransitionDuration;
		float m_TransitionProgress;

		// --------------------
		// Transformation
		// --------------------
		void Translate(int dx, int dy);
	};

	//

	class DesignInfo
	{
	public:
		DesignInfo(const char* name, bool isVisible, Vec2F position, Vec2F size)
		{
			mName = name;
			mIsVisible = isVisible;
			mPosition = position;
			mSize = size;
		}

		const char* mName;
		bool mIsVisible;
		Vec2F mPosition;
		Vec2F mSize;
	};

	void DesignApply(Menu* menu, DesignInfo* designInfoList, int designInfoCount);

	#define DESIGN_APPLY(menu, designInfoList) DesignApply(menu, designInfoList, sizeof(designInfoList) / sizeof(DesignInfo))
}
