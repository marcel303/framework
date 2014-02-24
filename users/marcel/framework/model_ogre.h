#pragma once

#include "model.h"

namespace AnimModel
{
	class LoaderOgreXML : public Loader
	{
	public:
		MeshSet * loadMeshSet(const char * filename, const BoneSet * boneSet);
		BoneSet * loadBoneSet(const char * filename);
		AnimSet * loadAnimSet(const char * filename, const BoneSet * boneSet);
	};
}
