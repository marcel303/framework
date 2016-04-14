#include "framework.h"
#include "gamedefs.h"
#include "optione.h"
#include "uicommon.h"

#include "main.h" // fixme : remove. needed for escape = quit hack for now

OptioneMenu::OptioneMenu()
	: m_back(0)
	, m_audio(0)
	, m_display(0)
	, m_video(0)
	, m_menuNav(0)
	, m_buttonLegend(0)
{
	m_back = new Button(0, 0, "mainmenu-button.png", "mainmenu-button.png", 0, MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE, MAINMENU_BUTTON_TEXT_COLOR);
	m_audio = new Button(GFX_SX/2, GFX_SY/3, "mainmenu-button.png", "mainmenu-button.png", "menu-audio", MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE, MAINMENU_BUTTON_TEXT_COLOR);
	m_display = new Button(GFX_SX/2, GFX_SY/3 + 150, "mainmenu-button.png", "mainmenu-button.png", "menu-display", MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE, MAINMENU_BUTTON_TEXT_COLOR);
	m_video = new Button(GFX_SX/2, GFX_SY/3 + 300, "mainmenu-button.png", "mainmenu-button.png", "menu-video", MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE, MAINMENU_BUTTON_TEXT_COLOR);

	m_menuNav = new MenuNav();
	m_menuNav->addElem(m_audio);
	m_menuNav->addElem(m_display);
	m_menuNav->addElem(m_video);

	m_buttonLegend = new ButtonLegend();
	m_buttonLegend->addElem(ButtonLegend::kButtonId_B, m_back, "ui-legend-back");
	m_buttonLegend->addElem(ButtonLegend::kButtonId_ESCAPE, m_back, "ui-legend-back");
}

OptioneMenu::~OptioneMenu()
{
	delete m_buttonLegend;

	delete m_menuNav;

	delete m_back;
	delete m_audio;
	delete m_display;
	delete m_video;
}

void OptioneMenu::onEnter()
{
	inputLockAcquire(); // fixme : remove. needed for escape = quit hack for now
}

void OptioneMenu::onExit()
{
	g_app->saveUserSettings();

	inputLockRelease(); // fixme : remove. needed for escape = quit hack for now
}

bool OptioneMenu::tick(float dt)
{
	m_menuNav->tick(dt);

	m_buttonLegend->tick(dt, UI_BUTTONLEGEND_X, UI_BUTTONLEGEND_Y);

	//

	if (m_audio && m_audio->isClicked())
	{
		g_app->m_menuMgr->push(new OptioneAudioMenu());
	}
	else if (m_display && m_display->isClicked())
	{
		g_app->m_menuMgr->push(new OptioneDisplayMenu());
	}
	else if (m_video && m_video->isClicked())
	{
		g_app->m_menuMgr->push(new OptioneVideoMenu());
	}
	else if (m_back->isClicked() || gamepad[0].wentDown(GAMEPAD_B) || keyboard.wentDown(SDLK_ESCAPE)) // fixme : generalize and remove hardcoded gamepad index
	{
		g_app->playSound("ui/sounds/menu-back.ogg");
		return true;
	}

	return false;
}

void OptioneMenu::draw()
{
	m_buttonLegend->draw(UI_BUTTONLEGEND_X, UI_BUTTONLEGEND_Y);

	if (m_audio)
		m_audio->draw();
	if (m_display)
		m_display->draw();
	if (m_video)
		m_video->draw();
}

//

