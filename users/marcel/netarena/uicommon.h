#pragma once

class Sprite;

class Button
{
	Sprite * m_sprite;
	int m_x;
	int m_y;
	bool m_isMouseDown;

public:
	Button(int x, int y, const char * filename);
	~Button();

	bool isClicked();
	void draw();
};
