#pragma once

#include <vector>

class Button;
class Sprite;

class MenuNavElem
{
public:
	bool m_hasFocus;
	bool m_hasFocusLock;
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

class ButtonLegend
{
public:
	enum ButtonId
	{
		kButtonId_B,
		kButtonId_ESCAPE
	};

	struct Elem
	{
		ButtonId buttonId;
		Button * button;
		const char * localString;
	};

	struct DrawElem
	{
		Button * button;
		int clickX1;
		int clickY1;
		int clickX2;
		int clickY2;

		const char * sprite;
		int spriteX;
		int spriteY;

		const char * text;
		int textX;
		int textY;
	};

	std::vector<Elem> m_elems;

	ButtonLegend();

	void tick(float dt, int x, int y);
	void draw(int x, int y);

	void addElem(ButtonId buttonId, Button * button, const char * localString);
	std::vector<DrawElem> getDrawElems(int x, int y);
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

	int m_moveX;
	int m_moveY;
	float m_startOpacity;
	float m_moveTime;
	float m_moveTimeStart;

public:
	Button(int x, int y, const char * filename, const char * localString, int textX, int textY, int textSize);
	~Button();

	void setPosition(int x, int y);
	void setAnimation(int moveX, int moveY, float opacity, float time);

	bool isClicked();
	void draw();

	virtual void getPosition(int & x, int & y) const;
	virtual bool hitTest(int x, int y) const;
	virtual void onFocusChange(bool hasFocus, bool isAutomaticSelection);
	virtual void onSelect();
};

class SpinButton : public MenuNavElem
{
public:
	Sprite * m_sprite;
	int m_x;
	int m_y;
	int m_value;
	int m_min;
	int m_max;

	const char * m_localString;
	int m_textX;
	int m_textY;
	int m_textSize;

public:
	SpinButton(int x, int y, int min, int max, int value, const char * filename, const char * localString, int textX, int textY, int textSize);
	~SpinButton();

	void setPosition(int x, int y);
	void changeValue(int delta);

	bool hasChanged();
	void draw();

	virtual void getPosition(int & x, int & y) const;
	virtual bool hitTest(int x, int y) const;
	virtual void onFocusChange(bool hasFocus, bool isAutomaticSelection);
	virtual void onSelect();
};

class CheckButton : public MenuNavElem
{
public:
	Sprite * m_sprite;
	int m_x;
	int m_y;
	bool m_value;
	bool m_isMouseDown;
	bool m_hasBeenSelected;

	const char * m_localString;
	int m_textX;
	int m_textY;
	int m_textSize;

public:
	CheckButton(int x, int y, bool value, const char * filename, const char * localString, int textX, int textY, int textSize);
	~CheckButton();

	void setPosition(int x, int y);

	bool isClicked();
	void draw();

	virtual void getPosition(int & x, int & y) const;
	virtual bool hitTest(int x, int y) const;
	virtual void onFocusChange(bool hasFocus, bool isAutomaticSelection);
	virtual void onSelect();
};

class Slider : public MenuNavElem
{
public:
	Sprite * m_sprite;
	int m_x;
	int m_y;
	float m_value;
	float m_min;
	float m_max;

	const char * m_localString;
	int m_textX;
	int m_textY;
	int m_textSize;

public:
	Slider(int x, int y, float min, float max, float value, const char * filename, const char * localString, int textX, int textY, int textSize);
	~Slider();

	void setPosition(int x, int y);
	void setValue(float value);
	void changeValue(float delta);

	bool hasChanged();
	void draw();

	virtual void getPosition(int & x, int & y) const;
	virtual bool hitTest(int x, int y) const;
	virtual void onFocusChange(bool hasFocus, bool isAutomaticSelection);
	virtual void onSelect();
};

void setLocal(const char * local);
const char * getLocalString(const char * localString);
