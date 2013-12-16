#include <string.h>
#include "Benchmark.h"
#include "BinaryData.h"
#include "Exception.h"
#include "FileStream.h"
#include "FontMap.h"
#include "Log.h"
#include "Res.h"
#include "ResIO.h"
#include "ResMgr.h"
#include "SoundEffect.h"
#include "StringBuilder.h"
#include "System.h"
#include "TextureAtlas.h"
#include "TextureDDS.h"
#include "TextureRGBA.h"
#include "TexturePVR.h"
#include "VectorComposition.h"
#include "VectorShape.h"

//

static void Res_Open(void* obj, void* arg);
static void Res_Close(void* obj, void* arg);
static void Res_Delete(void* obj, void* arg);

//

ResMgr::ResMgr()
{
	m_ResourceCount = 0;
	m_Resources = 0;
}

ResMgr::~ResMgr()
{
	for (int i = 0; i < m_ResourceCount; ++i)
	{
		Res* res = m_Resources[i];

		if (!res)
			continue;

		ResTypes type = res->m_Type;

		if (type == ResTypes_Font || type == ResTypes_TextureAtlas || type == ResTypes_VectorGraphic || type == ResTypes_VectorComposition)
			res->Close();

#ifdef PSP
		// PSP stores all resources in main memory. no need to open/close them dynamically / on upload
		if (type == ResTypes_SoundEffect || type == ResTypes_TextureRGBA)
			res->Close();
#endif

		Assert(res->m_OpenCount == 0);

		delete m_Resources[i];
		m_Resources[i] = 0;
	}

	delete[] m_Resources;
	m_Resources = 0;
}

void ResMgr::Initialize(const char* fontPrefix)
{
	m_Resources = 0;
	
	m_FontPrefix = fontPrefix;
}

void ResMgr::Load(const CompiledResInfo* resList, int count)
{
	try
	{
		LOG(LogLevel_Info, "Loading resource index array");
		
		m_Resources = new Res*[count];
		m_ResourceCount = count;
		
		for (int i = 0; i < count; ++i)
			CreateResource(i, resList[i].dataSetId, resList[i].type, resList[i].file);
	}
	catch (Exception& e)
	{
		LOG(LogLevel_Error, "Failed to load resource index array: %s", e.what());
		
		throw e;
	}
}

//

void ResMgr::CreateResource(int index, int dataSetId, const char* type, const char* fileName)
{
#define CASE(_type, create, ext) \
	else if (!strcmp(type, _type)) \
	{ \
		char temp[64]; \
		strcpy(temp, fileName); \
		strcat(temp, ext); \
		m_Resources[index] = create(temp); \
		m_Resources[index]->m_DataSetId = dataSetId; \
	}
	
	if (false) { }
	CASE("binary", CreateBinaryData, "")
	CASE("dds", CreateTextureDDS, ".dds")
	CASE("dds_alpha", CreateTextureDDS, ".dds")
	CASE("font", CreateFont, ".fnt")
//	CASE("texture", CreateTextureRGBA, ".tex")
	else if (!strcmp(type, "texture"))
	{
		m_Resources[index] = new Res(ResTypes_BinaryData, "", this);
	}
	CASE("image", CreateTextureRGBA, ".img")
	CASE("pvr2", CreateTexturePVR, ".pvr2")
	CASE("pvr4", CreateTexturePVR, ".pvr4")
	CASE("sound", CreateSoundEffect, ".sfx")
	CASE("vector", CreateVectorGraphic, ".vgc")
	CASE("composition", CreateVectorComposition, ".vcc")
	else
	{
#ifdef DEBUG
		throw ExceptionVA("unknown resource type: %s", type);
#endif
	}
	
#if defined(DEBUG) && !defined(WIN32) && !defined(LINUX) && !defined(MACOS) && !defined(PSP) && !defined(IPHONEOS) && !defined(BBOS)
	try
	{
		if (strcmp(type, "texture"))
		{
			FileStream stream;
			
			stream.Open(g_System.GetResourcePath(m_Resources[index]->m_FileName).c_str(), OpenMode_Read);
		}
	}
	catch (std::exception e)
	{
		throw ExceptionVA("resource does not exist: %s", m_Resources[index]->m_FileName);
	}
#endif
}

