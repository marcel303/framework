#pragma once

#include "BoundingBox2.h"
#include "CallBack.h"
#include "GuiElement.h"
#include "MenuTypes.h"
#include "TempRender.h"

namespace GameMenu
{
	class Button : public GuiElementBase
	{
	public:
		Button();
		
		void Setup_Dummy(const char* name, const Vec2F& pos, int tag, CallBack onClick);
		void Setup_Shape(const char* name, const Vec2F& pos, const VectorShape* shape, int tag, CallBack onClick);
		void Setup_Text(const char* name, const Vec2F& pos, const Vec2F& size, const char* text, int tag, CallBack onClick);
		void Setup_TextEx(const char* name, const Vec2F& pos, Vec2F size, int fontId, TextAlignment textAlignment, const char* text, int tag, CallBack onClick);
		void Setup_Custom(const char* name, const Vec2F& pos, const Vec2F& size, int tag, CallBack onClick, CallBack onRender);
		
		static Button Make_Dummy(const char* name, const Vec2F& pos, int tag, CallBack onClick);
		static Button Make_Shape(const char* name, const Vec2F& pos, const VectorShape* shape, int tag, CallBack onClick);
		static Button Make_Text(const char* name, const Vec2F& pos, const Vec2F& size, const char* text, int tag, CallBack onClick);
		static Button Make_TextEx(const char* name, const Vec2F& pos, int fontId, const char* text, int tag, CallBack onClick);
		static Button Make_TextEx2(const char* name, const Vec2F& pos, const Vec2F& size, int fontId, TextAlignment textAlignment, const char* text, int tag, CallBack onClick);
		static Button Make_Custom(const char* name, const Vec2F& pos, const Vec2F& size, int tag, CallBack onClick, CallBack onRender);
		
		virtual GuiElementType Type_get() const;
		virtual void Update(float dt);
		virtual void Render();
		virtual bool HandleEvent(const Event& e);
		
		// --------------------
		// Hit effect
		// --------------------
		void SetHitEffect(HitEffect effect);
		
		HitEffect m_HitEffect;
		
		// --------------------
		// Drawing
		// --------------------
		enum Mode
		{
			Mode_Undefined,
			Mode_Dummy,
			Mode_Text,
			Mode_Shape,
			Mode_Custom
		};
		
		inline Vec2F Size_get() const
		{
			return m_Box.Max - m_Box.Min;
		}

		virtual void UpdateDesign(bool isVisible, const Vec2F& _pos, const Vec2F& _size);

		inline virtual RectF HitBox_get() const
		{
			return RectF(m_Box.Min, Size_get());
		}

		Mode m_Mode;
		const VectorShape* m_Shape;
		const char* m_Text;
		int m_FontId;
		TextAlignment m_TextAlignment;
		float m_CustomOpacity;
		CallBack OnRender;
		CallBack OnTouchMove;
		
		// --------------------
		// Hit test
		// --------------------
	public:
		virtual XBOOL HitTest(const Vec2F& pos) const
		{
			return m_Box.Inside(pos);
		}
		
		virtual void HandleTouchBegin(const Vec2F& pos);
		virtual void HandleTouchMove(const Vec2F& pos);
		
	private:
		BoundingBox2 m_Box;
	public:
		CallBack OnClick;
		Vec2F m_TouchPos;
		Vec2F m_TouchPos01;
		
		// --------------------
		// Logic
		// --------------------
		int m_Info;
		float m_InfoF;

		// --------------------
		// Transformation
		// --------------------
		virtual void Translate(int dx, int dy);
	};
}
