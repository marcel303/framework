#include "GameState.h"
#include "GuiListSlider.h"
#include "Mat3x2.h"
#include "SoundEffectMgr.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"

namespace GameMenu
{
	GuiListSlider::GuiListSlider(const char* name, const GuiListItem* itemList, int textFont, bool wrapAround, int tag, CallBack onChanged)
		:
		GuiElementBase(),
		mValueCurr(-1),
		mValuePrev(-1),
		mValueCount(0),
		mItemList(itemList),
		mTextFont(textFont),
		mTag(tag),
		OnChanged(onChanged),
		mWrapAround(wrapAround)
	{
		m_Name = name;
		//m_IsEnabled = false;

		while (mItemList[mValueCount].mText)
		{
			mValueCount++;
			Assert(mValueCount < 1000);
		}

		mAnimTimer.Initialize(g_GameState->m_TimeTracker_Global, false);
	}

	int GuiListSlider::Value_get() const
	{
		return mValueCurr;
	}

	void GuiListSlider::Value_set(int value, bool raiseEvent)
	{
		value = ClampValue(value);

		if (value == mValueCurr)
			return;

		mValuePrev = mValueCurr;
		mValueCurr = value;
		mAnimTimer.Start(AnimTimerMode_TimeBased, false, 0.3f, AnimTimerRepeat_None);

		if (raiseEvent && OnChanged.IsSet())
		{
			OnChanged.Invoke(this);
		}
	}

	const GuiListItem* GuiListSlider::Curr_get() const
	{
		if (mValueCurr < 0 || mValueCurr >= mValueCount)
		{
			Assert(false);
			return 0;
		}

		return mItemList + mValueCurr;
	}

	bool GuiListSlider::SelectPrev()
	{
		int value = mWrapAround ? (mValueCurr == 0 ? mValueCount - 1 : mValueCurr - 1) : ClampValue(mValueCurr - 1);

		if (value == mValueCurr)
			return false;

		Value_set(value, true);

		return true;
	}

	bool GuiListSlider::SelectNext()
	{
		int value = mWrapAround ? (mValueCurr + 1) % mValueCount : ClampValue(mValueCurr + 1);

		if (value == mValueCurr)
			return false;

		Value_set(value, true);

		return true;
	}

	GuiElementType GuiListSlider::Type_get() const
	{
		return GuiElementType_ListSlider;
	}

	void GuiListSlider::Render()
	{
		if (m_HasFocus)
		{
			RenderRect(Position_get(), mHitBox.m_Size, 0.0f, 0.0f, 1.0f, 0.25f, g_GameState->GetTexture(Textures::COLOR_WHITE));
		}

		const Vec2F position = Position_get() + mHitBox.m_Size / 2.0f;
		const char* text1 = TextPrev_get();
		const char* text2 = TextCurr_get();
		const float t1 = !mAnimTimer.IsRunning_get() ? 0.0f : 1.0f - mAnimTimer.Progress_get();
		const float t2 = !mAnimTimer.IsRunning_get() ? 1.0f : 1.0f - t1;
		Mat3x2 mat1s;
		Mat3x2 mat2s;
		mat1s.MakeScaling(t1, t1);
		mat2s.MakeScaling(t2, t2);
		Mat3x2 matT;
		matT.MakeTranslation(position);
		Mat3x2 mat1 = matT * mat1s;
		Mat3x2 mat2 = matT * mat2s;
		const FontMap* font = g_GameState->GetFont(mTextFont);
		const SpriteColor c1 = SpriteColor_MakeF(1.0f, 1.0f, 1.0f, t1);
		const SpriteColor c2 = SpriteColor_MakeF(1.0f, 1.0f, 1.0f, t2);
		if (text1 && t1 != 0.0f)
			RenderText(mat1, font, c1, TextAlignment_Center, TextAlignment_Center, text1);
		if (text2 && t2 != 0.0f)
			RenderText(mat2, font, c2, TextAlignment_Center, TextAlignment_Center, text2);
	}

	void GuiListSlider::Update(float dt)
	{
	}

	XBOOL GuiListSlider::HitTest(const Vec2F& pos) const 
	{
		return mHitBox.IsInside(pos);
	}

	Vec2F GuiListSlider::HitBoxCenter_get() const
	{
		return mHitBox.m_Position + Vec2F(10.0f, mHitBox.m_Size[1] / 2.0f);
	}

	bool GuiListSlider::HandleEvent(const Event& e)
	{
		if (e.type == EVT_KEY)
		{
			if (e.key.key == IK_LEFT)
			{
				if (e.key.state)
				{
					SelectPrev();
					g_GameState->m_SoundEffects->Play(Resources::SOUND_MENU_CLICK, SfxFlag_MustFinish);
				}
				return true;
			}
			if (e.key.key == IK_RIGHT)
			{
				if (e.key.state)
				{
					SelectNext();
					g_GameState->m_SoundEffects->Play(Resources::SOUND_MENU_CLICK, SfxFlag_MustFinish);
				}
				return true;
			}
		}

		return false;
	}

	RectF GuiListSlider::HitBox_get() const
	{
		return mHitBox;
	}

	void GuiListSlider::UpdateDesign(bool isVisible, const Vec2F& pos, const Vec2F& size)
	{
		m_IsVisible = isVisible;
		m_Position = pos;
		mSize = size;
		mHitBox.m_Position = pos;
		mHitBox.m_Size = size;
	}

	void GuiListSlider::Translate(int dx, int dy)
	{
		m_Position[0] += dx;
		m_Position[1] += dy;
		mHitBox.m_Position[0] += dx;
		mHitBox.m_Position[1] += dy;
	}

	int GuiListSlider::ClampValue(int value) const
	{
		if (value < 0)
			value = 0;
		if (value >= mValueCount)
			value = mValueCount - 1;

		return value;
	}

	const char* GuiListSlider::TextCurr_get() const
	{
		if (mValueCurr < 0 || mValueCurr >= mValueCount)
			return 0;

		return mItemList[mValueCurr].mText;
	}

	const char* GuiListSlider::TextPrev_get() const
	{
		if (mValuePrev < 0 || mValuePrev >= mValueCount)
			return 0;

		return mItemList[mValuePrev].mText;
	}
}
