#include "model.h"

namespace Model
{
	class LoaderOgreXML : public Loader
	{
	public:
		MeshSet * loadMeshSet(const char * filename);
		BoneSet * loadBoneSet(const char * filename);
		AnimSet * loadAnimSet(const char * filename, const BoneSet * boneSet);
	};
}
