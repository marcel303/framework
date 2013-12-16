#include <kernel.h>
#include <string.h>
#include <utility/utility_savedata.h>
#include "ArrayStream.h"
#include "Benchmark.h"
#include "Debugging.h"
#include "Exception.h"
#include "FileStream.h"
#include "FixedSizeString.h"
#include "Log.h"
#include "MemoryStream.h"
#include "PspMessageBox.h"
#include "PspSaveData.h"
#include "System.h"
#include "Types.h"

#define FILENAME "GAMESAVE.BIN"
#define FILEDESC "Critical Wave"
#define DESCLONG "High scores, settings and auto-save"

// load:
// - load GAMESAVE.BIN
// - parse to file list
// - find requested file
// - copy data to destination buffer

// save:
// - load GAMESAVE.BIN
// - create file
// - replace file
// - save GAMESAVE.BIN

#include "StreamReader.h"
#include "StreamWriter.h"

typedef FixedSizeString<16> PspSaveName;

class PspSaveData
{
public:
	void Load(Stream* stream)
	{
		StreamReader reader(stream, false);

		const uint32_t byteCount = reader.ReadUInt32();

		if (byteCount > 65536)
		{
			throw ExceptionVA("load IO error");
		}

		Name = reader.ReadText_Binary().c_str();

		Data = MemoryStream(byteCount);

		if (stream->Read((void*)Data.Bytes_get(), byteCount) != byteCount)
		{
			throw ExceptionVA("load IO error");
		}

		LOG_INF("load data: %s [%d bytes]", Name.c_str(), byteCount);
	}

	void Save(Stream* stream)
	{
		StreamWriter writer(stream, false);

		writer.WriteUInt32(Data.Length_get());

		writer.WriteText_Binary(Name.c_str());

		stream->Write(Data.Bytes_get(), Data.Length_get());

		LOG_INF("save data: %s [%d bytes]", Name.c_str(), Data.Length_get());
	}

	PspSaveName Name;
	MemoryStream Data;
};

class PspSaveList
{
public:
	~PspSaveList()
	{
		Clear();
	}

	void Clear()
	{
		for (std::vector<PspSaveData*>::iterator i = DataList.begin(); i != DataList.end(); ++i)
		{
			PspSaveData* item = *i;

			delete item;
			item = 0;
		}

		DataList.clear();
	}

	void Remove(const char* name)
	{
		for (std::vector<PspSaveData*>::iterator i = DataList.begin(); i != DataList.end();)
		{
			PspSaveData* item = *i;

			if (item->Name == name)
			{
				delete item;
				i = DataList.erase(i);
			}
			else
			{
				++i;
			}
		}
	}

	bool Exists(const char* name)
	{
		return Find(name) != 0;
	}

	void Load(Stream* stream)
	{
		StreamReader reader(stream, false);

		const uint32_t count = reader.ReadUInt32();

		if (count > 10)
		{
			throw ExceptionVA("load IO error");
		}

		for (int i = 0; i < count; ++i)
		{
			PspSaveData* data = new PspSaveData();
			data->Load(stream);
			Replace(data->Name, data);
		}
	}

	void Save(Stream* stream)
	{
		StreamWriter writer(stream, false);

		writer.WriteUInt32(DataList.size());

		for (std::vector<PspSaveData*>::iterator i = DataList.begin(); i != DataList.end(); ++i)
		{
			PspSaveData* item = *i;

			item->Save(stream);
		}
	}

	void Replace(const PspSaveName& name, PspSaveData* data)
	{
		Remove(name.c_str());

		DataList.push_back(data);
	}

	PspSaveData* Find(const PspSaveName& name)
	{
		for (std::vector<PspSaveData*>::iterator i = DataList.begin(); i != DataList.end(); ++i)
		{
			PspSaveData* item = *i;

			if (item->Name == name)
				return item;
		}

		return 0;
	}

	std::vector<PspSaveData*> DataList;
};

//

#define LANG_SETTING   SCE_UTILITY_LANG_ENGLISH
#define BUTTON_SETTING SCE_UTILITY_CTRL_ASSIGN_CROSS_IS_ENTER

static SceUtilitySavedataParam sParam;
static bool                    sSaveIsActive = false;
static FixedSizeString<128>    sSaveErrorMessage;
static const uint8_t           sSecurityId[16] = { 13, 63, 194, 23, 3, 11, 49, 133, 132, 100, 11, 54, 93, 34, 22, 45 };
static const char*             sSaveModeStr = "N/A";
static uint8_t                 sSaveDataBytes[1024 * 8];
static MemoryStream            sSaveIcon0;
static MemoryStream            sSavePic1;
static PspSaveList             sSaveDataList;
static bool                    sSaveDataListIsLoaded = false;

