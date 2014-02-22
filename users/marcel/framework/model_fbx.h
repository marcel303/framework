#pragma once

#include "model.h"

namespace Model
{
	class LoaderFbxBinary : public Loader
	{
	public:
		MeshSet * loadMeshSet(const char * filename, const BoneSet * boneSet);
		BoneSet * loadBoneSet(const char * filename);
		AnimSet * loadAnimSet(const char * filename, const BoneSet * boneSet);
		
		static bool readFile(const char * filename, std::vector<unsigned char> & bytes);
	};
}
