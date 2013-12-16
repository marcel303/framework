#include <kernel.h>
#include <libgu.h>
#include <string.h>
#include <utility/utility_msgdialog.h>
#include "Debugging.h"
#include "Exception.h"
#include "Log.h"
#include "PspMessageBox.h"

#define BIT_ERROR      1
#define LANG_SETTING   SCE_UTILITY_LANG_ENGLISH
#define BUTTON_SETTING SCE_UTILITY_CTRL_ASSIGN_CROSS_IS_ENTER

static SceUtilityMsgDialogParam param;
static int sId = -1;
static PspMbFinishedHandler sHandleFinish = 0;
static bool sIsRunning = false;

static int TranslateType(PspMbButtons buttons)
{
	switch (buttons)
	{
	case PspMbButtons_YesNo:
		return SCE_UTILITY_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO;
	case PspMbButtons_Ok:
		return SCE_UTILITY_MSGDIALOG_TYPE_BUTTON_TYPE_OK;
	default:
#ifndef DEPLOYMENT
		throw ExceptionNA();
#else
		return SCE_UTILITY_MSGDIALOG_TYPE_BUTTON_TYPE_NONE;
#endif
	}
}

static bool TranslateResult(int result)
{
	switch (result)
	{
	case SCE_UTILITY_MSGDIALOG_BUTTON_YES:
	//case SCE_UTILITY_MSGDIALOG_BUTTON_OK: // same as _YES
		return true;
	case SCE_UTILITY_MSGDIALOG_BUTTON_NO:
		return false;
	default:
#ifndef DEPLOYMENT
		LOG_ERR("unknown message box result: 0x%08x", result);
		throw ExceptionNA();
#else
		return false;
#endif
	}
}

bool PspMessageBox_ShowDialog(const char* text, int id, PspMbButtons buttons, PspMbFinishedHandler handleFinish)
{
	if (sIsRunning)
	{
		PspMessageBox_Finish();
	}

	Assert(sIsRunning == false);

	//

	sId = id;
	sHandleFinish = handleFinish;

	memset(&param, 0x00, sizeof(param));

	param.base.size = sizeof(param);
	param.base.ctrl_assign = BUTTON_SETTING;
	param.base.main_thread_priority = SCE_KERNEL_USER_HIGHEST_PRIORITY + 1;
	param.base.sub_thread_priority = SCE_KERNEL_USER_HIGHEST_PRIORITY + 3;
	param.base.font_thread_priority = SCE_KERNEL_USER_HIGHEST_PRIORITY + 2;
	param.base.sound_thread_priority = SCE_KERNEL_USER_HIGHEST_PRIORITY;
	param.base.message_lang = LANG_SETTING;
	strcpy(param.msgString, text);
	param.type = SCE_UTILITY_MSGDIALOG_TYPE_STRING;
	param.optionType = TranslateType(buttons);

	int ret = sceUtilityMsgDialogInitStart(&param);

	if (ret != 0)
	{
		LOG_ERR("failed to start message box dialog: 0x%08x", ret);
		return false;
	}

	sIsRunning = true;

	return true;
}

bool PspMessageBox_ShowError(int errorCode)
{
	if (sIsRunning)
	{
		PspMessageBox_Finish();
	}

	Assert(sIsRunning == false);

	//

	sId = -1;
	sHandleFinish = 0;

	memset(&param, 0x00, sizeof(param));

	param.base.size = sizeof(param);
	param.base.ctrl_assign = BUTTON_SETTING;
	param.base.main_thread_priority = SCE_KERNEL_USER_HIGHEST_PRIORITY + 1;
	param.base.sub_thread_priority = SCE_KERNEL_USER_HIGHEST_PRIORITY + 3;
	param.base.font_thread_priority = SCE_KERNEL_USER_HIGHEST_PRIORITY + 2;
	param.base.sound_thread_priority = SCE_KERNEL_USER_HIGHEST_PRIORITY;
	param.base.message_lang = LANG_SETTING;
	param.errorNum	= errorCode;
	param.type = SCE_UTILITY_MSGDIALOG_TYPE_NUMBER;
	param.optionType = 0;

	int ret = sceUtilityMsgDialogInitStart(&param);

	if (ret != 0)
	{
		LOG_ERR("failed to start message box dialog: 0x%08x", ret);
		return false;
	}

	sIsRunning = true;

	return true;
}

bool PspMessageBox_Update()
{
	if (sIsRunning == false)
	{
		return false;
	}

	int ret = 0;
	int status = sceUtilityMsgDialogGetStatus();

	switch (status)
	{
	case SCE_UTILITY_COMMON_STATUS_INITIALIZE:
		LOG_DBG("sceUtilityMsgDialog: SCE_UTILITY_COMMON_STATUS_INITIALIZE", 0);
		// todo: input block
		break;

	case SCE_UTILITY_COMMON_STATUS_RUNNING:
		LOG_DBG("sceUtilityMsgDialog: SCE_UTILITY_COMMON_STATUS_RUNNING", 0);
		ret = sceUtilityMsgDialogUpdate(1);
		if (ret != 0)
		{
			LOG_ERR("failed to update message box dialog: 0x%08x", ret);
		}
		break;

	case SCE_UTILITY_COMMON_STATUS_FINISHED:
		LOG_DBG("sceUtilityMsgDialog: SCE_UTILITY_COMMON_STATUS_FINISHED", 0);
		ret = sceUtilityMsgDialogShutdownStart();
		if (ret != 0)
		{
			LOG_ERR("failed to shutdown message box dialog: 0x%08x", ret);
		}
		break;

	case SCE_UTILITY_COMMON_STATUS_SHUTDOWN:
		LOG_DBG("sceUtilityMsgDialog: SCE_UTILITY_COMMON_STATUS_SHUTDOWN", 0);
		break;

	case SCE_UTILITY_COMMON_STATUS_NONE:
		LOG_DBG("sceUtilityMsgDialog: SCE_UTILITY_COMMON_STATUS_NONE", 0);
		sIsRunning = false;
		if (sHandleFinish)
		{
			bool result = TranslateResult(param.buttonResult);
			sHandleFinish(sId, result);
		}
		break;

	default:
		LOG_ERR("sceUtilityMsgDialog: unknown state. abort: 0x%08x", status);
		sIsRunning = false;
		break;
	}

	if (sIsRunning == false)
	{
		// todo: release input block
	}

	return sIsRunning;
}

void PspMessageBox_Finish()
{
	while (PspMessageBox_Update())
	{
		LOG_DBG("waiting for request end", 0);

		sceKernelCheckCallback();

		sceKernelDelayThread(1);
	}
}

bool PspMessageBox_IsActive()
{
	return sIsRunning;
}
