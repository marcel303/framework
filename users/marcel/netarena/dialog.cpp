#include "dialog.h"
#include "framework.h"
#include "gamedefs.h"

#define UI_DIALOG_FADEIN_TIME .2f
#define UI_DIALOG_FADEOUT_TIME .2f

#define UI_DIALOG_SX 700
#define UI_DIALOG_SY 400
#define UI_DIALOG_PX ((GFX_SX -  UI_DIALOG_SX) / 2)
#define UI_DIALOG_PY ((GFX_SY -  UI_DIALOG_SY) / 2)
#define UI_DIALOG_FONT_SIZE 32

void Dialog::tick(float dt)
{
	switch (m_state)
	{
	case State_Initial:
		m_state = State_FadeIn;
		m_fadeTime = UI_DIALOG_FADEIN_TIME;
		break;
	case State_FadeIn:
		m_fadeTime -= dt;
		if (m_fadeTime <= 0.f)
		{
			m_state = State_Active;
			m_fadeTime = 0.f;
		}
		break;
	case State_Active:
		{
			// fixme

			int selection = -1;

			if (gamepad[0].wentDown(GAMEPAD_A))
				selection = 0;
			if (gamepad[0].wentDown(GAMEPAD_B))
				selection = 1;

			if (selection != -1)
			{
				m_state = State_FadeOut;
				m_fadeTime = UI_DIALOG_FADEOUT_TIME;
			}
		}
		break;
	case State_FadeOut:
		m_fadeTime -= dt;
		if (m_fadeTime <= 0.f)
		{
			m_state = State_Done;
			m_fadeTime = 0.f;
		}
		break;
	case State_Done:
		break;
	}
}

void Dialog::draw()
{
	float opacity = 0.f;

	if (m_state == State_FadeIn)
		opacity = 1.f - m_fadeTime / UI_DIALOG_FADEIN_TIME;
	if (m_state == State_Active)
		opacity = 1.f;
	if (m_state == State_FadeOut)
		opacity = m_fadeTime / UI_DIALOG_FADEOUT_TIME;

	if (opacity != 0.f)
	{
		setColor(0, 0, 0, opacity * 255.f);
		drawRect(
			UI_DIALOG_PX,
			UI_DIALOG_PY,
			UI_DIALOG_PX + UI_DIALOG_SX,
			UI_DIALOG_PY + UI_DIALOG_SY);

		setColor(255, 255, 255, opacity * 255.f);
		setFont("calibri.ttf");
		drawText(
			UI_DIALOG_PX + UI_DIALOG_SX/2,
			UI_DIALOG_PY + UI_DIALOG_SY/2,
			UI_DIALOG_FONT_SIZE,
			0.f, 0.f,
			"%s", 
			m_text[0].c_str());
	}
}

void DialogMgr::tick(float dt)
{
	Dialog * dialog = getActiveDialog();
	if (dialog)
	{
		dialog->tick(dt);

		if (dialog->m_state == Dialog::State_Done)
			pop();
	}
}

void DialogMgr::draw()
{
	Dialog * dialog = getActiveDialog();
	if (dialog)
	{
		dialog->draw();
	}
}

void DialogMgr::push(DialogType type, std::string text1, std::string text2)
{
	Dialog dialog;
	dialog.m_type = type;
	dialog.m_state = Dialog::State_Initial;
	dialog.m_text[0] = text1;
	dialog.m_text[1] = text2;

	m_dialogs.push_back(dialog);
}

void DialogMgr::pop()
{
	m_dialogs.pop_back();
}

void DialogMgr::reset()
{
	m_dialogs.clear();
}

Dialog * DialogMgr::getActiveDialog()
{
	return m_dialogs.empty() ? 0 : &m_dialogs.back();
}
