#pragma once

#include "AnimTimer.h"
#include "CallBack.h"
#include "Types.h"

#undef VK_SPACE
#undef VK_HOME
#undef VK_END
#undef VK_LEFT
#undef VK_RIGHT
#undef VK_DELETE
#undef VK_CANCEL

namespace Game
{
	class VirtualKey;
	class VirtualKeyBoard;
	
	enum VK
	{
		VK_UPPERCASE = 3, // key alt is uppercase
		VK_NOCHANGE = 2, // key has no alternate key
		VK_ROWBREAK = 1,
		// numeric
		VK_0 = '0',
		VK_1 = '1',
		VK_2 = '2',
		VK_3 = '3',
		VK_4 = '4',
		VK_5 = '5',
		VK_6 = '6',
		VK_7 = '7',
		VK_8 = '8',
		VK_9 = '9',
		// a-b-c
		VK_A = 'a',
		VK_B = 'b',
		VK_C = 'c',
		VK_D = 'd',
		VK_E = 'e',
		VK_F = 'f',
		VK_G = 'g',
		VK_H = 'h',
		VK_I = 'i',
		VK_J = 'j',
		VK_K = 'k',
		VK_L = 'l',
		VK_M = 'm',
		VK_N = 'n',
		VK_O = 'o',
		VK_P = 'p',
		VK_Q = 'q',
		VK_R = 'r',
		VK_S = 's',
		VK_T = 't',
		VK_U = 'u',
		VK_V = 'v',
		VK_W = 'w',
		VK_X = 'x',
		VK_Y = 'y',
		VK_Z = 'z',
		// special (printable)
		VK_APOSTROPHE = '\'',
		VK_MINUS = '-',
		VK_PLUS = '+',
		VK_BRACKET_OPEN = '[',
		VK_BRACKET_CLOSE = ']',
		VK_BACKSLASH = '\\',
		VK_SEMICOLON = ';',
		VK_LT = '<',
		VK_GT = '>',
		VK_SLASH = '/',
		VK_EXCLAIM = '!',
		VK_ASTERISK = '@',
		VK_HASH = '#',
		VK_DOLLAR = '$',
		VK_PERCENTAGE = '%',
		VK_CARET = '^', // ^
		VK_AND = '&',
		VK_STAR = '*',
		VK_BRACE_OPEN = '(',
		VK_BRACE_CLOSE = ')',
		VK_UNDERSCORE = '_',
		VK_EQUALS = '=',
		VK_TILDE = '~',
		VK_COLON = ':',
		VK_ACCENT = '`',
		VK_COMMA = ',',
		VK_DOT = '.',
		VK_QUOTE = '"',
		VK_QUESTION = '?',
		// special (non printable)
		VK_SPACE = ' ',
		VK_ENTER = 254,
		VK_BACKSPACE = 253,
		VK_HOME = 252,
		VK_END = 251,
		VK_LEFT = 250,
		VK_RIGHT = 249,
		VK_DELETE = 248,
		VK_DOT2 = 247, // ???????
		VK_READY = 246,
		VK_CANCEL = 245
	};
	
	typedef struct KeyInfo
	{
		KeyInfo();
		KeyInfo(VK base, VK shift);
		
		VK keyCodes[2];
	} KeyInfo;
	
	class VirtualKey
	{
	public:
		VirtualKey();
		void Initialize(VirtualKeyBoard* keyBoard, KeyInfo keyInfo, Vec2F startPos, Vec2F targetPos, Vec2F size, const VectorShape* shape);
		
		void Update(float dt, float animProgress);
		void Render(const Vec2F& pos, bool alt);
		
		inline bool HitTest(const Vec2F& pos) const
		{
			return pos[0] >= m_Pos[0] && pos[1] >= m_Pos[1] && pos[0] < m_Pos[0] + m_Size[0] && pos[1] < m_Pos[1] + m_Size[1];
		}
		
