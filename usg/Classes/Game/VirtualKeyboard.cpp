#include "FontMap.h"
#include "GameSettings.h"
#include "GameState.h"
#include "Log.h"
#include "SoundEffectMgr.h"
#include "StringEx.h"
#include "TempRender.h"
#include "Textures.h"
#include "TouchDLG.h"
#include "UsgResources.h"
#include "VirtualKeyboard.h"

#define FONT g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE)
#define ALT_TRESHOLD 30.0f
#define KEY_COUNT (int)(sizeof(keys) / sizeof(KeyInfo))
#define SPECIAL_COUNT (int)(sizeof(specialKeys) / sizeof(KeyInfo))

const static float KEY_HIT_EFFECT_DURATION = 0.25f;
const static float KEY_HIT_EFFECT_SCALE = 1.0f / KEY_HIT_EFFECT_DURATION;

namespace Game
{
	const static KeyInfo keys[] =
	{
		KeyInfo(VK_1, VK_EXCLAIM),
		KeyInfo(VK_2, VK_ASTERISK),
		KeyInfo(VK_3, VK_HASH),
		KeyInfo(VK_4, VK_DOLLAR),
		KeyInfo(VK_5, VK_PERCENTAGE),
		KeyInfo(VK_6, VK_CARET),
		KeyInfo(VK_7, VK_AND),
		KeyInfo(VK_8, VK_STAR),
		KeyInfo(VK_9, VK_TILDE),
		KeyInfo(VK_0, VK_APOSTROPHE),
		KeyInfo(VK_PLUS, VK_MINUS),
		KeyInfo(VK_EQUALS, VK_TILDE),
		KeyInfo(VK_ROWBREAK, VK_ROWBREAK),
		
		KeyInfo(VK_Q, VK_UPPERCASE),
		KeyInfo(VK_W, VK_UPPERCASE),
		KeyInfo(VK_E, VK_UPPERCASE),
		KeyInfo(VK_R, VK_UPPERCASE),
		KeyInfo(VK_T, VK_UPPERCASE),
		KeyInfo(VK_Y, VK_UPPERCASE),
		KeyInfo(VK_U, VK_UPPERCASE),
		KeyInfo(VK_I, VK_UPPERCASE),
		KeyInfo(VK_O, VK_UPPERCASE),
		KeyInfo(VK_P, VK_UPPERCASE),
		KeyInfo(VK_BRACE_OPEN, VK_BRACE_CLOSE),
		KeyInfo(VK_BRACKET_OPEN, VK_BRACKET_CLOSE),
		KeyInfo(VK_ROWBREAK, VK_ROWBREAK),
		
		KeyInfo(VK_A, VK_UPPERCASE),
		KeyInfo(VK_S, VK_UPPERCASE),
		KeyInfo(VK_D, VK_UPPERCASE),
		KeyInfo(VK_F, VK_UPPERCASE),
		KeyInfo(VK_G, VK_UPPERCASE),
		KeyInfo(VK_H, VK_UPPERCASE),
		KeyInfo(VK_J, VK_UPPERCASE),
		KeyInfo(VK_K, VK_UPPERCASE),
		KeyInfo(VK_L, VK_UPPERCASE),
		KeyInfo(VK_COLON, VK_SEMICOLON),
		KeyInfo(VK_QUOTE, VK_UNDERSCORE),
		KeyInfo(VK_ROWBREAK, VK_ROWBREAK),
		
		KeyInfo(VK_Z, VK_UPPERCASE),
		KeyInfo(VK_X, VK_UPPERCASE),
		KeyInfo(VK_C, VK_UPPERCASE),
		KeyInfo(VK_V, VK_UPPERCASE),
		KeyInfo(VK_B, VK_UPPERCASE),
		KeyInfo(VK_N, VK_UPPERCASE),
		KeyInfo(VK_M, VK_UPPERCASE),
		KeyInfo(VK_LT, VK_GT),
		KeyInfo(VK_COMMA, VK_DOT2),
		KeyInfo(VK_DOT, VK_QUESTION),
		KeyInfo(VK_BACKSLASH, VK_SLASH),
		KeyInfo(VK_ROWBREAK, VK_ROWBREAK)
	};
	
