#include "Calc.h"
#include "framework.h"
#include "uicommon.h"

#pragma optimize("", off)

MenuNavElem::MenuNavElem()
	: m_hasFocus(false)
	, m_next(0)
{
}

void MenuNavElem::getPosition(int & x, int & y) const
{
	x = 0;
	y = 0;
}

void MenuNavElem::onFocusChange(bool hasFocus)
{
	m_hasFocus = hasFocus;
}

void MenuNavElem::onSelect()
{
}

//

MenuNav::MenuNav()
	: m_first(0)
	, m_selection(0)
{
}

void MenuNav::tick(float dt)
{
	// fixme : don't hard code gamepad index. select main controller at start or let all navigate?

	const int gamepadIndex = 0;

	if (gamepad[gamepadIndex].wentDown(DPAD_UP))
		moveSelection(0, -1);
	if (gamepad[gamepadIndex].wentDown(DPAD_DOWN))
		moveSelection(0, +1);
	if (gamepad[gamepadIndex].wentDown(DPAD_LEFT))
		moveSelection(-1, 0);
	if (gamepad[gamepadIndex].wentDown(DPAD_RIGHT))
		moveSelection(+1, 0);
	if (gamepad[gamepadIndex].wentDown(GAMEPAD_A))
		handleSelect();
}

void MenuNav::addElem(MenuNavElem * elem)
{
	elem->m_next = m_first;
	m_first = elem;

	if (!m_selection)
		setSelection(elem);
}

void MenuNav::moveSelection(int dx, int dy)
{
	MenuNavElem * newSelection = 0;

	int minDistance = 0;

	if (!m_selection)
	{
		newSelection = m_first;
	}
	else
	{
		int oldX, oldY;
		m_selection->getPosition(oldX, oldY);

		for (MenuNavElem * elem = m_first; elem != 0; elem = elem->m_next)
		{
			if (elem != m_selection)
			{
				int newX, newY;
				elem->getPosition(newX, newY);

				int distX = newX - oldX;
				int distY = newY - oldY;

				if (dx == 0)
					distX = 0;
				else if (Calc::Sign(dx) != Calc::Sign(distX))
					continue;

				if (dy == 0)
					distY = 0;
				else if (Calc::Sign(dy) != Calc::Sign(distY))
					continue;

				const int distance = distX * distX + distY * distY;

				if (distance < minDistance || minDistance == 0)
				{
					minDistance = distance;

					newSelection = elem;
				}
			}
		}
	}

	if (newSelection)
		setSelection(newSelection);
}

void MenuNav::setSelection(MenuNavElem * elem)
{
	if (m_selection)
		m_selection->onFocusChange(false);

	m_selection = elem;

	if (m_selection)
		m_selection->onFocusChange(true);
}

void MenuNav::handleSelect()
{
	if (m_selection)
		m_selection->onSelect();
}

//

Button::Button(int x, int y, const char * filename)
	: m_sprite(new Sprite(filename))
	, m_isMouseDown(false)
	, m_x(0)
	, m_y(0)
	, m_hasBeenSelected(false)
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
	if (m_hasBeenSelected)
	{
		m_hasBeenSelected = false;
		return true;
	}

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
	if (m_hasFocus)
		setColor(colorWhite);
	else
		setColor(255, 255, 255, 255, 63);
	m_sprite->drawEx(m_x, m_y);
}

void Button::getPosition(int & x, int & y) const
{
	x = m_x;
	y = m_y;
}

void Button::onFocusChange(bool hasFocus)
{
	MenuNavElem::onFocusChange(hasFocus);
}

void Button::onSelect()
{
	m_hasBeenSelected = true;
}
