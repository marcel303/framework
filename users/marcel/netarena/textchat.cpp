#include "Debugging.h"
#include "framework.h"
#include "gamedefs.h"
#include "main.h"
#include "textchat.h"

static bool isAllowed(int c)
{
	if (c >= 32 && c <= 126)
		return true;
	return false;
}

TextChat::TextChat(int x, int y, int sx, int sy)
	: m_isActive(false)
	, m_x(x)
	, m_y(y)
	, m_sx(sx)
	, m_sy(sy)
	, m_bufferSize(0)
	, m_caretTimer(0.f)
{
}

bool TextChat::tick(float dt)
{
	bool result = false;

	// SDL key codes are basically just ASCII codes, so that's easy!

	if (m_isActive)
	{
		m_caretTimer += dt;

		for (int i = 0; i < 256; ++i)
		{
			if (keyboard.wentDown((SDLKey)i) && isAllowed(i))
			{
				int c = i;

				if (keyboard.isDown(SDLK_LSHIFT))
					c = toupper(c);

				// todo : generate shift version for arbitrary characters

				addChar(c);
			}
		}

		if (keyboard.wentDown(SDLK_BACKSPACE) && m_bufferSize > 0)
			m_bufferSize--;

		if (keyboard.wentDown(SDLK_RETURN))
		{
			if (m_bufferSize > 0)
				result = true;

			complete();
		}

		if (keyboard.wentDown(SDLK_ESCAPE))
			close();
	}

	return result;
}

void TextChat::draw()
{
	if (m_isActive)
	{
		m_buffer[m_bufferSize] = 0;

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
			+1.f, +1.f, "%s", m_buffer);

		// draw caret

		if (std::fmodf(m_caretTimer, .5f) < .25f)
		{
			float sx, sy;
			measureText(UI_TEXTCHAT_FONT_SIZE, sx, sy, "%s", m_buffer);

			const int x = m_x + UI_TEXTCHAT_PADDING_X + sx + UI_TEXTCHAT_CARET_PADDING_X;

			setColor(63, 127, 255);
			drawRect(x, m_y + UI_TEXTCHAT_CARET_PADDING_Y, x + UI_TEXTCHAT_CARET_SX, m_y + m_sy - UI_TEXTCHAT_CARET_PADDING_Y * 2);
		}
	}
}

void TextChat::setActive(bool isActive)
{
	if (isActive != m_isActive)
	{
		m_isActive = isActive;

		m_bufferSize = 0;
		m_caretTimer = 0.f;

		if (isActive)
			g_keyboardLock++;
		else
			g_keyboardLock--;
	}
}

bool TextChat::isActive() const
{
	return m_isActive;
}

std::string TextChat::getText() const
{
	return m_buffer;
}

void TextChat::complete()
{
	m_buffer[m_bufferSize] = 0;

	close();
}

void TextChat::close()
{
	Assert(m_isActive);

	setActive(false);
}

void TextChat::addChar(char c)
{
	Assert(m_isActive);

	if (m_bufferSize < kMaxBufferSize)
	{
		m_buffer[m_bufferSize++] = c;
	}
}
