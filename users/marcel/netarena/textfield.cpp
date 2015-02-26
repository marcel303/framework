#include "Debugging.h"
#include "framework.h"
#include "gamedefs.h"
#include "main.h"
#include "textfield.h"

static SDLKey s_shiftMap[256];
static bool s_shiftMapIsInit = false;

static void ensureShiftMap()
{
	if (s_shiftMapIsInit)
		return;

	s_shiftMapIsInit = true;

	memset(s_shiftMap, 0, sizeof(s_shiftMap));

	s_shiftMap[SDLK_1] = SDLK_EXCLAIM;
	s_shiftMap[SDLK_2] = SDLK_AT;
	s_shiftMap[SDLK_3] = SDLK_HASH;
	s_shiftMap[SDLK_4] = SDLK_DOLLAR;
	s_shiftMap[SDLK_5] = SDLK_PERCENT;
	s_shiftMap[SDLK_6] = SDLK_CARET;
	s_shiftMap[SDLK_7] = SDLK_AMPERSAND;
	s_shiftMap[SDLK_8] = SDLK_ASTERISK;
	s_shiftMap[SDLK_9] = SDLK_LEFTPAREN;
	s_shiftMap[SDLK_0] = SDLK_RIGHTPAREN;

	s_shiftMap[SDLK_SLASH] = SDLK_QUESTION;
	s_shiftMap[SDLK_SEMICOLON] = SDLK_COLON;

	s_shiftMap[SDLK_MINUS] = SDLK_UNDERSCORE;
	s_shiftMap[SDLK_EQUALS] = SDLK_PLUS;
	s_shiftMap[SDLK_PERIOD] = SDLK_GREATER;

	s_shiftMap[SDLK_QUOTE] = SDLK_QUOTEDBL;
	s_shiftMap[SDLK_BACKQUOTE] = SDLK_RIGHTPAREN;
	s_shiftMap[SDLK_COMMA] = SDLK_LESS;

	s_shiftMap[SDLK_LEFTBRACKET] = SDLK_KP_LEFTBRACE;
	s_shiftMap[SDLK_RIGHTBRACKET] = SDLK_KP_RIGHTBRACE;
	s_shiftMap[SDLK_BACKSLASH] = SDLK_KP_VERTICALBAR;
}

static bool isAllowed(int c)
{
	if (c >= 32 && c <= 126)
		return true;
	return false;
}

TextField::TextField(int x, int y, int sx, int sy)
	: m_isActive(false)
	, m_canCancel(false)
	, m_x(x)
	, m_y(y)
	, m_sx(sx)
	, m_sy(sy)
	, m_maxBufferSize(0)
	, m_bufferSize(0)
	, m_caretPosition(0)
	, m_caretTimer(0.f)
{
}

bool TextField::tick(float dt)
{
	bool result = false;

	// SDL key codes are basically just ASCII codes, so that's easy!

	ensureShiftMap();

	if (m_isActive)
	{
		Assert(m_caretPosition <= m_bufferSize);

		m_caretTimer += dt;

		for (int i = 0; i < 256; ++i)
		{
			if (keyboard.wentDown((SDLKey)i) && isAllowed(i))
			{
				int c = i;

				if (keyboard.isDown(SDLK_LSHIFT))
					c = toupper(c);

				if (s_shiftMap[c])
					c = s_shiftMap[c];

				addChar(c);
			}
		}

		if (keyboard.wentDown(SDLK_LEFT) && m_caretPosition > 0)
			m_caretPosition--;
		if (keyboard.wentDown(SDLK_RIGHT) && m_caretPosition < m_bufferSize)
			m_caretPosition++;

		if (keyboard.wentDown(SDLK_DELETE) && m_caretPosition < m_bufferSize)
		{
			m_caretPosition++;
			removeChar();
		}

		if (keyboard.wentDown(SDLK_BACKSPACE) && m_caretPosition > 0)
			removeChar();

		if (keyboard.wentDown(SDLK_RETURN))
		{
			if (m_bufferSize > 0)
				result = true;

			complete();
		}

		if (keyboard.wentDown(SDLK_ESCAPE) && m_canCancel)
			close();
	}

	return result;
}

void TextField::draw()
{
	if (m_isActive)
	{
		const char * text = getText();

		// draw box

		setColor(0, 0, 0, 127);
		drawRect(m_x, m_y, m_x + m_sx, m_y + m_sy);

		setColor(63, 127, 255);
		drawRectLine(m_x, m_y, m_x + m_sx, m_y + m_sy);

		// draw text

		setColor(127, 227, 255);
		setFont("calibri.ttf");
		drawText(
			m_x + UI_TEXTCHAT_PADDING_X,
			m_y + UI_TEXTCHAT_PADDING_Y, UI_TEXTCHAT_FONT_SIZE,
			+1.f, +1.f, "%s", text);

		// draw caret

		if (std::fmodf(m_caretTimer, .5f) < .25f)
		{
			char textToCaretPosition[kMaxBufferSize + 1];
			strcpy_s(textToCaretPosition, sizeof(textToCaretPosition), text);

			textToCaretPosition[m_caretPosition] = 0;

			float sx, sy;
			measureText(UI_TEXTCHAT_FONT_SIZE, sx, sy, "%s", textToCaretPosition);

			const int x = m_x + UI_TEXTCHAT_PADDING_X + sx + UI_TEXTCHAT_CARET_PADDING_X;

			setColor(63, 127, 255);
			drawRect(x, m_y + UI_TEXTCHAT_CARET_PADDING_Y, x + UI_TEXTCHAT_CARET_SX, m_y + m_sy - UI_TEXTCHAT_CARET_PADDING_Y * 2);
		}

		setColor(colorWhite);
	}
}

void TextField::open(int maxLength, bool canCancel)
{
	Assert(!m_isActive);

	m_isActive = true;
	m_canCancel = canCancel;

	m_maxBufferSize = maxLength < kMaxBufferSize ? maxLength : kMaxBufferSize;
	m_bufferSize = 0;

	m_caretPosition = 0;
	m_caretTimer = 0.f;

	g_keyboardLock++;
}

void TextField::close()
{
	Assert(m_isActive);

	m_isActive = false;
	m_canCancel = false;

	m_maxBufferSize = 0;
	m_bufferSize = 0;

	m_caretPosition = 0;
	m_caretTimer = 0.f;

	Assert(g_keyboardLock > 0);
	g_keyboardLock--;
}

bool TextField::isActive() const
{
	return m_isActive;
}

const char * TextField::getText() const
{
	const_cast<char*>(m_buffer)[m_bufferSize] = 0;

	return m_buffer;
}

void TextField::complete()
{
	m_buffer[m_bufferSize] = 0;

	close();
}

void TextField::addChar(char c)
{
	Assert(m_isActive);

	if (m_bufferSize < kMaxBufferSize && m_bufferSize < m_maxBufferSize)
	{
		if (m_caretPosition < m_bufferSize)
			memcpy(&m_buffer[m_caretPosition + 1], &m_buffer[m_caretPosition], m_bufferSize - m_caretPosition);

		m_buffer[m_caretPosition] = c;

		m_bufferSize++;
		m_caretPosition++;
	}
}

void TextField::removeChar()
{
	Assert(m_isActive);

	if (m_caretPosition > 0)
	{
		memcpy(&m_buffer[m_caretPosition - 1], &m_buffer[m_caretPosition], m_bufferSize - 1);

		m_bufferSize--;
		m_caretPosition--;
	}
}
