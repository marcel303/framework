#include <kernel.h>
#include <string.h>
#include <utility/utility_osk.h>
#include "Debugging.h"
#include "FixedSizeString.h"
#include "Log.h"
#include "PspKeyboard.h"
#include "PspMessageBox.h"

#define LANG_SETTING   SCE_UTILITY_LANG_ENGLISH
#define BUTTON_SETTING SCE_UTILITY_CTRL_ASSIGN_CROSS_IS_ENTER

static bool sIsActive = false;
static SceUtilityOskParam sParam;
static SceUtilityOskInputFieldInfo sInputField[1];

// "First input field"
static SceWChar16 messageA []={
	0x3072, 0x3068, 0x3064, 0x3081, 0x306e, 0x5165,
	0x529b, 0x30d5, 0x30a3, 0x30fc, 0x30eb, 0x30c9, 0x0000};
static SceWChar16 resultBufferA[64];
static FixedSizeString<32> sResultText;

static FixedSizeString<32> TranslateText(const SceWChar16* chars, int charCount)
{
	FixedSizeString<32> result;

	for (int i = 0; i < charCount; ++i)
	{
		SceWChar16 c = chars[i];

		//if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
		if (c >= 32 && c <= 127)
		{
			result.push_back((char)c);
		}
	}

	return result;
}

void PspKeyboard_Show()
{
	Assert(sIsActive == false);

	memset(&sParam, 0, sizeof(sParam));

	sParam.base.size = sizeof(SceUtilityOskParam);
	sParam.base.message_lang = LANG_SETTING;
	sParam.base.ctrl_assign = BUTTON_SETTING;
	sParam.base.main_thread_priority = SCE_KERNEL_USER_HIGHEST_PRIORITY + 1;
	sParam.base.sub_thread_priority = SCE_KERNEL_USER_HIGHEST_PRIORITY + 3;
	sParam.base.font_thread_priority = SCE_KERNEL_USER_HIGHEST_PRIORITY + 2;
	sParam.base.sound_thread_priority = SCE_KERNEL_USER_HIGHEST_PRIORITY + 0;

	sParam.num_input_fields = 1;
	sParam.input_field_info = sInputField;

	memset(sInputField, 0, sizeof(sInputField));

	sInputField[0].input_method_type = SCE_UTILITY_OSK_INPUT_METHOD_NONE  ;
	sInputField[0].input_method_attributes = 0;
	sInputField[0].writing_language_type = SCE_UTILITY_OSK_WRITING_LANGUAGE_TYPE_ENGLISH;
	sInputField[0].input_character_type =
		SCE_UTILITY_OSK_INPUT_CHARACTER_TYPE_LATIN_NUMBER |
		SCE_UTILITY_OSK_INPUT_CHARACTER_TYPE_LATIN_LOWERCASE_ALPHABET |
		SCE_UTILITY_OSK_INPUT_CHARACTER_TYPE_LATIN_UPPERCASE_ALPHABET;
	sInputField[0].message = messageA;
	sInputField[0].init_text = 0; // todo: "PLAYER"
	sInputField[0].result_text_buffer_size = sizeof(resultBufferA) / sizeof(SceWChar16);
	sInputField[0].result_text_buffer = resultBufferA;
	sInputField[0].num_lines = 1;

	int ret = sceUtilityOskInitStart(&sParam);

	if (ret != 0)
	{
		LOG_ERR("failed to show keyboard: %08x", ret);

		return;
	}

	sIsActive = true;
}

bool PspKeyboard_Update()
{
	if (sIsActive == false)
	{
		return false;
	}

	int ret = 0;
	int status = sceUtilityOskGetStatus();

	switch (status)
	{
	case SCE_UTILITY_COMMON_STATUS_INITIALIZE:
		LOG_DBG("sceUtilityOsk: SCE_UTILITY_COMMON_STATUS_INITIALIZE", 0);
		break;

	case SCE_UTILITY_COMMON_STATUS_RUNNING:
		LOG_DBG("sceUtilityOsk: SCE_UTILITY_COMMON_STATUS_RUNNING", 0);
		ret = sceUtilityOskUpdate(1);
		if (ret != 0)
		{
			LOG_DBG("sceUtilityOskUpdate failed: %0x8", ret);
		}
		break;

	case SCE_UTILITY_COMMON_STATUS_FINISHED:
		LOG_DBG("sceUtilityOsk: SCE_UTILITY_COMMON_STATUS_FINISHED", 0);
		ret = sceUtilityOskShutdownStart();
		if (ret != 0)
		{
			LOG_DBG("sceUtilityOskShutdownStart failed: %0x8", ret);
		}
		break;

	case SCE_UTILITY_COMMON_STATUS_SHUTDOWN:
		LOG_DBG("sceUtilityOsk: SCE_UTILITY_COMMON_STATUS_SHUTDOWN", 0);
		break;

	case SCE_UTILITY_COMMON_STATUS_NONE:
		LOG_DBG("sceUtilityOsk: SCE_UTILITY_COMMON_STATUS_NONE", 0);
		sIsActive = false;
		break;

	default:
		LOG_ERR("sceUtilityOsk: unknown state. abort", 0);
		sIsActive = false;
		break;
	}

	if (sIsActive == false)
	{
		if (sParam.base.result == 0)
		{
			sResultText = TranslateText(sInputField[0].result_text_buffer, sInputField[0].result_text_buffer_size);
			LOG_INF("OSK: OK: %s", sResultText.c_str());
		}
		else
		{
			LOG_ERR("OSK: error: %08x", sParam.base.result);
			PspMessageBox_ShowError(sParam.base.result);
		}
	}

	return sIsActive;
}

bool PspKeyboard_IsActive()
{
	return sIsActive;
}