	const static KeyInfo specialKeys[] =
	{
//		KeyInfo(VK_LEFT, VK_NOCHANGE),
//		KeyInfo(VK_RIGHT, VK_NOCHANGE),
//		KeyInfo(VK_HOME, VK_NOCHANGE),
//		KeyInfo(VK_END, VK_NOCHANGE),
//		KeyInfo(VK_DELETE, VK_NOCHANGE)
		KeyInfo(VK_SPACE, VK_NOCHANGE),
		KeyInfo(VK_READY, VK_NOCHANGE),
		KeyInfo(VK_CANCEL, VK_NOCHANGE),
		KeyInfo(VK_BACKSPACE, VK_NOCHANGE)
	};
	
	//
	
	KeyInfo::KeyInfo()
	{
	}
	
	KeyInfo::KeyInfo(VK base, VK shift)
	{
		keyCodes[0] = base;
		keyCodes[1] = shift;
	}
	
	//
	
	VirtualKey::VirtualKey()
	{
		m_KeyBoard = 0;
	}
	
	void VirtualKey::Initialize(VirtualKeyBoard* keyBoard, KeyInfo keyInfo, Vec2F startPos, Vec2F targetPos, Vec2F size, const VectorShape* shape)
	{
		m_KeyBoard = keyBoard;
		m_KeyInfo = keyInfo;
		m_Pos = m_StartPos;
		m_StartPos = startPos;
		m_TargetPos = targetPos;
		m_Size = size;
		m_Shape = shape;
		m_Size = m_Shape->m_Shape.m_BoundingBox.ToRect().m_Size.ToF();
		m_IsConfirmed = false;
		
		m_HitEffect = 0.0f;
	}
	
	void VirtualKey::Update(float dt, float animProgress)
	{
		m_HitEffect -= dt;
		
		if (m_HitEffect < 0.0f)
			m_HitEffect = 0.0f;
		
		m_Pos = m_StartPos + (m_TargetPos - m_StartPos) * animProgress;
	}
	
	void VirtualKey::Render(const Vec2F& pos, bool alt)
	{
		char c;
		
		if (alt)
		{
			if (m_KeyInfo.keyCodes[1] == VK_UPPERCASE)
				c = (char)m_KeyInfo.keyCodes[0] + 'A' - 'a';
			else
				c = (char)m_KeyInfo.keyCodes[1];
		}
		else
		{
			c = (char)m_KeyInfo.keyCodes[0];
		}
		
		g_GameState->Render(m_Shape, pos, 0.0f, SpriteColors::White);
		
		char text[2] = { c, 0 };
		
		RenderText(pos, m_Size, FONT, SpriteColors::White, TextAlignment_Center, TextAlignment_Center, true, text);
		
		if (m_HitEffect != 0.0f)
		{
			const float v = m_HitEffect * KEY_HIT_EFFECT_SCALE;
			
			RenderRect(m_Pos, m_Size, 1.0f, 1.0f, 1.0f, v, g_GameState->GetTexture(Textures::MENU_COLOR_WHITE));
		}
	}
	
	void VirtualKey::HandleKeyPress()
	{
		m_HitEffect = KEY_HIT_EFFECT_DURATION;
		
		if (m_IsConfirmed == false)
		{
			if (m_KeyInfo.keyCodes[0] == VK_CANCEL)
			{
				m_Shape = g_GameState->GetShape(Resources::KB_KEY_CANCEL2);
			}
			
			m_IsConfirmed = true;
		}
	}
	
	bool VirtualKey::IsPrintable(VK vk)
	{
		return vk >= 32 && vk <= 127;
	}
	
	//
	
	VirtualKeyBoard::VirtualKeyBoard()
	{
		Initialize();
	}
	