		inline const KeyInfo& KeyInfo_get() const
		{
			return m_KeyInfo;
		}
		
		inline const Vec2F& Position_get() const
		{
			return m_Pos;
		}
		
		inline const Vec2F& Size_get() const
		{
			return m_Size;
		}
		
		inline bool IsConfirmed_get() const
		{
			return m_IsConfirmed;
		}
		
		void HandleKeyPress();
		
		static bool IsPrintable(VK vk);
		
	private:
		VirtualKeyBoard* m_KeyBoard;
		KeyInfo m_KeyInfo;
		Vec2F m_Pos;
		Vec2F m_StartPos;
		Vec2F m_TargetPos;
		Vec2F m_Size;
		const VectorShape* m_Shape;
		bool m_IsConfirmed;
		
		float m_HitEffect;
	};
	
	enum KeyPresentation
	{
		KeyPresentation_Classic,
		KeyPresentation_Circular,
	};
	
	class VirtualKeyBoard
	{
	public:
		VirtualKeyBoard();
		void Initialize();
		
		void Update(float dt);
		void Render();
		
		// --------------------
		// Activation
		// --------------------
	private:
		bool m_IsActive;
		
	public:
		void Activate(float fadeTime);
		void Deactivate(float fadeTime);
		
		void HandleKeyPress(VK vk);
		
		CallBack OnKeyPress;
		CallBack OnReady;
		CallBack OnCancel;
		
	private:
		// --------------------
		// Touch related
		// --------------------
		static bool HandleTouchBegin(void* obj, const TouchInfo& touchInfo);
		static bool HandleTouchMove(void* obj, const TouchInfo& touchInfo);
		static bool HandleTouchEnd(void* obj, const TouchInfo& touchInfo);
		
		VirtualKey* HitTest(Vec2F pos);
		
		VirtualKey* m_ActiveKey;
		
		// --------------------
		// Keys
		// --------------------
		enum Gesture
		{
			Gesture_None,
			Gesture_Alt,
			Gesture_Cancel
		};
		
		VirtualKey* m_Keys;
		int m_KeyCount;
		Vec2F m_TouchPos;
		Vec2F m_CurrPos;
		
		void Allocate(int keyCount);
		Gesture Gesture_get() const;
		bool UseAlt_get() const;
		bool UseCancel_get() const;
		
		// --------------------
		// Animation
		// --------------------
		inline bool IsAnimating_get() const
		{
			return m_AnimProgress != 1.0f;
		}
		
		KeyPresentation m_KeyPresentation;
		float m_AnimProgress;
		float m_FadeTime;
		
		void KeyPresentation_set(KeyPresentation presentation);
	};
	
	class VirtualInput
	{
	public:
		VirtualInput();
		~VirtualInput();
		void Initialize(int maxLength);
		
		void Update(float dt);
		void Render();
		
		void Caption_set(const char* caption);
		
		const char* ToString();
		void Text_set(const char* text);

		// --------------------
		// Activation
		// --------------------
		void Activate(float fadeTime);
		void Deactivate(float fadeTime);
		
		// --------------------
		// Input
		// --------------------
		void HandleKeyPress(VK vk);
		static void HandleKeyPress(void* obj, void* arg);
		
		// --------------------
		// Animation
		// --------------------
		inline bool IsAnimating_get() const
		{
			return m_AnimProgress != 1.0f;
		}
		
		void DBG_Render();
		
	private:
		bool m_IsActive;
		FixedSizeString<256> m_Caption;
		char* m_Text;
		int m_Length;
		int m_MaxLength;
		int m_Cursor;
		float m_Blink;
		float m_BlinkTime;
		float m_AnimProgress;
		float m_FadeTime;
		
		void Allocate(int maxLength);
		
		void HandleMoveLeft();
		void HandleMoveRight();
		void HandleMoveHome();
		void HandleMoveEnd();
		void HandleBackSpace();
		void HandleDelete();
		void HandleChar(char c);
	};
}
