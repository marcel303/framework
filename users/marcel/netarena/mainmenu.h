#pragma once

class Button;

class MainMenu
{
	Button * m_newGame;
	Button * m_findGame;
	Button * m_quitApp;

public:
	MainMenu();
	~MainMenu();

	void tick(float dt);
	void draw();
};
