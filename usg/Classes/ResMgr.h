#pragma once

#include <stddef.h>
#include "FixedSizeString.h"
#include "Res.h"
#include "ResIndex.h"

class ResMgr
{
public:
	ResMgr();
	~ResMgr();
	void Initialize(const char* fontPrefix);
	
	void Load(const CompiledResInfo* resList, int count);
	
	void CreateResource(int index, int dataSetId, const char* type, const char* fileName);
	
	Res* Get(int index);
	const Res* Get(int index) const;

	Res* CreateBinaryData(const char* fileName);
	Res* CreateFont(const char* fileName);
	Res* CreateSoundEffect(const char* fileName);
	Res* CreateTextureAtlas(const char* fileName);
	Res* CreateTextureDDS(const char* fileName);
	Res* CreateTexturePVR(const char* fileName);
	Res* CreateTextureRGBA(const char* fileName);
	Res* CreateVectorGraphic(const char* fileName);
	Res* CreateVectorComposition(const char* fileName);
	
	Res* Find(const char* fileName, const char* extension);
	
	void HandleMemoryWarning();

private:
	Res* GetOrCreate(const char* fileName, ResTypes type);

public:
	int m_ResourceCount;
	Res** m_Resources;
	FixedSizeString<32> m_FontPrefix;
};