static void PspSaveData_Cache(const char* appName);
static void PspSaveData_Flush(const char* appName, bool allowAsync);

bool PspSaveData_Update()
{
	if (sSaveIsActive == false)
	{
		return false;
	}

	int ret = 0;
	int status = sceUtilitySavedataGetStatus();

	switch (status)
	{
	case SCE_UTILITY_COMMON_STATUS_INITIALIZE:
		LOG_DBG("sceUtilitySavedata: SCE_UTILITY_COMMON_STATUS_INITIALIZE", 0);
		break;

	case SCE_UTILITY_COMMON_STATUS_RUNNING:
		LOG_DBG("sceUtilitySavedata: SCE_UTILITY_COMMON_STATUS_RUNNING", 0);
		ret = sceUtilitySavedataUpdate(1);
		if (ret != 0)
		{
			LOG_DBG("sceUtilitySavedataUpdate failed: %0x8", ret);
		}
		break;

	case SCE_UTILITY_COMMON_STATUS_FINISHED:
		LOG_DBG("sceUtilitySavedata: SCE_UTILITY_COMMON_STATUS_FINISHED", 0);
		ret = sceUtilitySavedataShutdownStart();
		if (ret != 0)
		{
			LOG_DBG("sceUtilitySavedataShutdownStart failed: %0x8", ret);
		}
		break;

	case SCE_UTILITY_COMMON_STATUS_SHUTDOWN:
		LOG_DBG("sceUtilitySavedata: SCE_UTILITY_COMMON_STATUS_SHUTDOWN", 0);
		break;

	case SCE_UTILITY_COMMON_STATUS_NONE:
		LOG_DBG("sceUtilitySavedata: SCE_UTILITY_COMMON_STATUS_NONE", 0);
		sSaveIsActive = false;
		break;

	default:
		LOG_ERR("sceUtilitySavedata: unknown state. abort", 0);
		sSaveIsActive = false;
		break;
	}

	if (sSaveIsActive == false)
	{
		switch (sParam.base.result)
		{
		case SCE_UTILITY_COMMON_RESULT_OK:
		case SCE_UTILITY_COMMON_RESULT_CANCELED:
		case SCE_UTILITY_SAVEDATA_ERROR_LOAD_NO_DATA:
		case SCE_UTILITY_SAVEDATA_ERROR_LOAD_NO_FILE:
			break;

		case SCE_UTILITY_COMMON_RESULT_ABORTED:
			sSaveErrorMessage = "request aborted";
			break;

		case SCE_UTILITY_SAVEDATA_ERROR_LOAD_NO_MS:
		case SCE_UTILITY_SAVEDATA_ERROR_SAVE_NO_MS:
			sSaveErrorMessage = "no memorystick";
			break;

		case SCE_UTILITY_SAVEDATA_ERROR_LOAD_EJECT_MS:
		case SCE_UTILITY_SAVEDATA_ERROR_SAVE_EJECT_MS:
			sSaveErrorMessage = "memorystick ejected";
			break;

		case SCE_UTILITY_SAVEDATA_ERROR_SAVE_MS_NOSPACE:
			sSaveErrorMessage = "memorystick out of space";
			break;

		default:
			sSaveErrorMessage = "unknown error";
			break;
		}

		if (sParam.base.result == 0)
		{
			LOG_INF("%s OK: %s", sSaveModeStr, sParam.fileName);
		}
		else
		{
			LOG_ERR("%s error: %08x: %s", sSaveModeStr, sParam.base.result, sParam.fileName);
			PspMessageBox_ShowError(sParam.base.result);
		}

		sSaveIcon0 = MemoryStream();
		sSavePic1 = MemoryStream();
	}

	return sSaveIsActive;
}

void PspSaveData_Save(const char* appName, const char* saveName, const char* saveDesc, const char* saveDescLong, const void* bytes, int byteCount, bool allowAsync)
{
	Benchmark bm("PSP savedata save");

	// must finish previous request, if any, first

	PspSaveData_Finish(true);

	// cache save data list

	PspSaveData_Cache(appName);

	//

	LOG_DBG("saving savedata: %s [%d bytes]", saveName, byteCount);

	// create save data

	PspSaveData* data = new PspSaveData();

	data->Name = saveName;
	data->Data = MemoryStream(bytes, byteCount);

	// insert save data entry

	sSaveDataList.Replace(data->Name, data);

	//

	PspSaveData_Flush(appName, allowAsync);
}

