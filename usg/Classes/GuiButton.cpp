#include "FontMap.h"
#include "GameState.h"
#include "GuiButton.h"
#include "MenuRender.h"
#include "UsgResources.h"
#include "ResAccess.h"
#include "SoundEffectMgr.h"
#include "TempRender.h"
#include "Textures.h"

namespace GameMenu
{
	Button::Button() : GuiElementBase()
	{
		m_Mode = Mode_Undefined,
		m_Shape = 0; 
		m_Text = 0;
		m_FontId = -1;
		m_TextAlignment = TextAlignment_Center;
		m_CustomOpacity = 1.0f;
		
		m_HitEffect = HitEffect_Particles;
		
		m_Info = 0;
		m_InfoF = 0.0f;
	}
	
	void Button::Setup_Dummy(const char* name, const Vec2F& pos, int tag, CallBack onClick)
	{
		m_Mode = Mode_Undefined;
		m_Position = pos;
		m_Name = name;
		m_Info = tag;
		OnClick = onClick;
		
		m_Box = BoundingBox2(pos, pos + Vec2F(50.0f, 25.0f));
	}
	
	void Button::Setup_Shape(const char* name, const Vec2F& pos, const VectorShape* shape, int tag, CallBack onClick)
	{
		m_Mode = Mode_Shape;
		m_Position = pos;
		m_Shape = shape;
		m_Name = name;
		m_Info = tag;
		OnClick = onClick;
		
		m_Box = BoundingBox2(pos + shape->m_Shape.m_BoundingBox.m_Min.ToF(), pos + shape->m_Shape.m_BoundingBox.m_Max.ToF());
	}
	
	void Button::Setup_Text(const char* name, const Vec2F& pos, const Vec2F& size, const char* text, int tag, CallBack onClick)
	{
		m_Mode = Mode_Text;
		m_Position = pos;
		m_Text = text;
		m_Name = name;
		m_Info = tag;
		OnClick = onClick;
		
		m_Box = BoundingBox2(pos, pos + size);
	}
	
	void Button::Setup_TextEx(const char* name, const Vec2F& pos, Vec2F size, int fontId, TextAlignment textAlignment, const char* text, int tag, CallBack onClick)
	{
		const FontMap* font = g_GameState->GetFont(fontId);

		if (size[0] == 0.0f)
			size[0] = font->m_Font.MeasureText(text);
		if (size[1] == 0.0f)
			size[1] = font->m_Font.m_Height;
		
		m_Mode = Mode_Text;
		m_Position = pos;
		m_Text = text;
		m_FontId = fontId;
		m_TextAlignment = textAlignment;
		m_Name = name;
		m_Info = tag;
		OnClick = onClick;
		
		m_Box = BoundingBox2(pos, pos + size);
	}
	
	void Button::Setup_Custom(const char* name, const Vec2F& pos, const Vec2F& size, int tag, CallBack onClick, CallBack onRender)
	{
		m_Mode = Mode_Custom;
		m_Position = pos;
		m_Name = name;
		m_Info = tag;
		OnClick = onClick;
		OnRender = onRender;
		
		m_Box = BoundingBox2(pos, pos + size);
	}
	
	//
	
	Button Button::Make_Dummy(const char* name, const Vec2F& pos, int tag, CallBack onClick)
	{
		Button result;
		result.Setup_Dummy(name, pos, tag, onClick);
		return result;
	}
	
	Button Button::Make_Shape(const char* name, const Vec2F& pos, const VectorShape* shape, int tag, CallBack onClick)
	{
		Button result;
		result.Setup_Shape(name, pos, shape, tag, onClick);
		return result;
	}
	
	Button Button::Make_Text(const char* name, const Vec2F& pos, const Vec2F& size, const char* text, int tag, CallBack onClick)
	{
		Button result;
		result.Setup_Text(name, pos, size, text, tag, onClick);
		return result;
	}

	Button Button::Make_TextEx(const char* name, const Vec2F& pos, int fontId, const char* text, int tag, CallBack onClick)
	{
		Button result;
		result.Setup_TextEx(name, pos, Vec2F(), fontId, TextAlignment_Center, text, tag, onClick);
		return result;
	}

	Button Button::Make_TextEx2(const char* name, const Vec2F& pos, const Vec2F& size, int fontId, TextAlignment textAlignment, const char* text, int tag, CallBack onClick)
	{
		Button result;
		result.Setup_TextEx(name, pos, size, fontId, textAlignment, text, tag, onClick);
		return result;
	}
	
