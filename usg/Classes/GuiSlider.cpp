#include "EventManager.h"
#include "GameState.h"
#include "GuiSlider.h"
#include "ResAccess.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"

namespace GameMenu
{
	static GuiSlider* sActiveSlider = 0;

	class SliderEventHandler : public EventHandler
	{
	public:
		virtual bool OnEvent(Event& event)
		{
			if (event.type == EVT_KEY)
			{
				if (event.key.key == IK_LEFT)
				{
					Assert(sActiveSlider != 0);
					sActiveSlider->AutoMoveEnable(-1, event.key.state != 0);
				}
				if (event.key.key == IK_RIGHT)
				{
					Assert(sActiveSlider != 0);
					sActiveSlider->AutoMoveEnable(+1, event.key.state != 0);
				}
				if (event.key.key == IK_ENTER)
				{
					if (event.key.state)
						Disable();
				}
			}
			if (event.type == EVT_JOYMOVE_ABS && event.joy_move.axis == 0)
			{
				const float scale = 32700.0f;
				sActiveSlider->AbsoluteMove(event.joy_move.value / scale);
			}

			bool close = false;

			if (event.type == EVT_JOYBUTTON)
			{
			#ifdef PSP
				close |= event.joy_button.state && event.joy_button.button == INPUT_BUTTON_PSP_CROSS;
				close |= event.joy_button.state && event.joy_button.button == INPUT_BUTTON_PSP_CIRCLE;
			#endif
			}

			close |= event.type == EVT_JOYDISCONNECT;
			close |= event.type == EVT_MENU_BACK;
			close |= event.type == EVT_MENU_SELECT;

			if (close)
			{
				Disable();
			}

			//

			if (event.type == EVT_MOUSEMOVE || event.type == EVT_MOUSEMOVE_ABS)
			{
				return false;
			}
			if (event.type == EVT_MOUSEBUTTON && event.mouse_button.state)
			{
				Disable();
				return false;
			}

			return true;
		}

		void Disable()
		{
			Assert(sActiveSlider != 0);
			sActiveSlider->AutoMoveEnable(-1, false);
			sActiveSlider->AutoMoveEnable(+1, false);
			sActiveSlider->LockEnable(false);
			EventManager::I().Disable(EVENT_PRIO_INTERFACE_SLIDER);
			sActiveSlider = 0;
		}
	};

	static SliderEventHandler sSliderEventHandler;
	static int sSliderIsInitialized = 0;

	static void SliderInit()
	{
		if (sSliderIsInitialized == 0)
			EventManager::I().AddEventHandler(&sSliderEventHandler, EVENT_PRIO_INTERFACE_SLIDER);

		sSliderIsInitialized++;
	}

	static void SliderShut()
	{
		sSliderIsInitialized--;

		if (sSliderIsInitialized == 0)
			EventManager::I().RemoveEventHandler(&sSliderEventHandler, EVENT_PRIO_INTERFACE_SLIDER);
	}

	GuiSlider::GuiSlider(const char* name, float min, float max, float value, const char* text, int textFont, int tag, CallBack onChange) : GuiElementBase()
	{
		SliderInit();

		m_Name = name;

		//

		mMin = min;
		mMax = max;
		mValue = ClampValue(value);
		mText = text;
		mTextFont = textFont >= 0 ? textFont : Resources::FONT_USUZI_SMALL;
		mTag = tag;
		mOnChange = onChange;
		mAbsoluteMoveSpeed = 0.0f;
		mLockEnable = false;
	}

	GuiSlider::~GuiSlider()
	{
		SliderShut();
	}

	float GuiSlider::Value_get() const
	{
		return mValue;
	}

	void GuiSlider::Value_set(float value, bool raiseEvent)
	{
		value = ClampValue(value);

		if (value == mValue)
			return;

		mValue = value;

		if (raiseEvent && mOnChange.IsSet())
			mOnChange.Invoke(this);
	}

	GuiElementType GuiSlider::Type_get() const
	{
		return GuiElementType_Slider;
	}

