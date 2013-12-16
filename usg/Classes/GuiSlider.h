#pragma once

#include "CallBack.h"
#include "GuiElement.h"
#include "TriggerTimerEx.h"

namespace GameMenu
{
	class GuiSlider : public GuiElementBase
	{
	public:
		GuiSlider(const char* name, float min, float max, float value, const char* text, int textFont, int tag, CallBack onChange);
		virtual ~GuiSlider();

		float Value_get() const;
		void Value_set(float value, bool raiseEvent);

		virtual GuiElementType Type_get() const;
		virtual void Render();
		virtual void Update(float dt);
		virtual XBOOL HitTest(const Vec2F& pos) const;
		virtual bool HandleEvent(const Event& e);

		virtual void HandleTouchBegin(const Vec2F& pos);
		virtual void HandleTouchMove(const Vec2F& pos);
		virtual void HandleTouchEnd(const Vec2F& pos);

		virtual RectF HitBox_get() const;
		virtual void UpdateDesign(bool isVisible, const Vec2F& pos, const Vec2F& size);

		virtual void Translate(int dx, int dy);

		void AutoMoveEnable(int direction, bool enable);
		void HandleAutoMove(int direction);
		void AbsoluteMove(float speed);
		void LockEnable(bool enable);

		int mTag;

	private:
		float ClampValue(float value) const;

		Vec2F mSize;
		RectF mHitBox;
		float mMin;
		float mMax;
		float mValue;
		const char* mText;
		int mTextFont;
		CallBack mOnChange;
		TriggerTimerG mAutoMove[2];
		float mAbsoluteMoveSpeed;
		bool mLockEnable;
	};
}
