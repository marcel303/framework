#include "GameState.h"
#include "GuiCheckbox.h"
#include "MenuRender.h"
#include "SoundEffectMgr.h"
#include "TempRender.h"
#include "UsgResources.h"

namespace GameMenu
{
	GuiCheckbox::GuiCheckbox(const char* name, bool isChecked, const char* text, int textFont, CallBack onChange) : GuiElementBase()
	{
		m_Name = name;
		m_IsVisible = true;
		m_IsEnabled = true;
		m_IsTouchOnly = false;
		m_IsChecked = isChecked;
		m_OnChange = onChange;
		m_Text = text;
		m_TextFont = textFont >= 0 ? textFont : Resources::FONT_USUZI_SMALL;

		m_Shape[0] = g_GameState->GetShape(Resources::CHECKBOX_0);
		m_Shape[1] = g_GameState->GetShape(Resources::CHECKBOX_1);
	}

	GuiElementType GuiCheckbox::Type_get() const
	{
		return GuiElementType_CheckBox;
	}

	void GuiCheckbox::Render()
	{
		g_GameState->Render(m_Shape[0], Position_get(), 0.0f, SpriteColor_MakeF(1.0f, 1.0f, 1.0f, m_Alpha));

		if (m_IsChecked)
			g_GameState->Render(m_Shape[1], Position_get(), 0.0f, SpriteColor_MakeF(1.0f, 1.0f, 1.0f, m_Alpha));
		
		Vec2F offset(35.0f, 0.0f);
		
		if (m_Text)
		{
			RenderText(
				Position_get() + offset, 
				Vec2F(0.0f, 0.0f),
				g_GameState->GetFont(m_TextFont),
				SpriteColor_MakeF(1.0f, 1.0f, 1.0f, m_Alpha),
				TextAlignment_Left,
				TextAlignment_Top,
				true, 
				m_Text);
		}
	}

	void GuiCheckbox::Update(float dt)
	{
	}

	XBOOL GuiCheckbox::HitTest(const Vec2F& pos) const
	{
		return m_IsEnabled && HitBox_get().IsInside(pos);
	}

	bool GuiCheckbox::HandleEvent(const Event& e)
	{
		if (e.type == EVT_MENU_SELECT)
		{
			HandleTouchBegin(HitBoxCenter_get());
			return true;
		}

		return false;
	}

	void GuiCheckbox::HandleTouchBegin(const Vec2F& pos)
	{
		IsChecked_set(!m_IsChecked, true);

		g_GameState->m_SoundEffects->Play(Resources::SOUND_MENU_CLICK, SfxFlag_MustFinish);

		HitEffect_Particles_Rect(RectF(m_HitBox.Min, m_HitBox.Max - m_HitBox.Min));
	}

	void GuiCheckbox::HandleTouchMove(const Vec2F& pos)
	{
	}

	void GuiCheckbox::HandleTouchEnd(const Vec2F& pos)
	{
	}

	//

	void GuiCheckbox::UpdateDesign(bool isVisible, const Vec2F& _pos, const Vec2F& _size)
	{
		Vec2F pos = _pos;
		Vec2F size = _size;

		m_Position = pos;
		m_IsVisible = isVisible;

		pos += m_Shape[0]->m_Shape.m_BoundingBox.m_Min.ToF();
		size = (m_Shape[0]->m_Shape.m_BoundingBox.m_Max - m_Shape[0]->m_Shape.m_BoundingBox.m_Min).ToF();

		m_HitBox.Min = pos;
		m_HitBox.Max = pos + size;
	}

	RectF GuiCheckbox::HitBox_get() const
	{
		return
			RectF(
				m_HitBox.Min.ToF(),
				m_HitBox.Max.ToF() - m_HitBox.Min.ToF());
	}

	//

	void GuiCheckbox::Translate(int dx, int dy)
	{
		m_Position[0] += dx;
		m_Position[1] += dy;
		m_HitBox.Min[0] += dx;
		m_HitBox.Min[1] += dy;
		m_HitBox.Max[0] += dx;
		m_HitBox.Max[1] += dy;
	}

	//

	bool GuiCheckbox::IsChecked_get() const
	{
		return m_IsChecked;
	}

	void GuiCheckbox::IsChecked_set(bool isChecked, bool generateEvent)
	{
		m_IsChecked = isChecked;

		if (generateEvent && m_OnChange.IsSet())
			m_OnChange.Invoke(this);
	}
}
