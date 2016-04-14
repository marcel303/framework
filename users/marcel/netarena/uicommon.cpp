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
	, m_hasFocusLock(false)
	, m_next(0)
{
}

void MenuNavElem::getPosition(int & x, int & y) const
{
	x = 0;
	y = 0;
}

void MenuNavElem::getSize(int & sx, int & sy) const
{
	sx = 0;
	sy = 0;
}

void MenuNavElem::getCenter(int & x, int & y) const
{
	getPosition(x, y);

	int sx, sy;
	getSize(sx, sy);

	x += sx / 2;
	y += sy / 2;
}

bool MenuNavElem::hitTest(int x, int y) const
{
	return false;
}

void MenuNavElem::onFocusChange(bool hasFocus, bool isAutomaticSelection)
{
	m_hasFocus = hasFocus;
	m_hasFocusLock = false;
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
	if (g_keyboardLock)
		return;

	// fixme : don't hard code gamepad index. select main controller at start or let all navigate?

	// todo : check if mouse is used. if true, do mouse hover collision test and change selection

	const bool isFocusLocked = m_selection && m_selection->m_hasFocusLock;

	const int gamepadIndex = 0;

	if (!isFocusLocked)
	{
		if (gamepad[gamepadIndex].wentDown(DPAD_UP) || keyboard.wentDown(SDLK_UP, true))
			moveSelection(0, -1);
		if (gamepad[gamepadIndex].wentDown(DPAD_DOWN) || keyboard.wentDown(SDLK_DOWN, true))
			moveSelection(0, +1);
		if (gamepad[gamepadIndex].wentDown(DPAD_LEFT) || keyboard.wentDown(SDLK_LEFT, true))
			moveSelection(-1, 0);
		if (gamepad[gamepadIndex].wentDown(DPAD_RIGHT) || keyboard.wentDown(SDLK_RIGHT, true))
			moveSelection(+1, 0);
	}

	if (gamepad[gamepadIndex].wentDown(GAMEPAD_A) || keyboard.wentDown(SDLK_RETURN, false))
		handleSelect();

	if (!isFocusLocked && (mouse.dx || mouse.dy))
	{
		MenuNavElem * newSelection = 0;

		for (MenuNavElem * elem = m_first; elem; elem = elem->m_next)
			if (elem->hitTest(mouse.x, mouse.y))
				newSelection = elem;

		if (newSelection || true)
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
		while (newSelection->m_next)
			newSelection = newSelection->m_next;
	}
	else
	{
		std::pair<int, int> move(dx, dy);
		auto moveItr = m_selection->m_moveSet.find(move);
		if (moveItr != m_selection->m_moveSet.end())
			newSelection = moveItr->second;

		if (!newSelection)
		{
			int oldX, oldY;
			m_selection->getCenter(oldX, oldY);

			for (MenuNavElem * elem = m_first; elem != 0; elem = elem->m_next)
			{
				if (elem != m_selection)
				{
					int newX, newY;
					elem->getCenter(newX, newY);

				#if 1
					int distX = newX - oldX;
					int distY = newY - oldY;

					if (dx == 0)
						distX *= 10;
					else if ((Calc::Sign(dx) != Calc::Sign(distX)) || distX == 0)
						continue;

					if (dy == 0)
						distY *= 10;
					else if ((Calc::Sign(dy) != Calc::Sign(distY)) || distY == 0)
						continue;

					const int distance = distX * distX + distY * distY;
				#else
					const float distX = newX - oldX;
					const float distY = newY - oldY;
					// todo : do intersection test against all elems, skip when hitting another one
					const float dist = std::sqrtf(distX * distX + distY * distY);
					const float dirX = distX / dist;
					const float dirY = distY / dist;
					const float dot = dirX * dx + dirY * dy;
					if (dot < 0.f)
						continue;
					const float distance = 1.f - dot;
				#endif

					if (distance < minDistance || minDistance == 0)
					{
						minDistance = distance;

						newSelection = elem;
					}
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

		if (g_currentMenuInputMode == kMenuInputMode_Keyboard &&
			mouse.x >= e.clickX1 && mouse.x <= e.clickX2 &&
			mouse.y >= e.clickY1 && mouse.y <= e.clickY2)
			setColor(255, 255, 255);
		else
			setColor(255, 255, 255, 255, 227); // todo : make button legend colors configurable somewhere
		setMainFont();
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
			d.spriteY = y - 5; // fixme : make this depend on actual sprite height

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

Button::Button(int x, int y, const char * filename, const char * localString, const char * textFont, int textX, int textY, int textSize)
	: m_sprite(new Sprite(filename))
	, m_isMouseDown(false)
	, m_x(0)
	, m_y(0)
	, m_hasBeenSelected(false)
	, m_localString(localString)
	, m_textFont(textFont)
	, m_textX(textX)
	, m_textY(textY)
	, m_textSize(textSize)
	, m_moveX(0)
	, m_moveY(0)
	, m_startOpacity(0.f)
	, m_moveTime(0.f)
	, m_moveTimeStart(0.f)
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
#if 1
	m_x = x;
	m_y = y;
#else
	m_x = x - m_sprite->getWidth() / 2;
	m_y = y - m_sprite->getHeight() / 2;
#endif
}

void Button::setAnimation(int moveX, int moveY, float startOpacity, float time)
{
	m_moveX = moveX;
	m_moveY = moveY;
	m_startOpacity = startOpacity;
	m_moveTime = time;
	m_moveTimeStart = framework.time;
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
	const float animT = saturate(m_moveTime == 0.f ? 1.f : ((framework.time - m_moveTimeStart) / m_moveTime));
	const float opacity = m_startOpacity * (1.f - animT) + animT;

	gxPushMatrix();
	{
		gxTranslatef(m_moveX * (animT - 1.f), m_moveY * (animT - 1.f), 0.f);

	#if 0
		if (m_hasFocus)
			setColorf(1.f, 1.f, 1.f, opacity);
		else
			setColorf(1.f, 1.f, 1.f, opacity, .5f);
	#else
		setColorMode(COLOR_ADD);
		if (m_hasFocus)
			setColorf(.15f, .15f, .15f, opacity);
		else
			setColorf(0.f, 0.f, 0.f, opacity * .9f);
	#endif

		m_sprite->drawEx(m_x, m_y);

		if (m_hasFocus)
			setColorf(1.f, 1.f, 1.f, opacity);
		else
			setColorf(1.f, 1.f, 1.f, opacity, .75f);

		if (m_localString)
		{
			setFont(m_textFont);
			drawText(m_x + m_textX, m_y + m_textY, m_textSize, +1.f, +1.f, "%s", getLocalString(m_localString));

			setMainFont();
		}

		setColorMode(COLOR_MUL);

	#if 0
		setColor(colorGreen);
		int cx, cy;
		getCenter(cx, cy);
		drawRect(
			cx - 10.f,
			cy - 10.f,
			cx + 10.f,
			cy + 10.f);
	#endif
	}
	gxPopMatrix();
}

void Button::getPosition(int & x, int & y) const
{
	x = m_x;
	y = m_y;
}

void Button::getSize(int & sx, int & sy) const
{
	sx = m_sprite->getWidth();
	sy = m_sprite->getHeight();
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

SpinButton::SpinButton(int x, int y, int min, int max, int value, const char * filename, const char * localString, int textX, int textY, int textSize)
	: m_sprite(new Sprite(filename))
	, m_x(0)
	, m_y(0)
	, m_value(Calc::Clamp(value, min, max))
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
		if (mouse.wentDown(BUTTON_LEFT))
		{
			if (mouse.x >= m_x && mouse.x < m_x + m_sprite->getWidth() / 2 && mouse.y >= m_y && mouse.y < m_y + m_sprite->getHeight())
				changeValue(-1);

			if (mouse.x >= m_x + m_sprite->getWidth() / 2 && mouse.x < m_x + m_sprite->getWidth() && 	mouse.y >= m_y && mouse.y < m_y + m_sprite->getHeight())
				changeValue(+1);
		}

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
		setMainFont();
		drawText(m_x + m_textX, m_y + m_textY, m_textSize, +1.f, +1.f, "%s", getLocalString(m_localString));
	}

	const int kArrowSpacing = -30;
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

	if (g_devMode)
	{
		setColor(colorWhite);
		setMainFont();
		drawText(m_x, m_y, 18, +1.f, +1.f, "min=%d, max=%d, value=%d", m_min, m_max, m_value);
	}
}

void SpinButton::getPosition(int & x, int & y) const
{
	x = m_x;
	y = m_y;
}

void SpinButton::getSize(int & sx, int & sy) const
{
	sx = m_sprite->getWidth();
	sy = m_sprite->getHeight();
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

CheckButton::CheckButton(int x, int y, bool value, const char * filename, const char * localString, int textX, int textY, int textSize)
	: m_sprite(new Sprite(filename))
	, m_isMouseDown(false)
	, m_x(0)
	, m_y(0)
	, m_value(value)
	, m_hasBeenSelected(false)
	, m_localString(localString)
	, m_textX(textX)
	, m_textY(textY)
	, m_textSize(textSize)
{
	setPosition(x, y);
}

CheckButton::~CheckButton()
{
	delete m_sprite;
	m_sprite = 0;
}

void CheckButton::setPosition(int x, int y)
{
	m_x = x - m_sprite->getWidth() / 2;
	m_y = y - m_sprite->getHeight() / 2;
}

bool CheckButton::isClicked()
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
	{
		m_value = !m_value;

		g_app->playSound("ui/button/select.ogg");
	}

	return result;
}

void CheckButton::draw()
{
	if (m_hasFocus)
		setColor(colorWhite);
	else
		setColor(255, 255, 255, 255, 63);
	m_sprite->drawEx(m_x, m_y);

	if (m_localString)
	{
		if (m_value)
			setColor(colorWhite);
		else
			setColor(255, 255, 255, 255, 63);
		setMainFont();
		drawText(m_x + m_textX, m_y + m_textY, m_textSize, +1.f, +1.f, "%s", getLocalString(m_localString));
	}
}

void CheckButton::getPosition(int & x, int & y) const
{
	x = m_x;
	y = m_y;
}

void CheckButton::getSize(int & sx, int & sy) const
{
	sx = m_sprite->getWidth();
	sy = m_sprite->getHeight();
}

bool CheckButton::hitTest(int x, int y) const
{
	return
		x >= m_x &&
		y >= m_y &&
		x < m_x + m_sprite->getWidth() &&
		y < m_y + m_sprite->getHeight();
}

void CheckButton::onFocusChange(bool hasFocus, bool isAutomaticSelection)
{
	MenuNavElem::onFocusChange(hasFocus, isAutomaticSelection);

	if (hasFocus && !isAutomaticSelection)
		g_app->playSound("ui/button/focus.ogg");
}

void CheckButton::onSelect()
{
	m_hasBeenSelected = true;
}

//

Slider::Slider(int x, int y, float min, float max, float value, const char * filename, const char * localString, int textX, int textY, int textSize)
	: m_sprite(new Sprite(filename))
	, m_x(0)
	, m_y(0)
	, m_value(Calc::Clamp(value, min, max))
	, m_min(min)
	, m_max(max)
	, m_localString(localString)
	, m_textX(textX)
	, m_textY(textY)
	, m_textSize(textSize)
{
	setPosition(x, y);
}

Slider::~Slider()
{
	delete m_sprite;
	m_sprite = 0;
}

void Slider::setPosition(int x, int y)
{
	m_x = x - m_sprite->getWidth() / 2;
	m_y = y - m_sprite->getHeight() / 2;
}

void Slider::setValue(float value)
{
	const float oldValue = m_value;

	m_value = Calc::Clamp(value, m_min, m_max);

	if (m_value != oldValue)
		g_app->playSound("ui/button/select.ogg");
}

void Slider::changeValue(float delta)
{
	setValue(m_value + delta);
}

bool Slider::hasChanged()
{
	bool result = false;

	const float oldValue = m_value;

	if (m_hasFocus)
	{
		if (keyboard.wentDown(SDLK_RETURN))
			m_hasFocusLock = !m_hasFocusLock;
	}

	if (m_hasFocus)
	{
		if (mouse.isDown(BUTTON_LEFT))
		{
			// todo : directly compute value

			const float t = Calc::Clamp((mouse.x - m_x) / float(m_sprite->getWidth()), 0.f, 1.f);
			const float v = m_min + t * (m_max - m_min);

			setValue(v);
		}
	}

	if (m_hasFocusLock)
	{
		const float step = (m_max - m_min) / 15.f;

		if (gamepad[0].wentDown(DPAD_LEFT) || keyboard.wentDown(SDLK_LEFT, true))
			changeValue(-step);
		if (gamepad[0].wentDown(DPAD_RIGHT) || keyboard.wentDown(SDLK_RIGHT, true))
			changeValue(+step);
	}

	return m_value != oldValue;
}

void Slider::draw()
{
	if (m_hasFocus)
		setColor(colorWhite);
	else
		setColor(255, 255, 255, 255, 63);
	m_sprite->drawEx(m_x, m_y);

	const float t = (m_value - m_min) / (m_max - m_min);
	const float x = m_x + m_sprite->getWidth() * t;

	if (m_localString)
	{
		setMainFont();
		drawText(m_x + m_textX, m_y + m_textY, m_textSize, +1.f, +1.f, "%s", getLocalString(m_localString));
	}

	setColor(colorGreen);
	drawLine(x, m_y, x, m_y + m_sprite->getHeight());

	if (g_devMode)
	{
		setColor(colorWhite);
		setMainFont();
		drawText(m_x, m_y, 18, +1.f, +1.f, "min=%d, max=%d, value=%d", m_min, m_max, m_value);
	}
}

void Slider::getPosition(int & x, int & y) const
{
	x = m_x;
	y = m_y;
}

void Slider::getSize(int & sx, int & sy) const
{
	sx = m_sprite->getWidth();
	sy = m_sprite->getHeight();
}

bool Slider::hitTest(int x, int y) const
{
	return
		x >= m_x &&
		y >= m_y &&
		x < m_x + m_sprite->getWidth() &&
		y < m_y + m_sprite->getHeight();
}

void Slider::onFocusChange(bool hasFocus, bool isAutomaticSelection)
{
	MenuNavElem::onFocusChange(hasFocus, isAutomaticSelection);

	if (hasFocus && !isAutomaticSelection)
		g_app->playSound("ui/button/focus.ogg");
}

void Slider::onSelect()
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

//

TileTransition::TileTransition()
	: m_surface(0)
	, m_time(0.f)
	, m_timeRcp(1.f)
{
	m_surface = new Surface(GFX_SX, GFX_SY);
}

TileTransition::~TileTransition()
{
	delete m_surface;
}

void TileTransition::begin(float time)
{
	m_time = time;
	m_timeRcp = 1.f / time;

	blitBackBufferToSurface(m_surface);
}

void TileTransition::tick(float dt)
{
	m_time-= dt;
	if (m_time < 0.f)
		m_time = 0.f;
}

void TileTransition::draw()
{
	const float t = m_time * m_timeRcp;

	if (t != 0.f)
	{
		gxSetTexture(m_surface->getTexture());

		const int quadSize = 50;
		const int numX = GFX_SX / quadSize;
		const int numY = GFX_SY / quadSize;
		const float sizeX = GFX_SX / float(numX);
		const float sizeY = GFX_SY / float(numY);

		gxBegin(GL_QUADS);
		{
			for (int y = 0; y < numY; ++y)
			{
				for (int x = 0; x < numX; ++x)
				{
					const float xf = (x + .5f) * sizeX;
					const float yf = (y + .5f) * sizeY;
					const float dx = GFX_SX/2.f - xf;
					const float dy = GFX_SY/2.f - yf;
					const float ds = std::sqrtf(dx * dx + dy * dy) / GFX_SY;

					const float t2 = saturate(t * (1.f + ds * 3.f));

					const float x1 = (x + .5f - .5f * t2) * sizeX;
					const float y1 = (y + .5f - .5f * t2) * sizeY;
					const float x2 = (x + .5f + .5f * t2) * sizeX;
					const float y2 = (y + .5f + .5f * t2) * sizeY;

					const float u1 = (x + 0) / float(numX);
					const float v1 = (y + 0) / float(numY);
					const float u2 = (x + 1) / float(numX);
					const float v2 = (y + 1) / float(numY);

					gxColor4f(1.f, 1.f, 1.f, t2 * t2);
					gxTexCoord2f(u1, 1.f - v1); gxVertex2f(x1, y1);
					gxTexCoord2f(u2, 1.f - v1); gxVertex2f(x2, y1);
					gxTexCoord2f(u2, 1.f - v2); gxVertex2f(x2, y2);
					gxTexCoord2f(u1, 1.f - v2); gxVertex2f(x1, y2);
				}
			}
		}
		gxEnd();

		gxSetTexture(0);
	}
}

TileTransition * g_tileTransition = 0;
