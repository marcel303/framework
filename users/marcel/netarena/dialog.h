#pragma once

#include <string>
#include <vector>

enum DialogType
{
	DialogType_Confirm,
	DialogType_YesNo
};

class Dialog
{
public:
	enum State
	{
		State_Initial,
		State_FadeIn,
		State_Active,
		State_FadeOut,
		State_Done
	};

	DialogType m_type;
	State m_state;
	std::string m_text[2];
	float m_fadeTime;

	void tick(float dt);
	void draw();
};

class DialogMgr
{
	std::vector<Dialog> m_dialogs;

public:
	void tick(float dt);
	void draw();

	void push(DialogType type, std::string text1, std::string text2);
	void pop();
	void reset();

	Dialog * getActiveDialog();
};