	void VirtualKeyBoard::Initialize()
	{
		m_Keys = 0;
		m_KeyCount = 0;
		
		// allocate & initialize keys
		
		int ox = (VIEW_SX-480)/2;
		int oy = (VIEW_SY-320)/2;

		Vec2F size(36.0f, 36.0f);
		float space = 1.0f;
		float x0 = ox+10.0f;
		float xstep = 10.0f;
		
		int count = 0;
		
		for (int i = 0; i < KEY_COUNT; ++i)
			if (keys[i].keyCodes[0] != VK_ROWBREAK)
				count++;
		
		count += SPECIAL_COUNT;
		
		Allocate(count);
		
		float x = x0;
		float y = oy+90.0f;
		
		int index = 0;
		
		int row = 0;
		
		for (int i = 0; i < KEY_COUNT; ++i)
		{
			if (keys[i].keyCodes[0] == VK_ROWBREAK)
			{
				row++;
				x = x0 + xstep * row;
				y += size[1] + space;
			}
			else
			{
				VirtualKey& key = m_Keys[index];
				
				Vec2F delta = Vec2F::FromAngle(Calc::Random(Calc::m2PI)) * 600.0f;
				
				key.Initialize(this, keys[i], -size / 2.0f + delta, Vec2F(x, y), size, g_GameState->GetShape(Resources::KB_KEY));
				
				x += size[0] + space;
				
				index++;
			}
		}
		
		x = x0 + xstep;
		y += 10.0f;
		
		for (int i = 0; i < SPECIAL_COUNT; ++i)
		{
			VirtualKey& key = m_Keys[index];
			
			Vec2F size2;
			
			const VectorShape* shape = g_GameState->GetShape(Resources::KB_KEY);
			
			switch (specialKeys[i].keyCodes[0])
			{
				case VK_LEFT:
					size2 = size;
					shape = g_GameState->GetShape(Resources::KB_KEY_LEFT);
					break;
				case VK_RIGHT:
					size2 = size;
					shape = g_GameState->GetShape(Resources::KB_KEY_RIGHT);
					break;
				case VK_SPACE:
					size2 = Vec2F(200.0f, size[1]);
					shape = g_GameState->GetShape(Resources::KB_KEY_SPACE);
					break;
				case VK_HOME:
					size2 = size;
					shape = g_GameState->GetShape(Resources::KB_KEY);
					break;
				case VK_END:
					size2 = size;
					shape = g_GameState->GetShape(Resources::KB_KEY);
					break;
				case VK_BACKSPACE:
					size2 = size;
					shape = g_GameState->GetShape(Resources::KB_KEY_BACK);
					break;
				case VK_DELETE:
					size2 = size;
					shape = g_GameState->GetShape(Resources::KB_KEY_DEL);
					break;
				case VK_READY:
					size2 = size;
					shape = g_GameState->GetShape(Resources::KB_KEY_READY);
					break;
				case VK_CANCEL:
					size2 = size;
					shape = g_GameState->GetShape(Resources::KB_KEY_CANCEL);
					break;
				default:
					Assert(false);
			}
			
			float y2 = y;
			
			if (specialKeys[i].keyCodes[0] == VK_CANCEL)
				y2 += 2.0f;
			
			key.Initialize(this, specialKeys[i], -size2, Vec2F(x, y2), size2, shape);
			
			x += key.Size_get()[0] + space;
			
			index++;
		}
		
		// animation
		
		m_IsActive = false;
		m_AnimProgress = 0.0f;
		
		// register for touches
		
		TouchListener listener;
		listener.Setup(this, HandleTouchBegin, HandleTouchEnd, HandleTouchMove);
		g_GameState->m_TouchDLG->Register(USG::TOUCH_PRIO_KEYBOARD, listener);
		
		m_ActiveKey = 0;
	}
	
	void VirtualKeyBoard::Update(float dt)
	{
		float dap = (m_IsActive ? dt : -dt) / m_FadeTime;
		
		m_AnimProgress += dap;
		
		if (m_AnimProgress < 0.0f)
			m_AnimProgress = 0.0f;
		if (m_AnimProgress > 1.0f)
			m_AnimProgress = 1.0f;
		
		// update animation
		
		for (int i = 0; i < m_KeyCount; ++i)
			m_Keys[i].Update(dt, m_AnimProgress);
	}
	