Res* ResMgr::Get(int index)
{
	Assert(index >= 0 && index < m_ResourceCount);
	
	return m_Resources[index];
}

const Res* ResMgr::Get(int index) const
{
	Assert(index >= 0 && index < m_ResourceCount);
	
	return m_Resources[index];
}

//

Res* ResMgr::CreateBinaryData(const char* fileName)
{
	return GetOrCreate(fileName, ResTypes_BinaryData);
}

Res* ResMgr::CreateFont(const char* fileName)
{
	StringBuilder<256> sb;
	sb.Append(m_FontPrefix.c_str());
	sb.Append(fileName);
	return GetOrCreate(sb.ToString(), ResTypes_Font);
}

Res* ResMgr::CreateSoundEffect(const char* fileName)
{
	return GetOrCreate(fileName, ResTypes_SoundEffect);
}

Res* ResMgr::CreateTextureAtlas(const char* fileName)
{
	return GetOrCreate(fileName, ResTypes_TextureAtlas);
}

Res* ResMgr::CreateTextureDDS(const char* fileName)
{
	return GetOrCreate(fileName, ResTypes_TextureDDS);
}

Res* ResMgr::CreateTexturePVR(const char* fileName)
{
	return GetOrCreate(fileName, ResTypes_TexturePVR);
}

Res* ResMgr::CreateTextureRGBA(const char* fileName)
{
	return GetOrCreate(fileName, ResTypes_TextureRGBA);
}

Res* ResMgr::CreateVectorGraphic(const char* fileName)
{
	return GetOrCreate(fileName, ResTypes_VectorGraphic);
}

Res* ResMgr::CreateVectorComposition(const char* fileName)
{
	return GetOrCreate(fileName, ResTypes_VectorComposition);
}

//

Res* ResMgr::Find(const char* fileName, const char* extension)
{
	char temp[128];
	
	strcpy(temp, fileName);
	strcat(temp, extension);
	
	for (int i = 0; i < m_ResourceCount; ++i)
		if (!strcmp(m_Resources[i]->m_FileName, temp))
			return m_Resources[i];
	
	Assert(false);
	
	return 0;
}

//

void ResMgr::HandleMemoryWarning()
{
	for (int i = 0; i < m_ResourceCount; ++i)
	{
		if (!m_Resources[i])
			continue;
		
		m_Resources[i]->HandleMemoryWarning();
	}
}

//

Res* ResMgr::GetOrCreate(const char* fileName, ResTypes type)
{
	Res* res = new Res(type, fileName, this);
	
	res->OnOpen = CallBack(res, Res_Open);
	res->OnClose = CallBack(res, Res_Close);
	res->OnDelete.Add(CallBack(res, Res_Delete));
	
	if (type == ResTypes_Font || type == ResTypes_TextureAtlas || type == ResTypes_VectorGraphic || type == ResTypes_VectorComposition)
		res->OpenAlways();

#ifdef PSP
	// PSP stores all resources in main memory. no need to open/close them dynamically / on upload
	if (type == ResTypes_SoundEffect || type == ResTypes_TextureRGBA)
		res->OpenAlways();
#endif
	
	//Set(fileName, res);
	
	return res;
}

//