	void GuiSlider::Render()
	{
		Vec2F borderSize(2.0f, 2.0f);
		
		// render text
		
		if (mText)
		{
			RenderText(
				Position_get() - Vec2F(10.0f, 0.0f),
				Vec2F(0.0f, mSize[1]),
				g_GameState->GetFont(mTextFont),
				SpriteColor_MakeF(1.0f, 1.0f, 1.0f, m_Alpha),
				TextAlignment_Right,
				TextAlignment_Center,
				true,
				mText);
		}
		
		// render gauge background
		
		RenderRect(
			Position_get() - borderSize, 
			mSize + borderSize * 2.0f,
			0.0f, 0.0f, 0.0f, 0.3f * m_Alpha,
			g_GameState->GetTexture(Textures::MENU_COLOR_WHITE));
		
		// render gauge
		
		const float alpha = 0.5f + (sinf(g_TimerRT.Time_get() * Calc::mPI * 2.0f) + 1.0f) / 2.0f * 0.4f;

		RenderRect(
			Position_get(),
			mSize ^ Vec2F(mValue, 1.0f),
			1.0f, 1.0f, 1.0f, (mLockEnable ? alpha : 0.6f) * m_Alpha,
			g_GameState->GetTexture(Textures::OPTIONSVIEW_VOLUME));
	}

	void GuiSlider::Update(float dt)
	{
		for (int i = 0; i < 2; ++i)
		{
			if (mAutoMove[i].Read())
			{
				const int direction = i == 0 ? -1 : +1;
				HandleAutoMove(direction);
				mAutoMove[i].Start(0.015f);
			}
		}

		if (mAbsoluteMoveSpeed != 0.0f)
		{
			const float d = mMax - mMin;
			const float value = mValue + d * mAbsoluteMoveSpeed * dt;
			Value_set(value, true);
		}
	}

	XBOOL GuiSlider::HitTest(const Vec2F& pos) const
	{
		return mHitBox.IsInside(pos);
	}

	bool GuiSlider::HandleEvent(const Event& e)
	{
#if USE_MENU_SELECT
		if (e.type == EVT_MENU_SELECT)
		{
			Assert(sActiveSlider == 0);
			EventManager::I().Enable(EVENT_PRIO_INTERFACE_SLIDER);
			sActiveSlider = this;
			sActiveSlider->LockEnable(true);
			return true;
		}
#endif

		return false;
	}

	void GuiSlider::HandleTouchBegin(const Vec2F& pos)
	{
		const float value = (pos[0] - mHitBox.m_Position[0]) / mHitBox.m_Size[0];

		Value_set(value, true);
	}

	void GuiSlider::HandleTouchMove(const Vec2F& pos)
	{
		const float value = (pos[0] - mHitBox.m_Position[0]) / mHitBox.m_Size[0];

		Value_set(value, true);
	}

	void GuiSlider::HandleTouchEnd(const Vec2F& pos)
	{
		const float value = (pos[0] - mHitBox.m_Position[0]) / mHitBox.m_Size[0];

		Value_set(value, true);
	}

	RectF GuiSlider::HitBox_get() const
	{
		return mHitBox;
	}

	void GuiSlider::UpdateDesign(bool isVisible, const Vec2F& pos, const Vec2F& size)
	{
		m_IsVisible = isVisible;
		m_Position = pos;
		mSize = size;
		mHitBox.m_Position = pos;
		mHitBox.m_Size = size;
	}

	void GuiSlider::Translate(int dx, int dy)
	{
		m_Position[0] += dx;
		m_Position[1] += dy;
		mHitBox.m_Position[0] += dx;
		mHitBox.m_Position[1] += dy;
	}

	void GuiSlider::AutoMoveEnable(int direction, bool enable)
	{
		Assert(direction == -1 || direction == +1);
		const int idx = direction < 0 ? 0 : 1;
		if (enable)
		{
			HandleAutoMove(direction);
			mAutoMove[idx].Start(0.5f);
		}
		else
			mAutoMove[idx].Stop();
	}

	void GuiSlider::HandleAutoMove(int direction)
	{
		const float d = mMax - mMin;
		const float step = d / 100.0f;
		Value_set(Value_get() + step * direction, true);
	}

	void GuiSlider::AbsoluteMove(float v)
	{
		mAbsoluteMoveSpeed = v;
	}

	void GuiSlider::LockEnable(bool enable)
	{
		mLockEnable = enable;
	}

	float GuiSlider::ClampValue(float value) const
	{
		return Calc::Clamp(value, mMin, mMax);
	}
}
