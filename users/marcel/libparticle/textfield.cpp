#include "framework.h"
#include "StringEx.h"
#include "textfield.h"

#define CARET_PADDING_X 0
#define CARET_PADDING_Y 5 // todo : look these up
#define CARET_SX 2

static bool isAllowed(const int c)
{
	if (c >= 32 && c <= 126)
		return true;
	return false;
}

EditorTextField::EditorTextField()
	: m_isActive(false)
	, m_canCancel(false)
	, m_maxBufferSize(0)
	, m_bufferSize(0)
	, m_caretPosition(0)
	, m_caretTimer(0.f)
	, m_textIsSelected(false)
{
	m_buffer[m_bufferSize] = 0;
}

bool EditorTextField::tick(const float dt)
{
	bool result = false;

	// SDL key codes are basically just ASCII codes, so that's easy!

	if (m_isActive)
	{
		fassert(m_caretPosition <= m_bufferSize);

		m_caretTimer += dt;
		
		for (auto & e : framework.events)
		{
			if (e.type != SDL_KEYDOWN)
				continue;
			
			int c = e.key.keysym.sym;
			
			if (isAllowed(c))
			{
				if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
				{
					c = toupper(c);
				}
				
				if (m_textIsSelected)
				{
					m_textIsSelected = false;
					setText("");
				}

				addChar(c);
			}
		}
		
		if (m_textIsSelected)
		{
			if (keyboard.wentDown(SDLK_HOME) || keyboard.wentDown(SDLK_LEFT))
			{
				m_textIsSelected = false;
				m_caretPosition = 0;
			}
			if (keyboard.wentDown(SDLK_END) || keyboard.wentDown(SDLK_RIGHT))
			{
				m_textIsSelected = false;
				m_caretPosition = m_bufferSize;
			}
			
			if (keyboard.wentDown(SDLK_DELETE) || keyboard.wentDown(SDLK_BACKSPACE))
			{
				m_textIsSelected = false;
				setText("");
			}
		}
		else
		{
			if (keyboard.wentDown(SDLK_HOME))
				m_caretPosition = 0;
			if (keyboard.wentDown(SDLK_END))
				m_caretPosition = m_bufferSize;

			if (keyboard.wentDown(SDLK_LEFT, true) && m_caretPosition > 0)
				m_caretPosition--;
			if (keyboard.wentDown(SDLK_RIGHT, true) && m_caretPosition < m_bufferSize)
				m_caretPosition++;

			if (keyboard.wentDown(SDLK_DELETE, true) && m_caretPosition < m_bufferSize)
			{
				m_caretPosition++;
				removeChar();
			}

			if (keyboard.wentDown(SDLK_BACKSPACE, true) && m_caretPosition > 0)
				removeChar();
		}

		if (keyboard.wentDown(SDLK_RETURN))
		{
			result = true;

			complete();
		}
		else if (keyboard.wentDown(SDLK_ESCAPE) && m_canCancel)
		{
			close();
		}
	}

	return result;
}

void EditorTextField::draw(const int ax, const int ay, const int asy, const int fontSize)
{
	//if (m_isActive)
	{
		const char * text = getText();
		
		// draw text
		
		if (m_textIsSelected)
		{
			float textSx;
			float textSy;
			
			measureText(fontSize, textSx, textSy, "%s", text);
			
			float x1 = -2 + ax;
			float x2 = +2 + ax + textSx;
			float y1 = -2 + ay + asy/2 - textSy/2;
			float y2 = +2 + ay + asy/2 + textSy/2;
			
			setColor(63, 63, 63);
			drawRect(x1, y1, x2, y2);
		}
		
		if (m_isActive)
			setColor(127, 227, 255);
		else
			setColor(127, 127, 127);
		drawText(ax, ay+asy/2, fontSize, +1.f, 0.f, "%s", text);
		
		if (m_textIsSelected == false)
		{
			// draw caret

			if (m_isActive && std::fmodf(m_caretTimer, .5f) < .25f)
			{
				char textToCaretPosition[kMaxBufferSize + 1];
				strcpy_s(textToCaretPosition, sizeof(textToCaretPosition), text);

				textToCaretPosition[m_caretPosition] = 0;

				float sx, sy;
				measureText(fontSize, sx, sy, "%s", textToCaretPosition);

				const int x = ax + sx + CARET_PADDING_X;

				setColor(63, 127, 255);
				drawRect(x, ay + CARET_PADDING_Y, x + CARET_SX, ay + asy - CARET_PADDING_Y);
			}
		}

		setColor(colorWhite);
	}
}

void EditorTextField::open(const int maxLength, const bool canCancel, const bool clearText)
{
	fassert(!m_isActive);

	m_isActive = true;
	m_canCancel = canCancel;

	m_maxBufferSize = maxLength < kMaxBufferSize ? maxLength : kMaxBufferSize;
	m_buffer[m_maxBufferSize] = 0;

	if (clearText)
	{
		m_bufferSize = 0;
		m_buffer[m_bufferSize] = 0;
	}

	m_caretPosition = m_bufferSize;
	m_caretTimer = 0.f;
	
	m_textIsSelected = false;
}

void EditorTextField::close()
{
	fassert(m_isActive);

	m_isActive = false;
	m_canCancel = false;

	m_maxBufferSize = 0;
	m_bufferSize = 0;

	m_caretPosition = 0;
	m_caretTimer = 0.f;
	
	m_textIsSelected = false;
}

bool EditorTextField::isActive() const
{
	return m_isActive;
}

void EditorTextField::setText(const char * text)
{
	strcpy_s(m_buffer, m_maxBufferSize, text);
	m_bufferSize = strlen(m_buffer);
	
	m_caretPosition = m_bufferSize;
}

const char * EditorTextField::getText() const
{
	return m_buffer;
}

void EditorTextField::setTextIsSelected(const bool isSelected)
{
	m_textIsSelected = isSelected;
}

void EditorTextField::complete()
{
	close();
}

void EditorTextField::addChar(const char c)
{
	fassert(m_isActive);
	
	if (m_bufferSize < kMaxBufferSize && m_bufferSize < m_maxBufferSize)
	{
		if (m_caretPosition < m_bufferSize)
			memmove(&m_buffer[m_caretPosition + 1], &m_buffer[m_caretPosition], m_bufferSize - m_caretPosition);

		m_buffer[m_caretPosition] = c;

		m_bufferSize++;
		m_buffer[m_bufferSize] = 0;
		m_caretPosition++;
	}
}

void EditorTextField::removeChar()
{
	fassert(m_isActive);

	if (m_caretPosition > 0)
	{
		memmove(&m_buffer[m_caretPosition - 1], &m_buffer[m_caretPosition], m_bufferSize - 1);

		m_bufferSize--;
		m_buffer[m_bufferSize] = 0;
		m_caretPosition--;
	}
}
