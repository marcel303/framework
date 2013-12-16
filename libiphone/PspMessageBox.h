#pragma once

enum PspMbButtons
{
	PspMbButtons_YesNo,
	PspMbButtons_Ok
};

typedef void (*PspMbFinishedHandler)(int id, bool result);

bool PspMessageBox_ShowDialog(const char* text, int id, PspMbButtons buttons, PspMbFinishedHandler handleFinish); // finish handler is optional
bool PspMessageBox_ShowError(int errorCode);
bool PspMessageBox_Update();
void PspMessageBox_Finish();
bool PspMessageBox_IsActive();
