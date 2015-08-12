#pragma once

class Sprite;

class MenuNavElem
{
public:
	bool m_hasFocus;
	MenuNavElem * m_next;

public:
	MenuNavElem();

	virtual void getPosition(int & x, int & y) const;
	virtual bool hitTest(int x, int y) const;
	virtual void onFocusChange(bool hasFocus, bool isAutomaticSelection);
	virtual void onSelect();
};

class MenuNav
{
public:
	MenuNavElem * m_first;
	MenuNavElem * m_selection;

public:
	MenuNav();

	void tick(float dt);

	void addElem(MenuNavElem * elem);
	void moveSelection(int dx, int dy);
	void setSelection(MenuNavElem * elem, bool isAutomaticSelection);
	void handleSelect();
};

class Button : public MenuNavElem
{
public:
	Sprite * m_sprite;
	int m_x;
	int m_y;
	bool m_isMouseDown;
	bool m_hasBeenSelected;

	const char * m_localString;
	int m_textX;
	int m_textY;
	int m_textSize;

public:
	Button(int x, int y, const char * filename, const char * localString, int textX, int textY, int textSize);
	~Button();

	void setPosition(int x, int y);

	bool isClicked();
	void draw();

	virtual void getPosition(int & x, int & y) const;
	virtual bool hitTest(int x, int y) const;
	virtual void onFocusChange(bool hasFocus, bool isAutomaticSelection);
	virtual void onSelect();
};

void setLocal(const char * local);
const char * getLocalString(const char * localString);
