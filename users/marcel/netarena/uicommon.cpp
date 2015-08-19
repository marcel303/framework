#include "Calc.h"
#include "FileStream.h"
#include "framework.h"
#include "gamedefs.h"
#include "main.h"
#include "menu.h"
#include "StreamReader.h"
#include "uicommon.h"

//#pragma optimize("", off)

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

bool MenuNavElem::hitTest(int x, int y) const
{
	return false;
}

void MenuNavElem::onFocusChange(bool hasFocus, bool isAutomaticSelection)
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

	// todo : check if mouse is used. if true, do mouse hover collision test and change selection

	const int gamepadIndex = 0;

	if (gamepad[gamepadIndex].wentDown(DPAD_UP) || keyboard.wentDown(SDLK_UP, true))
		moveSelection(0, -1);
	if (gamepad[gamepadIndex].wentDown(DPAD_DOWN) || keyboard.wentDown(SDLK_DOWN, true))
		moveSelection(0, +1);
	if (gamepad[gamepadIndex].wentDown(DPAD_LEFT) || keyboard.wentDown(SDLK_LEFT, true))
		moveSelection(-1, 0);
	if (gamepad[gamepadIndex].wentDown(DPAD_RIGHT) || keyboard.wentDown(SDLK_RIGHT, true))
		moveSelection(+1, 0);
	if (gamepad[gamepadIndex].wentDown(GAMEPAD_A) || keyboard.wentDown(SDLK_RETURN, false))
		handleSelect();

	if (mouse.dx || mouse.dy)
	{
		MenuNavElem * newSelection = 0;

		for (MenuNavElem * elem = m_first; elem; elem = elem->m_next)
			if (elem->hitTest(mouse.x, mouse.y))
				newSelection = elem;

		if (newSelection)
			setSelection(newSelection, false);
	}
}

void MenuNav::addElem(MenuNavElem * elem)
{
	elem->m_next = m_first;
	m_first = elem;

	if (!m_selection)
		setSelection(elem, true);
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
					distX /= 10;
				else if ((Calc::Sign(dx) != Calc::Sign(distX)) || distX == 0)
					continue;

				if (dy == 0)
					distY /= 10;
				else if ((Calc::Sign(dy) != Calc::Sign(distY)) || distY == 0)
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
		setSelection(newSelection, false);
}

void MenuNav::setSelection(MenuNavElem * elem, bool isAutomaticSelection)
{
	if (elem != m_selection)
	{
		if (m_selection)
			m_selection->onFocusChange(false, isAutomaticSelection);

		m_selection = elem;

		if (m_selection)
			m_selection->onFocusChange(true, isAutomaticSelection);
	}
}

void MenuNav::handleSelect()
{
	if (m_selection)
		m_selection->onSelect();
}

//

ButtonLegend::ButtonLegend()
{
}

void ButtonLegend::tick(float dt, int x, int y)
{
	auto elems = getDrawElems(x, y);

	for (auto e : elems)
	{
		if (e.button)
		{
			if (mouse.wentDown(BUTTON_LEFT) &&
				mouse.x >= e.clickX1 && mouse.x <= e.clickX2 &&
				mouse.y >= e.clickY1 && mouse.y <= e.clickY2)
			{
				e.button->onSelect();
			}
		}
	}
}

void ButtonLegend::draw(int x, int y)
{
	auto elems = getDrawElems(x, y);

	for (auto e : elems)
	{
		setColor(colorWhite);
		Sprite(e.sprite).drawEx(e.spriteX, e.spriteY);

		drawText(e.textX, e.textY, UI_BUTTONLEGEND_FONT_SIZE, +1.f, +1.f, "%s", e.text);
	}
}

void ButtonLegend::addElem(ButtonId buttonId, Button * button, const char * localString)
{
	Elem e;
	e.buttonId = buttonId;
	e.button = button;
	e.localString = localString;
	m_elems.push_back(e);
}