	void VirtualKeyBoard::Render()
	{
		// render keys
		
		for (int i = 0; i < m_KeyCount; ++i)
		{
			bool alt = m_ActiveKey == &m_Keys[i] && UseAlt_get();
			
			m_Keys[i].Render(m_Keys[i].Position_get(), alt);
		}
		
		// render slide indicator
		
		Vec2F offset((1.0f + m_AnimProgress) * 150.0f, 0.0f);
		
		if (m_ActiveKey && m_ActiveKey->KeyInfo_get().keyCodes[1] != VK_NOCHANGE)
		{
			Vec2F pos(420.0f, 260.0f);
			
			m_ActiveKey->Render(pos, true);
			
			const VectorShape* shape = 0;
			
			Gesture gesture = Gesture_get();
			
			if (gesture == Gesture_None)
				shape = g_GameState->GetShape(Resources::KB_SLIDE);
			if (gesture == Gesture_Alt)
				shape = g_GameState->GetShape(Resources::KB_SLIDE_ALT);
			if (gesture == Gesture_Cancel)
				shape = g_GameState->GetShape(Resources::KB_SLIDE_CANCEL);
			
			g_GameState->Render(shape, pos + offset, 0.0f, SpriteColors::White);
			
			if (!IsAnimating_get())
				g_GameState->Render(shape, m_ActiveKey->Position_get(), 0.0f, SpriteColors::White);
		}
	}
	
	// --------------------
	// Activation
	// --------------------
	void VirtualKeyBoard::Activate(float fadeTime)
	{
		m_IsActive = true;
		m_FadeTime = fadeTime;
		
		g_GameState->m_TouchDLG->Enable(USG::TOUCH_PRIO_KEYBOARD);
	}
	
	void VirtualKeyBoard::Deactivate(float fadeTime)
	{
		m_IsActive = false;
		m_FadeTime = fadeTime;
		
		g_GameState->m_TouchDLG->Disable(USG::TOUCH_PRIO_KEYBOARD);
	}
	
	void VirtualKeyBoard::HandleKeyPress(VK vk)
	{
		LOG(LogLevel_Debug, "key pressed: %d", (int)vk);
		
		if (vk == VK_READY)
		{
			if (OnReady.IsSet())
				OnReady.Invoke(&vk);
		}
		else if (vk == VK_CANCEL)
		{
			if (OnCancel.IsSet())
				OnCancel.Invoke(&vk);
		}
		else
		{
			if (OnKeyPress.IsSet())
				OnKeyPress.Invoke(&vk);
		}
	}
	
	// --------------------
	// Touch related
	// --------------------
	
	bool VirtualKeyBoard::HandleTouchBegin(void* obj, const TouchInfo& touchInfo)
	{
		VirtualKeyBoard* self = (VirtualKeyBoard*)obj;
		
		VirtualKey* key = self->HitTest(touchInfo.m_LocationView);
		
		self->m_ActiveKey = key;
		
		if (!key)
			return false;
		
		self->m_TouchPos = self->m_CurrPos = touchInfo.m_LocationView;
		
		g_GameState->m_SoundEffects->Play(Resources::SOUND_KEYBOARD_KEY1, 0);
		
		return true;
	}
	
	bool VirtualKeyBoard::HandleTouchMove(void* obj, const TouchInfo& touchInfo)
	{
		VirtualKeyBoard* self = (VirtualKeyBoard*)obj;
		
		self->m_CurrPos = touchInfo.m_LocationView;
		
		return true;
	}
	
	bool VirtualKeyBoard::HandleTouchEnd(void* obj, const TouchInfo& touchInfo)
	{
		VirtualKeyBoard* self = (VirtualKeyBoard*)obj;
		
		Assert(self->m_ActiveKey != 0);
		
		if (!self->m_ActiveKey)
			return true;
		
		VirtualKey* key = self->m_ActiveKey;
		
		self->m_ActiveKey = 0;
		
		Gesture gesture = self->Gesture_get();
		
		if (gesture == Gesture_Cancel)
			return true;
		
		VK vk = gesture == Gesture_Alt ? key->KeyInfo_get().keyCodes[1] : key->KeyInfo_get().keyCodes[0];
		
		if (vk == VK_UPPERCASE)
			vk = (VK)(key->KeyInfo_get().keyCodes[0] + 'A' - 'a');
		
		bool trigger = false;
		
		if (vk == VK_CANCEL)
			trigger = key->IsConfirmed_get();
		else
			trigger = true;
		
		if (trigger)
		{
			self->HandleKeyPress(vk);
			
			g_GameState->m_SoundEffects->Play(Resources::SOUND_KEYBOARD_KEY2, 0);
		}
		
		key->HandleKeyPress();
		
		return true;
	}
	
