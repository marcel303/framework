#pragma once

#include "CallBack.h"
#include "IRes.h"

class ResMgr;

// note: ResType declared in MacOS headers. have to deviate from regular naming convention

enum ResTypes
{
	ResTypes_BinaryData,
	ResTypes_Font,
	ResTypes_TextureDDS,
	ResTypes_TextureRGBA,
	ResTypes_TexturePVR,
	ResTypes_TextureAtlas,
	ResTypes_SoundEffect,
	ResTypes_Sound3D,
	ResTypes_VectorGraphic,
	ResTypes_VectorComposition,
	ResTypes_CollisionMask
};

class Res : public IRes
{
public:
	Res(ResTypes type, const char* fileName, ResMgr* resMgr);
	virtual ~Res();
	
	virtual void Open();
	virtual void Close();
	void OpenAlways();
	virtual void HandleMemoryWarning();
	virtual bool IsType(const char* typeName);

	virtual void* Data_get()
	{
		return data;
	}

	virtual void* DeviceData_get()
	{
		return device_data;
	}

	virtual void DeviceData_set(void* deviceData)
	{
		device_data = deviceData;
	}

	virtual void* Description_get()
	{
		return info;
	}

	virtual void Description_set(void* description)
	{
		info = description;
	}

	virtual void OnMemoryWarning_set(CallBack callBack)
	{
		OnMemoryWarning = callBack;
	}
	
	CallBack OnOpen;
	CallBack OnClose;
	CallBack OnMemoryWarning;
	CallBackList<4> OnDelete;
	
	int m_DataSetId;
	ResTypes m_Type;
	char m_FileName[64];
	ResMgr* m_ResMgr;
	int m_OpenCount;
	void* data;
	void* info;
	void* device_data;
};
