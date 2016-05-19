#pragma once

// class which renders the background during the menus

class UiBackground
{
public:
	enum State
	{
		kState_Hidden, // hidden
		kState_Visible, // visible, with Tilly and logo
		kState_VisibleClean // visible, but minimally intrusive
	};

	State m_state;
	float m_animTime;
	
	UiBackground();
	~UiBackground();
	
	void tick(float dt);
	void draw();
	
	void setState(State state);
};

extern UiBackground * g_uiBackground;
