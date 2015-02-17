#include "framework.h"
#include "uicommon.h"

Button::Button(int x, int y, const char * filename)
	: m_sprite(new Sprite(filename))
	, m_isMouseDown(false)
	, m_x(0)
	, m_y(0)
{
	setPosition(x, y);
}

Button::~Button()
{
	delete m_sprite;
	m_sprite = 0;
}

void Button::setPosition(int x, int y)
{
	m_x = x - m_sprite->getWidth() / 2;
	m_y = y - m_sprite->getHeight() / 2;
}

bool Button::isClicked()
{
	const bool isInside =
		mouse.x >= m_x &&
		mouse.y >= m_y &&
		mouse.x < m_x + m_sprite->getWidth() &&
		mouse.y < m_y + m_sprite->getHeight();
	const bool isDown = isInside && mouse.isDown(BUTTON_LEFT);
	const bool isClicked = isInside && !isDown && m_isMouseDown;
	if (mouse.wentDown(BUTTON_LEFT))
		m_isMouseDown = isInside;
	if (!mouse.isDown(BUTTON_LEFT))
		m_isMouseDown = false;
	return isClicked;
}

void Button::draw()
{
	m_sprite->drawEx(m_x, m_y);
}
