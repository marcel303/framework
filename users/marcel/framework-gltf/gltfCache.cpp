#include "gltfCache.h"

#include "gltf.h"
#include "gltf-draw.h"
#include "gltf-loader.h"

#include "internal.h"

#include "Benchmark.h"

//

GltfCache & gltfCache()
{
	// note : access to the cache is through a function
	//        to ensure the constructuor for the cache
	//        is run after Framework's constructor,
	//        which in turn guarantees it's safe to
	//        register the cache with Framework
	
	static GltfCache cache;
	
	return cache;
}

//

GltfCacheElem::GltfCacheElem()
	: m_scene(0)
	, m_bufferCache(0)
	, m_aabb(0)
{
}

void GltfCacheElem::free()
{
	delete m_scene;
	m_scene = 0;
	
	delete m_bufferCache;
	m_bufferCache = nullptr;
	
	delete m_aabb;
	m_aabb = nullptr;
}

void GltfCacheElem::load(const char * filename)
{
	ScopedLoadTimer loadTimer(filename);

	free();
	
	m_scene = new gltf::Scene();
	m_bufferCache = new gltf::BufferCache();
	m_aabb = new gltf::BoundingBox();
	
	if (gltf::loadScene(filename, *m_scene))
	{
		logInfo("loaded GLTF scene %s", filename);
		
		// initialize buffer cache
	
		m_bufferCache->init(*m_scene);
		
		// compute AABB
		
		gltf::calculateSceneMinMax(*m_scene, *m_aabb);
	}
	else
	{
		logError("failed to load GLTF scene: %s", filename);
	}
}

//

GltfCache::GltfCache()
{
	framework.registerResourceCache(this);
}

GltfCache::~GltfCache()
{
	Assert(m_map.empty());
}

void GltfCache::clear()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.free();
	}
	
	m_map.clear();
}

void GltfCache::reload()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.load(i->first.c_str());
	}
}

void GltfCache::handleFileChange(const std::string & filename, const std::string & extension)
{
	auto i = m_map.find(filename);
	
	if (i != m_map.end())
	{
		auto & elem = i->second;
		
		elem.load(i->first.c_str());
	}
}

GltfCacheElem & GltfCache::findOrCreate(const char * name)
{
	Key key = name;
	
	Map::iterator i = m_map.find(key);
	
	if (i != m_map.end())
	{
		return i->second;
	}
	else
	{
		const char * resolved_filename = framework.resolveResourcePath(name);
		
		GltfCacheElem elem;
		
		elem.load(resolved_filename);
		
		i = m_map.insert(Map::value_type(key, elem)).first;
		
		return i->second;
	}
}