	Button Button::Make_Custom(const char* name, const Vec2F& pos, const Vec2F& size, int tag, CallBack onClick, CallBack onRender)
	{
		Button result;
		result.Setup_Custom(name, pos, size, tag, onClick, onRender);
		return result;
	}

	//
	
	GuiElementType Button::Type_get() const
	{
		return GuiElementType_Button;
	}

	void Button::Update(float dt)
	{
	}

	void Button::Render()
	{
		Vec2F pos = Position_get();
		
#ifdef DEBUG
		RenderRect(m_Box.Min, m_Box.Size_get(), 0.0f, 1.0f, 0.0f, 0.2f, g_GameState->GetTexture(Textures::COLOR_WHITE));
#endif

		float opacity = m_Alpha * m_CustomOpacity;
		
		if (m_Shape)
		{
			const SpriteColor color = SpriteColor_Make(255, 255, 255, (int)((opacity) * 255.0f));
			
			g_GameState->Render(m_Shape, pos, 0.0f, color);
		}
		else if (OnRender.IsSet())
		{
			// nop
		}
		else if (m_Text)
		{
			//RenderQuad(pos, pos + Size_get(), 1.0f, 1.0f, 1.0f, opacity, g_GameState->GetTexture(Textures::COLOR_BLACK));
			
			int fontId = m_FontId < 0 ? Resources::FONT_LGS : m_FontId;
			
			RenderText(pos, Size_get(), g_GameState->GetFont(fontId), SpriteColors::White, m_TextAlignment, TextAlignment_Center, true, m_Text);
		}
		else
		{
			RenderQuad(m_Box.Min, m_Box.Max, 1.0f, 1.0f, 1.0f, opacity, g_GameState->GetTexture(Textures::COLOR_WHITE));
		}
		
		if (OnRender.IsSet())
			OnRender.Invoke(this);
	}

	bool Button::HandleEvent(const Event& e)
	{
		if (e.type == EVT_MENU_SELECT)
		{
			HandleTouchBegin(HitBoxCenter_get());
			return true;
		}

		return false;
	}
	
	void Button::SetHitEffect(HitEffect effect)
	{
		m_HitEffect = effect;
	}
	
	void Button::UpdateDesign(bool isVisible, const Vec2F& _pos, const Vec2F& _size)
	{
		Vec2F pos = _pos;
		Vec2F size = _size;

		m_IsVisible = isVisible;
		m_Position = pos;

		if (m_Mode == Mode_Shape)
		{
			pos += m_Shape->m_Shape.m_BoundingBox.m_Min.ToF();
			size = (m_Shape->m_Shape.m_BoundingBox.m_Max - m_Shape->m_Shape.m_BoundingBox.m_Min).ToF();
		}

		m_Box.Min = pos;
		m_Box.Max = pos + size;
	}
	
	void Button::HandleTouchBegin(const Vec2F& pos)
	{	
		m_TouchPos = pos - Position_get();
		m_TouchPos01 = Vec2F(Calc::Mid(m_TouchPos[0] / Size_get()[0], 0.0f, 1.0f), Calc::Mid(m_TouchPos[1] / Size_get()[1], 0.0f, 1.0f));
		
		if (OnClick.IsSet())
			OnClick.Invoke(this);
		
		//
		
		g_GameState->m_SoundEffects->Play(Resources::SOUND_MENU_CLICK, SfxFlag_MustFinish);
		
		//
		
		switch (m_HitEffect)
		{
			case HitEffect_None:
				break;
				
			case HitEffect_Particles:
			{
				HitEffect_Particles_Rect(RectF(m_Box.Min, m_Box.Max - m_Box.Min));
				break;
			}
		}
	}
	
	void Button::HandleTouchMove(const Vec2F& pos)
	{
		m_TouchPos = pos - Position_get();
		m_TouchPos01 = Vec2F(Calc::Mid(m_TouchPos[0] / Size_get()[0], 0.0f, 1.0f), Calc::Mid(m_TouchPos[1] / Size_get()[1], 0.0f, 1.0f));
		
		if (OnTouchMove.IsSet())
			OnTouchMove.Invoke(this);
	}

	void Button::Translate(int dx, int dy)
	{
		m_Position[0] += dx;
		m_Position[1] += dy;
		m_Box.Min[0] += dx;
		m_Box.Min[1] += dy;
		m_Box.Max[0] += dx;
		m_Box.Max[1] += dy;
	}
}
