#pragma once

class IRes
{
public:
	virtual ~IRes();

	virtual void Open() = 0;
	virtual void Close() = 0;
	virtual void HandleMemoryWarning() = 0;
	virtual bool IsType(const char* typeName) = 0;

	virtual void* Data_get() = 0;
	virtual void* DeviceData_get() = 0;
	virtual void DeviceData_set(void* data) = 0;
	virtual void* Description_get() = 0;
	virtual void Description_set(void* description) = 0;

	virtual void OnMemoryWarning_set(CallBack callBack) = 0;
};
