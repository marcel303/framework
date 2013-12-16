#pragma once

#include "ResIO.h"

class ResMgr;

namespace ResIO
{
	void* LoadBinaryData(const char* fileName); // Returns BinaryData
	void* LoadFont(const char* fileName); // Returns FontMap
	void* LoadTexture_DDS(const char* fileName); // Returns TextureDDS
	void* LoadTexture_TGA(const char* fileName); // Returns TextureRGBA
	//void* LoadTexture_PNG(const char* fileName); // Returns TextureRGBA
	void* LoadTexture_PVR(const char* fileName); // Returns TexturePVR
	void* LoadTexture_Atlas(const char* fileName, ResMgr* resMgr); // Returns TextureAtlas
	void* LoadSoundEffect(const char* fileName); // Returns SoundEffect
	void* LoadSound3D(const char* fileName); // Returns SoundEffect with certain guarantees regarding 3D audio
	void* LoadVectorGraphic(const char* fileName); // Returns VectorShape
	void* LoadVectorComposition(const char* fileName); // Returns VectorComposition
	void* LoadCollisionMask(const char* fileName); // Returns CollisionMask
	
	std::string GetBundleFileName(const char* fileName);
};
