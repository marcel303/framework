#pragma once

#include "AnimTimer.h"
#include "GuiElement.h"

namespace GameMenu
{
	class GuiListItem
	{
	public:
		GuiListItem() : mText(0), mTag(0), mTagPtr(0) { }
		GuiListItem(const char* text, int tag) : mText(text), mTag(tag), mTagPtr(0) { }
		GuiListItem(const char* text, void* tagPtr) : mText(text), mTag(0), mTagPtr(tagPtr) { }

		const char* mText;
		int mTag;
		void* mTagPtr;
	};

	class GuiListSlider : public GuiElementBase
	{
	public:
		GuiListSlider(const char* name, const GuiListItem* itemList, int textFont, bool wrapAround, int tag, CallBack onChanged);

		int Value_get() const;
		void Value_set(int value, bool raiseEvent);
		const GuiListItem* Curr_get() const;

		bool SelectPrev();
		bool SelectNext();

		virtual GuiElementType Type_get() const;
		virtual void Render();
		virtual void Update(float dt);
		virtual XBOOL HitTest(const Vec2F& pos) const;
		virtual Vec2F HitBoxCenter_get() const;
		virtual bool HandleEvent(const Event& e);

		virtual RectF HitBox_get() const;
		virtual void UpdateDesign(bool isVisible, const Vec2F& pos, const Vec2F& size);
		virtual void Translate(int dx, int dy);

		CallBack OnChanged;

	private:
		int ClampValue(int value) const;
		const char* TextCurr_get() const;
		const char* TextPrev_get() const;

		Vec2F mSize;
		RectF mHitBox;
		int mValueCurr;
		int mValuePrev;
		int mValueCount;
		const GuiListItem* mItemList;
		int mTextFont;
		int mTag;
		bool mWrapAround;
		AnimTimer mAnimTimer;
	};
}