std::vector<ButtonLegend::DrawElem> ButtonLegend::getDrawElems(int x, int y)
{
	std::vector<ButtonLegend::DrawElem> result;

	for (auto & e : m_elems)
	{
		const char * filename = 0;

		switch (e.buttonId)
		{
		case kButtonId_B:
			if (g_currentMenuInputMode == kMenuInputMode_Gamepad)
				filename = "ui/legend/gamepad-b.png";
			break;
		case kButtonId_ESCAPE:
			if (g_currentMenuInputMode == kMenuInputMode_Keyboard)
				filename = "ui/legend/keyboard-escape.png";
			break;

		default:
			fassert(false);
		}

		if (filename)
		{
			DrawElem d;

			d.button = e.button;
			d.clickX1 = x;
			d.clickY1 = y;

			d.sprite = filename;
			d.spriteX = x;
			d.spriteY = y;

			Sprite sprite(filename);

			x += sprite.getWidth();
			x += UI_BUTTONLEGEND_ICON_SPACING;

			d.text = getLocalString(e.localString);
			d.textX = x;
			d.textY = y;

			float sx, sy;
			measureText(UI_BUTTONLEGEND_FONT_SIZE, sx, sy, "%s", d.text);

			x += (int)sx;

			d.clickX2 = x;
			d.clickY2 = y + sprite.getHeight();

			x += UI_BUTTONLEGEND_TEXT_SPACING;

			result.push_back(d);
		}
	}

	return result;
}

//

Button::Button(int x, int y, const char * filename, const char * localString, int textX, int textY, int textSize)
	: m_sprite(new Sprite(filename))
	, m_isMouseDown(false)
	, m_x(0)
	, m_y(0)
	, m_hasBeenSelected(false)
	, m_localString(localString)
	, m_textX(textX)
	, m_textY(textY)
	, m_textSize(textSize)
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
	bool result = false;

	if (m_hasBeenSelected)
	{
		m_hasBeenSelected = false;
		result = true;
	}
	else
	{
		const bool isInside = hitTest(mouse.x, mouse.y);
		const bool isDown = isInside && mouse.isDown(BUTTON_LEFT);
		result = isInside && !isDown && m_isMouseDown;

		if (mouse.wentDown(BUTTON_LEFT))
			m_isMouseDown = isInside;
		if (!mouse.isDown(BUTTON_LEFT))
			m_isMouseDown = false;
	}

	if (result)
		g_app->playSound("ui/button/select.ogg");

	return result;
}

void Button::draw()
{
	if (m_hasFocus)
		setColor(colorWhite);
	else
		setColor(255, 255, 255, 255, 63);
	m_sprite->drawEx(m_x, m_y);

	if (m_localString)
	{
		setFont("calibri.ttf");
		drawText(m_x + m_textX, m_y + m_textY, m_textSize, +1.f, +1.f, "%s", getLocalString(m_localString));
	}
}

void Button::getPosition(int & x, int & y) const
{
	x = m_x;
	y = m_y;
}

bool Button::hitTest(int x, int y) const
{
	return
		x >= m_x &&
		y >= m_y &&
		x < m_x + m_sprite->getWidth() &&
		y < m_y + m_sprite->getHeight();
}

void Button::onFocusChange(bool hasFocus, bool isAutomaticSelection)
{
	MenuNavElem::onFocusChange(hasFocus, isAutomaticSelection);

	if (hasFocus && !isAutomaticSelection)
		g_app->playSound("ui/button/focus.ogg");
}

void Button::onSelect()
{
	m_hasBeenSelected = true;
}

//

SpinButton::SpinButton(int x, int y, int min, int max, const char * filename, const char * localString, int textX, int textY, int textSize)
	: m_sprite(new Sprite(filename))
	, m_x(0)
	, m_y(0)
	, m_value(min)
	, m_min(min)
	, m_max(max)
	, m_localString(localString)
	, m_textX(textX)
	, m_textY(textY)
	, m_textSize(textSize)
{
	setPosition(x, y);
}

SpinButton::~SpinButton()
{
	delete m_sprite;
	m_sprite = 0;
}

void SpinButton::setPosition(int x, int y)
{
	m_x = x - m_sprite->getWidth() / 2;
	m_y = y - m_sprite->getHeight() / 2;
}

