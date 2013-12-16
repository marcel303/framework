#pragma once

#include "BoundingBox2.h"
#include "CallBack.h"
#include "GuiElement.h"
#include "VectorShape.h"

namespace GameMenu
{
	class GuiCheckbox : public GuiElementBase
	{
	public:
		GuiCheckbox(const char* name, bool isChecked, const char* text, int textFont, CallBack onChange);

		virtual GuiElementType Type_get() const;
		virtual void Render();
		virtual void Update(float dt);
		virtual XBOOL HitTest(const Vec2F& pos) const;
		virtual bool HandleEvent(const Event& e);
		virtual void HandleTouchBegin(const Vec2F& pos);
		virtual void HandleTouchMove(const Vec2F& pos);
		virtual void HandleTouchEnd(const Vec2F& pos);

		//

		virtual void UpdateDesign(bool isVisible, const Vec2F& pos, const Vec2F& size);
		virtual RectF HitBox_get() const;

		//

		virtual void Translate(int dx, int dy);

		//

		bool IsChecked_get() const;
		void IsChecked_set(bool isChecked, bool generateEvent);
		
	private:
		BoundingBox2 m_HitBox;
		CallBack m_OnChange;
		const VectorShape* m_Shape[2];
		bool m_IsChecked;
		const char* m_Text;
		int m_TextFont;
	};
}