static void Res_Open(void* obj, void* arg)
{
	Res* res = (Res*)obj;
	
#ifdef DEBUG
	char benchmarkName[128];
	sprintf(benchmarkName, "Opening %s", res->m_FileName);
	
	UsingBegin(Benchmark b(benchmarkName))
	{
#endif
		switch (res->m_Type)
		{
		case ResTypes_BinaryData:
			{
				BinaryData* data = (BinaryData*)ResIO::LoadBinaryData(res->m_FileName);
				res->data = data;
			}
			break;
		
		case ResTypes_Font:
			{
				FontMap* font = (FontMap*)ResIO::LoadFont(res->m_FileName);
				res->data = font;
			}
			break;
			
		case ResTypes_SoundEffect:
			{
				SoundEffect* sound = (SoundEffect*)ResIO::LoadSoundEffect(res->m_FileName);
				
				SoundEffectInfo* info = new SoundEffectInfo();
				*info = sound->m_Info;
				
				res->data = sound;
				res->info = info;
			}
			break;
				
		case ResTypes_TextureAtlas:
			{
				TextureAtlas* textureAtlas = (TextureAtlas*)ResIO::LoadTexture_Atlas(res->m_FileName, res->m_ResMgr);
				res->data = textureAtlas;
			}
			break;
			
		case ResTypes_TextureDDS:
			{
				TextureDDS* texture = (TextureDDS*)ResIO::LoadTexture_DDS(res->m_FileName);
				res->data = texture;
			}
			break;

		case ResTypes_TexturePVR:
			{
				TexturePVR* texture = (TexturePVR*)ResIO::LoadTexture_PVR(res->m_FileName);
				res->data = texture;
			}
			break;
				
		case ResTypes_TextureRGBA:
			{
				TextureRGBA* texture = (TextureRGBA*)ResIO::LoadTexture_TGA(res->m_FileName);
				res->data = texture;
			}
			break;
						
		case ResTypes_VectorGraphic:
			{
				VectorShape* vectorGraphic = (VectorShape*)ResIO::LoadVectorGraphic(res->m_FileName);
				res->data = vectorGraphic;
			}
			break;
				
		case ResTypes_VectorComposition:
			{
				VectorComposition* vectorComposition = (VectorComposition*)ResIO::LoadVectorComposition(res->m_FileName);
				res->data = vectorComposition;
			}
			break;
				
		default:
			{
#ifndef DEPLOYMENT
				throw ExceptionVA("unknown resource type");
#else
				break;
#endif
			}
		}
#ifdef DEBUG
	}
	UsingEnd()
#endif

#ifdef DEBUG
	//DBG_PrintAllocState();
#endif
}

static void Res_Close(void* obj, void* arg)
{
	Res* res = (Res*)obj;
	
	switch (res->m_Type)
	{
	case ResTypes_BinaryData:
		{
			BinaryData* data = (BinaryData*)res->data;
			
			delete data;
			data = 0;
		}
		break;
			
	case ResTypes_Font:
		{
			FontMap* font = (FontMap*)res->data;
			
			delete font;
			font = 0;
		}
		break;
			
	case ResTypes_SoundEffect:
		{
			SoundEffect* sound = (SoundEffect*)res->data;
			
			delete sound;
			sound = 0;
		}
		break;
	
	case ResTypes_TextureAtlas:
		{
			TextureAtlas* textureAtlas = (TextureAtlas*)res->data;
			
			delete textureAtlas;
			textureAtlas = 0;
		}
		break;

	case ResTypes_TextureDDS:
		{
			TextureDDS* texture = (TextureDDS*)res->data;

			delete texture;
			texture = 0;
		}
		break;
		
	case ResTypes_TexturePVR:
		{
#ifdef IPHONEOS
			TexturePVR* texture = (TexturePVR*)res->data;
#elif defined(WIN32) || defined(LINUX) || defined(MACOS) || defined(PSP) || defined(BBOS)
			TextureRGBA* texture = (TextureRGBA*)res->data;
#else
	#error
#endif
			
			delete texture;
			texture = 0;
		}
		break;
			
	case ResTypes_TextureRGBA:
		{
			TextureRGBA* texture = (TextureRGBA*)res->data;
			
			delete texture;
			texture = 0;
		}
		break;
			
	case ResTypes_VectorGraphic:
		{
			VectorShape* vectorGraphic = (VectorShape*)res->data;
			
			delete vectorGraphic;
			vectorGraphic = 0;
		}
		break;
			
	case ResTypes_VectorComposition:
		{
			VectorComposition* vectorComposition = (VectorComposition*)res->data;
			
			delete vectorComposition;
			vectorComposition = 0;
		}
		break;

	case ResTypes_Sound3D:
	case ResTypes_CollisionMask:
		// todo
		break;

	default:
		{
#ifndef DEPLOYMENT
			throw ExceptionVA("unknown resource type");
#else
			break;
#endif
		}
	}
	
	res->data = 0;

#ifdef DEBUG
	LOG_DBG("Closed %s", res->m_FileName);
	//DBG_PrintAllocState();
#endif
}

static void Res_Delete(void* obj, void* arg)
{
	Res* res = (Res*)obj;
	
	switch (res->m_Type)
	{
		case ResTypes_SoundEffect:
		{
			SoundEffectInfo* info = (SoundEffectInfo*)res->info;
			delete info;
			res->info = 0;
			break;
		}
			
		default:
			break;
	}
}