void SpinButton::changeValue(int delta)
{
	const int oldValue = m_value;

	m_value = Calc::Clamp(m_value + delta, m_min, m_max);

	if (m_value != oldValue)
		g_app->playSound("ui/button/select.ogg");
}

bool SpinButton::hasChanged()
{
	bool result = false;

	const int oldValue = m_value;

	if (m_hasFocus)
	{
		if (gamepad[0].wentDown(DPAD_LEFT) || keyboard.wentDown(SDLK_LEFT))
			changeValue(-1);
		if (gamepad[0].wentDown(DPAD_RIGHT) || keyboard.wentDown(SDLK_RIGHT))
			changeValue(+1);
	}

	return m_value != oldValue;
}

void SpinButton::draw()
{
	if (m_hasFocus)
		setColor(colorWhite);
	else
		setColor(255, 255, 255, 255, 63);
	m_sprite->drawEx(m_x, m_y);

	if (m_localString)
	{
		drawText(m_x + m_textX, m_y + m_textY, m_textSize, +1.f, +1.f, "%s", getLocalString(m_localString));
	}

	if (g_devMode)
	{
		drawText(m_x, m_y, 18, +1.f, +1.f, "min=%d, max=%d, value=%d", m_min, m_max, m_value);
	}

	const int kArrowSpacing = 30;
	const int kArrowSx = 20;
	const int kArrowSy = 25;
	
	const int arrowY = m_y + m_sprite->getHeight() / 2;

	if (m_value > m_min)
	{
		gxBegin(GL_TRIANGLES);
		gxColor3ub(255, 255, 255);
		gxVertex2f(m_x - kArrowSpacing + kArrowSx, arrowY - kArrowSy/2);
		gxVertex2f(m_x - kArrowSpacing + kArrowSx, arrowY + kArrowSy/2);
		gxVertex2f(m_x - kArrowSpacing, arrowY);
		gxEnd();
	}

	if (m_value < m_max)
	{
		gxBegin(GL_TRIANGLES);
		gxColor3ub(255, 255, 255);
		gxVertex2f(m_x + m_sprite->getWidth() + kArrowSpacing - kArrowSx, arrowY - kArrowSy/2);
		gxVertex2f(m_x + m_sprite->getWidth() + kArrowSpacing - kArrowSx, arrowY + kArrowSy/2);
		gxVertex2f(m_x + m_sprite->getWidth() + kArrowSpacing, arrowY);
		gxEnd();
	}
}

void SpinButton::getPosition(int & x, int & y) const
{
	x = m_x;
	y = m_y;
}

bool SpinButton::hitTest(int x, int y) const
{
	return
		x >= m_x &&
		y >= m_y &&
		x < m_x + m_sprite->getWidth() &&
		y < m_y + m_sprite->getHeight();
}

void SpinButton::onFocusChange(bool hasFocus, bool isAutomaticSelection)
{
	MenuNavElem::onFocusChange(hasFocus, isAutomaticSelection);

	if (hasFocus && !isAutomaticSelection)
		g_app->playSound("ui/button/focus.ogg");
}

void SpinButton::onSelect()
{
}

//

std::map<std::string, std::string> s_localStrings;

void setLocal(const char * local)
{
	s_localStrings.clear();

	try
	{
		const std::string filename = std::string("local-") + local + ".txt";

		FileStream stream;
		stream.Open(filename.c_str(), (OpenMode)(OpenMode_Read | OpenMode_Text));
		StreamReader reader(&stream, false);
		std::vector<std::string> lines = reader.ReadAllLines();
		for (auto & line : lines)
		{
			auto p = line.find_first_of(':');
			if (p == std::string::npos)
				logError("invalid local string definition: %s", line.c_str());
			else
			{
				std::string key = line.substr(0, p);
				std::string value = line.substr(p + 1);

				if (s_localStrings.count(key) != 0)
					logError("duplicate local string definition: %s", line.c_str());
				else
					s_localStrings[key] = value;
			}
		}
	}
	catch (std::exception & e)
	{
		logError(e.what());
	}
}

const char * getLocalString(const char * localString)
{
	auto i = s_localStrings.find(localString);
	if (i == s_localStrings.end())
		return localString;
	else
		return i->second.c_str();
}
