#pragma once

#include <map>
#include <string>

namespace gltf
{
	struct Scene;
	struct BoundingBox;
	struct BufferCache;
}

class GltfCacheElem
{
public:
	gltf::Scene * m_scene;
	gltf::BufferCache * m_bufferCache;
	gltf::BoundingBox * m_aabb;

	GltfCacheElem();
	void free();
	void load(const char * filename);
};

class GltfCache
{
public:
	typedef std::string Key;
	typedef std::map<Key, GltfCacheElem> Map;
	
	Map m_map;
	
	~GltfCache();
	void clear();
	void reload();
	GltfCacheElem & findOrCreate(const char * name);
};

extern GltfCache g_gltfCache;