OptioneAudioMenu::OptioneAudioMenu()
	: m_back(0)
	, m_menuNav(0)
	, m_buttonLegend(0)
{
	m_back = new Button(0, 0, "mainmenu-button.png", "mainmenu-button.png", 0, MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE, MAINMENU_BUTTON_TEXT_COLOR);
	m_musicEnabled = new CheckButton(GFX_SX/3, GFX_SY/3, g_app->m_userSettings->audio.musicEnabled, "mainmenu-button.png", "menu-music-enabled", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_musicVolume = new Slider(GFX_SX/3 + 650, GFX_SY/3, 0.f, 1.f, g_app->m_userSettings->audio.musicVolume, "mainmenu-button.png", "menu-music-volume", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_soundEnabled = new CheckButton(GFX_SX/3, GFX_SY/3 + 150, g_app->m_userSettings->audio.soundEnabled, "mainmenu-button.png", "menu-sound-enabled", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_soundVolume = new Slider(GFX_SX/3 + 650, GFX_SY/3 + 150, 0.f, 1.f, g_app->m_userSettings->audio.soundVolume, "mainmenu-button.png", "menu-sound-volume", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_announcerEnabled = new CheckButton(GFX_SX/3, GFX_SY/3 + 300, g_app->m_userSettings->audio.announcerEnabled, "mainmenu-button.png", "menu-announcer-enabled", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);
	m_announcerVolume = new Slider(GFX_SX/3 + 650, GFX_SY/3 + 300, 0.f, 1.f, g_app->m_userSettings->audio.announcerVolume, "mainmenu-button.png", "menu-announcer-volume", MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE);

	m_menuNav = new MenuNav();
	m_menuNav->addElem(m_musicEnabled);
	m_menuNav->addElem(m_musicVolume);
	m_menuNav->addElem(m_soundEnabled);
	m_menuNav->addElem(m_soundVolume);
	m_menuNav->addElem(m_announcerEnabled);
	m_menuNav->addElem(m_announcerVolume);

	m_buttonLegend = new ButtonLegend();
	m_buttonLegend->addElem(ButtonLegend::kButtonId_B, m_back, "ui-legend-back");
	m_buttonLegend->addElem(ButtonLegend::kButtonId_ESCAPE, m_back, "ui-legend-back");
}

OptioneAudioMenu::~OptioneAudioMenu()
{
	delete m_buttonLegend;

	delete m_menuNav;

	delete m_back;
	delete m_musicEnabled;
	delete m_musicVolume;
	delete m_soundEnabled;
	delete m_soundVolume;
	delete m_announcerEnabled;
	delete m_announcerVolume;
}

void OptioneAudioMenu::onEnter()
{
	inputLockAcquire(); // fixme : remove. needed for escape = quit hack for now
}

void OptioneAudioMenu::onExit()
{
	inputLockRelease(); // fixme : remove. needed for escape = quit hack for now
}

bool OptioneAudioMenu::tick(float dt)
{
	m_menuNav->tick(dt);

	m_buttonLegend->tick(dt, UI_BUTTONLEGEND_X, UI_BUTTONLEGEND_Y);

	// check for changes and apply the new settings

	if (m_musicEnabled->isClicked())
		g_app->m_userSettings->audio.musicEnabled = m_musicEnabled->m_value;
	if (m_musicVolume->hasChanged())
		g_app->m_userSettings->audio.musicVolume = m_musicVolume->m_value;
	if (m_soundEnabled->isClicked())
		g_app->m_userSettings->audio.soundEnabled = m_soundEnabled->m_value;
	if (m_soundVolume->hasChanged())
		g_app->m_userSettings->audio.soundVolume = m_soundVolume->m_value;
	if (m_announcerEnabled->isClicked())
		g_app->m_userSettings->audio.announcerEnabled = m_announcerEnabled->m_value;
	if (m_announcerVolume->hasChanged())
		g_app->m_userSettings->audio.announcerVolume = m_announcerVolume->m_value;

	if (m_back->isClicked() || gamepad[0].wentDown(GAMEPAD_B) || keyboard.wentDown(SDLK_ESCAPE)) // fixme : generalize and remove hardcoded gamepad index
	{
		g_app->playSound("ui/sounds/menu-back.ogg");
		return true;
	}

	return false;
}

void OptioneAudioMenu::draw()
{
	m_buttonLegend->draw(UI_BUTTONLEGEND_X, UI_BUTTONLEGEND_Y);

	m_musicEnabled->draw();
	m_musicVolume->draw();
	m_soundEnabled->draw();
	m_soundVolume->draw();
	m_announcerEnabled->draw();
	m_announcerVolume->draw();
}

//

OptioneDisplayMenu::OptioneDisplayMenu()
	: m_back(0)
	, m_menuNav(0)
	, m_buttonLegend(0)
{
	m_back = new Button(0, 0, "mainmenu-button.png", "mainmenu-button.png", 0, MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE, MAINMENU_BUTTON_TEXT_COLOR);

	m_menuNav = new MenuNav();

	m_buttonLegend = new ButtonLegend();
	m_buttonLegend->addElem(ButtonLegend::kButtonId_B, m_back, "ui-legend-back");
	m_buttonLegend->addElem(ButtonLegend::kButtonId_ESCAPE, m_back, "ui-legend-back");
}

OptioneDisplayMenu::~OptioneDisplayMenu()
{
	delete m_buttonLegend;

	delete m_menuNav;

	delete m_back;
}

void OptioneDisplayMenu::onEnter()
{
	inputLockAcquire(); // fixme : remove. needed for escape = quit hack for now
}

void OptioneDisplayMenu::onExit()
{
	inputLockRelease(); // fixme : remove. needed for escape = quit hack for now
}

bool OptioneDisplayMenu::tick(float dt)
{
	m_menuNav->tick(dt);

	m_buttonLegend->tick(dt, UI_BUTTONLEGEND_X, UI_BUTTONLEGEND_Y);

	//

	if (m_back->isClicked() || gamepad[0].wentDown(GAMEPAD_B) || keyboard.wentDown(SDLK_ESCAPE)) // fixme : generalize and remove hardcoded gamepad index
	{
		g_app->playSound("ui/sounds/menu-back.ogg");
		return true;
	}

	return false;
}

void OptioneDisplayMenu::draw()
{
	m_buttonLegend->draw(UI_BUTTONLEGEND_X, UI_BUTTONLEGEND_Y);
}

//

OptioneVideoMenu::OptioneVideoMenu()
	: m_back(0)
	, m_menuNav(0)
	, m_buttonLegend(0)
{
	m_back = new Button(0, 0, "mainmenu-button.png", "mainmenu-button.png", 0, MAINMENU_BUTTON_FONT, MAINMENU_BUTTON_TEXT_X, MAINMENU_BUTTON_TEXT_Y, MAINMENU_BUTTON_TEXT_SIZE, MAINMENU_BUTTON_TEXT_COLOR);

	m_menuNav = new MenuNav();

	m_buttonLegend = new ButtonLegend();
	m_buttonLegend->addElem(ButtonLegend::kButtonId_B, m_back, "ui-legend-back");
	m_buttonLegend->addElem(ButtonLegend::kButtonId_ESCAPE, m_back, "ui-legend-back");
}

OptioneVideoMenu::~OptioneVideoMenu()
{
	delete m_buttonLegend;

	delete m_menuNav;

	delete m_back;
}

void OptioneVideoMenu::onEnter()
{
	inputLockAcquire(); // fixme : remove. needed for escape = quit hack for now
}

void OptioneVideoMenu::onExit()
{
	inputLockRelease(); // fixme : remove. needed for escape = quit hack for now
}

bool OptioneVideoMenu::tick(float dt)
{
	m_menuNav->tick(dt);

	m_buttonLegend->tick(dt, UI_BUTTONLEGEND_X, UI_BUTTONLEGEND_Y);

	//

	if (m_back->isClicked() || gamepad[0].wentDown(GAMEPAD_B) || keyboard.wentDown(SDLK_ESCAPE)) // fixme : generalize and remove hardcoded gamepad index
	{
		g_app->playSound("ui/sounds/menu-back.ogg");
		return true;
	}

	return false;
}

void OptioneVideoMenu::draw()
{
	m_buttonLegend->draw(UI_BUTTONLEGEND_X, UI_BUTTONLEGEND_Y);
}
