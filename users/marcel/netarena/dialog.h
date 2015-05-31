#pragma once

#include <string>
#include <vector>
#include "framework.h"

enum DialogType
{
	DialogType_Undefined,
	DialogType_Confirm,
	DialogType_YesNo
};

enum DialogResult
{
	DialogResult_Yes,
	DialogResult_No,
	DialogResult_Cancelled
};

typedef void (*DialogCallback)(void * arg, int dialogId, DialogResult result);

class Dialog
{
	bool m_isCompleted;
	int m_selection;
	int m_numSelections;
	SpriterState m_spriterState;

public:
	enum State
	{
		State_Initial,
		State_FadeIn,
		State_Active,
		State_FadeOut,
		State_Done
	};

	int m_id;
	DialogType m_type;
	State m_state;
	std::string m_text[2];
	float m_fadeTime;

	DialogCallback m_callback;
	void * m_callbackArg;

	Dialog();

	void tick(float dt);
	void draw();

	void complete(DialogResult result);
	void applySelection(bool instant);
};

class DialogMgr
{
	int m_nextDialogId;
	std::vector<Dialog> m_dialogs;

public:
	DialogMgr();

	void tick(float dt);
	void draw();

	int push(DialogType type, std::string text1, std::string text2, DialogCallback callback, void * callbackArg);
	void pop();
	void reset();
	void dismiss(int id);

	Dialog * getActiveDialog();
};
