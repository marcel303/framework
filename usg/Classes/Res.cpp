#include <string.h>
#include "Exception.h"
#include "Res.h"

IRes::~IRes()
{
}

//

Res::Res(ResTypes type, const char* fileName, ResMgr* resMgr)
{
	m_DataSetId = 0;
	m_Type = type;
	//m_FileName = fileName;
	strcpy(m_FileName, fileName);
	m_ResMgr = resMgr;
	m_OpenCount = 0;
	data = 0;
	info = 0;
	device_data = 0;
}

Res::~Res()
{
	while (m_OpenCount > 0)
		Close();
		
	if (OnDelete.IsSet())
		OnDelete.Invoke(this);
}

void Res::Open()
{
	if (m_OpenCount == 0)
	{
		if (OnOpen.IsSet())
			OnOpen.Invoke(this);
	}
	
	++m_OpenCount;
}

void Res::Close()
{
	--m_OpenCount;
	
	if (m_OpenCount == 0)
	{
		if (OnClose.IsSet())
			OnClose.Invoke(this);
	}
}

void Res::OpenAlways()
{
	Open();
}

void Res::HandleMemoryWarning()
{
	if (OnMemoryWarning.IsSet())
		OnMemoryWarning.Invoke(this);
}

bool Res::IsType(const char* typeName)
{
	switch (m_Type)
	{
	case ResTypes_TexturePVR:
		return !strcmp(typeName, "tex_pvr");
	case ResTypes_TextureRGBA:
		return !strcmp(typeName, "tex_rgba");
	default:
		throw ExceptionNA();
	}
}