	VirtualKey* VirtualKeyBoard::HitTest(Vec2F pos)
	{
		for (int i = 0; i < m_KeyCount; ++i)
		{
			if (m_Keys[i].HitTest(pos))
				return &m_Keys[i];
		}
		
		return 0;
	}

	// --------------------
	// Keys
	// --------------------
	
	void VirtualKeyBoard::Allocate(int keyCount)
	{
		delete[] m_Keys;
		m_Keys = 0;
		m_KeyCount = 0;
		
		if (keyCount == 0)
			return;
		
		m_Keys = new VirtualKey[keyCount];
		m_KeyCount = keyCount;
	}
	
	VirtualKeyBoard::Gesture VirtualKeyBoard::Gesture_get() const
	{
		const Vec2F delta = m_CurrPos - m_TouchPos;
		
		if (delta.LengthSq_get() < ALT_TRESHOLD * ALT_TRESHOLD)
			return Gesture_None;
		
		if (delta[1] <= 0.0f)
			return Gesture_Alt;
		
		if (delta[1] >= 0.0f)
			return Gesture_Cancel;
		
		return Gesture_None;
	}
	
	bool VirtualKeyBoard::UseAlt_get() const
	{
		return Gesture_get() == Gesture_Alt;
	}
	
	bool VirtualKeyBoard::UseCancel_get() const
	{
		return Gesture_get() == Gesture_Cancel;
	}
	
	// --------------------
	// Animation
	// --------------------
	
	void VirtualKeyBoard::KeyPresentation_set(KeyPresentation presentation)
	{
		m_KeyPresentation = presentation;
	}
	
	//
	
	VirtualInput::VirtualInput()
	{
		m_Text = 0;
		m_Length = 0;
		m_MaxLength = 0;
		m_Cursor = 0;
		
		Initialize(20);
	}
	
	VirtualInput::~VirtualInput()
	{
		Allocate(0);
	}
	
	void VirtualInput::Initialize(int maxLength)
	{
		m_IsActive = false;
		m_Text = 0;
		m_Length = 0;
		m_MaxLength = 0;
		m_Cursor = 0;
		m_Blink = 0.0f;
		m_BlinkTime = 0.3f;
		m_AnimProgress = 0.0f;
		m_FadeTime = 1.0f;
		
		Allocate(maxLength);
	}
	
	void VirtualInput::Update(float dt)
	{
		float dap = (m_IsActive ? dt : -dt) / m_FadeTime;
		
		m_AnimProgress += dap;
		
		if (m_AnimProgress < 0.0f)
			m_AnimProgress = 0.0f;
		if (m_AnimProgress > 1.0f)
			m_AnimProgress = 1.0f;
		
		//
		
		m_Blink -= dt;
		
		if (m_Blink < 0.0f)
			m_Blink = m_BlinkTime;
	}
	
	void VirtualInput::Render()
	{
		const float x0 = 10.0f;
		const float y0 = 10.0f;
		
		int ox = 0;
		int oy = (VIEW_SY-320)/2;

		const Vec2F offset(ox+0.0f, oy-(1.0f - m_AnimProgress) * 100.0f);
		
		const FontMap* font = g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE);
		
		RenderRect(Vec2F(0.0f, y0 + 0.0f) + offset, Vec2F((float)VIEW_SX, 90.0f), 1.0f, 1.0f, 1.0f, 1.0f, g_GameState->GetTexture(Textures::TEXTFIELD_BACK));
		
		RenderText(Vec2F(x0, y0 + 23.0f) + offset, Vec2F(0.0f, 0.0f), font, SpriteColors::White, TextAlignment_Left, TextAlignment_Top, true, ToString());
		
		const float v = m_Blink / m_BlinkTime;
		const float x = font->m_Font.MeasureText(ToString());
		
		RenderRect(Vec2F(x0 + x, y0 + 24.0f) + offset, Vec2F(10.0f, font->m_Font.m_Height), 1.0f, 1.0f, 1.0f, v, g_GameState->GetTexture(Textures::MENU_COLOR_WHITE));
		
