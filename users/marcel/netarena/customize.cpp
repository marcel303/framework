#include "customize.h"
#include "gamedefs.h"
#include "main.h"
#include "player.h"
#include "uicommon.h"

CustomizeMenu::CustomizeMenu()
	: m_back(0)
	, m_menuNav(0)
	, m_buttonLegend(0)
{
	m_menuNav = new MenuNav();

	m_back = new Button(0, 0, "mainmenu-button.png", 0, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);

	for (int i = 0; i < MAX_CHARACTERS; ++i)
	{
		const int cellX = i % 4;
		const int cellY = i / 4;

		const int x = CUSTOMIZEMENU_PORTRAIT_BASE_X + cellX * CUSTOMIZEMENU_PORTRAIT_SPACING_X;
		const int y = CUSTOMIZEMENU_PORTRAIT_BASE_Y + cellY * CUSTOMIZEMENU_PORTRAIT_SPACING_Y;

		char portraitName[64];
		sprintf_s(portraitName, sizeof(portraitName), "ui/characters/%02d-portrait.png", i);

		m_characters[i] = new Button(x, y, portraitName, 0, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);

		m_menuNav->addElem(m_characters[i]);
	}

	m_buttonLegend = new ButtonLegend();
	m_buttonLegend->addElem(ButtonLegend::kButtonId_B, m_back, "ui-legend-back");
	m_buttonLegend->addElem(ButtonLegend::kButtonId_ESCAPE, m_back, "ui-legend-back");
}

CustomizeMenu::~CustomizeMenu()
{
	delete m_buttonLegend;

	delete m_menuNav;

	for (int i = 0; i < MAX_CHARACTERS; ++i)
		delete m_characters[i];

	delete m_back;
}

void CustomizeMenu::onEnter()
{
	inputLockAcquire(); // fixme : remove. needed for escape = quit hack for now
}

void CustomizeMenu::onExit()
{
	inputLockRelease(); // fixme : remove. needed for escape = quit hack for now
}

bool CustomizeMenu::tick(float dt)
{
	if (g_devMode)
	{
		for (int i = 0; i < MAX_CHARACTERS; ++i)
		{
			const int cellX = i % 4;
			const int cellY = i / 4;

			const int x = CUSTOMIZEMENU_PORTRAIT_BASE_X + cellX * CUSTOMIZEMENU_PORTRAIT_SPACING_X;
			const int y = CUSTOMIZEMENU_PORTRAIT_BASE_Y + cellY * CUSTOMIZEMENU_PORTRAIT_SPACING_Y;

			m_characters[i]->setPosition(x, y);
		}
	}

	m_menuNav->tick(dt);

	m_buttonLegend->tick(dt, UI_BUTTONLEGEND_X, UI_BUTTONLEGEND_Y);

	//

	bool clicked = false;

	for (int i = 0; i < MAX_CHARACTERS; ++i)
	{
		if (m_characters[i]->isClicked())
		{
			g_app->m_menuMgr->push(new CharacterMenu(i));
			break;
		}
	}

	if (clicked)
	{
		// already handled
	}
	else if (m_back->isClicked() || gamepad[0].wentDown(GAMEPAD_B) || keyboard.wentDown(SDLK_ESCAPE)) // fixme : generalize and remove hardcoded gamepad index
	{
		g_app->playSound("ui/sounds/menu-back.ogg");
		return true;
	}

	return false;
}

void CustomizeMenu::draw()
{
	m_buttonLegend->draw(UI_BUTTONLEGEND_X, UI_BUTTONLEGEND_Y);

	for (int i = 0; i < MAX_CHARACTERS; ++i)
		m_characters[i]->draw();
}

//

CharacterMenu::CharacterMenu(int characterIndex)
	: m_characterIndex(characterIndex)
	, m_back(0)
	, m_effects(0)
	, m_skin(0)
	, m_emblem(0)
	, m_testGame(0)
	, m_menuNav(0)
	, m_buttonLegend(0)
{
	const int numSkins = 4;
	const int numEmblems = 4;

	const int buttonPosX = GFX_SX*1/4;

	m_back = new Button(0, 0, "mainmenu-button.png", 0, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_effects = new Button(buttonPosX, GFX_SY/3, "mainmenu-button.png", "menu-char-effects", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_skin = new SpinButton(buttonPosX, GFX_SY/3 + 150, 0, numSkins - 1, "mainmenu-button.png", "menu-char-skin", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_emblem = new SpinButton(buttonPosX, GFX_SY/3 + 300, 0, numEmblems - 1, "mainmenu-button.png", "menu-char-emblem", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_testGame = new Button(buttonPosX, GFX_SY/3 + 450, "mainmenu-button.png", "menu-char-test", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);

	m_menuNav = new MenuNav();

	if (m_effects)
		m_menuNav->addElem(m_effects);
	if (m_skin)
		m_menuNav->addElem(m_skin);
	if (m_emblem)
		m_menuNav->addElem(m_emblem);
	if (m_testGame)
		m_menuNav->addElem(m_testGame);

	m_buttonLegend = new ButtonLegend();
	m_buttonLegend->addElem(ButtonLegend::kButtonId_B, m_back, "ui-legend-back");
	m_buttonLegend->addElem(ButtonLegend::kButtonId_ESCAPE, m_back, "ui-legend-back");
}

CharacterMenu::~CharacterMenu()
{
	delete m_buttonLegend;
	delete m_menuNav;

	delete m_back;
	delete m_effects;
	delete m_skin;
	delete m_emblem;
	delete m_testGame;
}

void CharacterMenu::onEnter()
{
}

void CharacterMenu::onExit()
{
}

bool CharacterMenu::tick(float dt)
{
	m_menuNav->tick(dt);

	m_buttonLegend->tick(dt, UI_BUTTONLEGEND_X, UI_BUTTONLEGEND_Y);

	// todo : check for button clicks

	if (m_skin->hasChanged())
	{
		g_app->m_userSettings->chars[m_characterIndex].skin = m_skin->m_value;
	}
	else if (m_emblem->hasChanged())
	{
		g_app->m_userSettings->chars[m_characterIndex].emblem = m_emblem->m_value;
	}
	else if (m_back->isClicked() || gamepad[0].wentDown(GAMEPAD_B) || keyboard.wentDown(SDLK_ESCAPE)) // fixme : generalize and remove hardcoded gamepad index
	{
		g_app->playSound("ui/sounds/menu-back.ogg");
		return true;
	}

	return false;
}

void CharacterMenu::draw()
{
	m_buttonLegend->draw(UI_BUTTONLEGEND_X, UI_BUTTONLEGEND_Y);

	// draw the selected character

	const CharacterData * characterData = getCharacterData(m_characterIndex);
	const auto & characterSettings = g_app->m_userSettings->chars[m_characterIndex];

	fassert(characterData);
	if (characterData)
	{
		setColor(colorWhite);

		Spriter & spriter = *characterData->getSpriter();
		SpriterState spriterState;
		spriterState.x = GFX_SX*3/4;
		spriterState.y = GFX_SY/2;
		spriterState.startAnim(spriter, "Idle");
		spriterState.animTime = framework.time;
		spriterState.setCharacterMap(spriter, characterSettings.skin);
		spriter.draw(spriterState);
	}

	if (m_effects)
		m_effects->draw();
	if (m_skin)
		m_skin->draw();
	if (m_emblem)
		m_emblem->draw();
	if (m_testGame)
		m_testGame->draw();
}
