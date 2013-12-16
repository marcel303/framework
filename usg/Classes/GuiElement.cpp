#include "Calc.h"
#include "GuiElement.h"

namespace GameMenu
{
	IGuiElement::IGuiElement()
	{
		m_Name = 0;
		m_IsVisible = true;
		m_IsEnabled = true;
		m_IsTouchOnly = false;
		m_HasFocus = false;
	}

	IGuiElement::~IGuiElement()
	{
	}

	Vec2F IGuiElement::HitBoxCenter_get() const
	{
		RectF hitBox = HitBox_get();

		return hitBox.m_Position + hitBox.m_Size * 0.5f;
	}

	void IGuiElement::HandleTouchBegin(const Vec2F& pos)
	{
	}

	void IGuiElement::HandleTouchMove(const Vec2F& pos)
	{
	}

	void IGuiElement::HandleTouchEnd(const Vec2F& pos)
	{
	}

	// --------------
	// GuiElementBase
	// --------------

	GuiElementBase::GuiElementBase()
	{
		m_TransitionProgress = 0.0f;
		m_TransitionEffect = TransitionEffect_Menu;
		m_Alpha = 0.0f;
	}

	GuiElementBase::~GuiElementBase()
	{
	}

	void GuiElementBase::SetTransition(TransitionEffect effect, Vec2F vector)
	{
		m_TransitionEffect = effect;
		m_TransitionVector = vector;
	}
	
	void GuiElementBase::UpdateTransition(float progress)
	{
		m_TransitionProgress = progress;
		
		switch (m_TransitionEffect)
		{
			case TransitionEffect_Slide:
			{
				float offset = 1.0f - sinf(progress * Calc::mPI2);
				
				m_TransitionOffset = m_TransitionVector * offset;
				m_Alpha = 1.0f;
				
				break;
			}
				
			case TransitionEffect_Fade:
			{
				m_Alpha = m_TransitionProgress;
				
				break;
			}
				
			case TransitionEffect_None:
				m_Alpha = 1.0f;
				break;
			
#ifndef DEPLOYMENT
			case TransitionEffect_Menu:
				throw ExceptionVA("invalid transition effect");
#else
			case TransitionEffect_Menu:
				m_Alpha = 1.0f;
				break;
#endif
		}
	}
}