void PspSaveData_Cache(const char* appName)
{
	// don't bother if the data is already cached

	if (sSaveDataListIsLoaded)
	{
		LOG_DBG("save data already loaded", 0);

		return;
	}

	// must finish previous request, if any, first

	PspSaveData_Finish(true);

	// load save data

	sSaveModeStr = "load";

	memset(&sParam, 0x00, sizeof(SceUtilitySavedataParam));

	sParam.type = SCE_UTILITY_SAVEDATA_TYPE_AUTOLOAD;
	sParam.overWriteMode = SCE_UTILITY_SAVEDATA_OVERWRITEMODE_ON;
	strcpy(sParam.titleId, appName);
	strcpy(sParam.fileName, FILENAME);
	strcpy(sParam.userId, SCE_UTILITY_SAVEDATA_USERID_NULL);

	sParam.base.size = sizeof(SceUtilitySavedataParam);
	sParam.base.message_lang = LANG_SETTING;
	sParam.base.ctrl_assign = BUTTON_SETTING;
	sParam.base.main_thread_priority = SCE_KERNEL_USER_HIGHEST_PRIORITY + 1;
	sParam.base.sub_thread_priority = SCE_KERNEL_USER_HIGHEST_PRIORITY + 3;
	sParam.base.font_thread_priority = SCE_KERNEL_USER_HIGHEST_PRIORITY + 2;
	sParam.base.sound_thread_priority = SCE_KERNEL_USER_HIGHEST_PRIORITY + 0;

	sParam.pDataBuf = sSaveDataBytes;
	sParam.dataBufSize = sizeof(sSaveDataBytes);
	sParam.dataFileSize = 0;

	memcpy(sParam.secureFileId, sSecurityId, sizeof(sSecurityId));

	sParam.dataVersion = SCE_UTILITY_SAVEDATA_VERSION_CURRENT;
	// note: multi call not allowed for Minis due to PS3 emu?
	//sParam.mcStatus = SCE_UTILITY_SAVEDATA_MC_STATUS_START;
	sParam.mcStatus = SCE_UTILITY_SAVEDATA_MC_STATUS_SINGLE;

	//

	int ret = sceUtilitySavedataInitStart(&sParam);

	if (ret != 0)
	{
		LOG_ERR("failed to load data: %08x", ret);

		return;
	}

	sSaveIsActive = true;

	PspSaveData_Finish(false);

	sSaveDataListIsLoaded = true;

	// load IO failed

	if (sParam.base.result != 0)
	{
		return;
	}

	// deserialize save data list

	ArrayStream stream(sSaveDataBytes, sParam.dataFileSize);

	try
	{
		sSaveDataList.Load(&stream);
	}
	catch (std::exception& e)
	{
		LOG_ERR("failed to deserialize save data list. creating new", 0);

		sSaveDataList.Clear();
	}
}