		g_GameState->Render(g_GameState->GetShape(Resources::KB_CLEAR), Vec2F(461.0f, y0 + 39.0f) + offset, 0.0f, SpriteColors::White);
		
		RenderText(Vec2F(5.0f, 0.0f) + offset, Vec2F(0.0f, 0.0f), g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColors::White, TextAlignment_Left, TextAlignment_Top, true, m_Caption.c_str());
	}
	
	void VirtualInput::Caption_set(const char* caption)
	{
		m_Caption = caption;
	}
	
	const char* VirtualInput::ToString()
	{
		m_Text[m_Length] = 0;
		
		return m_Text;
	}

	void VirtualInput::Text_set(const char* text)
	{
		int length = (int)strlen(text);

		if (length > m_MaxLength)
			length = m_MaxLength;

		std::string temp = String::SubString(text, 0, length);

		strcpy(m_Text, temp.c_str());

		m_Cursor = length;
		m_Length = length;
	}
	
	// --------------------
	// Activation
	// --------------------
	void VirtualInput::Activate(float fadeTime)
	{
		m_IsActive = true;
		m_FadeTime = fadeTime;
	}
	
	void VirtualInput::Deactivate(float fadeTime)
	{
		m_IsActive = false;
		m_FadeTime = fadeTime;
	}
	
	void VirtualInput::HandleKeyPress(VK vk)
	{
		if (VirtualKey::IsPrintable(vk))
		{
			HandleChar((char)vk);
		}
		else
		{
			switch (vk)
			{
				case VK_LEFT:
					HandleMoveLeft();
					break;
				case VK_RIGHT:
					HandleMoveRight();
					break;
				case VK_HOME:
					HandleMoveHome();
					break;
				case VK_END:
					HandleMoveEnd();
					break;
				case VK_BACKSPACE:
					HandleBackSpace();
					break;
				case VK_DELETE:
					HandleDelete();
					break;
					
				default:
					LOG(LogLevel_Debug, "unhanbled key: %d", (int)vk);
			}
		}
	}
	
	void VirtualInput::HandleKeyPress(void* obj, void* arg)
	{
		VirtualInput* self = (VirtualInput*)obj;
		VK* vk = (VK*)arg;
		
		self->HandleKeyPress(*vk);
	}
	
	void VirtualInput::DBG_Render()
	{
		for (int i = 0; i < m_Length; ++i)
		{
		}
	}
	
	void VirtualInput::Allocate(int maxLength)
	{
		delete[] m_Text;
		m_Text = 0;
		m_Length = 0;
		m_MaxLength = 0;
		m_Cursor = 0;
		
		if (maxLength > 0)
		{
			m_Text = new char[maxLength + 1];
			m_Text[m_Cursor] = 0;
			m_MaxLength = maxLength;
		}
	}
	
	void VirtualInput::HandleMoveLeft()
	{
		if (m_Cursor > 0)
			m_Cursor--;
	}
	
	void VirtualInput::HandleMoveRight()
	{
		if (m_Cursor < m_Length)
			m_Cursor++;
	}
	
	void VirtualInput::HandleMoveHome()
	{
		m_Cursor = 0;
	}
	
	void VirtualInput::HandleMoveEnd()
	{
		m_Cursor = m_Length;
	}
	
	void VirtualInput::HandleBackSpace()
	{
		// todo: shift all chars right -1, reduce length by 1
		
		if (m_Cursor == 0)
			return;
		
		for (int i = m_Cursor; i < m_Length; ++i)
			m_Text[i - 1] = m_Text[i];
		
		m_Cursor--;
		m_Length--;
	}
	
	void VirtualInput::HandleDelete()
	{
		// todo: shift all chars right -1, reducae length by 1
		
		for (int i = m_Cursor + 1; i < m_Length - 1; ++i)
		{
			m_Text[i] = m_Text[i + 1];
		}
		
		m_Length--;
	}
	
	void VirtualInput::HandleChar(char c)
	{
		if (m_Cursor == m_MaxLength)
			return;
		
		m_Text[m_Cursor] = c;
		
		m_Cursor++;
		
		if (m_Cursor > m_Length)
			m_Length = m_Cursor;
	}
}
