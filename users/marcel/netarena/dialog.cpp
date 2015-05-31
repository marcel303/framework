#include "Debugging.h"
#include "dialog.h"
#include "framework.h"
#include "gamedefs.h"
#include "main.h" // g_devMode

#define UI_DIALOG_FADEIN_TIME .2f
#define UI_DIALOG_FADEOUT_TIME .2f

#define UI_DIALOG_SX 700
#define UI_DIALOG_SY 400
#define UI_DIALOG_PX ((GFX_SX -  UI_DIALOG_SX) / 2)
#define UI_DIALOG_PY ((GFX_SY -  UI_DIALOG_SY) / 2)
#define UI_DIALOG_FONT_SIZE 32

#define DIALOG_SPRITER Spriter(m_type == DialogType_Confirm ? "ui/dialog/confirm.scml" : "ui/dialog/yesno.scml")

Dialog::Dialog()
	: m_isCompleted(false)
	, m_selection(0)
	, m_numSelections(0)
	, m_id(-1)
	, m_type(DialogType_Undefined)
	, m_state(State_Initial)
	, m_fadeTime(0.f)
	, m_callback(0)
	, m_callbackArg(0)
{
}

void Dialog::tick(float dt)
{
	m_spriterState.updateAnim(DIALOG_SPRITER, dt);

	switch (m_state)
	{
	case State_Initial:
		m_state = State_FadeIn;
		m_fadeTime = UI_DIALOG_FADEIN_TIME;
		if (m_type == DialogType_Confirm)
			m_numSelections = 1;
		else
			m_numSelections = 2;
		applySelection(true);
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

			int newSelection = m_selection;

			if (gamepad[0].wentDown(DPAD_LEFT) && newSelection > 0)
				newSelection--;
			if (gamepad[0].wentDown(DPAD_RIGHT) && newSelection < m_numSelections - 1)
				newSelection++;

			if (newSelection != m_selection)
			{
				m_selection = newSelection;

				applySelection(false);

				Sound("ui/dialog/change.ogg").play();
			}

			if (gamepad[0].wentDown(GAMEPAD_A) && m_selection != -1)
			{
				complete(m_selection == 0 ? DialogResult_Yes : DialogResult_No);

				m_state = State_FadeOut;
				m_fadeTime = UI_DIALOG_FADEOUT_TIME;

				Sound("ui/dialog/select.ogg").play();
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
		setColor(255, 255, 255, opacity * 255.f);
		m_spriterState.x = UI_DIALOG_PX;
		m_spriterState.y = UI_DIALOG_PY;
		DIALOG_SPRITER.draw(m_spriterState);

		if (g_devMode)
		{
			setColor(colorGreen);
			drawRectLine(
				UI_DIALOG_PX,
				UI_DIALOG_PY,
				UI_DIALOG_PX + UI_DIALOG_SX,
				UI_DIALOG_PY + UI_DIALOG_SY);
		}

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

void Dialog::complete(DialogResult result)
{
	if (!m_isCompleted)
	{
		m_isCompleted = true;

		if (m_callback)
		{
			m_callback(m_callbackArg, m_id, result);
		}
	}
}

void Dialog::applySelection(bool instant)
{
	m_spriterState.startAnim(DIALOG_SPRITER, m_selection);
}

//

DialogMgr::DialogMgr()
	: m_nextDialogId(0)
{
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

int DialogMgr::push(DialogType type, std::string text1, std::string text2, DialogCallback callback, void * callbackArg)
{
	Dialog dialog;
	dialog.m_id = m_nextDialogId++;
	dialog.m_type = type;
	dialog.m_state = Dialog::State_Initial;
	dialog.m_text[0] = text1;
	dialog.m_text[1] = text2;
	dialog.m_callback = callback;
	dialog.m_callbackArg = callbackArg;

	m_dialogs.push_back(dialog);

	return dialog.m_id;
}

void DialogMgr::pop()
{
	m_dialogs.pop_back();
}

void DialogMgr::reset()
{
	m_dialogs.clear();
}

void DialogMgr::dismiss(int id)
{
	for (auto i = m_dialogs.begin(); i != m_dialogs.end(); ++i)
	{
		if (i->m_id == id)
		{
			i->complete(DialogResult_Cancelled);
			m_dialogs.erase(i);
			return;
		}
	}
}

Dialog * DialogMgr::getActiveDialog()
{
	return m_dialogs.empty() ? 0 : &m_dialogs.back();
}