static void PspSaveData_Flush(const char* appName, bool allowAsync)
{
	// must finish previous request, if any, first

	PspSaveData_Finish(true);

	// serialize save data list

	MemoryStream stream;

	sSaveDataList.Save(&stream);

	if (stream.Length_get() > sizeof(sSaveDataBytes))
	{
		throw ExceptionVA("save IO error. stream length too long");
	}

	memcpy(sSaveDataBytes, stream.Bytes_get(), stream.Length_get());
	const int saveDataByteCount = stream.Length_get();

	// initiate save

	sSaveModeStr = "save";

	memset(&sParam, 0x00, sizeof(SceUtilitySavedataParam));

	sParam.type = SCE_UTILITY_SAVEDATA_TYPE_AUTOSAVE;
	sParam.overWriteMode = SCE_UTILITY_SAVEDATA_OVERWRITEMODE_ON;
	strcpy(sParam.titleId, appName);
	strcpy(sParam.fileName, FILENAME);
	strcpy(sParam.userId, SCE_UTILITY_SAVEDATA_USERID_NULL);
	strcpy(sParam.systemFile.title, appName);
	strcpy(sParam.systemFile.savedataTitle, FILEDESC);
	strcpy(sParam.systemFile.detail, DESCLONG);
	sParam.systemFile.parentalLev = 1;
#if 0
	sParam.systemFile.typeWriteRemoveUpdateParam =
		SCE_UTILITY_SAVEDATA_UPDATE_TITLE |
		SCE_UTILITY_SAVEDATA_UPDATE_SDTITLE |
		SCE_UTILITY_SAVEDATA_UPDATE_DETAIL |
		SCE_UTILITY_SAVEDATA_UPDATE_PARENTALLEV;
#endif

	sParam.base.size = sizeof(sParam);
	sParam.base.message_lang = LANG_SETTING;
	sParam.base.ctrl_assign = BUTTON_SETTING;
	sParam.base.main_thread_priority = SCE_KERNEL_USER_HIGHEST_PRIORITY + 1;
	sParam.base.sub_thread_priority = SCE_KERNEL_USER_HIGHEST_PRIORITY + 3;
	sParam.base.font_thread_priority = SCE_KERNEL_USER_HIGHEST_PRIORITY + 2;
	sParam.base.sound_thread_priority = SCE_KERNEL_USER_HIGHEST_PRIORITY + 0;

	sParam.pDataBuf = sSaveDataBytes;
	sParam.dataBufSize = saveDataByteCount;
	sParam.dataFileSize = saveDataByteCount;

	memcpy(sParam.secureFileId, sSecurityId, sizeof(sSecurityId));
	
	sParam.dataVersion = SCE_UTILITY_SAVEDATA_VERSION_CURRENT;
	// note: multi call not allowed for Minis due to PS3 emu?
	//sParam.mcStatus = SCE_UTILITY_SAVEDATA_MC_STATUS_START;
	sParam.mcStatus = SCE_UTILITY_SAVEDATA_MC_STATUS_SINGLE;

#if 1
	// icon0
	try
	{
		std::string fileName = g_System.GetDocumentPath("psp_save_icon0.bin");
		FileStream stream(fileName.c_str(), OpenMode_Read);
		StreamExtensions::StreamTo(&stream, &sSaveIcon0, 1024 * 16);
		sParam.icon0.dataBufSize = sSaveIcon0.Length_get();
		sParam.icon0.dataFileSize = sSaveIcon0.Length_get();
		sParam.icon0.pDataBuf = (void*)sSaveIcon0.Bytes_get();
	}
	catch (std::exception& e)
	{
		LOG_ERR("failed to set save icon", 0);
		sParam.icon0.dataBufSize = 0;
		sParam.icon0.dataFileSize = 0;
		sParam.icon0.pDataBuf = 0;
	}
#endif

#if 0
	// pic1
	try
	{
		std::string fileName = g_System.GetDocumentPath("psp_save_pic1.png");
		FileStream stream(fileName.c_str(), OpenMode_Read);
		StreamExtensions::StreamTo(&stream, &sSavePic1, 1024 * 16);
		sParam.pic1.dataBufSize = sSavePic1.Length_get();
		sParam.pic1.dataFileSize = sSavePic1.Length_get();
		sParam.pic1.pDataBuf = (void*)sSavePic1.Bytes_get();
	}
	catch (std::exception& e)
	{
		LOG_ERR("failed to set save icon", 0);
		sParam.pic1.dataBufSize = 0;
		sParam.pic1.dataFileSize = 0;
		sParam.pic1.pDataBuf = 0;
	}
#endif

	SceUtilitySavedataListSaveNewData newData;
	memset(&newData, 0, sizeof(newData));
	newData.icon0.dataBufSize = sParam.icon0.dataBufSize;
	newData.icon0.dataFileSize = sParam.icon0.dataFileSize;
	newData.icon0.pDataBuf = sParam.icon0.pDataBuf;
	newData.pTitle = sParam.systemFile.title;

	//sParam.pNewData = &newData;

	//

	int ret = sceUtilitySavedataInitStart(&sParam);

	if (ret != 0)
	{
		LOG_ERR("failed to save data: %08x", ret);
		return;
	}

	sSaveIsActive = true;

	if (!allowAsync)
	{
		PspSaveData_Finish(false);
	}
}

bool PspSaveData_Load(const char* appName, const char* saveName, void* bytes, int byteCount, int& o_byteCount)
{
	Benchmark bm("PSP savedata load");

	PspSaveData_Cache(appName);

	PspSaveData* data = sSaveDataList.Find(saveName);

	if (data == 0)
	{
		LOG_DBG("no save data exists", 0);

		o_byteCount = 0;

		return false;
	}

	if (data->Data.Length_get() > byteCount)
		throw ExceptionVA("destination buffer too small");

	memcpy(bytes, data->Data.Bytes_get(), data->Data.Length_get());

	o_byteCount = data->Data.Length_get();

	LOG_DBG("loaded savedata: %s [%d bytes]", data->Name.c_str(), data->Data.Length_get());

	return true;
}

void PspSaveData_Finish(bool warn)
{
	if (sSaveIsActive && warn)
	{
		LOG_WRN("force finishing load or save. may cause slowness", 0);
	}

	while (PspSaveData_Update())
	{
		LOG_DBG("waiting for request end", 0);

		sceKernelCheckCallback();

		sceKernelDelayThread(1);
	}
}

bool PspSaveData_IsSaveInProgress()
{
	return sSaveIsActive;
}

void PspSaveData_Remove(const char* appName, const char* saveName)
{
	PspSaveData_Cache(appName);

	sSaveDataList.Remove(saveName);

	PspSaveData_Flush(appName, true);
}

bool PspSaveData_Exists(const char* appName, const char* saveName)
{
	PspSaveData_Cache(appName);

	return sSaveDataList.Exists(saveName);
}
