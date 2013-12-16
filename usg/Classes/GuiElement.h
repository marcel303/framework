#pragma once

#include "Event.h"
#include "MenuTypes.h"
#include "Types.h"

namespace GameMenu
{
	enum GuiElementType
	{
		GuiElementType_Button = 0,
		GuiElementType_Slider = 1,
		GuiElementType_CheckBox = 2,
		GuiElementType_TextField = 3,
		GuiElementType_VectorShape = 4,
		GuiElementType_ListSlider = 5
	};

	class IGuiElement
	{
	public:
		IGuiElement();
		virtual ~IGuiElement();

		virtual GuiElementType Type_get() const = 0;
		virtual void Render() = 0;
		virtual void Update(float dt) = 0;
		virtual XBOOL HitTest(const Vec2F& pos) const = 0;
		virtual bool HandleEvent(const Event& e) = 0;
		virtual void HandleTouchBegin(const Vec2F& pos);
		virtual void HandleTouchMove(const Vec2F& pos);
		virtual void HandleTouchEnd(const Vec2F& pos);
		virtual void UpdateTransition(float progress) = 0;
		virtual void HandleFocusChanged(bool hasFocus) { m_HasFocus = hasFocus; }

		//

		virtual void UpdateDesign(bool isVisible, const Vec2F& pos, const Vec2F& size) = 0;
		virtual RectF HitBox_get() const = 0;
		virtual Vec2F HitBoxCenter_get() const;

		//

		virtual void Translate(int dx, int dy) = 0;

		//

		const char* m_Name;
		bool m_IsVisible;
		bool m_IsEnabled;
		bool m_IsTouchOnly;
		bool m_HasFocus;
	};

	class GuiElementBase : public IGuiElement
	{
	public:
		GuiElementBase();
		virtual ~GuiElementBase();

		Vec2F m_Position;

		// --------------------
		// Transition
		// --------------------
		void SetTransition(TransitionEffect effect, Vec2F vector);
		//void StartTransition();
		void UpdateTransition(float progress);

		TransitionEffect m_TransitionEffect;
		Vec2F m_TransitionVector;
		float m_TransitionProgress;
		float m_Alpha;
		Vec2F m_TransitionOffset;

		// --------------------
		// Drawing
		// --------------------

		inline Vec2F Position_get() const
		{
			return m_Position + m_TransitionOffset;
		}
	};
}
